# 从零开始：一个能被 Windows 识别的输入法

这是一个**最小但完整**的 TSF（Text Services Framework）输入法工程。它不是"带候选框的普通窗口程序"，而是一个会被 Windows 加载进每个应用进程的 COM DLL，会出现在系统的键盘列表里，能被 `Win + 空格` 切换，能在记事本、资源管理器、浏览器里打字。

> 现代 Windows 只接受 TSF 架构的输入法；老的 IMM32 会被系统阻止。微软官方的自定义 IME 要求文档明确写了这一点。

## 它能做什么

- 输入拼音字母 → 屏幕上出现带下划线的组合串（composition）
- 弹出候选窗，按 `空格` / `1`~`9` 上屏汉字，`Backspace` 删字母，`Esc` 取消
- 按住 `Shift` 打字时临时放行英文；`Ctrl` / `Alt` / `Win` 快捷键不抢
- 内置一个玩具词库，试试 `nihao`、`woaini`、`shurufa`、`zhongwen`
- 失焦、切换文档、应用强制终止组合时都能正确收尾

它**不**包含：真实词库、云联想、皮肤、设置面板、用户词频、繁体、双拼、手写、语音、数字签名、安装包。这些是"能跑起来之后"的事，文末列了路线。

## 环境

- Windows 10 / 11（x64）
- Visual Studio 2022，勾选「使用 C++ 的桌面开发」+ Windows 10/11 SDK
- 管理员权限（注册要写 `HKLM`）

> **没有 Visual Studio / 不想装编译环境？** 看 `NO_VS_GUIDE.md`：工程已配好 GitHub Actions 云端编译，点一下就能下载编好的 DLL，本地零安装。

## 三步跑起来

### 1. 编译

用 VS 打开 `MinimalTSFIME.sln`，选 `Release | x64`，生成解决方案，得到：

```
x64\Release\MinimalTSFIME.dll
```

**再切到 `Release | Win32` 编译一次**，得到 `Win32\Release\MinimalTSFIME.dll`。

> 为什么两个都要？64 位程序只能加载 64 位 TIP，32 位程序只能加载 32 位 TIP。两份 DLL 用同一个 CLSID 分别注册，系统会自动选。只装 x64 的话，32 位程序里会静默打不了字。

### 2. 注册

右键 `install.bat` → **以管理员身份运行**。它做的事是：

```
regsvr32 MinimalTSFIME.dll
    └─ DllRegisterServer()
        ├─ 写 HKCR\CLSID\{...}\InprocServer32（COM 服务器登记）
        ├─ ITfInputProcessorProfiles::Register            登记 TIP
        ├─ ITfInputProcessorProfiles::AddLanguageProfile   登记"简体中文 + 语言配置"
        ├─ ITfCategoryMgr::RegisterCategory               声明这是键盘型 TIP
        └─ InstallLayoutOrTip                             加入用户键盘列表并持久化
```

注册 32 位 DLL 时，改用：

```bat
%windir%\SysWOW64\regsvr32.exe Win32\Release\MinimalTSFIME.dll
```

（普通 `regsvr32` 在 64 位系统上是 64 位版本，会拒绝加载 32 位 DLL。）

### 3. 验证

1. `设置 → 时间和语言 → 语言和区域 → 中文(简体，中国) → 语言选项 → 键盘`
   应该能看到 **极简拼音 (TSF 示例)**；没看到就点「添加键盘」勾上它
2. `Win + 空格` 切到它，托盘区域会出现输入指示器
3. 打开记事本，敲 `nihao` → 空格 → 出现「你好」
4. 再打开 Edge / 设置界面试一次，确认不是只在传统 Win32 程序里能用

卸载：右键 `uninstall.bat` → 以管理员身份运行（或 `regsvr32 /u`），必要时注销一次。完整的回滚与急救步骤见文末「完全撤销 / 回滚」。

## 代码结构

| 文件 | 作用 |
|---|---|
| `Server.cpp` | `DllMain`、类工厂、四个 COM 标准导出 |
| `Register.cpp` | 注册 / 反注册的全部逻辑，最值得先读 |
| `TextService.h/.cpp` | TIP 主体：激活、事件接收器、组合串状态机、玩具词库 |
| `KeyEventSink.cpp` | 按键拦截与拼音状态机 |
| `EditSession.cpp` | TSF 编辑会话（异步改文档的唯一合法途径） |
| `CandidateWindow.cpp` | 极简 GDI 候选窗 |
| `Globals.h` | CLSID / Profile GUID / 名称 / LANGID |
| `MinimalTSFIME.def` | DLL 导出表 |
| `Resource.rc` + `ime.ico` | 托盘和键盘列表用的图标（必须嵌在 DLL 里） |
| `install.bat` / `uninstall.bat` | 管理员权限注册 / 反注册 |
| `rescue.bat` / `rescue.ps1` | 急救卸载：DLL 丢了也能清干净注册表 |
| `.github/workflows/build.yml` | 免费云端编译，本地不用装 VS |
| `NO_VS_GUIDE.md` | 无编译环境的三种解法 |

## 几个关键机制（想改代码前先看）

**为什么不能直接改文本？**
`ITfKeyEventSink::OnKeyDown` 里禁止直接读写文档。所有改动都必须放进 `ITfEditSession::DoEditSession`，通过 `RequestEditSession` 投递。本工程把每次按键的动作快照塞进编辑会话，利用"同一 context 的会话按投递顺序执行"保证组合串创建→更新→结束串行不乱序。

**`OnTestKeyDown` 和 `OnKeyDown` 必须给出相同的吃键结论**，否则应用会收到半截按键。本工程用同一个 `_HandleKey(..., bTestOnly)` 实现，测试态绝不改状态。

**组合串**：`ITfContextComposition::StartComposition` → `ITfComposition::GetRange` → `ITfRange::SetText` → `ITfComposition::EndComposition`。组合串的下划线由 TSF 自动渲染。

**注册别手写 `HKLM\SOFTWARE\Microsoft\CTF\TIP`**。那是系统内部状态，官方只提供 `ITfInputProcessorProfiles` / `ITfInputProcessorProfileMgr` / `ITfCategoryMgr` 三组接口。手写注册表在 Win11 上很容易出现"列表里有但加载不了"。

**三个 GUID**：`CLSID_MinimalIME`（COM 类）、`GUID_MinimalProfile`（语言配置）都在 `Globals.h`。正式发布前请换成自己生成的，别和别人撞车。

## 常见翻车点

| 现象 | 原因 / 处理 |
|---|---|
| `regsvr32` 报"找不到指定模块" | 位数不对，或用 Debug 版但缺 VC++ 运行库；先用 Release 版 |
| 注册成功但设置里没出现 | 没调 `InstallLayoutOrTip`，或没提权；手动「添加键盘」勾一次 |
| 记事本能打，Edge/设置里不能 | 类别没注册全；本例只注册了 `GUID_TFCAT_TIP_KEYBOARD`，要完整支持 UWP/沉浸式应用需按 SDK 情况补 `GUID_TFCAT_TIPCAP_IMMERSIVESUPPORT`、`GUID_TFCAT_TIPCAP_SYSTRAYSUPPORT`、`GUID_TFCAT_TIPCAP_UIELEMENTENABLED` |
| 32 位程序里打不了字 | 只装了 x64 DLL |
| 改了代码重新注册失败 | DLL 被进程占着；先 `regsvr32 /u`，必要时注销或重启 |
| 候选窗位置飘 | 本例用 `GetCaretPos` 近似定位；正式做法是在只读编辑会话里用 `ITfContextView::GetTextExt` 取组合串矩形 |

调试技巧：VS 里「附加到进程」选 `explorer.exe` 或目标程序，在 `OnKeyDown` / `ApplyState` 下断点。别在 `DllMain` 里下断点或调 COM。

## 完全撤销 / 回滚

**先吃颗定心丸**：这个输入法不替换系统文件、不写 `C:\Windows\System32`、不改启动项、不动系统自带输入法，只在注册表写三处登记（COM CLSID、TSF TIP、用户键盘列表条目）。DLL 跑在应用进程里，最坏情况是某个程序里打不了字或候选窗乱飘，**不会蓝屏、不会让整个输入系统瘫痪**。按 `Win + 空格` 切回微软拼音随时能正常打字。

### 三层回滚

**第 1 层 · 正常卸载（首选）**
右键 `uninstall.bat` → 以管理员身份运行。它调 `regsvr32 /u` → `DllUnregisterServer`，依次执行：`ILOT_UNINSTALL` 撤键盘列表 → `UnregisterCategory` → `Unregister`（连带删掉所有语言配置）→ 删除 COM CLSID 键。然后**注销或重启一次**，让 `ctfmon` 重新加载输入法列表。

若 DLL 是 32 位版，卸载也要用 32 位注册器：
```bat
%windir%\SysWOW64\regsvr32.exe /u Win32\Release\MinimalTSFIME.dll
```

**第 2 层 · 急救脚本（DLL 删了 / 损坏 / 被占用时用）**
右键 `rescue.bat` → 以管理员身份运行。它**不依赖 DLL 能加载**，直接做四件事：尝试正规反注册 → P/Invoke 调 `InstallLayoutOrTip(ILOT_UNINSTALL)` 撤键盘列表 → 删除 HKCR / HKLM / HKCU 下所有相关注册表键（含 WOW6432Node 视图）→ 提示切回微软拼音并重启。

**第 3 层 · 系统还原点（终极兜底）**
安装前建议建一个：
```powershell
Checkpoint-Computer -Description "Before_MinimalTSFIME" -RestorePointType "MODIFY_SETTINGS"
```
万一前两层都没清干净，回滚到安装前即可。

### 还能手动做的收尾

- `设置 → 时间和语言 → 语言和区域 → 中文(简体) → 语言选项 → 键盘`，若还留着「极简拼音」条目，点它删掉
- `设置 → 时间和语言 → 输入 → 高级键盘设置`，把「替代默认输入法」改回微软拼音
- 检查 `HKLM\SOFTWARE\Microsoft\CTF\TIP\` 下有没有 `{B7901AAF-...}` 开头的残留键，有就删

> 唯一要注意的：**改了代码想重新注册时，先卸载再编译**。DLL 被进程占着时 `regsvr32` 会失败，注销一次即可释放。

---

## 从这里走向"真输入法"

1. **换词库**：替换 `TextService.cpp` 的 `Convert()`，接 Rime / libpinyin / 自己的词库文件（注意 DLL 在应用容器里，读文件受权限限制）
2. **候选窗**：换成 `ITfCandidateListUIElement` + `ITfUIElementSink`，加 UI Automation（`IME_Candidate_Window`、元素选中事件），用 DirectWrite 渲染高分屏
3. **交互**：翻页、逐字选择、中英文标点、Shift 切换、临时英文、模式图标
4. **系统级补齐**：`ITfDisplayAttributeProvider`（组合串样式）、输入模式 compartment、系统托盘图标、`InstallLayoutOrTip` 的默认输入法处理
5. **工程化**：代码签名（微软明确建议）、提权的 MSI/EXE 安装器（不要纯 MSIX，运行时自注册会因权限失败）、崩溃上报、双架构并行安装、卸载清理

如果只是想快速做一个能用的输入法而不是练系统编程，建议直接在 **Rime / 小狼毫** 上做二次开发，上面大半的坑它已经替你踩过了。

---

### 说明

本工程已在 GitHub Actions（windows-2022 镜像 / VS2022 v143 / Windows 10.0.26100 SDK）上完成 **Release x64 与 Release Win32 双架构编译验证**，产物位于仓库 `dist/MinimalTSFIME-预编译包.zip`，也可在 Actions 的 Artifacts 里下载。建议先原样跑通注册流程，再动手改功能。
