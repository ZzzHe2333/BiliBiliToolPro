using Ray.BiliBiliTool.Config;
using Ray.BiliBiliTool.Config.Options;
using Xunit;

namespace ConfigTest;

public class ChargeTaskOptionsDefaultTest
{
    [Fact]
    public void AutoCharge_IsEnabledByDefault_AndAccountCanOptOut()
    {
        const long accountUid = 123456789;
        var options = new ChargeTaskOptions();

        Assert.True(options.IsEnable);
        Assert.True(options.IsEnabledFor(accountUid));
        Assert.Equal(Constants.FallbackAutoChargeUpId, options.GetAutoChargeUpIdFor(accountUid));

        options.Accounts[accountUid.ToString()] = new ChargeAccountOptions { IsEnable = false };

        Assert.False(options.IsEnabledFor(accountUid));
    }
}
