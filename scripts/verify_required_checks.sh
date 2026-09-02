#!/usr/bin/env bash
# Wait until the current commit has passed the repository's required push checks.
# Publication/deployment workflows use this as a server-side safety net even when
# GitHub branch protection has not yet been enabled.

set -euo pipefail

sha="${1:-${GITHUB_SHA:-}}"
if [ -z "$sha" ]; then
  echo "Missing commit SHA" >&2
  exit 2
fi

repo="${GITHUB_REPOSITORY:-}"
if [ -z "$repo" ]; then
  echo "Missing GITHUB_REPOSITORY" >&2
  exit 2
fi

if ! command -v gh >/dev/null 2>&1 || ! command -v jq >/dev/null 2>&1; then
  echo "gh and jq are required to verify GitHub checks" >&2
  exit 2
fi

wait_seconds="${REQUIRED_CHECKS_WAIT_SECONDS:-1200}"
interval_seconds="${REQUIRED_CHECKS_POLL_SECONDS:-10}"
case "$wait_seconds" in
  ''|*[!0-9]*) echo "Invalid REQUIRED_CHECKS_WAIT_SECONDS: $wait_seconds" >&2; exit 2 ;;
esac
case "$interval_seconds" in
  ''|*[!0-9]*|0) echo "Invalid REQUIRED_CHECKS_POLL_SECONDS: $interval_seconds" >&2; exit 2 ;;
esac

required=("Repository audit" "CodeQL")
elapsed=0

while true; do
  runs="$(gh api "/repos/${repo}/actions/runs?head_sha=${sha}&per_page=100")"
  pending=false

  for workflow_name in "${required[@]}"; do
    record="$(
      jq -r --arg workflow_name "$workflow_name" --arg sha "$sha" '
        [
          .workflow_runs[]
          | select(.name == $workflow_name and .head_sha == $sha and .event == "push")
        ]
        | sort_by(.created_at)
        | last
        | if . == null then "missing\t\t" else [.status, (.conclusion // "")] | @tsv end
      ' <<<"$runs"
    )"

    IFS=$'\t' read -r status conclusion <<<"$record"
    if [ "$status" = "completed" ]; then
      if [ "$conclusion" != "success" ]; then
        echo "Required workflow failed for $sha: $workflow_name -> ${conclusion:-unknown}" >&2
        exit 1
      fi
      echo "Required workflow passed: $workflow_name"
      continue
    fi

    pending=true
    echo "Required workflow not ready: $workflow_name -> ${status:-missing}"
  done

  if [ "$pending" = "false" ]; then
    echo "All required workflows passed for $sha"
    exit 0
  fi

  if [ "$elapsed" -ge "$wait_seconds" ]; then
    echo "Timed out waiting for required workflows for $sha after ${wait_seconds}s" >&2
    exit 1
  fi

  sleep "$interval_seconds"
  elapsed=$((elapsed + interval_seconds))
done
