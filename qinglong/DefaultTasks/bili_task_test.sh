#!/usr/bin/env bash
# cron:0 0 1 1 *
# new Env("bili测试ck")

QL_DIR=${QL_DIR:-/ql}
BILI_REPO_ROOT="$QL_DIR/data/repo"
[ -d "$BILI_REPO_ROOT" ] || BILI_REPO_ROOT="$QL_DIR/repo"
BILI_REPO_DIR="$(find "$BILI_REPO_ROOT" -type d \( -iname 'ZzzHe2333_BiliBiliToolPro' -o -iname 'ZzzHe2333_BiliBiliToolPro_main' \) | head -1)"
[ -n "$BILI_REPO_DIR" ] || { echo '[Zzz-Bili] 未找到仓库目录' >&2; exit 1; }
. "$BILI_REPO_DIR/qinglong/DefaultTasks/bili_task_cleanup.inc"
. "$BILI_REPO_DIR/qinglong/DefaultTasks/bili_task_base.inc"

target_task_code="Test"
run_task "${target_task_code}"
