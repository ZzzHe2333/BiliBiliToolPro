# 青龙部署

本项目支持通过青龙面板的 **订阅管理** 直接拉取。青龙专用任务使用 `Zzz-Bili` 名称和 `Zzz_*` 环境变量，并启用严格配置隔离。

仓库：

```text
https://github.com/ZzzHe2333/BiliBiliToolPro.git
```

## 1. 新建订阅

青龙面板：

```text
订阅管理 -> 新建订阅
```

推荐填写：

```text
名称：Zzz-BiliBiliToolPro
类型：公开仓库
链接：https://github.com/ZzzHe2333/BiliBiliToolPro.git
分支：main
定时类型：crontab
定时规则：2 2 * * *
白名单：zzz_bili_task_.+\.sh
文件后缀：sh
```

其余项目留空即可。新版青龙直接使用“文件后缀”字段；旧版如果仍使用 `RepoFileExtensions`，请确保其中包含 `sh`。

> 如果你此前使用旧文档中的 `2 2 28 * *`（每月更新一次），请手动执行一次该订阅，并把 Cron 改成 `2 2 * * *`。仓库中的修复无法反向更新一份长期未重新拉取的青龙本地副本。

保存后执行一次订阅，青龙会从：

```text
qinglong/SubscriptionTasks/
```

建立 `Zzz-Bili ...` 定时任务。

每日 02:02 刷新订阅的目的，是让安全修复、接口兼容修复和任务退出修复能在较短时间内进入青龙本地仓库；它只更新订阅仓库，不等于每天执行所有 BiliBiliToolPro 业务任务。

## 2. 当前订阅任务

当前会生成：

- `Zzz-Bili 每日任务`
- `Zzz-Bili 免费B币券充电任务`
- `Zzz-Bili 测试Cookie`
- `Zzz-Bili 扫码登录`
- `Zzz-Bili 直播粉丝牌`
- `Zzz-Bili 漫画任务`
- `Zzz-Bili 领取大会员漫画权益任务`
- `Zzz-Bili 大会员大积分`
- `Zzz-Bili 领取大会员福利任务`

以下功能代码仍保留，但本项目默认关闭，并且订阅目录**没有**对应定时任务脚本：

- `LiveLottery` 天选时刻
- `Silver2Coin` 银瓜子兑换硬币
- `UnfollowBatched` 批量取关

## 3. 严格配置隔离

订阅公共脚本会设置：

```bash
Zzz_IsolatedMode=true
```

在该模式下，Console 的业务配置只从 `Zzz_*` 环境变量加载，并跳过本地 `cookies.json`。因此：

- 不读取 `Ray_*` 业务配置；
- 不读取无前缀业务环境变量；
- 不读取 `cookies.json`；
- `DOTNET_*` 等系统运行时环境变量不受影响。

旧的 `Ray_*` 兼容逻辑只保留给非严格模式，不建议用于本项目青龙订阅。

## 4. Cookie 与多账号

使用：

```bash
Zzz_BiliBiliCookies__0=<COOKIE>
Zzz_BiliBiliCookies__1=<COOKIE>
Zzz_BiliBiliCookies__2=<COOKIE>
```

`Zzz-Bili 测试Cookie` 可以用于快速确认当前严格模式实际加载了几个账号。

扫码登录后如需自动写回青龙环境变量，在青龙应用设置中创建 OpenAPI Application，然后配置：

```text
Zzz_QingLongConfig__ClientId
Zzz_QingLongConfig__ClientSecret
```

扫码登录只维护 `Zzz_BiliBiliCookies__*`。

## 5. B 币券充电

全局配置：

```bash
Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
Zzz_ChargeTaskConfig__ChargeComment=""
```

按账号 UID 单独配置：

```bash
Zzz_ChargeTaskConfig__Accounts__<B站UID>__IsEnable=true
Zzz_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=18461303
```

当账号配置未指定目标时，会继承全局配置；目标为空或为 `-1` 时，本项目最终兜底 UID 为 `18461303`。

`Zzz_ChargeTaskConfig__ChargeComment` 留空时会优先调用一言 API；调用失败时回退到内置随机留言。

## 6. 默认 Cron

| 任务 | Cron |
| --- | --- |
| 每日任务 | `0 9 * * *` |
| 免费 B 币券充电 | `0 12 * * *` |
| 直播粉丝牌 | `5 0 * * *` |
| 漫画任务 | `0 14 * * *` |
| 漫画权益 | `0 15 * * *` |
| 大会员大积分 | `7 1 * * *` |
| 大会员福利 | `0 1 * * *` |
| 测试 Cookie | `0 0 1 1 *` |
| 扫码登录 | `0 0 1 1 *` |

青龙实际执行时间以面板中的定时任务 Cron 为准。

## 7. 运行模式

默认：

```bash
Zzz_BILI_MODE=dotnet
```

支持：

```text
dotnet
bilitool
```

`bilitool` 模式使用本仓库维护的 `fork-main` 滚动预发布。安装脚本会校验：

```text
当前订阅仓库 commit == fork-main 构建记录的 commit
```

不一致时拒绝运行旧二进制，避免源码已经更新而二进制仍停留在旧版本。

支持：

```text
linux-x64
linux-musl-x64
linux-arm64
linux-arm
linux-musl-arm64
```

## 8. 中国大陆网络

订阅任务默认**不改写**青龙容器的 apt/apk 软件源：

```bash
Zzz_BILI_USE_CN_MIRROR=false
```

原因是系统软件源属于整个青龙容器的全局状态，修改它可能影响同一面板中的其他任务。

如果所在网络确实需要国内软件源，可以显式开启：

```bash
Zzz_BILI_USE_CN_MIRROR=true
```

开启后，安装环境时会把容器的 Debian/Alpine 软件源切换到 USTC 镜像；这不是仅对 BiliTool 生效的局部设置。已有自定义源或同面板运行其他项目时，建议保持 `false`。

GitHub Release 下载代理：

```bash
Zzz_BILI_GITHUB_PROXY=""
```

项目不会写死第三方 GitHub 代理地址，请仅配置自己信任的代理。

## 9. 任务互斥、硬超时、版本诊断与通知

所有 `Zzz-Bili` 任务共享源码和构建目录，公共脚本会加互斥锁，避免多个 `dotnet run` 并发造成 PDB/程序集锁冲突。

主程序默认有 2 小时硬超时，防止网络、接口或进程异常导致青龙任务无限挂起：

```bash
Zzz_BILI_TASK_TIMEOUT_SECONDS=7200
```

该超时覆盖 .NET 启动、编译、随机休眠和实际业务执行全过程。超过上限后会先终止主程序，必要时再强制杀死，并以退出码 `124` 标记超时失败；随后立即释放任务互斥锁，再进入已有的限时通知流程。该值必须是正整数，非法值或 `0` 会回退到默认 7200 秒。

`dotnet` 模式会把构建阶段和业务运行阶段分开。构建成功时，NuGet/MSBuild 的 warning 不再刷入青龙任务日志；如果构建失败，则会输出完整构建诊断并保留原始失败退出码。真正的 BiliBiliToolPro 运行日志仍会实时显示。

每个任务启动时会打印当前青龙本地仓库的应用版本、分支和 commit。该信息只读取本地 Git 元数据，不访问网络；如果本地已有的 `origin/main` 引用与当前 HEAD 不一致，也会提示重新运行青龙订阅。需要注意：长期未执行订阅时，`origin/main` 本身也可能已经过期，因此判断是否已部署最新修复仍应以“重新执行订阅后看到的 commit”为准。

任务完成后优先使用青龙面板系统通知，并在发送前对 Cookie、Authorization、Token、ClientSecret 等常见敏感字段脱敏。为了避免重复推送，任务运行时会屏蔽应用自身 `Zzz_Serilog__WriteTo__*` 推送配置。通知模块运行在独立 Node 子进程中，并有单独的硬超时，避免通知 provider 残留 timer/socket 继续占住青龙任务。

## 10. 常见问题

### 订阅成功但没有生成任务

检查：

1. 文件后缀是否包含 `sh`；
2. 白名单是否为 `zzz_bili_task_.+\.sh`；
3. 订阅日志是否拉到了 `qinglong/SubscriptionTasks/`；
4. 青龙版本是否支持从 Shell 注释解析 cron / Env 元数据。

### 任务日志显示旧版本，或已修复的问题仍然出现

先到“订阅管理”手动执行 `Zzz-BiliBiliToolPro`，然后确认订阅 Cron 已改为：

```text
2 2 * * *
```

重新执行任意 `Zzz-Bili ...` 任务后，开头应能看到：

```text
[Zzz-Bili] 本地仓库版本：...; branch=...; commit=...
```

如果仍显示旧版本/旧 commit，说明青龙实际执行的仓库目录没有更新，应检查订阅日志和任务命令指向的仓库路径。

### 仍然加载到旧账号或旧推送配置

请确认运行的是 `Zzz-Bili ...` 任务，而不是历史 `bili...` 定时任务。严格模式只读取 `Zzz_*`。历史任务如仍留在青龙面板，应停用或删除。

### dotnet 安装失败

可以切换：

```bash
Zzz_BILI_MODE=bilitool
```

或检查包源、网络以及 `.NET 8` 运行环境。若确实需要脚本修改容器系统源，再显式设置 `Zzz_BILI_USE_CN_MIRROR=true`。

### Couldn't find a valid ICU package

可按运行环境需要设置：

```text
DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
```

### inotify instances reached

可按运行环境需要设置：

```text
DOTNET_USE_POLLING_FILE_WATCHER=1
```

## 11. 独立仓库说明

本仓库不配置 Repo Sync、Pull App 或其他自动上游同步机制。青龙订阅、滚动二进制和相关下载均指向 `ZzzHe2333/BiliBiliToolPro`。
