#!/usr/bin/env node
'use strict';

const fs = require('fs');
const path = require('path');
const { spawnSync } = require('child_process');

const targetCode = process.argv[2] || 'Task';
const logFile = process.argv[3];
const taskStatus = Number(process.argv[4] || 0);
const qlDir = process.env.QL_DIR || '/ql';
const qlDataDir = (process.env.QL_DATA_DIR || path.join(qlDir, 'data')).replace(/\/$/, '');
const statusText = taskStatus === 0 ? '成功' : taskStatus === 2 ? '部分失败' : `失败(${taskStatus})`;
const title = `Zzz-Bili ${targetCode} - ${statusText}`;

function parseTimeoutMs() {
  const raw = Number(process.env.Zzz_BILI_NOTIFY_TIMEOUT_MS || 35000);
  if (!Number.isFinite(raw) || raw < 100 || raw > 120000) return 35000;
  return Math.floor(raw);
}

const notifyTimeoutMs = parseTimeoutMs();

function redactSensitiveContent(content) {
  content = content.replace(
    /\b(SESSDATA|bili_jct|csrf|csrf_token|DedeUserID__ckMd5|DedeUserID|sid|buvid3|buvid4|buvid_fp|buvid_fp_plain|b_nut|b_lsid|_uuid|ac_time_value|access_key|bili_ticket)\s*=\s*(?:"[^"]*"|'[^']*'|[^;&\s"'<>]+)/gi,
    '$1=[已隐藏]'
  );
  content = content.replace(
    /("(?:SESSDATA|bili_jct|csrf|csrf_token|DedeUserID__ckMd5|DedeUserID|sid|buvid3|buvid4|buvid_fp|buvid_fp_plain|b_nut|b_lsid|_uuid|ac_time_value|access_key|bili_ticket)"\s*:\s*")[^"]*(")/gi,
    '$1[已隐藏]$2'
  );
  content = content.replace(/(Zzz_BiliBiliCookies__\d+\s*[:=]\s*)[^\r\n]+/gi, '$1[已隐藏]');
  content = content.replace(/((?:Cookie|Set-Cookie|Authorization)\s*[:=]\s*)[^\r\n]+/gi, '$1[已隐藏]');
  content = content.replace(
    /("(?:Cookie|Set-Cookie|Authorization)"\s*:\s*")[^"]*(")/gi,
    '$1[已隐藏]$2'
  );
  content = content.replace(
    /("(?:access_token|refresh_token|client_secret|ClientSecret|ClientId|qrcode_key|readkey|scKey|turboScKey|botToken|sKey|secret|token|webhook|webHookUrl|apiKey|apikey)"\s*:\s*")[^"]*(")/gi,
    '$1[已隐藏]$2'
  );
  content = content.replace(
    /\b((?:access_token|refresh_token|client_secret|ClientSecret|ClientId|qrcode_key|readkey|scKey|turboScKey|botToken|sKey|secret|token|webhook|webHookUrl|apiKey|apikey)\s*[:=]\s*)(?:"[^"]*"|'[^']*'|[^&\s]+)/gi,
    '$1[已隐藏]'
  );
  return content;
}

function prepareContent() {
  let content = '';
  try {
    content = fs.readFileSync(logFile, 'utf8');
  } catch (error) {
    return `任务退出码：${taskStatus}\n读取任务日志失败：${error.message}`;
  }

  content = content.replace(/\x1B(?:[@-Z\\-_]|\[[0-?]*[ -\/]*[@-~])/g, '');
  const marker = 'BiliBiliToolPro 开始运行...';
  const markerIndex = content.indexOf(marker);
  if (markerIndex >= 0) {
    const lineStart = content.lastIndexOf('\n', markerIndex);
    content = content.slice(lineStart >= 0 ? lineStart + 1 : markerIndex);
  }

  content = redactSensitiveContent(content).trim();
  if (!content) content = `任务执行结束，退出码：${taskStatus}`;

  const maxLength = 16000;
  if (content.length > maxLength) {
    const half = Math.floor((maxLength - 80) / 2);
    content = `${content.slice(0, half)}\n\n……通知正文过长，中间已省略……\n\n${content.slice(-half)}`;
  }
  return content;
}

function childFailureDetail(result) {
  if (result.error?.code === 'ETIMEDOUT') return `timeout>${notifyTimeoutMs}ms`;
  return (result.stdout || result.stderr || result.error?.message || `exit=${result.status ?? 'unknown'}`).trim();
}

function trySystemNotify(content) {
  const clientPath = path.join(qlDir, 'shell/preload/client.js');
  if (!fs.existsSync(clientPath)) {
    console.warn(`[Zzz-Bili] 青龙 systemNotify 客户端不存在：${clientPath}`);
    return false;
  }

  const childCode = `
const fs = require('fs');
(async () => {
  const payload = JSON.parse(fs.readFileSync(0, 'utf8'));
  const api = require(${JSON.stringify(clientPath)});
  const result = await api.systemNotify(payload);
  if (typeof api.close === 'function') api.close();
  process.stdout.write(JSON.stringify(result || {}));
  process.exit(Number(result?.code) === 200 ? 0 : 2);
})().catch((error) => {
  console.error(error?.message || String(error));
  process.exit(1);
});`;

  const result = spawnSync(process.execPath, ['-e', childCode], {
    input: JSON.stringify({ title, content }),
    encoding: 'utf8',
    timeout: notifyTimeoutMs,
    killSignal: 'SIGKILL',
    env: process.env,
  });

  if (result.status === 0) {
    console.log('[Zzz-Bili] 已使用青龙面板系统通知发送结果');
    return true;
  }

  let detail = childFailureDetail(result);
  try {
    const response = JSON.parse(result.stdout || '{}');
    detail = `code=${response.code ?? 'unknown'}, message=${response.message ?? ''}`;
  } catch (_) {}
  console.warn(`[Zzz-Bili] 青龙面板系统通知失败：${detail}`);
  return false;
}

function tryEnvNotify(content) {
  const candidates = [
    path.join(qlDataDir, 'scripts/sendNotify.js'),
    path.join(qlDir, 'scripts/sendNotify.js'),
  ];
  const notifyPath = candidates.find((item) => fs.existsSync(item));

  if (!notifyPath) {
    console.warn('[Zzz-Bili] 未找到青龙 sendNotify.js，无法使用环境变量通知兜底');
    return false;
  }

  const childCode = `
const fs = require('fs');
(async () => {
  const payload = JSON.parse(fs.readFileSync(0, 'utf8'));
  const notifyModule = require(${JSON.stringify(notifyPath)});
  const sendNotify = notifyModule.sendNotify || notifyModule.send;
  if (typeof sendNotify !== 'function') {
    throw new Error('sendNotify/send is not exported');
  }
  await sendNotify(payload.title, payload.content);
  process.exit(0);
})().catch((error) => {
  console.error(error?.message || String(error));
  process.exit(1);
});`;

  const result = spawnSync(process.execPath, ['-e', childCode], {
    input: JSON.stringify({ title, content }),
    encoding: 'utf8',
    timeout: notifyTimeoutMs,
    killSignal: 'SIGKILL',
    env: process.env,
  });

  if (result.status === 0) {
    console.log('[Zzz-Bili] 已回退到青龙环境变量通知流程');
    return true;
  }

  console.warn(`[Zzz-Bili] 青龙环境变量通知失败：${childFailureDetail(result)}`);
  return false;
}

function main() {
  const content = prepareContent();
  if (!trySystemNotify(content)) {
    tryEnvNotify(content);
  }
}

try {
  main();
} catch (error) {
  console.warn(`[Zzz-Bili] 通知处理异常：${error?.message || String(error)}`);
}

// sendNotify.js frequently imports providers that leave timers/sockets behind.
// The wrapper has already waited for the isolated child (with a hard timeout),
// so force this helper to terminate instead of keeping the Qinglong task alive.
process.exit(0);
