#!/usr/bin/env bash
# cron:0 14 * * *
# new Env("Zzz-Bili 漫画任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.inc"

run_task "Manga"
