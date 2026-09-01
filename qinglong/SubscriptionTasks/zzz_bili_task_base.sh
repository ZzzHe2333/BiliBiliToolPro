#!/usr/bin/env bash
# ZzzHe2333/BiliBiliToolPro 青龙订阅专用公共脚本。
# 本文件没有 cron/new Env 元数据，不会在订阅后生成独立定时任务。

set -e
set -u
set -o pipefail

QL_DIR=${QL_DIR:-"/ql"}
dir_repo=${dir_repo:-"$QL_DIR/data/repo"}
if [ ! -d "$dir_repo" ] && [ -d "$QL_DIR/repo" ]; then
  dir_repo="$QL_DIR/repo"
fi

fork_repo_name="ZzzHe2333_BiliBiliToolPro"
fork_repo_dir="$(find "$dir_repo" -type d \( -iname "$fork_repo_name" -o -iname "${fork_repo_name}_main" \) | head -1)"

if [ -z "$fork_repo_dir" ]; then
  echo "[Zzz-Bili] 未找到订阅仓库目录：ZzzHe2333/BiliBiliToolPro" >&2
  exit 1
fi

# 开启 fork 专用严格隔离模式：Console 只读取 Zzz_* 业务配置。
export Zzz_IsolatedMode=true

# fork 可单独设置运行模式/下载代理；未设置时兼容原 BILI_* 变量。
export BILI_MODE="${Zzz_BILI_MODE:-${BILI_MODE:-dotnet}}"
export BILI_GITHUB_PROXY="${Zzz_BILI_GITHUB_PROXY:-${BILI_GITHUB_PROXY:-}}"

# 复用项目已有安装、架构检测和运行环境逻辑。
. "$fork_repo_dir/qinglong/DefaultTasks/bili_task_base.sh"

# 覆盖原 run_task：只写 Zzz_* 运行变量，避免同一青龙里的原版 Ray_* 配置冲突。
run_task() {
    local target_code=$1

    export Zzz_PlatformType=QingLong
    export Zzz_RunTasks="$target_code"

    cd "$qinglong_bili_repo_dir/src/Ray.BiliBiliTool.Console"

    if [ "$prefer_mode" == "dotnet" ]; then
        dotnet run --ENVIRONMENT=Production
    else
        cp -f "$qinglong_bili_repo_dir/bin/Ray.BiliBiliTool.Console" .
        chmod +x ./Ray.BiliBiliTool.Console
        ./Ray.BiliBiliTool.Console --ENVIRONMENT=Production
    fi
}
