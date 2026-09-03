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

# Reproduce Qinglong's eval/function-style execution with errexit enabled.
sim_repo="$tmp_dir/sim-repo"
mkdir -p "$sim_repo/qinglong/SubscriptionTasks" "$sim_repo/qinglong/DefaultTasks" \
  "$sim_repo/src/Ray.BiliBiliTool.Console" "$tmp_dir/bin"
cp "$repo_root/qinglong/SubscriptionTasks/zzz_bili_task_base.inc" "$sim_repo/qinglong/SubscriptionTasks/zzz_bili_task_base.inc"
cp "$helper" "$sim_repo/qinglong/SubscriptionTasks/zzz_bili_notify.js"
cat > "$sim_repo/common.props" <<'PROPS'
<Project>
  <PropertyGroup>
    <Version>9.9.9</Version>
  </PropertyGroup>
</Project>
PROPS
: > "$sim_repo/qinglong/DefaultTasks/bili_task_cleanup.inc"
cat > "$sim_repo/qinglong/DefaultTasks/bili_task_base.inc" <<'INC'
qinglong_bili_repo_dir="$BILI_REPO_DIR"
prefer_mode=dotnet
INC
cat > "$tmp_dir/bin/dotnet" <<'DOTNET'
#!/usr/bin/env bash
echo '[00:00:00 INF] BiliBiliToolPro 开始运行...'
echo '[00:00:01 ERR] simulated partial failure'
exit 0
DOTNET
chmod +x "$tmp_dir/bin/dotnet"

shell_output="$(PATH="$tmp_dir/bin:$PATH" QL_DIR="$tmp_dir/ql" BILI_REPO_DIR="$sim_repo" Zzz_BILI_NOTIFY_TIMEOUT_MS=300 bash -c '
  set -e
  eval '\'' . "$BILI_REPO_DIR/qinglong/SubscriptionTasks/zzz_bili_task_base.inc"; run_task "Regression"; status=$?; printf "AFTER_RUN:%s\n" "$status" '\''
' 2>&1)"
grep -Fq '[Zzz-Bili] 本地仓库版本：9.9.9; branch=unknown; commit=unknown' <<<"$shell_output"
grep -Fq 'AFTER_RUN:2' <<<"$shell_output"
! grep -Fq 'pop_var_context' <<<"$shell_output"

grep -Fq '定时规则：2 2 * * *' "$repo_root/qinglong/README.md"
grep -Fq '每日 02:02 刷新订阅' "$repo_root/qinglong/README.md"

echo 'Qinglong errexit/eval and subscription freshness regression test passed.'
