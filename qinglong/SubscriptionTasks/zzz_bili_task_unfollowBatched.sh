#!/usr/bin/env bash
# cron:0 12 1 * *
# new Env("Zzz-Bili 批量取关主播")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.sh"

run_task "UnfollowBatched"
