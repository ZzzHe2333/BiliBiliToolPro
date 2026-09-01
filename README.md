![2233](docs/imgs/2233.png)

<div align="center">

# BiliTool

[![GitHub Stars](https://img.shields.io/github/stars/ZzzHe2333/BiliBiliToolPro?style=flat-square)](https://github.com/ZzzHe2333/BiliBiliToolPro/stargazers)
[![GitHub Forks](https://img.shields.io/github/forks/ZzzHe2333/BiliBiliToolPro?style=flat-square)](https://github.com/ZzzHe2333/BiliBiliToolPro/network)
[![GitHub All Releases](https://img.shields.io/github/downloads/ZzzHe2333/BiliBiliToolPro/total?style=flat-square)](https://github.com/ZzzHe2333/BiliBiliToolPro/releases)
[![GitHub Release](https://img.shields.io/github/v/release/ZzzHe2333/BiliBiliToolPro?style=flat-square)](https://github.com/ZzzHe2333/BiliBiliToolPro/releases)
[![GitHub License](https://img.shields.io/github/license/ZzzHe2333/BiliBiliToolPro?style=flat-square)](LICENSE)

</div>

BiliTool 是一个自动执行 B 站日常任务的开源工具，支持青龙、Docker/Podman、Kubernetes/Helm、本地运行等部署方式。

> **独立维护说明**：`ZzzHe2333/BiliBiliToolPro` 当前是独立仓库，不属于其他仓库的 fork network，也不会自动拉取、合并或硬重置到其他代码库。所有发布包、青龙滚动二进制和容器镜像均以本仓库为准。

## 主要功能

- 扫码登录并维护 Cookie
- 每日任务：登录、观看、分享、投币等
- 直播粉丝牌任务
- 漫画签到与阅读
- 大会员漫画权益
- 大会员福利领取
- 大会员大积分任务
- 免费 B 币券充电，支持全局或按账号指定目标 UP
- B 币券临期保护与提醒
- 多账号
- 日志与通知推送

以下功能代码仍保留，但**本项目默认关闭，青龙订阅也不创建对应定时任务**：

- `LiveLottery`：天选时刻
- `Silver2Coin`：银瓜子兑换硬币
- `UnfollowBatched`：批量取关

如确有需要，可以通过显式配置自行启用。

## 部署

| 方式 | 文档 | 默认来源 |
| --- | --- | --- |
| 青龙 | [qinglong/README.md](qinglong/README.md) | 本仓库源码 / `fork-main` 滚动二进制 |
| Docker | [docker/README.md](docker/README.md) | `ghcr.io/zzzhe2333/bili_tool_web:latest` |
| Podman | [podman/README.md](podman/README.md) | `ghcr.io/zzzhe2333/bili_tool_web:latest` |
| Helm / Kubernetes | [helm/README.md](helm/README.md) | `ghcr.io/zzzhe2333/bili_tool_web:latest` |
| 本地 / 服务器 | [docs/runInLocal.md](docs/runInLocal.md) | 本仓库 Releases |

### 青龙推荐配置

青龙订阅任务启用严格隔离模式，只读取 `Zzz_*` 业务配置，并且不会读取本地 `cookies.json`：

```bash
Zzz_BiliBiliCookies__1=<COOKIE>
Zzz_BiliBiliCookies__2=<COOKIE>

Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
Zzz_ChargeTaskConfig__ChargeComment=""
```

按 B 站账号 UID 单独控制充电：

```bash
Zzz_ChargeTaskConfig__Accounts__<B站UID>__IsEnable=true
Zzz_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=18461303
```

扫码登录自动写回青龙时，可配置：

```text
Zzz_QingLongConfig__ClientId
Zzz_QingLongConfig__ClientSecret
```

更多说明见 [青龙部署文档](qinglong/README.md)。

### Docker / Podman / Web 配置

容器运行的是 Web 项目，环境变量使用标准无前缀配置键，例如：

```yaml
BiliBiliCookies__1: <COOKIE>
DailyTaskConfig__Cron: "0 0 15 * * ?"
```

`Zzz_*` 严格隔离约定主要用于本项目的青龙 Console 订阅任务。

## 任务说明

| 任务 | Code | 默认状态 | 建议频率 |
| --- | --- | --- | --- |
| 扫码登录 | `Login` | 可用 | 手动 |
| 测试 Cookie | `Test` | 可用 | 手动 |
| 每日任务 | `Daily` | 开启 | 每天一次 |
| 免费 B 币券充电 | `Charge` | 开启 | 每天检查 |
| 直播粉丝牌 | `LiveFansMedal` | 开启 | 每天一次 |
| 漫画任务 | `Manga` | 开启 | 每天一次 |
| 大会员漫画权益 | `MangaPrivilege` | 开启 | 每天一次 |
| 大会员大积分 | `VipBigPoint` | 开启 | 每天一次 |
| 大会员福利 | `VipPrivilege` | 开启 | 每天一次 |
| 天选时刻 | `LiveLottery` | 默认关闭 | 按需 |
| 银瓜子兑换硬币 | `Silver2Coin` | 默认关闭 | 按需 |
| 批量取关 | `UnfollowBatched` | 默认关闭 | 手动 |

## 多账号

青龙使用：

```text
Zzz_BiliBiliCookies__0
Zzz_BiliBiliCookies__1
Zzz_BiliBiliCookies__2
...
```

其他平台可以使用标准配置键或 `cookies.json`：

```json
{
  "BiliBiliCookies": [
    "cookie1",
    "cookie2"
  ]
}
```

## 配置与排错

- [配置说明](docs/configuration.md)
- [常见问题](docs/questions.md)
- [本地运行](docs/runInLocal.md)

## 发布与更新

- 正式程序包：本仓库 [Releases](https://github.com/ZzzHe2333/BiliBiliToolPro/releases)
- 青龙 `bilitool` 模式：使用本仓库 `fork-main` 滚动预发布，并校验构建 commit 与订阅仓库 commit 一致
- 容器镜像：`ghcr.io/zzzhe2333/bili_tool_web:latest`

本仓库不会通过 Repo Sync、Pull App 或其他自动机制同步其他仓库。

## 贡献

可以从本仓库创建分支并向 `main` 提交 Pull Request。提交前建议确认改动不会破坏：

- `Zzz_*` 青龙配置隔离
- 本仓库 Release / GHCR 发布链路
- B 币券保护逻辑
- 默认关闭任务策略
- 日志敏感信息脱敏

## License

项目按 [GNU GPL v3](LICENSE) 发布。项目由历史 BiliBiliToolPro 代码持续演进，原有版权、作者信息和许可证声明继续保留。
