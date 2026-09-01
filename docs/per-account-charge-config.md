# 多账号 B 币券充电配置

本分支为 `ChargeTaskConfig` 增加按 B 站账号 UID（登录后的 `UserInfo.Mid`）覆盖配置的能力。

## 配置优先级

每个账号先查找 `ChargeTaskConfig:Accounts:<账号UID>`：

- `IsEnable` 已配置：使用账号级值。
- `IsEnable` 未配置：继承全局 `ChargeTaskConfig:IsEnable`。
- `AutoChargeUpId` 已配置且非空：使用账号级目标 UP UID。
- `AutoChargeUpId` 未配置或为空：继承全局 `ChargeTaskConfig:AutoChargeUpId`。
- 最终目标为空或为 `-1`：保持原项目行为，使用 `FallbackAutoChargeUpId`。

未配置 `Accounts` 时，与原版行为完全一致。

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

如果只希望少数账号自动充电，可以把全局设为关闭，再单独开启指定 UID：

```yaml
environment:
  ChargeTaskConfig__IsEnable: "false"
  ChargeTaskConfig__AutoChargeUpId: "888888"

  ChargeTaskConfig__Accounts__222222__IsEnable: "true"
  ChargeTaskConfig__Accounts__222222__AutoChargeUpId: "999999"
```

此时除 `222222` 之外的账号都不会自动充电。

## 青龙 / Console 示例

Console / 青龙环境变量仍需添加 `Ray_` 前缀：

```bash
Ray_ChargeTaskConfig__IsEnable=false
Ray_ChargeTaskConfig__AutoChargeUpId=888888

Ray_ChargeTaskConfig__Accounts__222222__IsEnable=true
Ray_ChargeTaskConfig__Accounts__222222__AutoChargeUpId=999999
```

账号键使用 B 站 UID，而不是 `BiliBiliCookies__1`、`BiliBiliCookies__2` 的序号，因此调整 Cookie 顺序不会导致配置对应错账号。
