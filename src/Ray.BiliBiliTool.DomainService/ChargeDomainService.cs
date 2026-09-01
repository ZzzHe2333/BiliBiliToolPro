using System.Text.Json;
using Microsoft.Extensions.Logging;
using Microsoft.Extensions.Options;
using Ray.BiliBiliTool.Agent;
using Ray.BiliBiliTool.Agent.BiliBiliAgent.Dtos;
using Ray.BiliBiliTool.Agent.BiliBiliAgent.Interfaces;
using Ray.BiliBiliTool.Config.Options;
using Ray.BiliBiliTool.DomainService.Interfaces;

namespace Ray.BiliBiliTool.DomainService;

/// <summary>
/// 充电
/// </summary>
public class ChargeDomainService(
    ILogger<ChargeDomainService> logger,
    IOptionsMonitor<DailyTaskOptions> dailyTaskOptions,
    IOptionsMonitor<ChargeTaskOptions> chargeTaskOptions,
    IDailyTaskApi dailyTaskApi,
    IChargeApi chargeApi,
    IHttpClientFactory httpClientFactory,
    BCoinCouponStateStore bCoinCouponStateStore
) : IChargeDomainService
{
    private const string HitokotoClientName = "Hitokoto";
    private static readonly TimeSpan ReminderWindow = TimeSpan.FromDays(5);
    private static readonly TimeSpan AutoChargeWindow = TimeSpan.FromHours(48);

    private readonly DailyTaskOptions _dailyTaskOptions = dailyTaskOptions.CurrentValue;
    private readonly ChargeTaskOptions _chargeTaskOptions = chargeTaskOptions.CurrentValue;
    private readonly IDailyTaskApi _dailyTaskApi = dailyTaskApi;

    /// <summary>
    /// 自动充电：记录领取时间后，在预计到期前5天开始提醒，进入最后48小时才自动充电。
    /// </summary>
    public async Task Charge(UserInfo userInfo, BiliCookie ck)
    {
        //大会员类型
        VipType vipType = userInfo.GetVipType();
        if (vipType != VipType.Annual)
        {
            logger.LogInformation("不是年度大会员，跳过");
            return;
        }

        //B币券余额
        decimal couponBalance = userInfo.Wallet?.Coupon_balance ?? 0;
        logger.LogInformation("【B币券】{couponBalance}", couponBalance);

        BCoinCouponState? couponState = await bCoinCouponStateStore.GetAsync(userInfo.Mid);
        if (couponState == null)
        {
            logger.LogWarning("未找到该账号可信的B币券领取时间记录，为避免提前消费，本次不自动充电");
            return;
        }

        await bCoinCouponStateStore.UpdateSeenAsync(userInfo.Mid, couponBalance);

        if (couponState.AutoCharged)
        {
            logger.LogInformation("本期B币券已执行过自动充电，跳过");
            return;
        }

        DateTimeOffset now = DateTimeOffset.UtcNow;
        TimeSpan remaining = couponState.ExpireAtUtc - now;
        DateTimeOffset expireChina = couponState.ExpireAtUtc.ToOffset(TimeSpan.FromHours(8));
        logger.LogInformation(
            "【预计到期】{expire:yyyy-MM-dd HH:mm}（北京时间，按领取成功时间+30天计算）",
            expireChina
        );

        if (remaining <= TimeSpan.Zero)
        {
            logger.LogWarning("领取记录已超过预计30天有效期，为避免使用陈旧状态，本次不自动充电");
            return;
        }

        if (remaining > AutoChargeWindow)
        {
            if (remaining <= ReminderWindow && couponBalance > 0)
            {
                logger.LogWarning(
                    "【B币券临期提醒】当前余额 {balance}，距离预计到期约 {days:F1} 天（{hours:F0}小时）；请及时使用，进入最后48小时后将尝试自动充电",
                    couponBalance,
                    remaining.TotalDays,
                    remaining.TotalHours
                );
            }
            else
            {
                logger.LogInformation(
                    "距离预计到期约 {hours:F1} 小时，尚未进入48小时自动充电窗口，跳过",
                    remaining.TotalHours
                );
            }
            return;
        }

        if (couponBalance < 2)
        {
            if (couponBalance > 0)
            {
                logger.LogWarning(
                    "【B币券临期提醒】已进入最后48小时，但余额仅 {balance}，不足2无法充电，请在到期前手动使用",
                    couponBalance
                );
            }
            else
            {
                logger.LogInformation("已进入临期窗口，但B币券余额为0，无需充电");
            }
            return;
        }

        logger.LogWarning("B币券已进入到期前48小时窗口，开始自动充电");

        //账号级目标优先，未配置时继承全局目标；空值或-1使用fallback值
        string? configuredTargetUpId = _chargeTaskOptions.GetAutoChargeUpIdFor(userInfo.Mid);
        string targetUpId =
            string.IsNullOrWhiteSpace(configuredTargetUpId) || configuredTargetUpId == "-1"
                ? Config.Constants.FallbackAutoChargeUpId
                : configuredTargetUpId;

        logger.LogDebug("【当前账号】{uid}，【目标Up】{up}", userInfo.Mid, targetUpId);

        var request = new ChargeRequest(couponBalance, long.Parse(targetUpId), ck.BiliJct);

        BiliApiResponse<ChargeV2Response> response = await chargeApi.ChargeV2Async(
            request,
            ck.ToString()
        );

        if (response.Code == 0)
        {
            if (response.Data?.Status == 4)
            {
                logger.LogInformation("【充电结果】成功");
                logger.LogInformation("【充值个数】 {num}个B币", couponBalance);
                logger.LogInformation("经验+{exp} √", couponBalance);
                logger.LogInformation("在过期前使用成功，赠送的B币券没有浪费哦~");

                // 先持久化“已自动充电”，即使后续留言失败也不会让下一次任务误判为未充电。
                await bCoinCouponStateStore.MarkAutoChargedAsync(userInfo.Mid);

                //充电留言
                await ChargeComments(response.Data.Order_no, ck);
            }
            else
            {
                logger.LogInformation("【充电结果】失败");
                logger.LogError("【原因】{reason}", response.ToJsonStr());
            }
        }
        else
        {
            logger.LogInformation("【充电结果】失败");
            logger.LogError("【原因】{reason}", response.Message);
        }
    }

    /// <summary>
    /// 充电后留言。
    /// 显式配置ChargeComment时优先使用配置值；否则请求一言API，失败后使用内置随机留言。
    /// </summary>
    public async Task ChargeComments(string orderNum, BiliCookie ck)
    {
        string comment = await GetChargeCommentAsync();
        var request = new ChargeCommentRequest(orderNum, comment, ck.BiliJct);
        await chargeApi.ChargeCommentAsync(request, ck.ToString());

        logger.LogInformation("【留言】{comment}", comment);
    }

    private async Task<string> GetChargeCommentAsync()
    {
        if (_chargeTaskOptions.HasCustomChargeComment)
        {
            logger.LogDebug("使用配置的充电留言");
            return _chargeTaskOptions.CustomChargeComment!;
        }

        try
        {
            HttpClient client = httpClientFactory.CreateClient(HitokotoClientName);
            using HttpResponseMessage response = await client.GetAsync("?c=a");
            response.EnsureSuccessStatusCode();

            await using Stream stream = await response.Content.ReadAsStreamAsync();
            using JsonDocument document = await JsonDocument.ParseAsync(stream);

            if (document.RootElement.TryGetProperty("hitokoto", out JsonElement hitokotoElement))
            {
                string? hitokoto = hitokotoElement.GetString()?.Trim();
                if (!string.IsNullOrWhiteSpace(hitokoto))
                {
                    logger.LogDebug("从一言API获取充电留言成功");
                    return hitokoto;
                }
            }

            logger.LogWarning("一言API未返回有效hitokoto字段，使用内置随机留言");
        }
        catch (Exception ex)
        {
            logger.LogWarning("一言API获取失败，使用内置随机留言：{message}", ex.Message);
        }

        return _chargeTaskOptions.GetRandomDefaultComment();
    }
}
