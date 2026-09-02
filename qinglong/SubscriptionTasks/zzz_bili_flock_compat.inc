#!/usr/bin/env bash
# Zzz-Bili 订阅入口的轻量兼容/默认层。
#
# 默认不改写青龙容器的 apt/apk 软件源。国内镜像会影响整个容器，而不仅是当前任务；
# 如确有需要，用户可显式设置 Zzz_BILI_USE_CN_MIRROR=true。
export Zzz_BILI_USE_CN_MIRROR="${Zzz_BILI_USE_CN_MIRROR:-false}"

# BusyBox flock 不支持 util-linux 的 -w/--timeout。
# 只有系统确实存在 flock 二进制时才定义兼容函数；否则不要用函数名掩盖“未安装”状态，
# 公共任务锁会自动回退到 mkdir 原子锁。
# - flock -w N FD -> flock -n FD + 1 秒轮询，最多 N 秒
# - 其他参数（如 flock -u FD）原样转发给系统 flock
if type -P flock >/dev/null 2>&1; then
    flock() {
        if [ "${1:-}" = "-w" ] || [ "${1:-}" = "--timeout" ]; then
            local timeout="${2:-0}"
            local fd="${3:-}"
            local elapsed=0

            case "$timeout" in
                ''|*[!0-9]*) timeout=0 ;;
            esac

            while true; do
                if command flock -n "$fd"; then
                    return 0
                fi

                if [ "$elapsed" -ge "$timeout" ]; then
                    return 1
                fi

                sleep 1
                elapsed=$((elapsed + 1))
            done
        fi

        command flock "$@"
    }
fi
