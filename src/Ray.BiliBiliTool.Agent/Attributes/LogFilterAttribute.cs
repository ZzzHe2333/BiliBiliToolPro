using System.Reflection;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Logging;
using WebApiClientCore;
using WebApiClientCore.Attributes;

namespace Ray.BiliBiliTool.Agent.Attributes;

public class LogFilterAttribute(bool logError = true) : LoggingFilterAttribute
{
    protected override Task WriteLogAsync(ApiResponseContext context, LogMessage logMessage)
    {
        var loggerFactory = context.HttpContext.ServiceProvider.GetService<ILoggerFactory>();
        if (loggerFactory == null)
        {
            return Task.CompletedTask;
        }

        // WebApiClientCore captures full headers, query strings and bodies. Redact at the
        // common Agent boundary so no Console/Web/serverless sink can persist credentials.
        SensitiveLogRedactor.Redact(logMessage);
        string safeMessage =
            SensitiveLogRedactor.Redact(logMessage.ToString()) ?? "[HTTP diagnostic unavailable]";

        MethodInfo member = context.ActionDescriptor.Member;
        var strArray = new string?[5];
        var declaringType1 = member.DeclaringType;
        strArray[0] = declaringType1?.Namespace;
        strArray[1] = ".";
        var declaringType2 = member.DeclaringType;
        strArray[2] = declaringType2?.Name;
        strArray[3] = ".";
        strArray[4] = member.Name;
        string categoryName = string.Concat(strArray);
        ILogger logger = loggerFactory.CreateLogger(categoryName);

        if (logMessage.Exception == null)
        {
            logger.LogDebug(safeMessage);
        }
        else
        {
            if (logError)
                logger.LogError(safeMessage);
            else
                logger.LogDebug(safeMessage);
        }

        return Task.CompletedTask;
    }
}
