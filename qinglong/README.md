# 在青龙中运行

本 fork 支持通过青龙面板的 **订阅管理** 直接拉取，无需手动创建 `ql repo` 定时任务。

仓库地址：

```text
https://github.com/ZzzHe2333/BiliBiliToolPro.git
```

为避免与 `RayWangQvQ/BiliBiliToolPro` 在同一个青龙面板中冲突，本 fork 提供独立的 `Zzz-Bili` 订阅任务，并优先读取 `Zzz_` 前缀环境变量。

## 1. 订阅管理直接部署

### 1.1. 确认 Shell 文件可被订阅

如果你的青龙版本仍使用 `RepoFileExtensions` 配置，请在青龙面板 `配置文件` 中确保包含 `sh`，例如：

```bash
RepoFileExtensions="js py sh"
```

新版青龙如果在订阅管理中已经有“文件后缀”字段，直接填写 `sh` 即可。

### 1.2. 新建订阅

进入：

```text
青龙面板 -> 订阅管理 -> 新建订阅
```

填写：

```text
名称：Zzz-BiliBiliToolPro
类型：公开仓库
链接：https://github.com/ZzzHe2333/BiliBiliToolPro.git
分支：main
定时类型：crontab
定时规则：2 2 28 * *
白名单：zzz_bili_task_.+\.sh
文件后缀：sh
```

其余项目留空即可，**不需要再创建 `ql repo` 命令**。

保存后点击该订阅的“运行/立即执行”。青龙会拉取仓库，并根据脚本头部的 cron 自动建立 `Zzz-Bili ...` 定时任务。

订阅专用任务位于：

```text
qinglong/SubscriptionTasks/
```

当前会创建：

- `Zzz-Bili 每日任务`
- `Zzz-Bili 免费B币券充电任务`
- `Zzz-Bili 扫码登录`
- `Zzz-Bili 直播粉丝牌`
- `Zzz-Bili 天选时刻`
- `Zzz-Bili 漫画任务`
- `Zzz-Bili 领取大会员漫画权益任务`
- `Zzz-Bili 银瓜子兑换硬币任务`
- `Zzz-Bili 批量取关主播`
- `Zzz-Bili 大会员大积分`
- `Zzz-Bili 领取大会员福利任务`

`zzz_bili_task_base.sh` 也会随订阅拉取，但它没有 cron/new Env 元数据，因此不会生成独立定时任务。

## 2. 与原版在同一个青龙面板共存

原版建议继续使用：

```text
Ray_*
```

本 fork 建议使用：

```text
Zzz_*
```

例如：

```bash
Zzz_BiliBiliCookies__1=<COOKIE>
Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
Zzz_ChargeTaskConfig__ChargeComment=""
```

程序仍兼容旧的 `Ray_` 和无前缀环境变量，但 `Zzz_` 最后加载，因此本 fork 中 `Zzz_` 的值优先级更高。

同一青龙面板中推荐：

```text
原版 RayWangQvQ/BiliBiliToolPro -> Ray_*
本 fork ZzzHe2333/BiliBiliToolPro -> Zzz_*
```

这样 Cookie、充电目标、推送参数以及绝大多数业务配置可以分别维护。

## 3. 配置青龙 ClientId / ClientSecret（可选）

如果希望扫码登录成功后自动将 Cookie 写回青龙环境变量，需要在：

```text
青龙 -> 系统设置 -> 应用设置
```

创建 Application，然后在环境变量中添加：

```text
Zzz_QingLongConfig__ClientId
Zzz_QingLongConfig__ClientSecret
```

本 fork 扫码登录后的 Cookie 会保存为：

```text
Zzz_BiliBiliCookies__0
Zzz_BiliBiliCookies__1
Zzz_BiliBiliCookies__2
...
```

不会去更新原版使用的 `Ray_BiliBiliCookies__*`。

如果没有配置 Application，程序会在日志中提示需要手动添加的 `Zzz_BiliBiliCookies__*` 变量。

## 4. Bili 登录

订阅运行完成后，在青龙“定时任务”中找到：

```text
Zzz-Bili 扫码登录
```

点击运行，根据日志扫描二维码即可。

首次执行任务时会自动检查/安装运行环境，可能比后续执行耗时更长。

## 5. 充电配置

全局充电目标：

```bash
Zzz_ChargeTaskConfig__IsEnable=true
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303
```

按 B 站 UID 单独配置：

```bash
Zzz_ChargeTaskConfig__Accounts__<B站UID>__IsEnable=true
Zzz_ChargeTaskConfig__Accounts__<B站UID>__AutoChargeUpId=18461303
```

未设置充电目标、目标为空或目标为 `-1` 时，本 fork 最终兜底 UID 为：

```text
18461303
```

`Zzz_ChargeTaskConfig__ChargeComment` 留空时，会优先从一言 API 获取留言；请求失败时回退到程序内置随机留言。

## 6. 订阅中的任务时间

订阅脚本当前默认 cron：

| 任务 | Cron |
| --- | --- |
| 每日任务 | `0 9 * * *` |
| 免费 B 币券充电 | `0 12 * * *` |
| 直播粉丝牌 | `5 0 * * *` |
| 天选时刻 | `0 13 * * *` |
| 漫画任务 | `0 14 * * *` |
| 漫画权益 | `0 15 * * *` |
| 银瓜子兑换硬币 | `0 8 * * *` |
| 批量取关 | `0 12 1 * *` |
| 大会员大积分 | `7 1 * * *` |
| 大会员福利 | `0 1 * * *` |
| 扫码登录 | `0 0 1 1 *` |

青龙最终执行时间以面板中对应定时任务的 Cron 为准，可以直接在面板中修改。

## 7. fork 专用运行与网络变量

如果需要让本 fork 和原版使用不同运行模式，可使用：

```bash
Zzz_BILI_MODE=dotnet
```

可选值：

```text
dotnet
bilitool
```

GitHub Release 下载代理可单独配置：

```bash
Zzz_BILI_GITHUB_PROXY=""
```

### 7.1. 中国大陆包源

本 fork 的主要使用环境是中国大陆，因此青龙首次安装依赖/.NET 时，默认将 Debian/Alpine 官方软件源切换为中科大镜像：

```text
https://mirrors.ustc.edu.cn
```

支持传统 Debian `/etc/apt/sources.list`、Debian 12+ 容器常见的 `/etc/apt/sources.list.d/debian.sources`，以及 Alpine `/etc/apk/repositories`。修改前会保留一次 `.bak` 备份。

默认等价于：

```bash
Zzz_BILI_USE_CN_MIRROR=true
```

如果服务器位于境外、已有自己的软件源，或不希望脚本修改包源，可设置：

```bash
Zzz_BILI_USE_CN_MIRROR=false
```

脚本不再通过访问 Google 判断网络地区，避免中国大陆环境下误判或无意义等待。

未设置 `Zzz_BILI_MODE` / `Zzz_BILI_GITHUB_PROXY` / `Zzz_BILI_USE_CN_MIRROR` 时，会兼容读取对应的 `BILI_*` 变量；其中国内包源默认开启。

## 8. GitHub 加速

如果青龙所在服务器访问 GitHub 较慢，可以使用你自己信任的 GitHub 代理。代理服务可用性经常变化，不建议在项目中写死第三方代理地址。

## 9. 常见问题

### 9.1. 订阅成功但没有生成任务

检查：

1. “文件后缀”是否填写 `sh`；
2. 白名单是否为：

```text
zzz_bili_task_.+\.sh
```

3. 订阅日志中是否拉到了 `qinglong/SubscriptionTasks/` 下的脚本；
4. 青龙是否允许解析 Shell 脚本中的 cron 注释。

### 9.2. 与原版环境变量串了

本 fork 不要再新建 `Ray_BiliBiliCookies__*`，改用：

```text
Zzz_BiliBiliCookies__*
```

其他配置同理优先使用 `Zzz_`。

### 9.3. 安装 dotnet 失败

可以改用：

```bash
Zzz_BILI_MODE=bilitool
```

如果使用 `dotnet` 模式，则需要青龙容器能够正常安装/运行 .NET 8。

如果不希望使用默认中国大陆镜像，可先设置：

```bash
Zzz_BILI_USE_CN_MIRROR=false
```

### 9.4. Couldn't find a valid ICU package installed on the system

在青龙环境变量中添加：

```text
DOTNET_SYSTEM_GLOBALIZATION_INVARIANT=1
```

### 9.5. inotify instances reached

如果出现：

```text
The configured user limit (128) on the number of inotify instances has been reached
```

可以添加：

```text
DOTNET_USE_POLLING_FILE_WATCHER=1
```
