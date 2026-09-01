# Podman 部署

本项目统一使用当前仓库维护的 GHCR 镜像：

```text
ghcr.io/zzzhe2333/bili_tool_web:latest
```

## 运行

```bash
mkdir -p bili_tool_web/Logs bili_tool_web/config
cd bili_tool_web

curl -fL https://raw.githubusercontent.com/ZzzHe2333/BiliBiliToolPro/main/docker/sample/config/cookies.json -o config/cookies.json

podman pull ghcr.io/zzzhe2333/bili_tool_web:latest
podman run -itd \
  --name bili_tool_web \
  -p 22330:8080 \
  -v "$PWD/Logs:/app/Logs" \
  -v "$PWD/config:/app/config" \
  -e TZ=Asia/Shanghai \
  -e DailyTaskConfig__Cron="0 0 15 * * ?" \
  ghcr.io/zzzhe2333/bili_tool_web:latest
```

查看日志：

```bash
podman logs -f bili_tool_web
```

更新：

```bash
podman pull ghcr.io/zzzhe2333/bili_tool_web:latest
podman rm -f bili_tool_web
# 然后重新执行上面的 podman run
```

## 配置

Podman 与 Docker 一样运行 Web 项目，使用无前缀环境变量：

```text
BiliBiliCookies__1
DailyTaskConfig__Cron
Security__IntervalSecondsBetweenRequestApi
```

`LiveLottery`、`Silver2Coin`、`UnfollowBatched` 在本项目中默认关闭，不再在示例里主动开启。

## 自己构建

```bash
podman build -t ghcr.io/zzzhe2333/bili_tool_web:local .
```
