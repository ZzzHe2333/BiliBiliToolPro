#!/usr/bin/env bash
# cron:7 1 * * *
# new Env("bili大会员大积分")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="VipBigPoint"
run_task "${target_task_code}"
