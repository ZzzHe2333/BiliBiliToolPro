#!/usr/bin/env bash
# cron:0 0 1 1 *
# new Env("bili测试ck")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="Test"
run_task "${target_task_code}"
