#!/usr/bin/env bash
# ZzzHe2333/BiliBiliToolPro 青龙订阅专用公共脚本。
# 本文件没有 cron/new Env 元数据，不会在订阅后生成独立定时任务。

set -e
set -u
set -o pipefail

QL_DIR=${QL_DIR:-"/ql"}
dir_repo=${dir_repo:-"$QL_DIR/data/repo"}
if [ ! -d "$dir_repo" ] && [ -d "$QL_DIR/repo" ]; then
  dir_repo="$QL_DIR/repo"
fi

fork_repo_name="ZzzHe2333_BiliBiliToolPro"
fork_repo_dir="$(find "$dir_repo" -type d \( -iname "$fork_repo_name" -o -iname "${fork_repo_name}_main" \) | head -1)"

if [ -z "$fork_repo_dir" ]; then
  echo "[Zzz-Bili] 未找到订阅仓库目录：ZzzHe2333/BiliBiliToolPro" >&2
  exit 1
fi

# 开启 fork 专用严格隔离模式：Console 只读取 Zzz_* 业务配置。
export Zzz_IsolatedMode=true

# fork 可单独设置运行模式/下载代理；未设置时兼容原 BILI_* 变量。
export BILI_MODE="${Zzz_BILI_MODE:-${BILI_MODE:-dotnet}}"
export BILI_GITHUB_PROXY="${Zzz_BILI_GITHUB_PROXY:-${BILI_GITHUB_PROXY:-}}"

# 复用项目已有安装、架构检测和运行环境逻辑。
. "$fork_repo_dir/qinglong/DefaultTasks/bili_task_base.sh"

# 订阅任务统一由青龙负责通知，因此执行 BiliTool 时移除 Zzz_ Serilog 的 WriteTo 环境覆盖，
# 避免面板系统通知成功后又被 Server酱/PushPlus/Webhook 等重复推送。
disable_bilitool_env_notifications() {
    local env_name
    while IFS='=' read -r env_name _; do
        case "$env_name" in
            Zzz_Serilog__WriteTo__*) unset "$env_name" ;;
        esac
    done < <(env)
}

# 通知优先级：
# 1. 青龙面板“系统设置 -> 通知设置”（gRPC systemNotify）
# 2. 如果 systemNotify 不可用或发送失败，回退到青龙 sendNotify.js 环境变量通知
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
const statusText = taskStatus === 0 ? '成功' : `失败(${taskStatus})`;
const title = `Zzz-Bili ${targetCode} - ${statusText}`;

function prepareContent() {
  let content = '';
  try {
    content = fs.readFileSync(logFile, 'utf8');
  } catch (error) {
    return `任务退出码：${taskStatus}\n读取任务日志失败：${error.message}`;
  }

  // 去掉 ANSI 控制字符，避免污染通知正文。
  content = content.replace(/\x1B(?:[@-Z\\-_]|\[[0-?]*[ -\/]*[@-~])/g, '');

  // dotnet run 可能先输出编译告警。正常启动后只推送 BiliTool 本身的运行日志；
  // 如果连程序都没启动成功，则保留完整输出，便于排查编译/启动错误。
  const marker = 'BiliBiliToolPro 开始运行...';
  const markerIndex = content.indexOf(marker);
  if (markerIndex >= 0) {
    const lineStart = content.lastIndexOf('\n', markerIndex);
    content = content.slice(lineStart >= 0 ? lineStart + 1 : markerIndex);
  }

  content = content.trim();
  if (!content) content = `任务执行结束，退出码：${taskStatus}`;

  // 部分通知渠道有正文长度限制；超长时保留开头和结尾，避免整条推送失败。
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

  // 独立子进程调用青龙 client.js。这样即使 client.js 初始化时直接 process.exit，
  // 主通知进程仍能继续执行环境变量兜底。
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

# 覆盖原 run_task：只写 Zzz_* 运行变量，避免同一青龙里的原版 Ray_* 配置冲突。
run_task() {
    local target_code=$1
    local log_file
    local task_status

    export Zzz_PlatformType=QingLong
    export Zzz_RunTasks="$target_code"

    disable_bilitool_env_notifications

    log_file="$(mktemp "/tmp/zzz-bili-${target_code}.XXXXXX.log")"
    cd "$qinglong_bili_repo_dir/src/Ray.BiliBiliTool.Console"

    # 即使主程序失败也先保留退出码并发送一次结果通知，最后再把原退出码返回给青龙。
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

    send_qinglong_notification "$target_code" "$log_file" "$task_status" || true
    rm -f "$log_file"

    return "$task_status"
}
