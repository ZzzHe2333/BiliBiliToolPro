# 常见问题

本文档按当前 `ZzzHe2333/BiliBiliToolPro` 的实际维护方式整理。

## 1. 这个仓库会自动同步其他仓库吗？

不会。

当前仓库是独立仓库，不属于其他仓库的 fork network，也不配置 Repo Sync、Pull App、hard reset 或定时拉取其他仓库的工作流。

如果需要引入外部代码，应通过普通分支和 Pull Request 人工审查，而不是自动覆盖 `main`。

## 2. 青龙应该用什么环境变量？

本项目青龙订阅任务使用严格模式，只读取 `Zzz_*` 业务配置。

Cookie 示例：

```text
Zzz_BiliBiliCookies__0
Zzz_BiliBiliCookies__1
Zzz_BiliBiliCookies__2
```

充电示例：

```text
Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
```

不要给新的 `Zzz-Bili` 任务继续新增 `Ray_*` Cookie。`Ray_*` 仅作为普通非隔离 Console 模式的历史兼容逻辑保留。

## 3. 为什么青龙显示的账号数量比我配置的多？

优先检查你运行的是不是历史 `bili...` 任务。

正确的订阅任务名称以：

```text
Zzz-Bili
```

开头。严格模式下不会读取 `Ray_*`、无前缀业务环境变量或 `cookies.json`。

可以运行：

```text
Zzz-Bili 测试Cookie
```

确认实际加载账号数量。

## 4. 为什么会出现旧的 Server 酱或自定义推送？

新的 `Zzz-Bili` 青龙任务由公共脚本统一收集日志并调用青龙通知，同时会屏蔽应用自身的 `Zzz_Serilog__WriteTo__*`，避免重复推送。

如果日志仍出现不属于当前配置的旧推送，通常说明运行的是旧任务脚本或旧仓库副本。

## 5. 青龙 dotnet 安装失败怎么办？

程序要求 `.NET 8`。

青龙默认运行模式：

```bash
Zzz_BILI_MODE=dotnet
```

可以切换到本仓库自包含滚动二进制：

```bash
Zzz_BILI_MODE=bilitool
```

默认不会改写青龙容器的系统软件源：

```bash
Zzz_BILI_USE_CN_MIRROR=false
```

如果网络环境确实需要，可以显式设为 `true`。注意：开启后修改的是整个青龙容器的 apt/apk 源，同一面板中的其他任务也会受到影响。

## 6. bilitool 为什么提示 commit 不一致？

这是保护机制。

`bilitool` 会比较当前订阅仓库 commit 与 `fork-main` 滚动预发布记录的构建 commit。两者不一致时会拒绝运行旧二进制，等待 GitHub Actions 完成与当前源码匹配的新构建。

## 7. Docker / Podman / Helm 拉哪个镜像？

本项目维护的镜像是：

```text
ghcr.io/zzzhe2333/bili_tool_web:latest
```

Docker、Podman 和 Helm 示例均使用这个镜像，而不是其他账号下的历史镜像。仓库中的旧 Krew 插件已经移除，因为它绑定早期 Console 容器架构。

## 8. Docker / Web 环境变量需要 `Zzz_` 前缀吗？

不需要。

Web 容器使用标准无前缀配置键，例如：

```text
BiliBiliCookies__1
DailyTaskConfig__Cron
Security__RandomSleepMaxMin
```

`Zzz_*` 严格隔离主要用于青龙 Console 订阅任务，以及仓库提供的腾讯云 SCF 严格隔离示例。

## 9. 为什么天选时刻、银瓜子兑换、批量取关没有青龙任务？

这是本项目的默认策略。

以下三个功能保留代码，但最终有效默认状态为关闭，并且不在 `qinglong/SubscriptionTasks/` 中提供定时任务入口：

```text
LiveLottery
Silver2Coin
UnfollowBatched
```

基础 `appsettings.json` 中保留的历史 `true` 值会被本仓库的 `appsettings.ForkDefaults.json` 覆盖。如确实需要，可以通过显式配置和手工运行方式启用。

## 10. B 币券充电到谁？

全局目标：

```text
Zzz_ChargeTaskConfig__AutoChargeUpId=<UP UID>
```

按账号 UID：

```text
Zzz_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=<UP UID>
```

账号配置优先于全局配置。目标为空或为 `-1` 时，本项目最终兜底 UID 为 `18461303`。

## 11. 充电留言如何设置？

显式设置：

```text
Zzz_ChargeTaskConfig__ChargeComment=你的留言
```

留空时会尝试从一言 API 获取一句话，失败后使用内置随机留言。

## 12. 本地运行需要什么环境？

源码或 framework-dependent 包需要 `.NET 8`。自包含 Release 不需要单独安装对应 Runtime。

正式版本下载请从本仓库 Releases 获取。`fork-main` 是青龙 `bilitool` 模式使用的滚动预发布，不建议当作普通桌面/服务器正式版本下载入口。

当前正式发布包命名规则见 [本地运行说明](runInLocal.md)。

## 13. 为什么镜像标签和版本标签不能随便复用？

正式版本号必须唯一对应一个 Git commit。

本仓库现在的规则是：

- `main` 普通代码更新只发布 `latest` 和 `main-<commit>` 镜像标签；
- 正式版本标签（例如 `3.8.3`）只由正式 Release 流程创建；
- 如果同名 Git tag 已经指向其他 commit，正式发布会直接失败，要求先提升版本号；
- 正式 Release 完成后再发布同版本的容器镜像。

这样可以避免“Git 标签指向旧代码，但同名容器标签已经变成新代码”的来源错位。

## 14. 出现 B 站 `-352` 怎么办？

`-352` 通常是接口风险控制响应。HTTP 状态码可能仍然是 200，但业务 JSON 中 `code` 为负数。

遇到时应优先：

1. 查看当前版本是否已有接口适配；
2. 降低短时间请求频率；
3. 检查 User-Agent、WBI 或请求指纹相关配置；
4. 不要把 `-352` 响应当成包含正常 `data` 的成功响应反序列化。

## 15. 如何提交代码修改？

从本仓库创建分支，完成修改后向 `main` 提交 Pull Request。不要通过自动上游同步工作流覆盖 `main`。
