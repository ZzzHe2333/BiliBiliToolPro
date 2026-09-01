# 青龙共存部署说明

本文说明如何部署 `ZzzHe2333/BiliBiliToolPro`，同时避免与已经存在的 `RayWangQvQ/BiliBiliToolPro` 青龙任务发生冲突。

## 结论

如果旧版和本 fork 需要使用不同的 Cookie、充电目标、推送参数或其他 `Ray_*` 配置，推荐使用 **第二个青龙实例**。这是唯一能够同时隔离：

- 定时任务
- 环境变量
- Cookie
- 青龙 ClientId / ClientSecret
- 仓库目录
- 日志
- 运行时配置

的方案。

在同一个青龙面板中同时拉取两个仓库，仓库目录本身通常不会冲突，因为本 fork 的基础脚本按仓库所有者计算目录名，例如 `ZzzHe2333_BiliBiliToolPro`；但两个项目仍使用相同的 `Ray_BiliBiliCookies__*`、`Ray_ChargeTaskConfig__*` 等全局环境变量，而且任务脚本的显示名称也基本相同，因此不属于完全隔离。

---

## 方案 A：第二个青龙实例（推荐）

假设原青龙已经使用：

- 容器名：`qinglong`
- 端口：`5700`
- 数据目录：`./ql-data`

为本 fork 单独启动一个青龙：

```yaml
services:
  qinglong_bilitool_zzz:
    image: whyour/qinglong:latest
    container_name: qinglong-bilitool-zzz
    restart: unless-stopped
    ports:
      - "5701:5700"
    volumes:
      - ./ql-data-bilitool-zzz:/ql/data
    environment:
      TZ: Asia/Shanghai
```

启动：

```bash
docker compose up -d
```

之后访问新实例的 `5701` 端口。

### 新青龙中添加订阅

订阅管理中填写：

```text
名称：BiliTool-ZzzHe2333
类型：公开仓库
链接：https://github.com/ZzzHe2333/BiliBiliToolPro.git
定时类型：crontab
定时规则：2 2 28 * *
白名单：bili_task_.+\.sh
文件后缀：sh
```

或者使用拉库任务：

```bash
ql repo https://github.com/ZzzHe2333/BiliBiliToolPro.git "bili_task_"
```

### 环境变量

新实例中单独配置：

```bash
Ray_BiliBiliCookies__1=<COOKIE>
Ray_ChargeTaskConfig__IsEnable=true
Ray_ChargeTaskConfig__AutoChargeUpId=18461303
```

如果需要按 B 站 UID 单独设置充电目标：

```bash
Ray_ChargeTaskConfig__Accounts__<B站UID>__IsEnable=true
Ray_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=18461303
```

这样旧实例和新实例之间完全没有环境变量冲突。

---

## 方案 B：同一个青龙实例共存（仅适合共享配置）

可以在已有青龙中额外添加本 fork：

```text
名称：BiliTool-ZzzHe2333
链接：https://github.com/ZzzHe2333/BiliBiliToolPro.git
白名单：bili_task_.+\.sh
文件后缀：sh
```

本 fork 基础脚本中的仓库标识是：

```bash
bili_repo="ZzzHe2333/BiliBiliToolPro"
```

所以仓库目录会与原仓库的 `RayWangQvQ/BiliBiliToolPro` 分开。

但是必须注意：两个项目仍读取同一套青龙全局环境变量，例如：

```text
Ray_BiliBiliCookies__1
Ray_DailyTaskConfig__NumberOfCoins
Ray_ChargeTaskConfig__AutoChargeUpId
Ray_Security__RandomSleepMaxMin
```

因此，同一个青龙实例中：

- 如果两个项目需要使用完全相同的账号和配置，可以共存；
- 如果两个项目需要不同 Cookie 或不同配置，不建议共存；
- 同名 Bili 定时任务也容易在面板中混淆，建议停用旧项目对应任务，或者使用第二个青龙实例。

---

## B 币券充电任务时间

青龙的充电任务由 shell 脚本自己的 cron 控制，当前：

```text
0 12 * * *
```

即每天 12:00 执行一次 `Charge`。

`Ray_ChargeTaskConfig__Cron` 是 .NET 配置项，但不会改变青龙面板中 shell 任务本身的触发时间。若要修改青龙执行时间，应直接修改青龙面板中的定时任务 Cron。

---

## 充电留言

没有显式设置：

```text
Ray_ChargeTaskConfig__ChargeComment
```

时，充电成功后程序会请求：

```text
https://v1.hitokoto.cn/?c=a
```

读取 JSON 中的 `hitokoto` 字段作为留言。

请求超时、HTTP 错误、JSON 解析失败或返回空内容时，会自动从程序内置留言列表随机选择一句，不会因为一言 API 故障阻塞充电任务。

如果显式设置：

```bash
Ray_ChargeTaskConfig__ChargeComment="固定留言"
```

则优先使用固定留言，不请求一言 API。
