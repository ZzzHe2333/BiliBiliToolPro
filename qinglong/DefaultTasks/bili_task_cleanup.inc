#!/usr/bin/env bash
# ZzzHe2333/BiliBiliToolPro 青龙定时任务清理库。
# .inc 文件只作为内部依赖，不应被青龙订阅识别为独立定时任务。

cleanup_bili_qinglong_tasks() {
    if [[ "${Zzz_BILI_SKIP_CRON_CLEANUP:-false}" == "true" || "${Zzz_BILI_SKIP_CRON_CLEANUP:-false}" == "1" ]]; then
        return 0
    fi

    if ! command -v node >/dev/null 2>&1; then
        echo "[Zzz-Bili] 跳过定时任务清理：未找到 node" >&2
        return 0
    fi

    local ql_dir="${QL_DIR:-/ql}"
    local client_path="$ql_dir/shell/preload/client.js"
    if [ ! -f "$client_path" ]; then
        echo "[Zzz-Bili] 跳过定时任务清理：青龙 gRPC client 不存在" >&2
        return 0
    fi

    export QL_DIR="$ql_dir"

    node - "$client_path" <<'NODE' || true
const clientPath = process.argv[2];
const api = require(clientPath);

const retiredNames = new Set([
  'bili_base',
  'bili银瓜子兑换硬币任务',
  'Zzz-Bili 银瓜子兑换硬币任务',
  'bili批量取关主播',
  'Zzz-Bili 批量取关主播',
  'bili天选时刻',
  'Zzz-Bili 天选时刻',
]);

const retiredFiles = [
  /(?:^|[\\/_-])bili_task_base\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])zzz_bili_task_base\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])bili_task_silver2coin\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])zzz_bili_task_silver2coin\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])bili_task_unfollowBatched\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])zzz_bili_task_unfollowBatched\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])bili_task_liveLottery\.sh(?:\s|$)/i,
  /(?:^|[\\/_-])zzz_bili_task_liveLottery\.sh(?:\s|$)/i,
];

const taskDefs = [
  ['daily', /zzz_bili_task_daily\.sh/i, ['Zzz-Bili 每日任务']],
  ['charge', /zzz_bili_task_charge\.sh/i, ['Zzz-Bili 免费B币券充电任务']],
  ['login', /zzz_bili_task_login\.sh/i, ['Zzz-Bili 扫码登录']],
  ['liveFansMedal', /zzz_bili_task_liveFansMedal\.sh/i, ['Zzz-Bili 直播粉丝牌']],
  ['manga', /zzz_bili_task_manga\.sh/i, ['Zzz-Bili 漫画任务']],
  ['mangaPrivilege', /zzz_bili_task_manga_privilege\.sh/i, ['Zzz-Bili 领取大会员漫画权益任务']],
  ['vipBigPoint', /zzz_bili_task_vipBigPoint\.sh/i, ['Zzz-Bili 大会员大积分']],
  ['vipPrivilege', /zzz_bili_task_vip_privilege\.sh/i, ['Zzz-Bili 领取大会员福利任务']],
  ['test', /zzz_bili_task_test\.sh/i, ['Zzz-Bili 测试Cookie']],
];

function belongsToFork(task) {
  const command = String(task.command || '');
  // 只接受本仓库（及历史 _main 目录）的 qinglong 路径；同前缀 backup/test 仓库不属于本项目。
  return /ZzzHe2333[\\/_-]BiliBiliToolPro(?:_main)?[\\/_-]qinglong[\\/_-]/i.test(command);
}

function isLegacyDefaultTask(task) {
  const command = String(task.command || '');
  const defaultTaskPath = /qinglong[\\/_-]DefaultTasks(?:[\\/_-]dev)?[\\/_-]bili(?:_dev)?_task_[^\s"'<>]*\.sh/i.test(command);
  return belongsToFork(task) && defaultTaskPath;
}

function logicalKey(task) {
  const command = String(task.command || '');
  const name = String(task.name || '');
  for (const [key, pattern, names] of taskDefs) {
    if (pattern.test(command) || names.includes(name)) return key;
  }
  return null;
}

function isRetired(task) {
  const command = String(task.command || '');
  const name = String(task.name || '');
  return isLegacyDefaultTask(task)
    || retiredNames.has(name)
    || retiredFiles.some((pattern) => pattern.test(command));
}

function preference(task) {
  const command = String(task.command || '');
  let score = 0;

  // 永远不删除当前正在运行的任务；下一次运行再清掉残留。
  if (Number(task.pid || 0) > 0) score += 1000;

  // 同一逻辑任务重复时，优先保留当前正式 SubscriptionTasks 路径。
  // 青龙不同版本可能使用真实路径，也可能把路径分隔符扁平化成 _ / -。
  if (/ZzzHe2333[\\/_-]BiliBiliToolPro[\\/_-]qinglong[\\/_-]SubscriptionTasks[\\/_-]/i.test(command)) {
    score += 200;
  } else if (/ZzzHe2333[\\/_-]BiliBiliToolPro_main[\\/_-]qinglong[\\/_-]SubscriptionTasks[\\/_-]/i.test(command)) {
    score += 100;
  }

  if (Number(task.status || 0) === 0) score += 10;
  return score;
}

(async () => {
  const response = await api.getCrons({ searchValue: '' });
  const crons = Array.isArray(response?.data) ? response.data : [];
  const deleteIds = new Set();
  const groups = new Map();

  for (const task of crons) {
    const id = Number(task.id);
    if (!Number.isInteger(id) || id <= 0) continue;

    // 只允许处理本仓库的任务，绝不按名字跨仓库清理。
    if (!belongsToFork(task)) continue;

    if (isRetired(task)) {
      deleteIds.add(id);
      continue;
    }

    const key = logicalKey(task);
    if (!key) continue;
    if (!groups.has(key)) groups.set(key, []);
    groups.get(key).push(task);
  }

  for (const tasks of groups.values()) {
    if (tasks.length <= 1) continue;
    tasks.sort((a, b) => {
      const scoreDiff = preference(b) - preference(a);
      if (scoreDiff !== 0) return scoreDiff;
      // 只有路径和状态都无差异时才保留更早创建的任务。
      return Number(a.id || 0) - Number(b.id || 0);
    });
    for (const duplicate of tasks.slice(1)) {
      const id = Number(duplicate.id);
      if (Number.isInteger(id) && id > 0) deleteIds.add(id);
    }
  }

  const ids = [...deleteIds].sort((a, b) => a - b);
  if (ids.length === 0) {
    console.log('[Zzz-Bili] 定时任务清理：无需处理');
    if (typeof api.close === 'function') api.close();
    return;
  }

  const result = await api.deleteCrons({ ids });
  const code = Number(result?.code ?? 0);
  if (code !== 200 && code !== 0) {
    throw new Error(`deleteCrons 返回 code=${code}, message=${result?.message || ''}`);
  }

  console.log(`[Zzz-Bili] 定时任务清理完成：删除 ${ids.length} 条本仓库遗留/重复任务`);
  if (typeof api.close === 'function') api.close();
})().catch((error) => {
  try {
    if (typeof api.close === 'function') api.close();
  } catch (_) {}
  console.warn(`[Zzz-Bili] 定时任务清理失败，不影响本次任务继续执行：${error?.message || String(error)}`);
});
NODE
}

cleanup_bili_qinglong_tasks
