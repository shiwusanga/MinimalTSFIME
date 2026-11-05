# 没有编译环境怎么办

先说清楚一件事：**我这边是 Linux 沙盒，没有 MSVC 和 Windows SDK，没法直接给你编好的 DLL**。Windows 的 TSF 输入法必须用微软的编译器和 SDK 构建，MinGW 交叉编译也不建议（COM 接口、`msctf.h`、资源编译器、GUID 常量都容易缺胳膊少腿，翻车率很高）。

所以给你三条能真正拿到 DLL 的路，按省心程度排序。

---

## 方案 A：GitHub Actions 云端编译（推荐，免费，本地零安装）

工程里已经放好了 `.github/workflows/build.yml`，GitHub 的 Windows 镜像自带 VS2022 + Windows SDK，你只需要点几下。

1. 注册 / 登录 [github.com](https://github.com)，新建仓库（**公开仓库**编译时长免费）
2. 把 `极简TSF输入法工程.zip` 解压，把 `MinimalTSFIME` 文件夹里的**所有内容**上传到仓库根目录
   （注意别漏了 `.github` 这个文件夹，workflow 在里面）
3. 打开仓库的 **Actions** 标签 → 左侧选「编译 TSF 输入法 DLL」→ 点 **Run workflow**
4. 等 2～5 分钟，刷新页面，点进这次运行，最下面 **Artifacts** 下载 `MinimalTSFIME-预编译包`
   （也可以直接在仓库根目录的 `dist/` 文件夹里下载，这个不会像 Artifact 那样过期）
5. 解压到你的 Windows 电脑上，右键 `install.bat` → 以管理员身份运行

它会自动编好 x64 和 Win32 两份 DLL，连同安装/卸载脚本一起打包。你全程不需要装 Visual Studio。

> 国内访问 GitHub 若不稳定，可以把仓库导入 Gitee 后用其镜像流水线，或换个网络环境；也可以让有环境的朋友帮你跑一次这个 workflow。

---

## 方案 B：只装命令行生成工具（比完整 VS 轻）

如果你更想在自己电脑上编：

- 下载 **Visual Studio 2022 Build Tools**（免费，不是完整 IDE）
- 安装时只勾「**使用 C++ 的桌面开发**」，右侧确保含 **Windows 10/11 SDK**
- 约 1～3 GB，比完整 VS（10 GB+）轻很多
- 装完打开「Developer Command Prompt」，cd 到工程目录：

```bat
msbuild MinimalTSFIME.sln /p:Configuration=Release /p:Platform=x64
msbuild MinimalTSFIME.sln /p:Configuration=Release /p:Platform=x86
```

产物在 `x64\Release\` 和 `Win32\Release\`。当然，装完整 VS Community 2022 直接打开 `.sln` 也行，只是更占空间。

---

## 方案 C：临时云主机

按需开一台 Windows 云主机（腾讯云 / Azure / 阿里云，几毛到几块一小时），装好 Build Tools 编完把 DLL 下载回来，然后销毁机器。适合不想在本地装东西、也不想用 GitHub 的情况。

---

## 如果你其实只是想要一个能用的输入法

不用编译，直接装 [Rime / 小狼毫](https://rime.im) 就行。它是已经做好签名、安装包、双架构、卸载清理的成熟 TSF 输入法，还能自定义词库和方案。这份工程的价值在于**从零理解系统怎么认识一个输入法**，不在于替代现成产品。

---

## 两个提醒

- **签名**：CI 编出来的 DLL 没有代码签名。自己机器上注册使用没问题（SmartScreen 可能提示一次，选「仍要运行」）；要分发给别人就需要买代码签名证书，微软官方也明确建议 IME 二进制做签名。
- **先建还原点**：安装前顺手建一个，心里更踏实：

```powershell
# 管理员 PowerShell
Checkpoint-Computer -Description "Before_MinimalTSFIME" -RestorePointType "MODIFY_SETTINGS"
```

（需要系统保护已开启；没开也没关系，下面的急救脚本足够兜底。）

---

## 出问题怎么撤销

见 README 的「完全撤销 / 回滚」一节，或直接跑包里的 `rescue.bat`。三层保障：正常卸载 → 急救脚本 → 系统还原点。
