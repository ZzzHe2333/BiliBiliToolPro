#!/usr/bin/env bash
# cron:0 1 * * *
# new Env("bili领取大会员福利任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="VipPrivilege"
run_task "${target_task_code}"
