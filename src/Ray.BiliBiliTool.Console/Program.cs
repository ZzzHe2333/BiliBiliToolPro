using System.Reflection;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Ray.BiliBiliTool.Agent.Extensions;
using Ray.BiliBiliTool.Application.Extensions;
using Ray.BiliBiliTool.Config.Extensions;
using Ray.BiliBiliTool.DomainService.Extensions;
using Ray.BiliBiliTool.Infrastructure;
using Serilog;
using Serilog.Debugging;

namespace Ray.BiliBiliTool.Console;

public class Program
{
    public static async Task<int> Main(string[] args)
    {
        System.Console.CancelKeyPress += (sender, eventArgs) =>
        {
            eventArgs.Cancel = true;
            Environment.Exit(0);
        };

        WarnDeprecatedCommandLineOptions(args);
        PrintLogo();

        IHost host = CreateHost(args);

        try
        {
            await host.RunAsync();
            return 0;
        }
        catch (Exception ex)
        {
            Log.Fatal(ex, "Host terminated unexpectedly!");
            return 1;
        }
        finally
        {
            await Log.CloseAndFlushAsync();
        }
    }

    public static IHost CreateHost(string[] args)
    {
        IHost host = CreateHostBuilder(args).UseConsoleLifetime().Build();
        Global.ServiceProviderRoot = host.Services;
        return host;
    }

    private static HostBuilder CreateHostBuilder(string[] args)
    {
        //IHostBuilder hostBuilder = Host.CreateDefaultBuilder();
        var hostBuilder = new HostBuilder();

        //hostBuilder.UseContentRoot(Directory.GetCurrentDirectory());

        hostBuilder.ConfigureHostConfiguration(hostConfigurationBuilder =>
        {
            hostConfigurationBuilder.AddEnvironmentVariables(prefix: "DOTNET_");

            if (args is { Length: > 0 })
            {
                hostConfigurationBuilder.AddCommandLine(args);
            }
        });

        hostBuilder.ConfigureAppConfiguration(
            (hostBuilderContext, configurationBuilder) =>
            {
                IHostEnvironment env = hostBuilderContext.HostingEnvironment;
                bool zzzIsolatedMode = IsZzzIsolatedMode();

                //json文件：
                string envName = hostBuilderContext.HostingEnvironment.EnvironmentName;
                configurationBuilder
                    .AddJsonFile("appsettings.json", true, true)
                    .AddJsonFile($"appsettings.{envName}.json", true, true)
                    // 本 fork 默认不启用银瓜子兑换、批量取关和天选时刻；
                    // 用户机密、环境变量、命令行仍可显式覆盖这些默认值。
                    .AddJsonFile("appsettings.ForkDefaults.json", true, true);

                //用户机密：
                if (env.IsDevelopment() && env.ApplicationName?.Length > 0)
                {
                    //var appAssembly = Assembly.Load(new AssemblyName(env.ApplicationName));
                    var appAssembly = Assembly.GetAssembly(typeof(Program));
                    configurationBuilder.AddUserSecrets(
                        appAssembly!,
                        optional: true,
                        reloadOnChange: true
                    );
                }

                if (zzzIsolatedMode)
                {
                    // 青龙订阅隔离模式：只读取本 fork 的 Zzz_* 业务配置，避免继承同面板中的
                    // Ray_* 或无前缀配置（Cookie、推送、充电目标等）。
                    // 旧版 DefaultTasks 仍会设置 Ray_RunTasks / Ray_PlatformType，因此仅把这两个
                    // 无敏感业务数据的控制字段作为兼容别名导入，保证旧入口不会重新串用 Ray_* 配置。
                    var legacyControlValues = new Dictionary<string, string?>();
                    string? legacyRunTasks = Environment.GetEnvironmentVariable("Ray_RunTasks");
                    string? legacyPlatformType = Environment.GetEnvironmentVariable("Ray_PlatformType");

                    if (!string.IsNullOrWhiteSpace(legacyRunTasks))
                    {
                        legacyControlValues["RunTasks"] = legacyRunTasks;
                    }

                    if (!string.IsNullOrWhiteSpace(legacyPlatformType))
                    {
                        legacyControlValues["PlatformType"] = legacyPlatformType;
                    }

                    configurationBuilder.AddInMemoryCollection(legacyControlValues);
                    configurationBuilder.AddEnvironmentVariables("Zzz_");
                }
                else
                {
                    // 普通模式保持历史兼容：Ray_、无前缀以及 Zzz_ 均可读取，Zzz_ 优先级最高。
                    configurationBuilder.AddEnvironmentVariables("Ray_");
                    configurationBuilder.AddEnvironmentVariables();
                    configurationBuilder.AddEnvironmentVariables("Zzz_");
                }

                //命令行：
                if (args is { Length: > 0 })
                {
                    configurationBuilder.AddCommandLine(
                        args,
                        Config.Constants.CommandLineMappingsDic
                    );
                }

                // 青龙订阅隔离模式只使用 Zzz_BiliBiliCookies__*，不再加载可能残留的本地 cookies.json。
                if (!zzzIsolatedMode)
                {
                    configurationBuilder.AddJsonFile("cookies.json", true, true);
                }
            }
        );

        SelfLog.Enable(x => System.Console.WriteLine(x ?? ""));
        hostBuilder.UseSerilog(
            (context, services, configuration) =>
                configuration.ReadFrom.Configuration(context.Configuration)
        );

        hostBuilder.ConfigureServices(
            (hostContext, services) =>
            {
                services.AddHostedService<BiliBiliToolHostedService>();

                services.AddBiliBiliConfigs(hostContext.Configuration);
                services.AddBiliBiliClientApi(hostContext.Configuration);
                services.AddDomainServices();
                services.AddAppServices();
            }
        );

        return hostBuilder;
    }

    private static void WarnDeprecatedCommandLineOptions(string[] args)
    {
        var deprecatedOptions = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
        {
            ["--dayOfAutoCharge"] = "已废弃。充电执行时间现在由任务 Cron 决定；青龙请直接修改“Zzz-Bili 免费B币券充电任务”的 Cron。",
            ["--dayOfReceiveVipPrivilege"] = "已废弃。大会员福利领取时间现在由任务 Cron 决定；青龙请直接修改“Zzz-Bili 领取大会员福利任务”的 Cron。",
        };

        foreach (string arg in args)
        {
            string optionName = arg.Split('=', 2)[0];
            if (deprecatedOptions.TryGetValue(optionName, out string? message))
            {
                System.Console.WriteLine($"bilitool: Warning: 命令行参数 {optionName} {message}");
            }
        }
    }

    private static bool IsZzzIsolatedMode()
    {
        string? value = Environment.GetEnvironmentVariable("Zzz_IsolatedMode");
        if (string.Equals(value, "true", StringComparison.OrdinalIgnoreCase) || value == "1")
        {
            return true;
        }

        // fork 中历史 DefaultTasks 会通过 Ray_PlatformType=QingLong 启动。
        // 将其自动视为隔离模式，但仅在配置阶段兼容两个控制字段，不读取其他 Ray_* 业务配置。
        string? legacyPlatformType = Environment.GetEnvironmentVariable("Ray_PlatformType");
        return string.Equals(
            legacyPlatformType,
            "QingLong",
            StringComparison.OrdinalIgnoreCase
        );
    }

    /// <summary>
    /// 输出本工具启动logo
    /// </summary>
    private static void PrintLogo()
    {
        System.Console.WriteLine(@"  ____    _   _____           _  ");
        System.Console.WriteLine(@" | __ ) _| |_|_   _|__   ___ | | ");
        System.Console.WriteLine(@" |  _ \(_) (_) | |/ _ \ / _ \| | ");
        System.Console.WriteLine(@" | |_) | | | | | | (_) | (_) | | ");
        System.Console.WriteLine(@" |____/|_|_|_| |_|\___/ \___/|_| ");
        System.Console.WriteLine();
    }
}
