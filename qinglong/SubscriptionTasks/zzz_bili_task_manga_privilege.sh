#!/usr/bin/env bash
# cron:0 15 * * *
# new Env("Zzz-Bili 领取大会员漫画权益任务")

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/zzz_bili_task_base.inc"

run_task "MangaPrivilege"
