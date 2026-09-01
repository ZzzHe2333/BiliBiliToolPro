using System.Net;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Options;
using Polly;
using Polly.Extensions.Http;
using Ray.BiliBiliTool.Agent.BiliBiliAgent;
using Ray.BiliBiliTool.Agent.BiliBiliAgent.Interfaces;
using Ray.BiliBiliTool.Agent.BiliBiliAgent.Services;
using Ray.BiliBiliTool.Agent.HttpClientDelegatingHandlers;
using Ray.BiliBiliTool.Agent.QingLong;
using Ray.BiliBiliTool.Config.Options;
using Ray.BiliBiliTool.Infrastructure.Cookie;

namespace Ray.BiliBiliTool.Agent.Extensions;

public static class ServiceCollectionExtension
{
    /// <summary>
    /// 注册强类型api客户端
    /// </summary>
    /// <param name="services"></param>
    /// <returns></returns>
    public static IServiceCollection AddBiliBiliClientApi(
        this IServiceCollection services,
        IConfiguration configuration
    )
    {
        //Cookie
        services.AddSingleton<CookieStrFactory<BiliCookie>>();

        //全局代理
        services.SetGlobalProxy(configuration);

        //DelegatingHandler
        services.Scan(scan =>
            scan.FromAssemblyOf<IBiliBiliApi>()
                .AddClasses(classes => classes.AssignableTo<DelegatingHandler>())
                .AsSelf()
                .WithTransientLifetime()
        );

        //服务
        services.AddScoped<IWbiService, WbiService>();

        //bilibli
        Action<IServiceProvider, HttpClient> config = (sp, c) =>
        {
            c.DefaultRequestHeaders.Add(
                "User-Agent",
                sp.GetRequiredService<IOptionsMonitor<SecurityOptions>>().CurrentValue.UserAgent
            );
        };
        Action<IServiceProvider, HttpClient> configApp = (sp, c) =>
        {
            c.DefaultRequestHeaders.Add(
                "User-Agent",
                sp.GetRequiredService<IOptionsMonitor<SecurityOptions>>().CurrentValue.UserAgentApp
            );
        };

        services.AddBiliBiliClientApi<IUserInfoApi>(BiliHosts.Api, config, true);

        services.AddBiliBiliClientApi<IUpInfoApi>(BiliHosts.Api, config);
        services.AddBiliBiliClientApi<IDailyTaskApi>(BiliHosts.Api, config);
        services.AddBiliBiliClientApi<IRelationApi>(BiliHosts.Api, config);
        services.AddBiliBiliClientApi<IChargeApi>(BiliHosts.Api, config);
        services.AddBiliBiliClientApi<IVideoApi>(BiliHosts.Api, config);
        services.AddBiliBiliClientApi<IVideoWithoutCookieApi>(BiliHosts.Api, config);
        services.AddBiliBiliClientApi<IArticleApi>(BiliHosts.Api, config);

        services.AddBiliBiliClientApi<IVipMallApi>(BiliHosts.Show, config);
        services.AddBiliBiliClientApi<IPassportApi>(BiliHosts.Passport, config);
        services.AddBiliBiliClientApi<ILiveTraceApi>(BiliHosts.LiveTrace, config);
        services.AddBiliBiliClientApi<IHomeApi>(BiliHosts.Www, config);
        services.AddBiliBiliClientApi<IMangaApi>(BiliHosts.Manga, config);
        services.AddBiliBiliClientApi<IAccountApi>(BiliHosts.Account, config);
        services.AddBiliBiliClientApi<ILiveApi>(BiliHosts.Live, config);

        services.AddBiliBiliClientApi<IVipBigPointApi>(BiliHosts.App, configApp);
        services.AddBiliBiliClientApi<IMallApi>(BiliHosts.Mall, configApp);

        //qinglong
        var qinglongHost = configuration["QL_URL"] ?? "http://localhost:5600";
        services
            .AddHttpApi<IQingLongApi>(o =>
            {
                o.HttpHost = new Uri(qinglongHost);
                o.UseDefaultUserAgent = false;
            })
            .ConfigureHttpClient(
                (sp, c) =>
                {
                    c.DefaultRequestHeaders.Add(
                        "User-Agent",
                        sp.GetRequiredService<
                            IOptionsMonitor<SecurityOptions>
                        >().CurrentValue.UserAgent
                    );
                }
            )
            .AddPolicyHandler(GetRetryPolicy());

        return services;
    }

    /// <summary>
    /// 封装Refit，默认将Cookie添加到Header中
    /// </summary>
    /// <typeparam name="TInterface"></typeparam>
    /// <param name="services"></param>
    /// <param name="host"></param>
    /// <returns></returns>
    private static IServiceCollection AddBiliBiliClientApi<TInterface>(
        this IServiceCollection services,
        string host,
        Action<IServiceProvider, HttpClient> config,
        bool ignorWrid = false
    )
        where TInterface : class
    {
        var uri = new Uri(host);
        IHttpClientBuilder httpClientBuilder = services
            .AddHttpApi<TInterface>(o =>
            {
                o.HttpHost = uri;
                o.UseDefaultUserAgent = false;
            })
            .ConfigureHttpClient(config)
            .AddHttpMessageHandler<IntervalDelegatingHandler>()
            .AddPolicyHandler(GetRetryPolicy());

        if (!ignorWrid)
        {
            httpClientBuilder.AddHttpMessageHandler<WridEncryptionDelegatingHandler>();
        }

        return services;
    }

    /// <summary>
    /// 设置全局HTTP/HTTPS代理（如果配置了代理）。
    /// 支持：host:port、http://host:port、http://user:pass@host:port，
    /// 并兼容历史格式 user:pass@http://host:port。
    /// </summary>
    private static IServiceCollection SetGlobalProxy(
        this IServiceCollection services,
        IConfiguration configuration
    )
    {
        string? proxyAddress = configuration["Security:WebProxy"]?.Trim();
        if (string.IsNullOrWhiteSpace(proxyAddress))
        {
            return services;
        }

        HttpClient.DefaultProxy = CreateWebProxy(proxyAddress);
        return services;
    }

    private static WebProxy CreateWebProxy(string proxyAddress)
    {
        string endpoint = proxyAddress.Trim();
        NetworkCredential? credentials = null;

        // 优先解析标准URI，例如 http://user:pass@host:port。
        if (TryCreateProxyUri(endpoint, out Uri standardUri))
        {
            if (!string.IsNullOrEmpty(standardUri.UserInfo))
            {
                credentials = ParseProxyCredentials(standardUri.UserInfo);
            }

            var uriBuilder = new UriBuilder(standardUri)
            {
                UserName = string.Empty,
                Password = string.Empty,
            };
            return CreateWebProxy(uriBuilder.Uri, credentials);
        }

        // 兼容旧格式 user:pass@http://host:port；使用最后一个@，避免密码中包含@时切错。
        int atIndex = endpoint.LastIndexOf('@');
        if (atIndex > 0 && atIndex < endpoint.Length - 1)
        {
            string credentialPart = endpoint[..atIndex];
            endpoint = endpoint[(atIndex + 1)..];
            credentials = ParseProxyCredentials(credentialPart);
        }

        if (!endpoint.Contains("://", StringComparison.Ordinal))
        {
            endpoint = "http://" + endpoint;
        }

        if (!TryCreateProxyUri(endpoint, out Uri proxyUri))
        {
            throw new FormatException(
                "代理地址格式无效。请使用 host:port、http://host:port、http://user:pass@host:port 或 user:pass@http://host:port"
            );
        }

        return CreateWebProxy(proxyUri, credentials);
    }

    private static bool TryCreateProxyUri(string value, out Uri uri)
    {
        if (
            Uri.TryCreate(value, UriKind.Absolute, out Uri? candidate)
            && !string.IsNullOrWhiteSpace(candidate.Host)
            && (
                candidate.Scheme.Equals("http", StringComparison.OrdinalIgnoreCase)
                || candidate.Scheme.Equals("https", StringComparison.OrdinalIgnoreCase)
            )
        )
        {
            uri = candidate;
            return true;
        }

        uri = null!;
        return false;
    }

    private static NetworkCredential ParseProxyCredentials(string userInfo)
    {
        int separatorIndex = userInfo.IndexOf(':');
        string user = separatorIndex >= 0 ? userInfo[..separatorIndex] : userInfo;
        string password = separatorIndex >= 0 ? userInfo[(separatorIndex + 1)..] : string.Empty;

        return new NetworkCredential(
            Uri.UnescapeDataString(user),
            Uri.UnescapeDataString(password)
        );
    }

    private static WebProxy CreateWebProxy(Uri proxyUri, NetworkCredential? credentials)
    {
        var webProxy = new WebProxy(proxyUri);
        if (credentials is not null)
        {
            webProxy.Credentials = credentials;
        }

        return webProxy;
    }

    static IAsyncPolicy<HttpResponseMessage> GetRetryPolicy()
    {
        return HttpPolicyExtensions
            .HandleTransientHttpError()
            .OrResult(msg => msg.StatusCode == System.Net.HttpStatusCode.NotFound)
            .WaitAndRetryAsync(1, retryAttempt => TimeSpan.FromSeconds(Math.Pow(2, retryAttempt)));
    }
}
