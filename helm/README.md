# Helm / Kubernetes 部署

Chart 位于：

```text
helm/bilibili-tool
```

默认镜像为本仓库维护的：

```text
ghcr.io/zzzhe2333/bili_tool_web:latest
```

## 安装

```bash
git clone https://github.com/ZzzHe2333/BiliBiliToolPro.git
cd BiliBiliToolPro/helm/bilibili-tool

helm install bilitool .
```

如需在安装前填写 Cookie：

```yaml
env:
  - name: BiliBiliCookies__1
    value: "<COOKIE>"
  - name: DailyTaskConfig__Cron
    value: "0 0 15 * * ?"
```

也可以不在 values 中放 Cookie，部署后通过 Web/扫码流程添加账号。

## 自定义镜像

```bash
helm install bilitool . \
  --set image.repository=ghcr.io/zzzhe2333/bili_tool_web \
  --set image.tag=latest
```

## 更新

```bash
git pull
helm upgrade bilitool ./helm/bilibili-tool
```

## 配置约定

Chart 运行的是 Web 项目，因此环境变量使用无前缀配置键：

```text
BiliBiliCookies__1
DailyTaskConfig__Cron
Security__RandomSleepMaxMin
```

不要把青龙专用 `Zzz_*` 严格隔离规则直接套到 Web Chart。

默认关闭的 `LiveLottery`、`Silver2Coin`、`UnfollowBatched` 不会因为使用 Helm 而自动开启。
