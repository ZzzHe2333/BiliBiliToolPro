using System.Text.RegularExpressions;
using WebApiClientCore.Attributes;

namespace Ray.BiliBiliTool.Agent.Attributes;

/// <summary>
/// Removes credentials from HTTP request/response diagnostics before they reach any log sink.
/// This is intentionally kept in the Agent layer so Console, Web and serverless hosts share the same protection.
/// </summary>
public static class SensitiveLogRedactor
{
    public const string RedactedValue = "[REDACTED]";

    private const string SensitiveNamePattern =
        "SESSDATA|bili_jct|csrf|csrf_token|DedeUserID__ckMd5|DedeUserID|sid|buvid3|buvid4|buvid_fp|buvid_fp_plain|b_nut|b_lsid|_uuid|ac_time_value|access_key|bili_ticket|access_token|refresh_token|client_secret|ClientSecret|ClientId|qrcode_key|readkey|scKey|turboScKey|botToken|sKey|secret|token|webhook|webHookUrl|apiKey|apikey";

    private static readonly Regex SensitiveHeaderRegex = new(
        @"^(Cookie|Set-Cookie|Authorization)\s*:\s*[^\r\n]*",
        RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase | RegexOptions.Multiline
    );

    private static readonly Regex SensitiveJsonRegex = new(
        "(\"(?:Cookie|Set-Cookie|Authorization|" + SensitiveNamePattern + ")\"\\s*:\\s*\")[^\"]*(\")",
        RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase
    );

    private static readonly Regex SensitiveAssignmentRegex = new(
        "(\\b(?:" + SensitiveNamePattern + ")\\s*=\\s*)(?:\"[^\"]*\"|'[^']*'|[^&;\\s\"'<>]+)",
        RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase
    );

    private static readonly Regex QingLongCookieEnvRegex = new(
        @"(Zzz_BiliBiliCookies__\d+\s*[:=]\s*)[^\r\n]+",
        RegexOptions.Compiled | RegexOptions.CultureInvariant | RegexOptions.IgnoreCase
    );

    public static string? Redact(string? content)
    {
        if (string.IsNullOrEmpty(content))
            return content;

        string result = SensitiveHeaderRegex.Replace(content, $"$1: {RedactedValue}");
        result = QingLongCookieEnvRegex.Replace(result, $"$1{RedactedValue}");
        result = SensitiveJsonRegex.Replace(result, $"$1{RedactedValue}$2");
        result = SensitiveAssignmentRegex.Replace(result, $"$1{RedactedValue}");
        return result;
    }

    public static void Redact(LogMessage logMessage)
    {
        logMessage.RequestHeaders = Redact(logMessage.RequestHeaders);
        logMessage.RequestContent = Redact(logMessage.RequestContent);
        logMessage.ResponseHeaders = Redact(logMessage.ResponseHeaders);
        logMessage.ResponseContent = Redact(logMessage.ResponseContent);
    }
}
