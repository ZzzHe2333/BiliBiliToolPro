#!/usr/bin/env bash
# ZzzHe2333/BiliBiliToolPro 青龙订阅专用公共脚本。
# 本文件是 .inc 内部依赖，不会在订阅后生成独立定时任务。

set -e
set -u
set -o pipefail

QL_DIR=${QL_DIR:-"/ql"}
dir_repo=${dir_repo:-"$QL_DIR/data/repo"}
if [ ! -d "$dir_repo" ] && [ -d "$QL_DIR/repo" ]; then
  dir_repo="$QL_DIR/repo"
fi

fork_repo_name="ZzzHe2333_BiliBiliToolPro"
fork_repo_dir="${BILI_REPO_DIR:-}"
if [ -z "$fork_repo_dir" ] || [ ! -d "$fork_repo_dir" ]; then
    if [ -d "$dir_repo/$fork_repo_name" ]; then
        fork_repo_dir="$dir_repo/$fork_repo_name"
    elif [ -d "$dir_repo/${fork_repo_name}_main" ]; then
        fork_repo_dir="$dir_repo/${fork_repo_name}_main"
    else
        fork_repo_dir="$(find "$dir_repo" -type d \( -iname "$fork_repo_name" -o -iname "${fork_repo_name}_main" \) | head -1)"
    fi
fi

if [ -z "$fork_repo_dir" ]; then
  echo "[Zzz-Bili] 未找到订阅仓库目录：ZzzHe2333/BiliBiliToolPro" >&2
  exit 1
fi

report_zzz_repo_revision() {
    local app_version="unknown"
    local local_branch="unknown"
    local local_commit="unknown"
    local cached_origin_main=""

    if [ -r "$fork_repo_dir/common.props" ]; then
        app_version="$(sed -n 's/.*<Version>\([^<][^<]*\)<\/Version>.*/\1/p' "$fork_repo_dir/common.props" | head -1)"
        [ -n "$app_version" ] || app_version="unknown"
    fi

    if command -v git >/dev/null 2>&1 && [ -d "$fork_repo_dir/.git" ]; then
        local_commit="$(git -C "$fork_repo_dir" rev-parse --verify HEAD 2>/dev/null || printf 'unknown')"
        local_branch="$(git -C "$fork_repo_dir" symbolic-ref --short -q HEAD 2>/dev/null || printf 'detached')"
        cached_origin_main="$(git -C "$fork_repo_dir" rev-parse --verify refs/remotes/origin/main 2>/dev/null || true)"
    fi

    echo "[Zzz-Bili] 本地仓库版本：${app_version}; branch=${local_branch}; commit=${local_commit}"

    # 这里只比较本地已有的 origin/main 引用，不访问网络，也不修改工作树。
    # 若订阅长时间没有执行，cached origin/main 本身也可能已经过期，因此该提示只是辅助诊断。
    if [ -n "$cached_origin_main" ] && [ "$local_commit" != "unknown" ] && [ "$local_commit" != "$cached_origin_main" ]; then
        echo "[Zzz-Bili] 注意：本地 HEAD 与已缓存 origin/main 不一致；请重新运行青龙订阅更新仓库" >&2
    fi
}

report_zzz_repo_revision

export Zzz_IsolatedMode=true

# 本项目青龙 Shell 辅助变量也使用 Zzz_ 命名空间，不继承通用 BILI_*，
# 避免同一个青龙中其他脚本的运行模式、代理或软件源设置串入本项目。
export BILI_MODE="${Zzz_BILI_MODE:-dotnet}"
export BILI_GITHUB_PROXY="${Zzz_BILI_GITHUB_PROXY:-}"
export BILI_USE_CN_MIRROR="${Zzz_BILI_USE_CN_MIRROR:-false}"

# 同一个青龙容器中的 Zzz-Bili 任务共享同一套源码、obj/bin 和 B站账号配置。
# 必须串行执行，否则多个 dotnet run 并发时会出现 PDB/程序集文件锁，也会同时请求 B站接口。
zzz_lock_fd=""
zzz_lock_dir=""
zzz_lock_owned=false

release_zzz_task_lock() {
    if [ -n "${zzz_lock_fd:-}" ]; then
        flock -u "$zzz_lock_fd" 2>/dev/null || true
        eval "exec ${zzz_lock_fd}>&-" 2>/dev/null || true
        zzz_lock_fd=""
    fi

    if [ "${zzz_lock_owned:-false}" = "true" ] && [ -n "${zzz_lock_dir:-}" ]; then
        local owner_pid=""
        if [ -r "$zzz_lock_dir/pid" ]; then
            read -r owner_pid < "$zzz_lock_dir/pid" || true
        fi
        if [ -z "$owner_pid" ] || [ "$owner_pid" = "$$" ]; then
            rm -rf "$zzz_lock_dir"
        fi
        zzz_lock_owned=false
    fi
}

acquire_zzz_task_lock() {
    local wait_seconds="${Zzz_BILI_LOCK_WAIT_SECONDS:-3600}"
    local elapsed=0
    local lock_file="/tmp/zzz-bilibili-tool.lock"

    case "$wait_seconds" in
        ''|*[!0-9]*)
            echo "[Zzz-Bili] Zzz_BILI_LOCK_WAIT_SECONDS 非有效非负整数，使用默认 3600 秒" >&2
            wait_seconds=3600
            ;;
    esac

    # 优先使用 flock；入口兼容层会把 BusyBox 不支持的 -w 转换为非阻塞轮询。
    if command -v flock >/dev/null 2>&1; then
        exec {zzz_lock_fd}>"$lock_file"
        echo "[Zzz-Bili] 等待任务互斥锁，最长 ${wait_seconds} 秒..."
        if flock -w "$wait_seconds" "$zzz_lock_fd"; then
            echo "[Zzz-Bili] 已获得任务互斥锁"
            return 0
        fi

        echo "[Zzz-Bili] 等待其他 Zzz-Bili 任务结束超时（${wait_seconds} 秒），本次不并发执行" >&2
        eval "exec ${zzz_lock_fd}>&-" 2>/dev/null || true
        zzz_lock_fd=""
        return 75
    fi

    # 系统如果没有 flock，使用原子 mkdir 作为兼容方案。
    zzz_lock_dir="${lock_file}.d"
    echo "[Zzz-Bili] 系统没有 flock，使用 mkdir 兼容锁；最长等待 ${wait_seconds} 秒"

    while ! mkdir "$zzz_lock_dir" 2>/dev/null; do
        local owner_pid=""
        if [ -r "$zzz_lock_dir/pid" ]; then
            read -r owner_pid < "$zzz_lock_dir/pid" || true
        fi

        # 上一次任务异常退出后自动清理失效锁。
        if [ -n "$owner_pid" ]; then
            case "$owner_pid" in
                *[!0-9]*) owner_pid="" ;;
            esac
        fi
        if [ -n "$owner_pid" ] && ! kill -0 "$owner_pid" 2>/dev/null; then
            echo "[Zzz-Bili] 检测到失效任务锁（PID $owner_pid），自动清理"
            rm -rf "$zzz_lock_dir"
            continue
        fi

        # mkdir 成功到写 pid 之间只有极短窗口；若目录长期没有 pid，也按失效锁处理。
        if [ -z "$owner_pid" ] && [ "$elapsed" -ge 10 ]; then
            echo "[Zzz-Bili] 检测到无 PID 的失效任务锁，自动清理"
            rm -rf "$zzz_lock_dir"
            continue
        fi

        if [ "$elapsed" -ge "$wait_seconds" ]; then
            echo "[Zzz-Bili] 等待其他 Zzz-Bili 任务结束超时（${wait_seconds} 秒），本次不并发执行" >&2
            return 75
        fi

        if [ $((elapsed % 30)) -eq 0 ]; then
            echo "[Zzz-Bili] 另一个 Zzz-Bili 任务仍在运行，已等待 ${elapsed} 秒..."
        fi
        sleep 5
        elapsed=$((elapsed + 5))
    done

    printf '%s\n' "$$" > "$zzz_lock_dir/pid"
    zzz_lock_owned=true
    echo "[Zzz-Bili] 已获得任务互斥锁（兼容模式）"
    return 0
}

if acquire_zzz_task_lock; then
    trap 'release_zzz_task_lock' EXIT
    trap 'release_zzz_task_lock; exit 130' INT
    trap 'release_zzz_task_lock; exit 143' TERM
else
    lock_status=$?
    exit "$lock_status"
fi

# 先清理本仓库废弃/重复定时任务，再安装/检查运行环境。
. "$fork_repo_dir/qinglong/DefaultTasks/bili_task_cleanup.inc"
. "$fork_repo_dir/qinglong/DefaultTasks/bili_task_base.inc"

disable_bilitool_env_notifications() {
    local env_name
    while IFS='=' read -r env_name _; do
        case "$env_name" in
            Zzz_Serilog__WriteTo__*) unset "$env_name" ;;
        esac
    done < <(env)
}

send_qinglong_notification() {
    local target_code=$1
    local log_file=$2
    local task_status=$3
    local notify_helper="$fork_repo_dir/qinglong/SubscriptionTasks/zzz_bili_notify.js"

    if ! command -v node >/dev/null 2>&1; then
        echo "[Zzz-Bili] 未找到 node，无法调用青龙通知接口" >&2
        return 0
    fi

    if [ ! -f "$notify_helper" ]; then
        echo "[Zzz-Bili] 未找到通知辅助脚本：$notify_helper" >&2
        return 0
    fi

    # 通知模块在独立 Node 进程内运行，并由辅助脚本负责硬超时和显式退出。
    # 即使 sendNotify.js 残留 timer/socket，也不能继续占住青龙定时任务。
    node "$notify_helper" "$target_code" "$log_file" "$task_status" || true
}

run_task() {
    local target_code=$1
    local log_file
    local task_status
    local logical_status

    export Zzz_PlatformType=QingLong
    export Zzz_RunTasks="$target_code"

    disable_bilitool_env_notifications

    log_file="$(mktemp "/tmp/zzz-bili-${target_code}.XXXXXX.log")"
    cd "$qinglong_bili_repo_dir/src/Ray.BiliBiliTool.Console"

    set +e
    if [ "$prefer_mode" == "dotnet" ]; then
        dotnet run --ENVIRONMENT=Production 2>&1 | tee "$log_file"
        task_status=${PIPESTATUS[0]}
    else
        cp -f "$qinglong_bili_repo_dir/bin/Ray.BiliBiliTool.Console" .
        chmod +x ./Ray.BiliBiliTool.Console
        ./Ray.BiliBiliTool.Console --ENVIRONMENT=Production 2>&1 | tee "$log_file"
        task_status=${PIPESTATUS[0]}
    fi
    set -e

    # 主程序结束后立即释放锁，通知发送不占用 BiliTool 的执行槽位。
    release_zzz_task_lock
    trap - EXIT INT TERM

    logical_status=$task_status
    if [ "$task_status" -eq 0 ] && grep -Eq '\[[^]]* (ERR|FTL)\]' "$log_file"; then
        logical_status=2
        echo "[Zzz-Bili] 主进程退出码为0，但检测到 Error/Fatal 级日志，本次任务标记为部分失败"
    fi

    send_qinglong_notification "$target_code" "$log_file" "$logical_status" || true
    rm -f "$log_file"

    # 青龙 otask.sh 会在函数/eval 上下文执行任务。Bash 5.2 在 errexit 开启时，
    # 若 eval 中的函数以非零状态返回，会触发 pop_var_context 内部诊断并干扰收尾。
    # 仅在需要返回失败/部分失败时关闭 errexit，让 Qinglong 正常接收退出状态。
    if [ "$logical_status" -ne 0 ]; then
        set +e
    fi
    return "$logical_status"
}
