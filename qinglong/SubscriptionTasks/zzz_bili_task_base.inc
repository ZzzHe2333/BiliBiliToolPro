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

export Zzz_IsolatedMode=true

# fork 专用运行/网络选项。
export BILI_MODE="${Zzz_BILI_MODE:-${BILI_MODE:-dotnet}}"
export BILI_GITHUB_PROXY="${Zzz_BILI_GITHUB_PROXY:-${BILI_GITHUB_PROXY:-}}"
export BILI_USE_CN_MIRROR="${Zzz_BILI_USE_CN_MIRROR:-${BILI_USE_CN_MIRROR:-true}}"

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

    # 优先使用 flock；大部分完整 Linux 环境都有。flock 会在进程退出时自动释放。
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

    # Alpine/精简镜像如果没有 flock，使用原子 mkdir 作为兼容方案。
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

    if ! command -v node >/dev/null 2>&1; then
        echo "[Zzz-Bili] 未找到 node，无法调用青龙通知接口" >&2
        return 0
    fi

    node - "$target_code" "$log_file" "$task_status" <<'NODE'
const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const targetCode = process.argv[2] || 'Task';
const logFile = process.argv[3];
const taskStatus = Number(process.argv[4] || 0);
const qlDir = process.env.QL_DIR || '/ql';
const qlDataDir = (process.env.QL_DATA_DIR || path.join(qlDir, 'data')).replace(/\/$/, '');
const statusText = taskStatus === 0 ? '成功' : taskStatus === 2 ? '部分失败' : `失败(${taskStatus})`;
const title = `Zzz-Bili ${targetCode} - ${statusText}`;

function redactSensitiveContent(content) {
  // B站 Cookie 单字段：兼容裸值、单/双引号赋值以及 JSON 序列化格式。
  content = content.replace(
    /\b(SESSDATA|bili_jct|DedeUserID__ckMd5|DedeUserID|sid|buvid3|buvid4|buvid_fp|buvid_fp_plain|b_nut|b_lsid|_uuid|ac_time_value|access_key|bili_ticket)\s*=\s*(?:"[^"]*"|'[^']*'|[^;\s"'<>]+)/gi,
    '$1=[已隐藏]'
  );
  content = content.replace(
    /("(?:SESSDATA|bili_jct|DedeUserID__ckMd5|DedeUserID|sid|buvid3|buvid4|buvid_fp|buvid_fp_plain|b_nut|b_lsid|_uuid|ac_time_value|access_key|bili_ticket)"\s*:\s*")[^"]*(")/gi,
    '$1[已隐藏]$2'
  );

  // 整条 Cookie / Authorization 以及本 fork 的 Cookie 环境变量。
  content = content.replace(/(Zzz_BiliBiliCookies__\d+\s*[:=]\s*)[^\r\n]+/gi, '$1[已隐藏]');
  content = content.replace(/((?:Cookie|Set-Cookie|Authorization)\s*[:=]\s*)[^\r\n]+/gi, '$1[已隐藏]');
  content = content.replace(
    /("(?:Cookie|Set-Cookie|Authorization)"\s*:\s*")[^"]*(")/gi,
    '$1[已隐藏]$2'
  );

  // 推送、青龙 OpenAPI、扫码登录等常见凭据，兼容 JSON、key=value 和 URL 查询参数。
  content = content.replace(
    /("(?:access_token|refresh_token|client_secret|ClientSecret|ClientId|qrcode_key|readkey|scKey|turboScKey|botToken|sKey|secret|token|webhook|webHookUrl|apiKey|apikey)"\s*:\s*")[^"]*(")/gi,
    '$1[已隐藏]$2'
  );
  content = content.replace(
    /\b((?:access_token|refresh_token|client_secret|ClientSecret|ClientId|qrcode_key|readkey|scKey|turboScKey|botToken|sKey|secret|token|webhook|webHookUrl|apiKey|apikey)\s*[:=]\s*)(?:"[^"]*"|'[^']*'|[^&\s]+)/gi,
    '$1[已隐藏]'
  );

  return content;
}

function prepareContent() {
  let content = '';
  try {
    content = fs.readFileSync(logFile, 'utf8');
  } catch (error) {
    return `任务退出码：${taskStatus}\n读取任务日志失败：${error.message}`;
  }

  content = content.replace(/\x1B(?:[@-Z\\-_]|\[[0-?]*[ -\/]*[@-~])/g, '');

  const marker = 'BiliBiliToolPro 开始运行...';
  const markerIndex = content.indexOf(marker);
  if (markerIndex >= 0) {
    const lineStart = content.lastIndexOf('\n', markerIndex);
    content = content.slice(lineStart >= 0 ? lineStart + 1 : markerIndex);
  }

  content = redactSensitiveContent(content);

  content = content.trim();
  if (!content) content = `任务执行结束，退出码：${taskStatus}`;

  const maxLength = 16000;
  if (content.length > maxLength) {
    const half = Math.floor((maxLength - 80) / 2);
    content = `${content.slice(0, half)}\n\n……通知正文过长，中间已省略……\n\n${content.slice(-half)}`;
  }
  return content;
}

function trySystemNotify(content) {
  const clientPath = path.join(qlDir, 'shell/preload/client.js');
  if (!fs.existsSync(clientPath)) {
    console.warn(`[Zzz-Bili] 青龙 systemNotify 客户端不存在：${clientPath}`);
    return false;
  }

  const childCode = `
const fs = require('fs');
(async () => {
  const payload = JSON.parse(fs.readFileSync(0, 'utf8'));
  const api = require(${JSON.stringify(clientPath)});
  const result = await api.systemNotify(payload);
  if (typeof api.close === 'function') api.close();
  process.stdout.write(JSON.stringify(result || {}));
  process.exit(Number(result?.code) === 200 ? 0 : 2);
})().catch((error) => {
  console.error(error?.message || String(error));
  process.exit(1);
});`;

  const result = spawnSync(process.execPath, ['-e', childCode], {
    input: JSON.stringify({ title, content }),
    encoding: 'utf8',
    timeout: 35000,
    env: process.env,
  });

  if (result.status === 0) {
    console.log('[Zzz-Bili] 已使用青龙面板系统通知发送结果');
    return true;
  }

  let detail = (result.stdout || result.stderr || '').trim();
  try {
    const response = JSON.parse(result.stdout || '{}');
    detail = `code=${response.code ?? 'unknown'}, message=${response.message ?? ''}`;
  } catch (_) {}
  console.warn(`[Zzz-Bili] 青龙面板系统通知失败：${detail || `exit=${result.status ?? 'unknown'}`}`);
  return false;
}

async function tryEnvNotify(content) {
  const candidates = [
    path.join(qlDataDir, 'scripts/sendNotify.js'),
    path.join(qlDir, 'scripts/sendNotify.js'),
  ];
  const notifyPath = candidates.find((item) => fs.existsSync(item));

  if (!notifyPath) {
    console.warn('[Zzz-Bili] 未找到青龙 sendNotify.js，无法使用环境变量通知兜底');
    return false;
  }

  try {
    const notifyModule = require(notifyPath);
    const sendNotify = notifyModule.sendNotify || notifyModule.send;
    if (typeof sendNotify !== 'function') {
      console.warn(`[Zzz-Bili] ${notifyPath} 未导出 sendNotify/send`);
      return false;
    }

    await sendNotify(title, content);
    console.log('[Zzz-Bili] 已回退到青龙环境变量通知流程');
    return true;
  } catch (error) {
    console.warn(`[Zzz-Bili] 青龙环境变量通知失败：${error.message}`);
    return false;
  }
}

(async () => {
  const content = prepareContent();
  if (trySystemNotify(content)) return;
  await tryEnvNotify(content);
})().catch((error) => {
  console.warn(`[Zzz-Bili] 通知处理异常：${error.message}`);
});
NODE
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

    return "$logical_status"
}
