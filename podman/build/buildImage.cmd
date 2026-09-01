@echo off

echo Start to build Podman image
@echo on
podman build -t ghcr.io/zzzhe2333/bili_tool_web:latest ../..
@echo off
pause
