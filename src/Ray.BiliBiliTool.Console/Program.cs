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
                    .AddJsonFile($"appsettings.{envName}.json", true, true);

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
                    // 青龙订阅隔离模式：只读取本 fork 的 Zzz_* 配置，避免继承同面板中的
                    // Ray_* 或无前缀业务配置（Cookie、推送、充电目标等）。
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

    private static bool IsZzzIsolatedMode()
    {
        string? value = Environment.GetEnvironmentVariable("Zzz_IsolatedMode");
        return string.Equals(value, "true", StringComparison.OrdinalIgnoreCase) || value == "1";
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
