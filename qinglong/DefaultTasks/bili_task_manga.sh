#!/usr/bin/env bash
# cron:0 14 * * *
# new Env("bili漫画任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/bili_task_cleanup.inc"
. "$SCRIPT_DIR/bili_task_base.inc"

target_task_code="Manga"
run_task "${target_task_code}"
