# 腾讯云云函数（SCF）部署

本目录中的腾讯云函数配置以当前独立仓库 `ZzzHe2333/BiliBiliToolPro` 为准，不依赖其他仓库的同步更新。

## 1. GitHub Actions 自动部署

仓库工作流：

```text
.github/workflows/auto-deploy-tencent-scf.yml
```

需要在当前仓库配置以下 Secrets：

```text
TENCENT_SECRET_ID
TENCENT_SECRET_KEY
```

可选：

```text
TENCENT_SERVERLESS_YML
IS_AUTO_DEPLOY_TENCENT_SCF
```

- `TENCENT_SERVERLESS_YML`：完整的自定义 `serverless.yml` 内容；不设置时使用仓库中的默认文件。
- `IS_AUTO_DEPLOY_TENCENT_SCF=true`：允许定时自动部署当前仓库代码。
- 也可以在 Actions 中手动运行 `auto-deploy-tencent-scf`。

自动部署只部署**当前仓库**，不会先同步或覆盖为其他仓库代码。

## 2. 环境变量

腾讯云函数运行的是 Console 项目，可以使用本项目的 `Zzz_*` 配置：

```yaml
environment:
  variables:
    Zzz_BiliBiliCookies__1: "<COOKIE>"
    Zzz_Security__RandomSleepMaxMin: "0"
```

真实 Cookie、Token、Secret 不要提交到 Git 仓库。推荐通过云端 Secret / 环境变量安全注入。

更多配置见：

```text
../docs/configuration.md
```

## 3. 默认触发器

当前示例 `serverless.yml` 只默认创建：

- `DailyTask` -> `Daily`
- `VipBigPointTask` -> `VipBigPoint`

本项目默认关闭的功能不会出现在 SCF 默认触发器中：

```text
LiveLottery
Silver2Coin
UnfollowBatched
```

如果需要其他任务，可根据腾讯云 SCF 定时触发器格式自行增加，并将任务 Code 放在触发参数中。

## 4. 手动部署

也可以从本仓库 Releases 下载适用于腾讯云函数的发布包，然后在腾讯云控制台创建 `CustomRuntime` 函数。

推荐配置：

```text
运行环境：CustomRuntime
执行方法：index.main_handler
初始化超时：30 秒
执行超时：按当前腾讯云限制设置
```

首次测试可以临时设置：

```text
Zzz_RunTasks=Test
```

确认 Cookie 正常后，再删除该变量并由触发器传入任务 Code。

## 5. 更新

自动部署模式下，更新来源就是当前仓库 `main`。无需也不应安装 Repo Sync、Pull App 或 hard reset 类型的同步工具。

手动部署模式则从当前仓库 Releases 获取新版本重新上传。
