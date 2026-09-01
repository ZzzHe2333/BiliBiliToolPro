namespace Ray.BiliBiliTool.DomainService;

/// <summary>
/// B站接口已正常返回HTTP响应，但业务code表示失败。
/// 这类错误通常属于风控、权限或接口状态，不应伪装成JSON解析异常。
/// </summary>
public sealed class BiliApiBusinessException(int code, string? message) : Exception
{
    public int Code { get; } = code;

    public override string Message =>
        $"B站接口业务错误，code={Code}, message={message ?? "(empty)"}";

    // 现有部分调用方会将Exception作为普通日志属性输出；覆盖ToString避免预期业务错误刷整段堆栈。
    public override string ToString() => Message;
}
