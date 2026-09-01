#!/usr/bin/env bash
set -euo pipefail

BASE_DIR="${BILI_TOOL_HOME:-${PWD}/bili_tool_web}"
RAW_BASE="${BILI_TOOL_RAW_BASE:-https://raw.githubusercontent.com/ZzzHe2333/BiliBiliToolPro/main}"
BILI_TOOL_IMAGE="${BILI_TOOL_IMAGE:-ghcr.io/zzzhe2333/bili_tool_web:latest}"
export BILI_TOOL_IMAGE

log() {
    printf '[BiliTool] %s\n' "$*"
}

download() {
    local url="$1"
    local output="$2"

    if command -v curl >/dev/null 2>&1; then
        curl -fL --retry 5 --retry-delay 2 --connect-timeout 15 "$url" -o "$output"
    elif command -v wget >/dev/null 2>&1; then
        wget --tries=5 --timeout=15 -O "$output" "$url"
    else
        echo "需要 curl 或 wget 才能下载安装文件。" >&2
        exit 1
    fi
}

install_docker_if_needed() {
    if command -v docker >/dev/null 2>&1; then
        return 0
    fi

    log "未检测到 Docker，尝试使用 get.docker.com 安装"
    local installer
    installer="$(mktemp)"
    download "https://get.docker.com" "$installer"
    sh "$installer"
    rm -f "$installer"
}

prepare_files() {
    mkdir -p "$BASE_DIR/config" "$BASE_DIR/Logs"

    if [ ! -f "$BASE_DIR/docker-compose.yml" ]; then
        download "$RAW_BASE/docker/sample/docker-compose.yml" "$BASE_DIR/docker-compose.yml"
    fi

    if [ ! -f "$BASE_DIR/config/cookies.json" ]; then
        download "$RAW_BASE/docker/sample/config/cookies.json" "$BASE_DIR/config/cookies.json"
    fi
}

start_container() {
    cd "$BASE_DIR"
    log "使用镜像：$BILI_TOOL_IMAGE"

    if docker compose version >/dev/null 2>&1; then
        docker compose pull
        docker compose up -d
    elif command -v docker-compose >/dev/null 2>&1; then
        docker-compose pull
        docker-compose up -d
    else
        echo "Docker 已安装，但未检测到 docker compose / docker-compose。" >&2
        exit 1
    fi

    log "容器已启动"
    docker ps --filter 'name=bili_tool_web'
    log "Web 默认地址：http://<服务器IP>:22330"
}

main() {
    install_docker_if_needed
    prepare_files
    start_container
}

main "$@"
