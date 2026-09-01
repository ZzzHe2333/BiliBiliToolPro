namespace Ray.BiliBiliTool.Agent.BiliBiliAgent.Dtos;

public class BiliApiResponse
{
    public int Code { get; set; } = int.MinValue;

    public string? Message { get; set; }
}

public class BiliApiResponse<TData> : BiliApiResponse
{
    public required TData Data { get; set; }
}

/// <summary>
/// 用于B站在业务错误时可能省略data字段的接口响应。
/// 仅在明确存在这种响应形态的接口上使用，避免放宽所有接口的Data约束。
/// </summary>
public class BiliApiResponseOptionalData<TData> : BiliApiResponse
{
    public TData? Data { get; set; }
}
