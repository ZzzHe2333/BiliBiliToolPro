# 配置说明

本文档以当前 `ZzzHe2333/BiliBiliToolPro` 独立仓库的实际代码为准。历史版本中的 `DayOfAutoCharge`、`DayOfReceiveVipPrivilege`、`DailyTaskConfig.AutoChargeUpId` 等配置已经废弃，不应继续使用。

## 1. 运行环境与配置方式

当前项目目标框架为 **.NET 8**。

配置可以来自：

1. `appsettings.json` / `appsettings.{Environment}.json`；
2. 环境变量；
3. 命令行参数；
4. 非青龙隔离模式下的 `cookies.json`（仅 Cookie）。

普通 Console 模式为了兼容历史配置，会读取无前缀、`Ray_` 和 `Zzz_` 环境变量；同一个配置键冲突时 `Zzz_` 后加载，优先级更高。

本仓库的青龙订阅任务会自动开启 `Zzz_IsolatedMode=true`，此时只读取 `Zzz_*` 业务环境变量，并且不加载 `cookies.json`，用于避免与同一面板中的旧配置串号。

Web/Docker 配置通常不需要前缀。腾讯云 SCF 示例也开启严格隔离，因此 SCF 示例使用 `Zzz_*`。

### 环境变量命名规则

.NET 配置中的冒号 `:` 在环境变量中写成双下划线 `__`。

例如：

```text
DailyTaskConfig:NumberOfCoins
```

对应 Web/Docker：

```bash
DailyTaskConfig__NumberOfCoins=3
```

对应本仓库青龙订阅 / 严格隔离 Console：

```bash
Zzz_DailyTaskConfig__NumberOfCoins=3
```

## 2. 青龙推荐配置

青龙部署说明见：`qinglong/README.md`。

Cookie：

```bash
Zzz_BiliBiliCookies__1=<COOKIE>
Zzz_BiliBiliCookies__2=<COOKIE>
```

不要把真实 Cookie 提交到 Git 仓库。

本仓库扫码登录自动写回青龙时，也只使用：

```text
Zzz_BiliBiliCookies__0
Zzz_BiliBiliCookies__1
Zzz_BiliBiliCookies__2
...
```

如需扫码后自动保存 Cookie，需要配置青龙 Application：

```bash
Zzz_QingLongConfig__ClientId=<CLIENT_ID>
Zzz_QingLongConfig__ClientSecret=<CLIENT_SECRET>
```

青龙订阅任务的通知默认优先使用面板“系统设置 -> 通知设置”，失败后再回退到青龙标准环境变量通知。详见 `qinglong/NOTIFICATION.md`。

## 3. 每日任务

主要配置：

| 配置键 | 默认值 | 说明 |
| --- | --- | --- |
| `DailyTaskConfig__IsEnable` | `true` | 是否启用每日任务 |
| `DailyTaskConfig__IsWatchVideo` | `true` | 是否执行观看视频 |
| `DailyTaskConfig__IsShareVideo` | `true` | 是否执行分享视频 |
| `DailyTaskConfig__IsDonateCoinForArticle` | `false` | 是否对专栏投币 |
| `DailyTaskConfig__NumberOfCoins` | `5` | 每日目标投币数，程序最大限制为 5 |
| `DailyTaskConfig__NumberOfProtectedCoins` | `0` | 希望保留的硬币余额 |
| `DailyTaskConfig__SaveCoinsWhenLv6` | `true` | 白嫖模式；Lv6 账号默认跳过投币 |
| `DailyTaskConfig__SelectLike` | `true` | 投币时是否同时点赞 |
| `DailyTaskConfig__SupportUpIds` | 空 | 优先支持的 UP UID，多个以英文逗号分隔 |
| `DailyTaskConfig__DevicePlatform` | `android` | 客户端操作平台 |

`SupportUpIds` 留空或配置为 `-1` 表示没有指定优先 UP，程序会继续从关注列表、排行榜等来源选择视频。

### 白嫖模式

当前默认：

```bash
Zzz_DailyTaskConfig__SaveCoinsWhenLv6=true
```

因此 Lv6 账号默认不会为了每日经验继续投币。

如确实需要 Lv6 继续投币：

```bash
Zzz_DailyTaskConfig__SaveCoinsWhenLv6=false
```

## 4. 安全与请求间隔

| 配置键 | Console 配置默认值 | 说明 |
| --- | ---: | --- |
| `Security__IsSkipDailyTask` | `false` | 全局跳过任务 |
| `Security__RandomSleepMaxMin` | `30` | 随机睡眠最大分钟数，0 表示关闭 |
| `Security__IntervalSecondsBetweenRequestApi` | `20` | 对指定 HTTP 方法的最短请求间隔 |
| `Security__IntervalMethodTypes` | `GET,POST` | 需要应用间隔的方法 |
| `Security__WebProxy` | 空 | HTTP 代理 |

青龙示例：

```bash
Zzz_Security__RandomSleepMaxMin=30
Zzz_Security__IntervalSecondsBetweenRequestApi=20
Zzz_Security__IntervalMethodTypes=GET,POST
```

代理使用：

```bash
Zzz_Security__WebProxy=http://host:port
```

如代理需要账号密码，请按实际代理软件支持的 URL 格式填写，避免把代理密码提交到仓库。

## 5. 免费 B 币券充电

主要配置：

```bash
Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
Zzz_ChargeTaskConfig__ChargeComment=""
```

### 充电目标

目标选择顺序：

1. 账号级 `AutoChargeUpId`；
2. 全局 `AutoChargeUpId`；
3. fallback UID `18461303`。

**空值或 `-1` 都表示使用 fallback UID `18461303`，不再表示“给自己充电”。**

账号级示例：

```bash
Zzz_ChargeTaskConfig__Accounts__<B站UID>__IsEnable=true
Zzz_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=18461303
```

账号键使用真实 B 站 UID，不是 Cookie 顺序编号。

### 充电留言

如果 `ChargeComment` 明确配置了非空值，则使用该值。

如果留空：

1. 尝试从一言 API 获取留言；
2. 请求失败、超时或返回无效内容时，使用程序内置随机留言。

### 临期保护

本仓库不会因为充电检查任务每天运行就每天消费 B 币券。

程序在成功领取月度 B 币券时记录领取时间，并按 `领取成功时间 + 30 天` 计算预计到期时间：

- 距预计到期超过 5 天：不提醒、不充电；
- 到期前 5 天内：有余额则发临期提醒；
- 到期前最后 48 小时：余额不少于 2 时才尝试自动充电；
- 没有可信领取记录：宁可不自动消费，也不会猜测到期时间。

详细说明见 `docs/bcoin-expiry-guard.md`。

## 6. 其他任务

下表是**本仓库最终生效的默认状态**。基础 `appsettings.json` 为保留历史兼容仍含部分旧默认值，但 `appsettings.ForkDefaults.json` 会在 Console/Web 启动时覆盖三个高风险任务为关闭状态。

| 配置节 | 本仓库有效默认状态 | 主要默认值 |
| --- | --- | --- |
| `MangaTaskConfig` | 开启 | `CustomComicId=27355`, `CustomEpId=381662` |
| `MangaPrivilegeTaskConfig` | 开启 | - |
| `Silver2CoinTaskConfig` | **关闭** | - |
| `VipPrivilegeConfig` | 开启 | - |
| `VipBigPointConfig` | 开启 | `ViewBangumis=33378` |
| `LiveLotteryTaskConfig` | **关闭** | 奖品过滤规则仍保留 |
| `LiveFansMedalTaskConfig` | 开启 | 心跳 70 分钟，默认跳过 20 级及以上粉丝牌 |
| `UnfollowBatchedTaskConfig` | **关闭** | 分组 `天选时刻`，每次 20 个 |

需要显式启用时使用：

```text
<Section>__IsEnable=true
```

青龙严格模式则加 `Zzz_` 前缀。青龙订阅目录本身不提供 `LiveLottery`、`Silver2Coin`、`UnfollowBatched` 的定时入口。

## 7. Cron / 定时任务

### 青龙

青龙实际执行时间由面板“定时任务”中的 Shell Cron 决定，**不是** `Zzz_*TaskConfig__Cron`。

因此例如修改 B 币券检查时间，应直接修改：

```text
Zzz-Bili 免费B币券充电任务
```

对应的青龙 Cron。

订阅脚本默认时间见 `qinglong/README.md`。

### Web / Quartz

Web 使用配置节中的 Quartz Cron，例如：

```bash
ChargeTaskConfig__Cron="0 0 12 * * ?"
```

当前 Web 默认每天 12:00 检查 B 币券临期状态；只有进入最后 48 小时且满足条件时才实际消费。

## 8. 青龙运行选项

本仓库青龙专用变量：

```bash
Zzz_BILI_MODE=dotnet
Zzz_BILI_GITHUB_PROXY=""
Zzz_BILI_USE_CN_MIRROR=false
Zzz_BILI_LOCK_WAIT_SECONDS=3600
```

说明：

- `Zzz_BILI_MODE`：`dotnet` 或 `bilitool`，默认 `dotnet`；
- `Zzz_BILI_GITHUB_PROXY`：访问 GitHub Release 的可选代理前缀；
- `Zzz_BILI_USE_CN_MIRROR`：默认 `false`。设为 `true` 后安装脚本会改写整个青龙容器的 apt/apk 软件源，因此只应在明确需要时开启；
- `Zzz_BILI_LOCK_WAIT_SECONDS`：同一青龙容器中等待其他 Zzz-Bili 任务结束的最长秒数，默认 3600。

`bilitool` 模式使用本仓库自己的 `fork-main` 滚动构建，并校验二进制构建 commit 与订阅仓库 HEAD，避免运行旧代码。

## 9. 当前有效命令行参数

命令行参数并不是主要推荐配置方式。当前映射以 `src/Ray.BiliBiliTool.Config/Constants.cs` 中的 `CommandLineMappingsDic` 为准。

常用示例：

```bash
dotnet run -- --runTasks=Daily --numberOfCoins=3 --autoChargeUpId=18461303
```

其中已修正：

```text
--autoChargeUpId -> ChargeTaskConfig:AutoChargeUpId
--isExchangeSilver2Coin -> Silver2CoinTaskConfig:IsEnable
--proxy -> Security:WebProxy
```

以下旧参数已经废弃：

```text
--dayOfAutoCharge
--dayOfReceiveVipPrivilege
```

继续传入时程序会输出明确警告。任务日期/时间请改对应 Cron。

## 10. 通知

青龙订阅部署请优先使用青龙面板系统通知，不建议再为本仓库重复配置一套 Serilog Server酱/PushPlus/Webhook，否则容易产生重复推送。

青龙通知策略、环境变量兜底和敏感信息脱敏见：

```text
qinglong/NOTIFICATION.md
```

Web 或普通 Console 如需直接使用 Serilog 通知 Sink，可参考对应 `appsettings.json` 中的 `Serilog.WriteTo` 配置。

## 11. 配置排查原则

遇到“我明明配置了但没生效”时，按以下顺序检查：

1. 青龙订阅是否使用 `Zzz_` 前缀；
2. 配置层级是否用双下划线 `__`；
3. 是否存在更高优先级的同名配置；
4. 青龙定时问题是否错误地修改了 `TaskConfig__Cron`，而没有修改面板 Shell Cron；
5. 多账号充电的 `Accounts__<UID>` 是否填了真实 B 站 UID；
6. 是否仍在使用本文明确标记为废弃的历史字段；
7. 是否误以为 `Zzz_BILI_USE_CN_MIRROR` 只影响当前任务——它开启时会修改整个青龙容器的软件源。
