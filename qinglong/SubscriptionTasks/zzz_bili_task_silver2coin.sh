#!/usr/bin/env bash
# cron:0 8 * * *
# new Env("Zzz-Bili 银瓜子兑换硬币任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.sh"

run_task "Silver2Coin"
