#!/usr/bin/env bash
# cron:5 0 * * *
# new Env("Zzz-Bili 直播粉丝牌")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.inc"

run_task "LiveFansMedal"
