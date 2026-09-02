namespace Ray.BiliBiliTool.Config.Options;

public class ChargeTaskOptions : BaseConfigOptions
{
    public ChargeTaskOptions()
    {
        // 免费B币券自动使用默认开启；账号级配置仍可显式关闭。
        IsEnable = true;
    }

    public override string SectionName => "ChargeTaskConfig";

    /// <summary>
    /// 默认充电Up主Id。未提供配置时使用fork统一兜底账号。
    /// </summary>
    public string? AutoChargeUpId { get; set; } =
        Ray.BiliBiliTool.Config.Constants.FallbackAutoChargeUpId;

    /// <summary>
    /// 按B站账号Uid覆盖充电配置。
    /// 未配置的字段继承全局ChargeTaskConfig配置。
    /// </summary>
    public Dictionary<string, ChargeAccountOptions> Accounts { get; set; } = new();

    /// <summary>
    /// 获取指定B站账号的独立充电配置
    /// </summary>
    public ChargeAccountOptions? GetAccountOptions(long accountUid)
    {
        return Accounts.TryGetValue(accountUid.ToString(), out var options) ? options : null;
    }

    /// <summary>
    /// 获取指定B站账号是否启用充电任务。账号级配置优先，否则继承全局开关。
    /// </summary>
    public bool IsEnabledFor(long accountUid)
    {
        return GetAccountOptions(accountUid)?.IsEnable ?? IsEnable;
    }

    /// <summary>
    /// 获取指定B站账号的充电目标。账号级配置优先，否则继承全局目标。
    /// </summary>
    public string? GetAutoChargeUpIdFor(long accountUid)
    {
        var accountOptions = GetAccountOptions(accountUid);
        return string.IsNullOrWhiteSpace(accountOptions?.AutoChargeUpId)
            ? AutoChargeUpId
            : accountOptions.AutoChargeUpId;
    }

    private string? _chargeComment;

    /// <summary>
    /// 充电后留言。显式配置时使用配置值；未配置时该属性保持原有行为，返回内置随机留言。
    /// 实际自动充电流程会优先尝试从一言 API 获取留言，失败后调用内置随机留言。
    /// </summary>
    public string ChargeComment
    {
        get => HasCustomChargeComment ? _chargeComment! : GetRandomDefaultComment();
        set => _chargeComment = value;
    }

    /// <summary>
    /// 是否显式配置了充电留言。
    /// </summary>
    public bool HasCustomChargeComment => !string.IsNullOrWhiteSpace(_chargeComment);

    /// <summary>
    /// 获取显式配置的充电留言；未配置时为 null/空值。
    /// </summary>
    public string? CustomChargeComment => _chargeComment;

    /// <summary>
    /// 从内置列表随机获取一条充电留言。
    /// </summary>
    public string GetRandomDefaultComment()
    {
        return DefaultComments[Random.Shared.Next(DefaultComments.Count)];
    }

    private static readonly List<string> DefaultComments =
    [
        "棒",
        "棒唉",
        "棒耶",
        "加油~",
        "UP加油!",
        "支持~",
        "支持支持！",
        "催更啦",
        "顶顶",
        "留下脚印~",
        "干杯",
        "bilibili干杯",
        "o(*￣▽￣*)o",
        "(｡･∀･)ﾉﾞ嗨",
        "(●ˇ∀ˇ●)",
        "( •̀ ω •́ )y",
        "(ง •_•)ง",
        ">.<",
        "^_~",
    ];

    public override Dictionary<string, string> ToConfigDictionary()
    {
        var config = new Dictionary<string, string>
        {
            { $"{SectionName}:{nameof(AutoChargeUpId)}", AutoChargeUpId ?? "" },
            { $"{SectionName}:{nameof(ChargeComment)}", _chargeComment ?? "" },
        };

        foreach (var (uid, accountOptions) in Accounts)
        {
            if (accountOptions.IsEnable.HasValue)
            {
                config[
                    $"{SectionName}:{nameof(Accounts)}:{uid}:{nameof(ChargeAccountOptions.IsEnable)}"
                ] = accountOptions.IsEnable.Value.ToString().ToLowerInvariant();
            }

            if (accountOptions.AutoChargeUpId is not null)
            {
                config[
                    $"{SectionName}:{nameof(Accounts)}:{uid}:{nameof(ChargeAccountOptions.AutoChargeUpId)}"
                ] = accountOptions.AutoChargeUpId;
            }
        }

        return MergeConfigDictionary(config);
    }
}

/// <summary>
/// 单个B站账号的充电覆盖配置
/// </summary>
public class ChargeAccountOptions
{
    /// <summary>
    /// 是否启用。null表示继承全局ChargeTaskConfig:IsEnable。
    /// </summary>
    public bool? IsEnable { get; set; }

    /// <summary>
    /// 充电Up主Id。空值表示继承全局ChargeTaskConfig:AutoChargeUpId。
    /// </summary>
    public string? AutoChargeUpId { get; set; }
}
