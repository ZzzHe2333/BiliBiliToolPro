@echo off

echo Start to build docker image
@echo on
docker build --tag ghcr.io/zzzhe2333/bili_tool_web:latest ../..
@echo off
pause
