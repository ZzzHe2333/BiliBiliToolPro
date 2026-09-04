#!/usr/bin/env bash
# cron:1,10 2 * * *
# new Env("Zzz-Bili 定时保护")

set -u

QL_DIR=${QL_DIR:-/ql}
BILI_REPO_ROOT="$QL_DIR/data/repo"
[ -d "$BILI_REPO_ROOT" ] || BILI_REPO_ROOT="$QL_DIR/repo"
BILI_REPO_DIR=""
for candidate in \
    "$BILI_REPO_ROOT/ZzzHe2333_BiliBiliToolPro" \
    "$BILI_REPO_ROOT/ZzzHe2333_BiliBiliToolPro_main"; do
    if [ -d "$candidate" ]; then
        BILI_REPO_DIR="$candidate"
        break
    fi
done
if [ -z "$BILI_REPO_DIR" ]; then
    BILI_REPO_DIR="$(find "$BILI_REPO_ROOT" -type d \( -iname 'ZzzHe2333_BiliBiliToolPro' -o -iname 'ZzzHe2333_BiliBiliToolPro_main' \) | head -1)"
fi
[ -n "$BILI_REPO_DIR" ] || { echo '[Zzz-Bili] 定时保护：未找到仓库目录' >&2; exit 0; }

CLIENT_PATH="$QL_DIR/shell/preload/client.js"
if ! command -v node >/dev/null 2>&1 || [ ! -f "$CLIENT_PATH" ]; then
    echo '[Zzz-Bili] 定时保护：青龙 client.js 或 node 不可用，跳过' >&2
    exit 0
fi

if [ -d "$QL_DIR/data/config" ]; then
    STATE_DIR="$QL_DIR/data/config"
else
    STATE_DIR="$QL_DIR/config"
fi
mkdir -p "$STATE_DIR"
STATE_FILE="$STATE_DIR/zzz_bili_manual_cron_overrides.json"

MODE="${Zzz_BILI_CRON_GUARD_MODE:-}"
if [ -z "$MODE" ]; then
    case "$(date +%H:%M)" in
        02:01) MODE="snapshot" ;;
        02:10) MODE="restore" ;;
        *)
            echo '[Zzz-Bili] 定时保护仅在 02:01 快照、02:10 恢复；手动运行可临时设置 Zzz_BILI_CRON_GUARD_MODE=snapshot 或 restore'
            exit 0
            ;;
    esac
fi

export QL_DIR
node - "$CLIENT_PATH" "$BILI_REPO_DIR" "$STATE_FILE" "$MODE" <<'NODE'
const fs = require('fs');
const path = require('path');

const clientPath = process.argv[2];
const repoDir = process.argv[3];
const stateFile = process.argv[4];
const mode = process.argv[5];
const api = require(clientPath);

const taskDefs = [
  ['daily', 'zzz_bili_task_daily.sh'],
  ['charge', 'zzz_bili_task_charge.sh'],
  ['login', 'zzz_bili_task_login.sh'],
  ['liveFansMedal', 'zzz_bili_task_liveFansMedal.sh'],
  ['manga', 'zzz_bili_task_manga.sh'],
  ['mangaPrivilege', 'zzz_bili_task_manga_privilege.sh'],
  ['vipBigPoint', 'zzz_bili_task_vipBigPoint.sh'],
  ['vipPrivilege', 'zzz_bili_task_vip_privilege.sh'],
  ['test', 'zzz_bili_task_test.sh'],
];

function normalizeCron(value) {
  return String(value || '').trim().replace(/\s+/g, ' ');
}

function belongsToFork(task) {
  const command = String(task.command || '');
  return /ZzzHe2333[\\/_-]BiliBiliToolPro(?:_main)?[\\/_-]qinglong[\\/_-]/i.test(command);
}

function findTask(crons, filename) {
  const escaped = filename.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
  const pattern = new RegExp(escaped, 'i');
  return crons.find((task) => belongsToFork(task) && pattern.test(String(task.command || '')));
}

function getRepoDefault(filename) {
  const filePath = path.join(repoDir, 'qinglong', 'SubscriptionTasks', filename);
  if (!fs.existsSync(filePath)) return '';
  const content = fs.readFileSync(filePath, 'utf8');
  const match = content.match(/^#\s*cron:\s*(.+)$/m);
  return normalizeCron(match ? match[1] : '');
}

async function getAllCrons() {
  const response = await api.getCrons({ searchValue: '' });
  return Array.isArray(response?.data) ? response.data : [];
}

async function snapshot() {
  const crons = await getAllCrons();
  const overrides = {};

  for (const [key, filename] of taskDefs) {
    const task = findTask(crons, filename);
    if (!task) continue;

    const current = normalizeCron(task.schedule);
    const repoDefault = getRepoDefault(filename);
    if (!current || !repoDefault) continue;

    // 只有“青龙当前时间 != 更新前仓库默认时间”才视为用户手动修改。
    if (current !== repoDefault) {
      overrides[key] = {
        filename,
        schedule: current,
        previousDefault: repoDefault,
      };
    }
  }

  fs.writeFileSync(
    stateFile,
    JSON.stringify({ version: 1, capturedAt: new Date().toISOString(), overrides }, null, 2),
    'utf8'
  );

  const count = Object.keys(overrides).length;
  console.log(`[Zzz-Bili] 定时保护：已记录 ${count} 个手工修改的 Cron`);
}

async function restore() {
  if (!fs.existsSync(stateFile)) {
    console.log('[Zzz-Bili] 定时保护：没有待恢复的手工 Cron');
    return;
  }

  let state;
  try {
    state = JSON.parse(fs.readFileSync(stateFile, 'utf8'));
  } catch (error) {
    console.warn(`[Zzz-Bili] 定时保护：状态文件解析失败：${error?.message || String(error)}`);
    return;
  }

  const overrides = state?.overrides || {};
  const crons = await getAllCrons();
  let restored = 0;

  for (const [key, data] of Object.entries(overrides)) {
    const filename = String(data?.filename || '');
    const wanted = normalizeCron(data?.schedule);
    if (!filename || !wanted) continue;

    const task = findTask(crons, filename);
    if (!task) {
      console.warn(`[Zzz-Bili] 定时保护：未找到任务 ${key}，跳过恢复`);
      continue;
    }

    const current = normalizeCron(task.schedule);
    if (current === wanted) continue;

    const id = Number(task.id);
    if (!Number.isInteger(id) || id <= 0) {
      console.warn(`[Zzz-Bili] 定时保护：任务 ${key} 的 id 无效，跳过恢复`);
      continue;
    }

    await api.updateCron({ id, schedule: wanted });
    restored += 1;
    console.log(`[Zzz-Bili] 定时保护：恢复 ${key} -> ${wanted}`);
  }

  try { fs.unlinkSync(stateFile); } catch (_) {}
  console.log(`[Zzz-Bili] 定时保护：恢复完成，共恢复 ${restored} 个 Cron`);
}

(async () => {
  if (mode === 'snapshot') {
    await snapshot();
  } else if (mode === 'restore') {
    await restore();
  } else {
    throw new Error(`未知模式：${mode}`);
  }

  if (typeof api.close === 'function') api.close();
})().catch((error) => {
  try {
    if (typeof api.close === 'function') api.close();
  } catch (_) {}
  console.warn(`[Zzz-Bili] 定时保护失败：${error?.message || String(error)}`);
  process.exitCode = 0;
});
NODE
