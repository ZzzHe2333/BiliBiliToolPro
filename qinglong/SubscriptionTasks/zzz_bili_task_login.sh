#!/usr/bin/env bash
# cron:0 0 1 1 *
# new Env("Zzz-Bili 扫码登录")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.inc"

run_task "Login"
