# 多账号 B 币券充电配置

`ChargeTaskConfig` 支持按 B 站账号 UID（登录后的 `UserInfo.Mid`）覆盖充电配置。

## 配置优先级

每个账号先查找 `ChargeTaskConfig:Accounts:<账号UID>`：

- `IsEnable` 已配置：使用账号级值。
- `IsEnable` 未配置：继承全局 `ChargeTaskConfig:IsEnable`。
- `AutoChargeUpId` 已配置且非空：使用账号级目标 UP UID。
- `AutoChargeUpId` 未配置或为空：继承全局 `ChargeTaskConfig:AutoChargeUpId`。
- 最终目标为空或为 `-1`：使用 `FallbackAutoChargeUpId`，当前为 `18461303`。

未配置 `Accounts` 时，使用全局配置。

## Docker / Web 环境变量示例

假设有三个 B 站账号：

- UID `111111`：关闭自动充电。
- UID `222222`：开启自动充电，并给 UP `999999` 充电。
- UID `333333`：没有账号级配置，继承全局配置。

```yaml
environment:
  ChargeTaskConfig__IsEnable: "true"
  ChargeTaskConfig__AutoChargeUpId: "888888"

  ChargeTaskConfig__Accounts__111111__IsEnable: "false"

  ChargeTaskConfig__Accounts__222222__IsEnable: "true"
  ChargeTaskConfig__Accounts__222222__AutoChargeUpId: "999999"
```

执行结果：

| 登录账号 UID | 是否自动充电 | 目标 UP UID |
| --- | --- | --- |
| `111111` | 否 | - |
| `222222` | 是 | `999999` |
| `333333` | 是 | `888888` |

## 默认关闭，只开放指定账号

```yaml
environment:
  ChargeTaskConfig__IsEnable: "false"
  ChargeTaskConfig__AutoChargeUpId: "18461303"

  ChargeTaskConfig__Accounts__222222__IsEnable: "true"
  ChargeTaskConfig__Accounts__222222__AutoChargeUpId: "999999"
```

此时除 `222222` 之外的账号都不会自动充电。

## 青龙订阅示例

本仓库青龙订阅使用严格隔离，只读取 `Zzz_*` 业务配置：

```bash
Zzz_ChargeTaskConfig__IsEnable=false
Zzz_ChargeTaskConfig__AutoChargeUpId=18461303

Zzz_ChargeTaskConfig__Accounts__222222__IsEnable=true
Zzz_ChargeTaskConfig__Accounts__222222__AutoChargeUpId=999999
```

普通、非隔离的 Console 模式为了历史兼容仍可读取无前缀、`Ray_*` 和 `Zzz_*` 配置，但不要把这套兼容规则套用到 `Zzz-Bili` 青龙订阅任务上。

账号键使用 B 站 UID，而不是 `BiliBiliCookies__1`、`BiliBiliCookies__2` 的序号，因此调整 Cookie 顺序不会导致配置对应错账号。
