# Docker 部署

本项目 Docker 镜像由当前仓库的 GitHub Actions 构建并发布到：

```text
ghcr.io/zzzhe2333/bili_tool_web:latest
```

不再使用其他账号或其他仓库维护的历史镜像。

## 一键安装

需要 `curl` 或 `wget`：

```bash
bash <(curl -fsSL https://raw.githubusercontent.com/ZzzHe2333/BiliBiliToolPro/main/docker/install.sh)
```

默认创建：

```text
./bili_tool_web/
├── Logs/
├── config/
│   └── cookies.json
└── docker-compose.yml
```

可选环境变量：

```bash
BILI_TOOL_HOME=/opt/bili_tool_web
BILI_TOOL_IMAGE=ghcr.io/zzzhe2333/bili_tool_web:latest
```

## Docker Compose

```bash
mkdir -p bili_tool_web/config bili_tool_web/Logs
cd bili_tool_web

curl -fLO https://raw.githubusercontent.com/ZzzHe2333/BiliBiliToolPro/main/docker/sample/docker-compose.yml
curl -fL https://raw.githubusercontent.com/ZzzHe2333/BiliBiliToolPro/main/docker/sample/config/cookies.json -o config/cookies.json

docker compose pull
docker compose up -d
```

查看日志：

```bash
docker logs -f bili_tool_web
```

更新：

```bash
docker compose pull && docker compose up -d
```

## 直接 docker run

```bash
docker pull ghcr.io/zzzhe2333/bili_tool_web:latest

docker run -d \
  --name bili_tool_web \
  --restart unless-stopped \
  -p 22330:8080 \
  -e TZ=Asia/Shanghai \
  -e DailyTaskConfig__Cron="0 0 15 * * ?" \
  -v "$PWD/Logs:/app/Logs" \
  -v "$PWD/config:/app/config" \
  ghcr.io/zzzhe2333/bili_tool_web:latest
```

## 配置

Docker 镜像运行的是 Web 项目，环境变量使用**无前缀**配置键，例如：

```text
BiliBiliCookies__1
DailyTaskConfig__Cron
Security__RandomSleepMaxMin
```

青龙专用的 `Zzz_*` 严格隔离规则不适用于 Web 容器。

也可以通过 Web 界面扫码添加账号。默认登录信息请以当前项目配置为准，首次登录后应立即修改默认密码。

## 自己构建

在仓库根目录：

```bash
docker build -t ghcr.io/zzzhe2333/bili_tool_web:local .
```

镜像基于 `.NET 8` 构建和运行。
