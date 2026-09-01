# BiliBiliPro Kubectl Plugin

## Prerequisites

- Kubernetes >= v1.23
- Go >= v1.18（仅构建插件时需要）
- kubectl
- krew（如需通过 krew 管理插件）

## 构建 / 安装插件

```bash
cd krew
make deploy
```

将生成的 `bilibilipro` 插件放到 `PATH` 中即可。

## Deployment

```bash
kubectl bilipro init --config config.yaml
```

可选参数：

```text
--image=ghcr.io/zzzhe2333/bili_tool_web:latest
--namespace=bilipro
--image-pull-secret=<secret>
--login
```

配置文件使用 Web 项目的无前缀环境变量：

```yaml
- name: BiliBiliCookies__1
  value: "<COOKIE>"
- name: DailyTaskConfig__Cron
  value: "0 0 15 * * ?"
```

本仓库不再使用其他账号下的历史容器镜像。

## Delete

```bash
kubectl bilipro delete --namespace=bilipro
```

## Version

```bash
kubectl bilipro version
```
