@echo off

echo Start to build docker image with amd64-arch
@echo on
docker buildx build --tag ghcr.io/zzzhe2333/bili_tool_web:latest --output "type=image,push=false" --platform linux/amd64 ../..
@echo off
pause
