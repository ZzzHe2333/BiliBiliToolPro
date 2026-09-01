# 青龙通知策略

`qinglong/SubscriptionTasks/zzz_bili_task_*.sh` 的任务结果通知统一由青龙处理。

优先级：

1. **青龙面板系统通知**：调用青龙自带 `systemNotify`，使用“系统设置 -> 通知设置”中的配置。
2. **环境变量通知兜底**：如果 `systemNotify` 不存在、调用异常或返回失败，则调用青龙自带 `sendNotify.js`，读取青龙标准通知环境变量。

这样可以避免 BiliBiliToolPro 自己的 Serilog 推送与青龙系统通知同时发送造成重复通知。订阅任务运行时会忽略 `Zzz_Serilog__WriteTo__*` 环境覆盖，日志仍正常输出到青龙任务日志。

常见的青龙环境变量兜底配置包括：

```text
PUSH_KEY
PUSH_PLUS_TOKEN
TG_BOT_TOKEN
TG_USER_ID
QYWX_KEY
GOTIFY_URL
GOTIFY_TOKEN
WEBHOOK_URL
```

具体支持项由当前青龙版本自带的 `sendNotify.js` 决定。

## 敏感信息保护

通知发送前会强制脱敏 B 站 Cookie、青龙 Cookie 环境变量、Authorization、access token、refresh token、ClientSecret、二维码 `qrcode_key` 等敏感字段。脱敏发生在系统通知和环境变量兜底通知之前，因此两种通知路径都不会发送这些真实值。

扫码登录自动写回青龙失败时，程序默认也不再把完整 Cookie 输出到日志。如果确实需要手工复制 Cookie，可临时设置：

```text
Zzz_QingLongConfig__ShowCookieWhenSaveFails=true
```

然后重新扫码。此开关只用于临时排障，使用后应立即关闭或删除。即使显式开启，青龙通知正文仍会继续强制脱敏；完整值只可能出现在本地任务日志中。

通知正文来自本次任务输出。正常启动时会去掉 `dotnet run` 在程序启动前产生的编译信息，只保留 BiliBiliToolPro 运行日志；如果程序没有成功启动，则保留启动/编译错误用于排查。超长正文会保留开头和结尾并截断中间部分。

任务本身的退出码不会因为通知成功或失败而改变：BiliBiliToolPro 成功仍返回成功，BiliBiliToolPro 失败仍按原退出码返回给青龙。
