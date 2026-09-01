#!/usr/bin/env bash
# cron:5 0 * * *
# new Env("bili直播粉丝牌")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="LiveFansMedal"
run_task "${target_task_code}"
