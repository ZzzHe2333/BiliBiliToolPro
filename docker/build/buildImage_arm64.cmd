@echo off

echo Start to build docker image with arm64-arch
@echo on
docker buildx build --platform linux/arm64 -o type=docker -t ghcr.io/zzzhe2333/bili_tool_web:latest ../..
@echo off
pause
