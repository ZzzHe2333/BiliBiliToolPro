using System.Text.Json.Serialization;

namespace Ray.BiliBiliTool.Agent.BiliBiliAgent.Dtos;

public class GetSpaceInfoResponse
{
    public long Mid { get; set; }

    public string Name { get; set; } = string.Empty;

    public SpaceLiveRoomInfoDto? Live_room { get; set; }
}

/// <summary>
/// 用户空间接口专用响应。
/// 已注销/不存在账号会返回 code=-404 且省略 data；这种情况按“无可用空间/直播间”处理，
/// 避免 WebApiClientCore 因缺少 required data 抛出 JsonException。
/// </summary>
public sealed class GetSpaceInfoApiResponse
{
    private int _rawCode = int.MinValue;

    [JsonPropertyName("code")]
    public int Code
    {
        get => _rawCode == -404 && Data.Live_room is null ? 0 : _rawCode;
        set => _rawCode = value;
    }

    [JsonIgnore]
    public int RawCode => _rawCode;

    [JsonPropertyName("message")]
    public string? Message { get; set; }

    [JsonPropertyName("data")]
    public GetSpaceInfoResponse Data { get; set; } = new();

    public static implicit operator BiliApiResponse<GetSpaceInfoResponse>(GetSpaceInfoApiResponse response)
    {
        return new BiliApiResponse<GetSpaceInfoResponse>
        {
            Code = response.Code,
            Message = response.Message,
            Data = response.Data,
        };
    }
}

public class SpaceLiveRoomInfoDto
{
    public string Title { get; set; } = string.Empty;

    public long Roomid { get; set; }
}
