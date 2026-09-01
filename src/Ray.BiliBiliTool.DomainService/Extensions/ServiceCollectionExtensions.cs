using Microsoft.Extensions.DependencyInjection;
using Ray.BiliBiliTool.DomainService.Interfaces;

namespace Ray.BiliBiliTool.DomainService.Extensions;

public static class ServiceCollectionExtensions
{
    public static IServiceCollection AddDomainServices(this IServiceCollection services)
    {
        services.AddHttpClient("Hitokoto", client =>
        {
            client.BaseAddress = new Uri("https://v1.hitokoto.cn/");
            client.Timeout = TimeSpan.FromSeconds(5);
        });

        services.AddSingleton<BCoinCouponStateStore>();

        services.Scan(scan =>
            scan.FromAssemblyOf<IAccountDomainService>()
                .AddClasses(classes => classes.AssignableTo<IDomainService>())
                .AsImplementedInterfaces()
                .WithTransientLifetime()
        );

        return services;
    }
}
