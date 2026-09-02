#!/usr/bin/env python3
"""Repository invariants for the independently maintained BiliBiliToolPro repo."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ERRORS: list[str] = []


def fail(message: str) -> None:
    ERRORS.append(message)


def read(path: str) -> str:
    target = ROOT / path
    if not target.is_file():
        fail(f"required file missing: {path}")
        return ""
    return target.read_text(encoding="utf-8-sig")


def tracked_files() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
    )
    return [ROOT / item.decode() for item in result.stdout.split(b"\0") if item]


def audit_versions() -> None:
    props = read("common.props")
    changelog = read("CHANGELOG.md")
    chart = read("helm/bilibili-tool/Chart.yaml")

    version_match = re.search(r"<Version>([^<]+)</Version>", props)
    changelog_match = re.search(r"^##\s+([^\s]+)", changelog, re.M)
    chart_match = re.search(r'^appVersion:\s*["\']?([^"\'\s]+)', chart, re.M)

    version = version_match.group(1) if version_match else None
    changelog_version = changelog_match.group(1) if changelog_match else None
    chart_version = chart_match.group(1) if chart_match else None

    if not version:
        fail("common.props has no <Version>")
    if version != changelog_version:
        fail(f"version mismatch: common.props={version}, CHANGELOG={changelog_version}")
    if version != chart_version:
        fail(f"version mismatch: common.props={version}, Helm appVersion={chart_version}")


def section_is_disabled(content: str, section: str) -> bool:
    pattern = re.compile(
        rf'"{re.escape(section)}"\s*:\s*\{{.*?"IsEnable"\s*:\s*(true|false)',
        re.S | re.I,
    )
    match = pattern.search(content)
    return bool(match and match.group(1).lower() == "false")


def audit_retired_tasks() -> None:
    sections = ("LiveLotteryTaskConfig", "Silver2CoinTaskConfig", "UnfollowBatchedTaskConfig")
    for path in (
        "src/Ray.BiliBiliTool.Console/appsettings.json",
        "src/Ray.BiliBiliTool.Console/appsettings.ForkDefaults.json",
        "src/Ray.BiliBiliTool.Web/appsettings.json",
        "src/Ray.BiliBiliTool.Web/appsettings.ForkDefaults.json",
    ):
        content = read(path)
        for section in sections:
            if not section_is_disabled(content, section):
                fail(f"retired task must default to false: {path} -> {section}")

    retired_scripts = (
        "zzz_bili_task_liveLottery.sh",
        "zzz_bili_task_silver2coin.sh",
        "zzz_bili_task_unfollowBatched.sh",
    )
    task_dir = ROOT / "qinglong/SubscriptionTasks"
    for name in retired_scripts:
        if (task_dir / name).exists():
            fail(f"retired Qinglong task entrypoint must not exist: {name}")


def audit_detachment() -> None:
    forbidden_paths = (
        ".github/pull.yml",
        ".github/workflows/repo-sync.yml",
        ".github/workflows/tag.yml",
        "krew",
    )
    for path in forbidden_paths:
        if (ROOT / path).exists():
            fail(f"legacy/upstream integration returned: {path}")

    banned = (
        "RayWangQvQ/BiliBiliToolPro",
        "ghcr.io/raywangqvq/bili_tool_web",
        "zai7lou/bili_tool_web",
        "zai7lou/bilibili_tool_pro",
    )
    # Historical changelog/license text may legitimately preserve provenance.
    excluded = {"CHANGELOG.md", "LICENSE"}
    text_suffixes = {
        ".md", ".yml", ".yaml", ".json", ".cs", ".csproj", ".props", ".targets",
        ".sh", ".ps1", ".cmd", ".bat", ".sln", ".xml", ".txt", ".razor",
    }
    for path in tracked_files():
        relative = path.relative_to(ROOT).as_posix()
        if relative in excluded or path.suffix.lower() not in text_suffixes:
            continue
        try:
            content = path.read_text(encoding="utf-8-sig")
        except UnicodeDecodeError:
            continue
        for token in banned:
            if token.lower() in content.lower():
                fail(f"active file contains legacy runtime/upstream reference {token!r}: {relative}")


def audit_solution_items() -> None:
    solution = read("Ray.BiliBiliTool.sln")
    for raw in solution.splitlines():
        line = raw.strip()
        if " = " not in line or not raw.startswith("\t\t"):
            continue
        left, right = line.split(" = ", 1)
        if left != right:
            continue
        if "\\" not in left and "/" not in left and "." not in Path(left).name:
            continue
        target = ROOT / left.replace("\\", "/")
        if not target.exists():
            fail(f"stale Solution Item: {left}")


def audit_qinglong_isolation() -> None:
    base = read("qinglong/SubscriptionTasks/zzz_bili_task_base.inc")
    notify_helper = read("qinglong/SubscriptionTasks/zzz_bili_notify.js")
    common_base = read("qinglong/DefaultTasks/bili_task_base.inc")
    cleanup = read("qinglong/DefaultTasks/bili_task_cleanup.inc")

    required = (
        'export BILI_MODE="${Zzz_BILI_MODE:-dotnet}"',
        'export BILI_GITHUB_PROXY="${Zzz_BILI_GITHUB_PROXY:-}"',
        'export BILI_USE_CN_MIRROR="${Zzz_BILI_USE_CN_MIRROR:-false}"',
        "export Zzz_IsolatedMode=true",
    )
    for fragment in required:
        if fragment not in base:
            fail(f"Qinglong isolation invariant missing: {fragment}")

    forbidden_fallbacks = ("${BILI_MODE:-", "${BILI_GITHUB_PROXY:-", "${BILI_USE_CN_MIRROR:-")
    for fragment in forbidden_fallbacks:
        if fragment in base:
            fail(f"Zzz-Bili must not inherit generic shell helper variable: {fragment}")

    if "bili_jct|csrf|csrf_token" not in notify_helper:
        fail("Qinglong notification redaction must cover csrf and csrf_token")
    if "spawnSync" not in notify_helper or "killSignal: 'SIGKILL'" not in notify_helper:
        fail("Qinglong notification providers must run in isolated hard-timeout child processes")
    if "process.exit(0);" not in notify_helper or "Zzz_BILI_NOTIFY_TIMEOUT_MS" not in notify_helper:
        fail("Qinglong notification helper must have a bounded deterministic exit path")

    if 'use_cn_mirror=${BILI_USE_CN_MIRROR:-"false"}' not in common_base:
        fail("Qinglong common base must not modify apt/apk mirrors unless explicitly enabled")
    if "bili_branch" in common_base or "QL_BRANCH" in common_base:
        fail("Qinglong common base must not retain legacy develop-branch routing")

    task_dir = ROOT / "qinglong/SubscriptionTasks"
    for path in task_dir.glob("zzz_bili_task_*.sh"):
        content = path.read_text(encoding="utf-8-sig")
        standard = '$BILI_REPO_ROOT/ZzzHe2333_BiliBiliToolPro'
        legacy = '$BILI_REPO_ROOT/ZzzHe2333_BiliBiliToolPro_main'
        if standard not in content or legacy not in content:
            fail(f"Qinglong task lacks deterministic repo candidates: {path.relative_to(ROOT)}")

    standard_preference = re.search(
        r"ZzzHe2333.*?BiliBiliToolPro.*?SubscriptionTasks.*?score\s*\+=\s*200",
        cleanup,
        re.S,
    )
    legacy_preference = re.search(
        r"ZzzHe2333.*?BiliBiliToolPro_main.*?SubscriptionTasks.*?score\s*\+=\s*100",
        cleanup,
        re.S,
    )
    if not standard_preference or not legacy_preference:
        fail("Qinglong deduplication must prefer the standard repository task path")


def audit_sensitive_logging() -> None:
    log_filter = read("src/Ray.BiliBiliTool.Agent/Attributes/LogFilterAttribute.cs")
    redactor = read("src/Ray.BiliBiliTool.Agent/Attributes/SensitiveLogRedactor.cs")

    if "SensitiveLogRedactor.Redact(logMessage);" not in log_filter:
        fail("WebApiClient diagnostics must be redacted before any log sink receives them")

    for token in ("SESSDATA", "bili_jct", "csrf", "csrf_token", "Authorization"):
        if token not in redactor:
            fail(f"HTTP log redactor is missing sensitive token coverage: {token}")

    if "RequestHeaders = Redact" not in redactor or "RequestContent = Redact" not in redactor:
        fail("HTTP log redactor must sanitize both request headers and request content")
    if "ResponseHeaders = Redact" not in redactor or "ResponseContent = Redact" not in redactor:
        fail("HTTP log redactor must sanitize both response headers and response content")


def has_main_provenance_guard(content: str) -> bool:
    return (
        "Verify main provenance" in content
        and "git rev-parse origin/main" in content
        and "GITHUB_SHA" in content
    )


def audit_release_workflows() -> None:
    image = read(".github/workflows/publish-image.yml")
    release = read(".github/workflows/publish-release.yml")
    rolling = read(".github/workflows/publish-fork-rolling-release.yml")
    scf = read(".github/workflows/auto-deploy-tencent-scf.yml")

    if re.search(r"^\s{2}release:\s*$", image, re.M):
        fail("publish-image.yml must not independently react to Release creation")
    if "main-${GITHUB_SHA::12}" not in image:
        fail("main container builds must have immutable commit-derived tag")
    if "concurrency:" not in image or "cancel-in-progress: true" not in image:
        fail("rolling image publication must prevent stale builds from overwriting latest")
    if "DOCKERHUB_" in image or "docker.io/" in image.lower():
        fail("container publishing must not have an implicit DockerHub side channel")
    manual_latest = re.search(
        r"workflow_dispatch:.*?publishLatest:.*?default:\s*false",
        image,
        re.S,
    )
    if not manual_latest:
        fail("manual image publication must default to not updating latest")
    if "Verify latest provenance" not in image or "git rev-parse origin/main" not in image:
        fail("manual latest publication must verify the selected ref is current main")

    if "uses: ./.github/workflows/publish-image.yml" not in release:
        fail("formal Release workflow must explicitly publish its matching container image")
    if "Validate release version" not in release:
        fail("formal Release workflow must validate version provenance")
    if "publishLatest: false" not in release:
        fail("formal Release container publication must not race the rolling latest tag")
    if not has_main_provenance_guard(release):
        fail("formal Release workflow must only publish current main")

    if '--target "$GITHUB_SHA"' not in rolling:
        fail("fork-main Release metadata must track current rolling commit")
    if not has_main_provenance_guard(rolling):
        fail("fork-main rolling workflow must only publish current main")

    if not has_main_provenance_guard(scf):
        fail("Tencent SCF deployment must only deploy current main")


def audit_repository_maintenance() -> None:
    branch_cleanup = read(".github/workflows/cleanup-merged-branch.yml")
    if "github.event.pull_request.merged == true" not in branch_cleanup:
        fail("merged-branch cleanup must only run for successfully merged pull requests")
    if "github.event.pull_request.head.repo.full_name == github.repository" not in branch_cleanup:
        fail("merged-branch cleanup must never attempt to delete branches in external repositories")
    if "gh api --method DELETE" not in branch_cleanup:
        fail("merged-branch cleanup workflow is missing the branch deletion action")


def main() -> int:
    audit_versions()
    audit_retired_tasks()
    audit_detachment()
    audit_solution_items()
    audit_qinglong_isolation()
    audit_sensitive_logging()
    audit_release_workflows()
    audit_repository_maintenance()

    if ERRORS:
        print("Repository audit failed:", file=sys.stderr)
        for error in ERRORS:
            print(f" - {error}", file=sys.stderr)
        return 1

    print("Repository audit passed: independent-repo invariants are consistent.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
