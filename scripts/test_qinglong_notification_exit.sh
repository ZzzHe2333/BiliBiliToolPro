#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
helper="$repo_root/qinglong/SubscriptionTasks/zzz_bili_notify.js"
tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT
mkdir -p "$tmp_dir/ql/data/scripts"

log_file="$tmp_dir/task.log"
cat > "$log_file" <<'LOG'
[00:00:00 INF] BiliBiliToolPro 开始运行...
[00:00:01 ERR] request failed
Cookie: SESSDATA=super-secret; bili_jct=csrf-secret
LOG

capture_file="$tmp_dir/capture.txt"
cat > "$tmp_dir/ql/data/scripts/sendNotify.js" <<'JS'
const fs = require('fs');
exports.sendNotify = async (_title, content) => {
  fs.writeFileSync(process.env.CAPTURE_FILE, content);
  setInterval(() => {}, 1000);
};
JS

output="$(QL_DIR="$tmp_dir/ql" QL_DATA_DIR="$tmp_dir/ql/data" CAPTURE_FILE="$capture_file" Zzz_BILI_NOTIFY_TIMEOUT_MS=500 timeout 5s node "$helper" LiveFansMedal "$log_file" 2 2>&1)"
grep -Fq '[Zzz-Bili] 已回退到青龙环境变量通知流程' <<<"$output"
[ -s "$capture_file" ]
! grep -Fq 'super-secret' "$capture_file"
! grep -Fq 'csrf-secret' "$capture_file"
grep -Fq 'Cookie: [已隐藏]' "$capture_file"

cat > "$tmp_dir/ql/data/scripts/sendNotify.js" <<'JS'
exports.sendNotify = async () => new Promise(() => setInterval(() => {}, 1000));
JS

start="$(date +%s)"
output="$(QL_DIR="$tmp_dir/ql" QL_DATA_DIR="$tmp_dir/ql/data" Zzz_BILI_NOTIFY_TIMEOUT_MS=300 timeout 5s node "$helper" LiveFansMedal "$log_file" 2 2>&1)"
elapsed=$(( $(date +%s) - start ))
[ "$elapsed" -lt 5 ]
grep -Fq '青龙环境变量通知失败：timeout>300ms' <<<"$output"

echo 'Qinglong notification exit regression test passed.'
