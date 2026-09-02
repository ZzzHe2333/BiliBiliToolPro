# 中国大陆 NuGet 恢复加速

本项目的青龙订阅会设置：

```text
Zzz_IsolatedMode=true
```

在该模式下，项目根目录 `Directory.Build.props` 默认额外加入以下 NuGet 恢复源：

```text
https://repo.huaweicloud.com/repository/nuget/v3/index.json
https://api.nuget.org/v3/index.json
```

华为云镜像用于改善中国大陆首次 `dotnet restore` / `dotnet run` 时的依赖下载体验；nuget.org 官方源仍然保留作为兜底。同时启用 `RestoreIgnoreFailedSources=true`，因此其中一个远程源临时不可访问时，只要其他来源能够提供所需包，恢复仍可继续。

## 关闭国内 NuGet 镜像

如果服务器位于境外、已有自己的 NuGet 配置，或者希望只使用系统默认源，在青龙环境变量中添加：

```bash
Zzz_BILI_USE_CN_NUGET_MIRROR=false
```

然后重新运行任务。

该设置仅在本项目的青龙隔离模式下生效。普通本地开发、Web/Docker 和 GitHub Actions 不会因为此文件自动切换到国内 NuGet 镜像。

## 与系统软件源的区别

`Zzz_BILI_USE_CN_MIRROR` 控制 Debian/Alpine 的 apt/apk 系统软件源。

`Zzz_BILI_USE_CN_NUGET_MIRROR` 控制 .NET 项目依赖的 NuGet 恢复源。

两者互相独立。当前推荐默认值是：

```bash
Zzz_BILI_USE_CN_MIRROR=false
Zzz_BILI_USE_CN_NUGET_MIRROR=true
```

原因是 NuGet 额外源只影响本项目依赖恢复，而修改 apt/apk 源会影响整个青龙容器。只有在明确希望本项目脚本帮助调整系统软件源时，才手工设置：

```bash
Zzz_BILI_USE_CN_MIRROR=true
```
