#!/usr/bin/env python3
"""Focused hardening invariants that complement audit_repository.py."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ERRORS: list[str] = []


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8-sig")


def require(condition: bool, message: str) -> None:
    if not condition:
        ERRORS.append(message)


def main() -> int:
    cleanup = read("qinglong/DefaultTasks/bili_task_cleanup.inc")
    require(
        "BiliBiliToolPro(?:_main)?[\\\\/_-]qinglong" in cleanup,
        "Qinglong cleanup ownership must require this repository's qinglong path",
    )

    state_store = read("src/Ray.BiliBiliTool.DomainService/BCoinCouponStateStore.cs")
    require(
        "ConcurrentDictionary<long, SemaphoreSlim>" in state_store,
        "B-coin state updates must have a per-account in-process gate",
    )
    require(
        "FileShare.None" in state_store and '.lock' in state_store,
        "B-coin state updates must hold a cross-process lock file",
    )

    rolling = read(".github/workflows/publish-fork-rolling-release.yml")
    require(
        "sha256sum" in rolling and "fork-main-sha256.txt" in rolling,
        "fork-main rolling assets must publish a SHA256 manifest",
    )

    scf = read(".github/workflows/auto-deploy-tencent-scf.yml")
    require(
        "serverless-cloud-framework@1.3.2" in scf,
        "Tencent SCF workflow must pin the serverless-cloud-framework CLI version",
    )

    check_gate = read("scripts/verify_required_checks.sh")
    require(
        'required=("Repository audit" "CodeQL")' in check_gate,
        "publication gate must require both Repository audit and CodeQL",
    )
    require(
        'select(.name == $workflow_name and .head_sha == $sha and .event == "push")' in check_gate,
        "publication gate must bind required checks to the exact push commit",
    )

    gated_workflows = (
        ".github/workflows/publish-image.yml",
        ".github/workflows/publish-fork-rolling-release.yml",
        ".github/workflows/publish-release.yml",
        ".github/workflows/auto-deploy-tencent-scf.yml",
    )
    for workflow_path in gated_workflows:
        workflow = read(workflow_path)
        require(
            "verify_required_checks.sh" in workflow,
            f"release/deployment workflow must enforce required checks: {workflow_path}",
        )
        require(
            "actions: read" in workflow,
            f"release/deployment workflow must grant only the actions read permission it needs: {workflow_path}",
        )

    if ERRORS:
        print("Hardening audit failed:", file=sys.stderr)
        for error in ERRORS:
            print(f" - {error}", file=sys.stderr)
        return 1

    print("Hardening audit passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
