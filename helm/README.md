# Helm / Kubernetes 部署

当前 Chart 直接部署本仓库维护的 Web 镜像，不再使用早期 Console + 容器内 cron 脚本架构。

默认镜像：

```text
ghcr.io/zzzhe2333/bili_tool_web:latest
```

Chart：

```text
helm/bilibili-tool
```

## 安装

```bash
git clone https://github.com/ZzzHe2333/BiliBiliToolPro.git
cd BiliBiliToolPro
helm install bilitool ./helm/bilibili-tool
```

默认创建：

- 1 个 Web Deployment
- 1 个 `ClusterIP` Service
- 容器端口 `8080`

本地访问示例：

```bash
kubectl port-forward service/bilitool-bilibili-tool 22330:8080
```

如果你设置了 `fullnameOverride` 或 Release 名不同，请以 `kubectl get service` 显示的 Service 名称为准。

## 配置账号

推荐部署后通过 Web 扫码添加账号；也可以通过你自己的 values 文件传入环境变量：

```yaml
env:
  - name: TZ
    value: "Asia/Shanghai"
  - name: BiliBiliCookies__1
    value: "<COOKIE>"
  - name: DailyTaskConfig__Cron
    value: "0 0 15 * * ?"
```

不要把真实 Cookie 提交到 Git 仓库。

Web 项目使用无前缀配置键，例如：

```text
BiliBiliCookies__1
DailyTaskConfig__Cron
Security__RandomSleepMaxMin
```

青龙专用的 `Zzz_*` 严格隔离约定不适用于 Web Chart。

## 对外暴露服务

默认：

```yaml
service:
  type: ClusterIP
  port: 8080
```

可以按集群环境改成 `NodePort` 或 `LoadBalancer`：

```bash
helm upgrade --install bilitool ./helm/bilibili-tool \
  --set service.type=LoadBalancer
```

## 持久化

默认不绑定宿主机目录。需要保存日志/配置时，可在自己的 values 文件中启用：

```yaml
persistence:
  logs:
    enabled: true
    hostPath: /srv/bilitool/logs
  config:
    enabled: true
    hostPath: /srv/bilitool/config
```

挂载位置分别为：

```text
/app/Logs
/app/config
```

## 更新

```bash
git pull
helm upgrade bilitool ./helm/bilibili-tool
```

默认 `image.pullPolicy=Always`，使用 `latest` 时会拉取本仓库最新 Web 镜像。

## 默认关闭任务

`LiveLottery`、`Silver2Coin`、`UnfollowBatched` 在程序默认配置中关闭。当前 Helm Chart 不再自行生成任何 Console cron，因此不会额外把这些功能打开。

## 关于旧 Krew 插件

仓库曾包含一套 `kubectl bilipro` Krew 插件，它绑定早期 Console 容器并直接在 Pod 中执行 `Ray.BiliBiliTool.Console.dll`。当前正式镜像已经是 Web 架构，因此该旧插件已从仓库移除。Kubernetes 部署统一使用本 Helm Chart。
