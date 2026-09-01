#!/usr/bin/env bash
# cron:0 12 * * *
# new Env("bili免费B币券充电任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="Charge"
run_task "${target_task_code}"
