#!/usr/bin/env bash
# ZzzHe2333/BiliBiliToolPro 青龙内部公共库；不要作为独立定时任务订阅。

set -e
set -u
set -o pipefail

verbose=false                          # 开启debug日志
bili_repo="ZzzHe2333/BiliBiliToolPro" # 仓库地址
prefer_mode=${BILI_MODE:-"dotnet"}     # dotnet或bilitool，需要通过环境变量配置
github_proxy=${BILI_GITHUB_PROXY:-""}  # 下载github release包时使用的代理，会拼在地址前面，需要通过环境变量配置
use_cn_mirror=${BILI_USE_CN_MIRROR:-"false"} # 默认不修改整个青龙容器的软件源
rolling_release_tag="fork-main"
export DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1

invocation='say_verbose "Calling: ${yellow:-}${FUNCNAME[0]} ${green:-}$*${normal:-}"'

exec 3>&1

if [ -t 1 ] && command -v tput >/dev/null; then
    ncolors=$(tput colors || echo 0)
    if [ -n "$ncolors" ] && [ $ncolors -ge 8 ]; then
        bold="$(tput bold || echo)"
        normal="$(tput sgr0 || echo)"
        black="$(tput setaf 0 || echo)"
        red="$(tput setaf 1 || echo)"
        green="$(tput setaf 2 || echo)"
        yellow="$(tput setaf 3 || echo)"
        blue="$(tput setaf 4 || echo)"
        magenta="$(tput setaf 5 || echo)"
        cyan="$(tput setaf 6 || echo)"
        white="$(tput setaf 7 || echo)"
    fi
fi

say_warning() {
    printf "%b\n" "${yellow:-}bilitool: Warning: $1${normal:-}" >&3
}

say_err() {
    printf "%b\n" "${red:-}bilitool: Error: $1${normal:-}" >&2
}

say() {
    printf "%b\n" "${cyan:-}bilitool:${normal:-} $1" >&3
}

say_verbose() {
    if [ "$verbose" = true ]; then
        say "$1"
    fi
}

QL_DIR=${QL_DIR:-"/ql"}
DefaultCronRule=${DefaultCronRule:-""}
CpuWarn=${CpuWarn:-""}
MemoryWarn=${MemoryWarn:-""}
DiskWarn=${DiskWarn:-""}

dir_repo=${dir_repo:-"$QL_DIR/data/repo"}
if [ ! -d "$dir_repo" ] && [ -d "$QL_DIR/repo" ]; then
  dir_repo="$QL_DIR/repo"
fi
dir_shell=$QL_DIR/shell
touch $dir_shell/env.sh && . $dir_shell/env.sh
touch /root/.bashrc && . /root/.bashrc

say "青龙repo目录: $dir_repo"
qinglong_bili_repo="$(echo "$bili_repo" | sed 's/\//_/g')"
qinglong_bili_repo_dir="${BILI_REPO_DIR:-}"
if [ -z "$qinglong_bili_repo_dir" ] || [ ! -d "$qinglong_bili_repo_dir" ]; then
    if [ -d "$dir_repo/$qinglong_bili_repo" ]; then
        qinglong_bili_repo_dir="$dir_repo/$qinglong_bili_repo"
    elif [ -d "$dir_repo/${qinglong_bili_repo}_main" ]; then
        qinglong_bili_repo_dir="$dir_repo/${qinglong_bili_repo}_main"
    else
        qinglong_bili_repo_dir="$(find "$dir_repo" -type d \( -iname "$qinglong_bili_repo" -o -iname "${qinglong_bili_repo}_main" \) | head -1)"
    fi
fi
say "bili仓库目录: $qinglong_bili_repo_dir"

current_linux_os="debian"
current_os="linux"
machine_architecture="x64"

cd $qinglong_bili_repo_dir
mkdir -p bin && cd $qinglong_bili_repo_dir/bin

machine_has() {
    eval $invocation
    command -v "$1" >/dev/null 2>&1
    return $?
}

get_machine_architecture() {
    eval $invocation

    if command -v uname >/dev/null; then
        CPUName=$(uname -m)
        case $CPUName in
        armv*l)
            echo "arm"
            return 0
            ;;
        aarch64 | arm64)
            echo "arm64"
            return 0
            ;;
        esac
    fi

    echo "x64"
    return 0
}

get_linux_platform_name() {
    eval $invocation

    if [ -e /etc/os-release ]; then
        . /etc/os-release
        echo "$ID${VERSION_ID:+.${VERSION_ID}}"
        return 0
    elif [ -e /etc/redhat-release ]; then
        local redhatRelease=$(</etc/redhat-release)
        if [[ $redhatRelease == "CentOS release 6."* || $redhatRelease == "Red Hat Enterprise Linux "*" release 6."* ]]; then
            echo "rhel.6"
            return 1
        fi
    fi

    echo "Linux specific platform name and version could not be detected: UName = $uname"
    return 1
}

is_musl_based_distro() {
    eval $invocation
    (ldd --version 2>&1 || true) | grep -q musl
}

get_current_os_name() {
    eval $invocation

    local uname=$(uname)
    if [ "$uname" = "Darwin" ]; then
        say_warning "当前系统：osx"
        echo "osx"
        return 1
    elif [ "$uname" = "FreeBSD" ]; then
        say_warning "当前系统：freebsd"
        echo "freebsd"
        return 1
    elif [ "$uname" = "Linux" ]; then
        local linux_platform_name=""
        linux_platform_name="$(get_linux_platform_name)" || true
        say "当前系统发行版本：$linux_platform_name"

        if [ "$linux_platform_name" = "rhel.6" ]; then
            echo $linux_platform_name
            return 1
        elif is_musl_based_distro; then
            echo "linux-musl"
            return 0
        elif [ "$linux_platform_name" = "linux-musl" ]; then
            echo "linux-musl"
            return 0
        else
            echo "linux"
            return 0
        fi
    fi

    say_err "OS name could not be detected: UName = $uname"
    return 1
}

configure_cn_package_mirror() {
    if [[ "$use_cn_mirror" != "true" && "$use_cn_mirror" != "1" ]]; then
        say "国内包源已关闭，保留系统原有软件源"
        return 0
    fi

    say "启用中国大陆优先包源：mirrors.ustc.edu.cn"

    if [ "$current_linux_os" = "debian" ]; then
        local source_file
        for source_file in /etc/apt/sources.list /etc/apt/sources.list.d/debian.sources; do
            [ -f "$source_file" ] || continue
            if [ ! -f "${source_file}.bak" ]; then
                cp "$source_file" "${source_file}.bak"
            fi
            sed -i \
                -e 's#https\?://deb.debian.org#https://mirrors.ustc.edu.cn#g' \
                -e 's#https\?://security.debian.org#https://mirrors.ustc.edu.cn#g' \
                "$source_file"
        done
    else
        if [ -f /etc/apk/repositories ]; then
            if [ ! -f /etc/apk/repositories.bak ]; then
                cp /etc/apk/repositories /etc/apk/repositories.bak
            fi
            sed -i \
                -e 's#https\?://dl-cdn.alpinelinux.org#https://mirrors.ustc.edu.cn#g' \
                /etc/apk/repositories
        fi
    fi
}

check_os() {
    eval $invocation

    current_os="$(get_current_os_name)"
    say "当前系统：$current_os"

    machine_architecture="$(get_machine_architecture)"
    say "当前架构：$machine_architecture"

    if [ "$current_os" = "linux" ]; then
        current_linux_os="debian"
        configure_cn_package_mirror
        if ! machine_has curl; then
            say "curl未安装，开始安装依赖..."
            apt-get update
            apt-get install -y curl
        fi
    else
        current_linux_os="alpine"
        configure_cn_package_mirror
        if ! machine_has curl; then
            say "curl未安装，开始安装依赖..."
            apk update
            apk add -y curl
        fi
    fi

    say "当前选择的运行方式：$prefer_mode"
}

check_unzip() {
    if [ "$current_linux_os" = "debian" ]; then
        if ! machine_has unzip; then
            say "unzip未安装，开始安装依赖..."
            apt-get update
            apt-get install -y unzip
        fi
    else
        if ! machine_has unzip; then
            say "unzip未安装，开始安装依赖..."
            apk update
            apk add -y unzip
        fi
    fi
}

check_dotnet() {
    eval $invocation

    if ! machine_has dotnet; then
        say "dotnet未安装"
        return 1
    fi

    dotnetVersion=$(dotnet --version)
    say "当前dotnet版本：$dotnetVersion"
    if [[ $(echo "$dotnetVersion" | grep -oE '^[0-9]+') -ge 8 ]]; then
        say "已安装，且版本满足"
        say "which dotnet: $(which dotnet)"
        return 0
    else
        say "dotnet版本低于8"
        return 1
    fi
}

install_dotnet_by_script() {
    eval $invocation

    say "再尝试使用官方脚本安装"
    curl -fL --retry 3 --connect-timeout 10 https://dot.net/v1/dotnet-install.sh | bash /dev/stdin --channel 8.0 --verbose

    say "添加到PATH"
    local exportFile="/root/.bashrc"
    touch $exportFile
    echo '' >>$exportFile
    echo 'export DOTNET_ROOT=$HOME/.dotnet' >>$exportFile
    echo 'export PATH=$PATH:$DOTNET_ROOT:$DOTNET_ROOT/tools' >>$exportFile
    . $exportFile
}

install_dotnet() {
    eval $invocation

    say "开始安装dotnet"
    say "当前系统：$current_linux_os"
    if [[ $current_linux_os == "debian" ]]; then
        say "使用apt安装"
        {
            . /etc/os-release
            curl -fL --retry 3 --connect-timeout 10 -o packages-microsoft-prod.deb https://packages.microsoft.com/config/debian/$VERSION_ID/packages-microsoft-prod.deb
            dpkg -i packages-microsoft-prod.deb
            rm packages-microsoft-prod.deb
            apt-get update && apt-get install -y dotnet-sdk-8.0
        } || {
            install_dotnet_by_script
        }
    else
        say "使用apk安装"
        {
            apk update
            apk add dotnet8-sdk
        } || {
            install_dotnet_by_script
        }
    fi
    dotnet --version && say "which dotnet: $(which dotnet)" && say "安装成功"
    return $?
}

get_rolling_release_url() {
    local file_name=$1
    local url="${github_proxy}https://github.com/ZzzHe2333/BiliBiliToolPro/releases/download/${rolling_release_tag}/${file_name}"
    echo "$url"
}

install_bilitool() {
    eval $invocation

    local source_sha
    local built_sha
    local stored_sha=""
    local marker_file
    local marker_url
    local asset_name
    local asset_url
    local zip_file_name

    source_sha="$(git -C "$qinglong_bili_repo_dir" rev-parse HEAD 2>/dev/null || true)"
    if [ -z "$source_sha" ]; then
        say_err "无法获取当前订阅仓库commit，拒绝安装无法校验版本的bilitool二进制"
        return 1
    fi

    if [ -f ./tag.txt ]; then
        stored_sha="$(tr -d '\r\n ' < ./tag.txt)"
    fi

    if [ -f ./Ray.BiliBiliTool.Console ] && [ "$stored_sha" = "$source_sha" ]; then
        say "bilitool二进制与当前订阅commit一致：$source_sha"
        return 0
    fi

    marker_file="$(mktemp /tmp/zzz-bilitool-commit.XXXXXX)"
    marker_url="$(get_rolling_release_url fork-main-commit.txt)"
    say "检查fork滚动构建版本"

    if ! curl -fL --retry 3 --connect-timeout 10 -o "$marker_file" "$marker_url"; then
        rm -f "$marker_file"
        say_err "无法下载fork滚动构建版本标记"
        return 1
    fi

    built_sha="$(tr -d '\r\n ' < "$marker_file")"
    rm -f "$marker_file"

    if [ -z "$built_sha" ]; then
        say_err "fork滚动构建版本标记为空"
        return 1
    fi

    if [ "$built_sha" != "$source_sha" ]; then
        say_err "fork滚动构建尚未对应当前订阅commit"
        say_err "当前源码：$source_sha"
        say_err "当前滚动构建：$built_sha"
        say_err "为避免运行旧版二进制，本次停止；可切换 Zzz_BILI_MODE=dotnet"
        return 1
    fi

    asset_name="bilibili-tool-pro-fork-main-${current_os}-${machine_architecture}.zip"
    asset_url="$(get_rolling_release_url "$asset_name")"
    zip_file_name="bilitool-${source_sha}.zip"

    say "下载与当前fork commit一致的自包含二进制：$asset_name"
    if ! curl -fL --retry 3 --connect-timeout 10 -o "$zip_file_name" "$asset_url"; then
        rm -f "$zip_file_name"
        say_err "下载fork自包含二进制失败"
        return 1
    fi

    check_unzip
    if ! unzip -jo "$zip_file_name" -d ./; then
        rm -f "$zip_file_name"
        say_err "解压fork自包含二进制失败"
        return 1
    fi
    rm -f "$zip_file_name"
    rm -f appsettings.*

    if [ ! -f ./Ray.BiliBiliTool.Console ]; then
        say_err "压缩包中未找到Ray.BiliBiliTool.Console"
        return 1
    fi

    chmod +x ./Ray.BiliBiliTool.Console
    echo "$source_sha" > ./tag.txt
    say "bilitool安装/更新成功，commit：$source_sha"
}

install() {
    eval $invocation

    if [ "$prefer_mode" == "dotnet" ]; then
        if check_dotnet; then
            say "环境正常，本次无需安装"
            return 0
        fi

        say "开始安装dotnet环境"
        install_dotnet || {
            say_err "dotnet安装失败"
            return 1
        }
        return 0
    fi

    if [ "$prefer_mode" == "bilitool" ]; then
        install_bilitool || {
            say_err "bilitool安装/更新失败"
            return 1
        }
        return 0
    fi

    say_err "未知运行方式：$prefer_mode，仅支持 dotnet 或 bilitool"
    return 1
}

run_task() {
    eval $invocation

    local target_code=$1

    export Ray_PlatformType=QingLong
    export Ray_RunTasks=$target_code

    cd $qinglong_bili_repo_dir/src/Ray.BiliBiliTool.Console

    if [ "$prefer_mode" == "dotnet" ]; then
        dotnet run --ENVIRONMENT=Production
    else
        cp -f $qinglong_bili_repo_dir/bin/Ray.BiliBiliTool.Console .
        chmod +x ./Ray.BiliBiliTool.Console && ./Ray.BiliBiliTool.Console --ENVIRONMENT=Production
    fi
}

check_os
install