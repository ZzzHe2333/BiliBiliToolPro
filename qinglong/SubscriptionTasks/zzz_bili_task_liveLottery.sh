#!/usr/bin/env bash
# cron:0 13 * * *
# new Env("Zzz-Bili 天选时刻")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.sh"

run_task "LiveLottery"
