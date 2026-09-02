# 下载程序包到本地或服务器运行

正式程序包统一从当前仓库 Releases 获取：

```text
https://github.com/ZzzHe2333/BiliBiliToolPro/releases
```

> `fork-main` 是青龙 `bilitool` 模式使用的滚动预发布，不是普通本地运行的正式版本。

## 1. 已安装 .NET 8：framework-dependent 包

如果系统已经安装 `.NET 8` Runtime，可以下载：

```text
bilibili-tool-pro-v<VERSION>-dotnet-dependent.zip
```

解压后进入 `dotnet-dependent` 目录，执行：

```bash
dotnet ./Ray.BiliBiliTool.Console.dll --runTasks=Login
```

扫码登录后即可运行其他任务，例如：

```bash
dotnet ./Ray.BiliBiliTool.Console.dll --runTasks=Daily
```

## 2. Windows 自包含包

无需单独安装 .NET Runtime，根据架构下载：

```text
bilibili-tool-pro-v<VERSION>-win-x64.zip
bilibili-tool-pro-v<VERSION>-win-x86.zip
bilibili-tool-pro-v<VERSION>-win-arm64.zip
```

常见 64 位 Intel/AMD Windows 使用 `win-x64`。

解压后进入对应运行时目录，例如：

```powershell
cd .\win-x64
.\Ray.BiliBiliTool.Console.exe --runTasks=Login
```

后续可使用 Windows 任务计划程序定时运行。

## 3. Linux 自包含包

根据系统和 CPU 选择：

```text
bilibili-tool-pro-v<VERSION>-linux-x64.zip
bilibili-tool-pro-v<VERSION>-linux-musl-x64.zip
bilibili-tool-pro-v<VERSION>-linux-arm64.zip
bilibili-tool-pro-v<VERSION>-linux-musl-arm64.zip
bilibili-tool-pro-v<VERSION>-linux-arm.zip
```

一般：

- Debian / Ubuntu x86_64：`linux-x64`
- Alpine x86_64：`linux-musl-x64`
- Debian / Ubuntu ARM64：`linux-arm64`
- Alpine ARM64：`linux-musl-arm64`

示例，假设当前正式版本为 `<VERSION>`：

```bash
wget https://github.com/ZzzHe2333/BiliBiliToolPro/releases/download/<VERSION>/bilibili-tool-pro-v<VERSION>-linux-x64.zip
unzip bilibili-tool-pro-v<VERSION>-linux-x64.zip
cd linux-x64
chmod +x Ray.BiliBiliTool.Console
./Ray.BiliBiliTool.Console --runTasks=Login
```

不要把 `<VERSION>` 原样复制；请替换为 Releases 页面显示的正式版本号。

## 4. macOS

当前正式发布脚本提供 Intel macOS 包：

```text
bilibili-tool-pro-v<VERSION>-osx-x64.zip
```

解压后：

```bash
cd osx-x64
chmod +x Ray.BiliBiliTool.Console
./Ray.BiliBiliTool.Console --runTasks=Login
```

当前正式脚本没有单独发布 `osx-arm64` 包。Apple Silicon 用户如需原生 ARM64 包，应从源码自行发布；不要把 `osx-x64` 误认为 ARM64 原生构建。

## 5. 从源码运行

需要 .NET 8 SDK：

```bash
git clone https://github.com/ZzzHe2333/BiliBiliToolPro.git
cd BiliBiliToolPro/src/Ray.BiliBiliTool.Console
dotnet run -- --runTasks=Login
```

## 6. 配置

普通本地 Console 可以使用 `appsettings.json`、环境变量、命令行以及 `cookies.json`。

环境变量兼容无前缀、`Ray_` 和 `Zzz_`；新的青龙订阅才会自动启用严格 `Zzz_*` 隔离。

详细配置见：

```text
docs/configuration.md
```
