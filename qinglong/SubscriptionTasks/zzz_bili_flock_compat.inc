#!/usr/bin/env bash
# BusyBox flock 不支持 util-linux 的 -w/--timeout。
# 公共任务锁使用 flock -w，因此在任务入口处提供统一兼容层：
# - flock -w N FD -> flock -n FD + 1 秒轮询，最多 N 秒
# - 其他参数（如 flock -u FD）原样转发给系统 flock

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
