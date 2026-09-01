# 青龙共存部署说明

本文说明如何在 **同一个青龙面板** 中同时使用：

- 原版 `RayWangQvQ/BiliBiliToolPro`
- 本 fork `ZzzHe2333/BiliBiliToolPro`

本 fork 已增加独立的订阅任务与 `Zzz_` 配置前缀，因此不再要求必须启动第二个青龙实例。

## 1. 隔离规则

建议严格按下面的前缀区分：

```text
原版：Ray_*
本 fork：Zzz_*
```

例如：

```text
原版 Cookie：Ray_BiliBiliCookies__1
本 fork Cookie：Zzz_BiliBiliCookies__1

原版充电目标：Ray_ChargeTaskConfig__AutoChargeUpId
本 fork充电目标：Zzz_ChargeTaskConfig__AutoChargeUpId
```

本 fork 的 Console 仍兼容 `Ray_` 和无前缀变量，但 `Zzz_` 最后加载，因此 `Zzz_` 优先级最高。

扫码登录后的 Cookie 持久化也已经单独改为：

```text
Zzz_BiliBiliCookies__0
Zzz_BiliBiliCookies__1
Zzz_BiliBiliCookies__2
...
```

不会修改原版的 `Ray_BiliBiliCookies__*`。

## 2. 用订阅管理直接拉取本 fork

进入：

```text
青龙面板 -> 订阅管理 -> 新建订阅
```

填写：

```text
名称：Zzz-BiliBiliToolPro
类型：公开仓库
链接：https://github.com/ZzzHe2333/BiliBiliToolPro.git
分支：main
定时类型：crontab
定时规则：2 2 28 * *
白名单：zzz_bili_task_.+\.sh
文件后缀：sh
```

未提到的字段留空即可。

保存后直接运行订阅，不需要另外执行：

```text
ql repo ...
```

订阅会从：

```text
qinglong/SubscriptionTasks/
```

拉取本 fork 专用脚本。

任务名称均带：

```text
Zzz-Bili
```

所以在同一个定时任务列表中也能与原版任务区分。

## 3. 本 fork 环境变量示例

```bash
Zzz_BiliBiliCookies__1=<COOKIE>

Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
Zzz_ChargeTaskConfig__ChargeComment=""

Zzz_Security__RandomSleepMaxMin=0
```

按 B 站 UID 单独控制充电：

```bash
Zzz_ChargeTaskConfig__Accounts__<B站UID>__IsEnable=true
Zzz_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=18461303
```

如果原版也在同一面板中，原版继续使用自己的 `Ray_*` 即可。

## 4. 青龙 ClientId / ClientSecret

本 fork 使用：

```text
Zzz_QingLongConfig__ClientId
Zzz_QingLongConfig__ClientSecret
```

原版继续使用：

```text
Ray_QingLongConfig__ClientId
Ray_QingLongConfig__ClientSecret
```

两套配置可以同时存在。

## 5. 运行模式也可以分别设置

本 fork：

```bash
Zzz_BILI_MODE=dotnet
Zzz_BILI_GITHUB_PROXY=""
```

如果没有设置 `Zzz_BILI_MODE`，本 fork 会兼容读取普通 `BILI_MODE`。

## 6. B 币券充电

本 fork 当前最终兜底充电 UID：

```text
18461303
```

充电任务的青龙 cron：

```text
0 12 * * *
```

即每天 12:00 运行一次；最终执行时间可以直接在青龙面板中修改。

未显式配置：

```text
Zzz_ChargeTaskConfig__ChargeComment
```

时，充电成功后会请求：

```text
https://v1.hitokoto.cn/?c=a
```

读取 `hitokoto` 作为留言；请求失败则回退内置随机留言。

## 7. 是否还需要第二个青龙实例

通常不需要。

当前本 fork 已经隔离：

- 仓库目录：按 `ZzzHe2333/BiliBiliToolPro` 区分
- 定时任务：使用 `Zzz-Bili ...` 名称
- 业务环境变量：使用 `Zzz_*`
- Cookie：使用 `Zzz_BiliBiliCookies__*`
- 青龙 OpenAPI 凭据：使用 `Zzz_QingLongConfig__*`

如果还需要连系统级变量、网络代理、容器资源、日志目录都完全隔离，再使用第二个青龙容器即可；普通共存场景可以直接使用同一个青龙面板。
