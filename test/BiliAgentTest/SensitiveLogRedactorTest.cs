using Ray.BiliBiliTool.Agent.Attributes;
using Xunit;

namespace BiliAgentTest;

public class SensitiveLogRedactorTest
{
    [Fact]
    public void Redact_HidesCookieHeaderAndQueryCsrf()
    {
        const string input =
            "POST /x/vip/privilege/receive?type=1&csrf=query-secret HTTP/1.1\r\n" +
            "Cookie: SESSDATA=cookie-secret; bili_jct=cookie-csrf\r\n" +
            "Content-Type: application/x-www-form-urlencoded\r\n\r\n" +
            "csrf=form-secret&type=1";

        string output = SensitiveLogRedactor.Redact(input)!;

        Assert.DoesNotContain("query-secret", output);
        Assert.DoesNotContain("cookie-secret", output);
        Assert.DoesNotContain("cookie-csrf", output);
        Assert.DoesNotContain("form-secret", output);
        Assert.Contains("csrf=[REDACTED]", output);
        Assert.Contains("Cookie: [REDACTED]", output);
    }

    [Fact]
    public void Redact_HidesJsonAndAuthorizationSecrets()
    {
        const string input =
            "Authorization: Bearer authorization-secret\r\n" +
            "{\"access_token\":\"json-token\",\"bili_jct\":\"json-csrf\",\"message\":\"ok\"}";

        string output = SensitiveLogRedactor.Redact(input)!;

        Assert.DoesNotContain("authorization-secret", output);
        Assert.DoesNotContain("json-token", output);
        Assert.DoesNotContain("json-csrf", output);
        Assert.Contains("Authorization: [REDACTED]", output);
        Assert.Contains("\"message\":\"ok\"", output);
    }
}
