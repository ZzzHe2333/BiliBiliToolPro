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

function isLegacyDefaultTask(task) {
  const command = String(task.command || '');
  // 青龙可能保存为斜杠路径，也可能把路径扁平化为下划线；两种都兼容。
  const belongsToFork = /ZzzHe2333[\/_-]BiliBiliToolPro/i.test(command);
  const defaultTaskPath = /qinglong[\\/_-]DefaultTasks(?:[\\/_-]dev)?[\\/_-]bili(?:_dev)?_task_[^\s"'<>]*\.sh/i.test(command);
  return belongsToFork && defaultTaskPath;
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
  let score = 0;
  if (Number(task.pid || 0) > 0) score += 1000;
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

    // 本 fork 的 DefaultTasks / DefaultTasks/dev 旧入口全部属于遗留任务，直接清理。
    if (isRetired(task)) {
      deleteIds.add(id);
      continue;
    }

    // SubscriptionTasks 自身如果因为重复订阅生成多条，也只保留一条。
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

  console.log(`[Zzz-Bili] 定时任务清理完成：删除 ${ids.length} 条 DefaultTasks 遗留/重复任务`);
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
