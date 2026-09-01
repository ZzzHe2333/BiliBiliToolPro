Write-Host "Build and push BiliTool Web multi-arch image"

docker buildx build `
  --tag "ghcr.io/zzzhe2333/bili_tool_web:3.8.2" `
  --tag "ghcr.io/zzzhe2333/bili_tool_web:latest" `
  --output "type=image,push=true" `
  --platform linux/amd64,linux/arm64 `
  ../..
