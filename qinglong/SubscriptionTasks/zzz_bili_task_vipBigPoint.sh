#!/usr/bin/env bash
# cron:7 1 * * *
# new Env("Zzz-Bili 大会员大积分")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.sh"

run_task "VipBigPoint"
