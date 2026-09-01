#!/usr/bin/env bash
# cron:0 9 * * *
# new Env("bili每日任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="Daily"
run_task "${target_task_code}"
