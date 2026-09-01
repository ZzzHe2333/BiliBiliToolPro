#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "$script_dir/.." && pwd)"
console_project="$repo_dir/src/Ray.BiliBiliTool.Console/Ray.BiliBiliTool.Console.csproj"
publish_dir="$script_dir/bin/publish"

rm -rf "$publish_dir"
mkdir -p "$publish_dir"

dotnet publish "$console_project" \
  --configuration Release \
  --runtime linux-x64 \
  --self-contained true \
  -p:PublishSingleFile=true \
  -p:DebugType=None \
  -p:DebugSymbols=false \
  -o "$publish_dir"

cp "$script_dir/bootstrap" "$script_dir/index.sh" "$publish_dir/"
chmod 755 "$publish_dir/index.sh" "$publish_dir/bootstrap"

(
  cd "$publish_dir"
  zip -r -q ../tencent-scf.zip ./*
)

echo "Tencent SCF package created: $script_dir/bin/tencent-scf.zip"
