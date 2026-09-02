using System.Collections.Concurrent;
using System.Text.Json;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Hosting;

namespace Ray.BiliBiliTool.DomainService;

public sealed class BCoinCouponState
{
    public long AccountUid { get; set; }
    public DateTimeOffset ReceivedAtUtc { get; set; }
    public DateTimeOffset ExpireAtUtc { get; set; }
    public DateTimeOffset? LastSeenAtUtc { get; set; }
    public decimal? LastSeenBalance { get; set; }
    public DateTimeOffset? UsedAtUtc { get; set; }
    public bool AutoCharged { get; set; }
}

/// <summary>
/// B币券领取状态持久化。
/// 青龙环境写到 /ql/data/config（或 QL_DATA_DIR/config），不放在订阅仓库目录中，
/// 因此重新拉取仓库、重启任务不会丢失。
/// </summary>
public sealed class BCoinCouponStateStore(
    IHostEnvironment hostingEnvironment,
    IConfiguration configuration
)
{
    private static readonly JsonSerializerOptions JsonOptions = new()
    {
        WriteIndented = true,
    };

    private static readonly ConcurrentDictionary<long, SemaphoreSlim> AccountGates = new();
    private static readonly TimeSpan LockRetryDelay = TimeSpan.FromMilliseconds(100);
    private static readonly TimeSpan LockTimeout = TimeSpan.FromSeconds(30);

    public async Task<BCoinCouponState?> GetAsync(long accountUid)
    {
        string path = GetStateFilePath(accountUid);
        if (!File.Exists(path))
            return null;

        try
        {
            string json = await File.ReadAllTextAsync(path);
            return JsonSerializer.Deserialize<BCoinCouponState>(json, JsonOptions);
        }
        catch
        {
            // 状态文件异常时宁可按“无可信状态”处理，也不能猜测到期时间后提前消费。
            return null;
        }
    }

    public async Task RecordReceivedAsync(long accountUid)
    {
        using IDisposable accountLock = await AcquireAccountLockAsync(accountUid);

        DateTimeOffset now = DateTimeOffset.UtcNow;
        await SaveAsync(
            new BCoinCouponState
            {
                AccountUid = accountUid,
                ReceivedAtUtc = now,
                ExpireAtUtc = now.AddDays(30),
                LastSeenAtUtc = now,
                AutoCharged = false,
            }
        );
    }

    public async Task UpdateSeenAsync(long accountUid, decimal balance)
    {
        using IDisposable accountLock = await AcquireAccountLockAsync(accountUid);

        BCoinCouponState? state = await GetAsync(accountUid);
        if (state == null)
            return;

        state.LastSeenAtUtc = DateTimeOffset.UtcNow;
        state.LastSeenBalance = balance;
        await SaveAsync(state);
    }

    public async Task MarkAutoChargedAsync(long accountUid)
    {
        using IDisposable accountLock = await AcquireAccountLockAsync(accountUid);

        BCoinCouponState? state = await GetAsync(accountUid);
        if (state == null)
            return;

        state.AutoCharged = true;
        state.UsedAtUtc = DateTimeOffset.UtcNow;
        state.LastSeenAtUtc = state.UsedAtUtc;
        state.LastSeenBalance = 0;
        await SaveAsync(state);
    }

    private async Task<IDisposable> AcquireAccountLockAsync(long accountUid)
    {
        SemaphoreSlim gate = AccountGates.GetOrAdd(accountUid, static _ => new SemaphoreSlim(1, 1));
        await gate.WaitAsync();

        FileStream? lockStream = null;
        try
        {
            string statePath = GetStateFilePath(accountUid);
            string directory = Path.GetDirectoryName(statePath)!;
            Directory.CreateDirectory(directory);
            string lockPath = statePath + ".lock";
            DateTime deadline = DateTime.UtcNow.Add(LockTimeout);

            while (true)
            {
                try
                {
                    lockStream = new FileStream(
                        lockPath,
                        FileMode.OpenOrCreate,
                        FileAccess.ReadWrite,
                        FileShare.None,
                        bufferSize: 1,
                        FileOptions.None
                    );
                    break;
                }
                catch (IOException) when (DateTime.UtcNow < deadline)
                {
                    await Task.Delay(LockRetryDelay);
                }
            }

            return new AccountLockHandle(gate, lockStream);
        }
        catch
        {
            lockStream?.Dispose();
            gate.Release();
            throw;
        }
    }

    private async Task SaveAsync(BCoinCouponState state)
    {
        string path = GetStateFilePath(state.AccountUid);
        string directory = Path.GetDirectoryName(path)!;
        Directory.CreateDirectory(directory);

        string tempPath = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            string json = JsonSerializer.Serialize(state, JsonOptions);
            await File.WriteAllTextAsync(tempPath, json);
            File.Move(tempPath, path, true);
        }
        finally
        {
            if (File.Exists(tempPath))
                File.Delete(tempPath);
        }
    }

    private string GetStateFilePath(long accountUid)
    {
        string platform = configuration["PlatformType"] ?? string.Empty;
        string root;

        if (string.Equals(platform, "QingLong", StringComparison.OrdinalIgnoreCase))
        {
            string qlDir = Environment.GetEnvironmentVariable("QL_DIR") ?? "/ql";
            string qlDataDir =
                Environment.GetEnvironmentVariable("QL_DATA_DIR") ?? Path.Combine(qlDir, "data");
            root = Path.Combine(qlDataDir, "config", "zzz-bilibili-tool");
        }
        else if (string.Equals(platform, "Web", StringComparison.OrdinalIgnoreCase))
        {
            root = Path.Combine(hostingEnvironment.ContentRootPath, "config", "zzz-bilibili-tool");
        }
        else
        {
            root = Path.Combine(hostingEnvironment.ContentRootPath, ".state", "zzz-bilibili-tool");
        }

        return Path.Combine(root, $"bcoin-coupon-{accountUid}.json");
    }

    private sealed class AccountLockHandle(SemaphoreSlim gate, FileStream lockStream) : IDisposable
    {
        private bool _disposed;

        public void Dispose()
        {
            if (_disposed)
                return;

            _disposed = true;
            lockStream.Dispose();
            gate.Release();
        }
    }
}
