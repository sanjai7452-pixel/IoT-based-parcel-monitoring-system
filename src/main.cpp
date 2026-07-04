#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ═══════════════════════════════════════════════════════════════════════
//  Smart Parcel Locker — v5.0
//  ESP32 + IR Sensor + Solenoid Lock + IP Camera
// ═══════════════════════════════════════════════════════════════════════

// ╔══════════════════════════════════╗
// ║   WIFI CONFIG — edit here only  ║
const char* WIFI_SSID     = "6661";
const char* WIFI_PASSWORD = "66616661";
// ╚══════════════════════════════════╝

// (WhatsApp notifications removed)


// ╔══════════════════════════════════════════════════════════════════╗
// ║  CAMERA IP — Your mobile IP Webcam app address                  ║
// ║  Android: Install "IP Webcam" app, start server, copy the IP    ║
// ║  Example: "http://192.168.1.25:8080"                            ║
#define CAMERA_IP  "http://10.135.168.6:8080"
// ╚══════════════════════════════════════════════════════════════════╝

#define RELAY_PIN             26
#define ULTRASONIC_TRIG_PIN   33
#define ULTRASONIC_ECHO_PIN   32
#define DISTANCE_THRESHOLD_CM 15
#define MAX_ECHO_TIME         30000  // microseconds

#define RELAY_UNLOCK  LOW    // LOW  = relay ON  = lock OPEN
#define RELAY_LOCK    HIGH   // HIGH = relay OFF = lock CLOSED

WebServer server(80);
bool isLocked = true;



// (notifications removed)

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// PWA ASSETS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

const char MANIFEST[] PROGMEM = R"json({
  "name":"Smart Parcel Locker",
  "short_name":"Parcel Locker",
  "start_url":"/","display":"standalone",
  "background_color":"#080e17","theme_color":"#3b82f6"
})json";

const char ICON_SVG[] PROGMEM = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100">
<rect width="100" height="100" rx="22" fill="#080e17"/>
<path d="M50 18L78 33v34L50 82 22 67V33z" fill="none" stroke="#3b82f6" stroke-width="3.5"/>
<polyline points="22,33 50,48 78,33" fill="none" stroke="#3b82f6" stroke-width="3.5"/>
<line x1="50" y1="48" x2="50" y2="82" stroke="#3b82f6" stroke-width="3.5"/>
</svg>)svg";

const char SW_JS[] PROGMEM = R"sw(
const C='parcel-v3';
self.addEventListener('install',e=>{e.waitUntil(caches.open(C).then(c=>c.add('/')));self.skipWaiting();});
self.addEventListener('activate',e=>{e.waitUntil(clients.claim());});
self.addEventListener('fetch',e=>{e.respondWith(fetch(e.request).catch(()=>caches.match(e.request)));});
)sw";

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// HTML — split into 3 parts so we can inject CAMERA_IP in the middle
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

const char HTML_A[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8"/>
<meta name="viewport" content="width=device-width,initial-scale=1.0"/>
<title>Smart Parcel Locker</title>
<link rel="manifest" href="/manifest.json">
<meta name="theme-color" content="#3b82f6">
<link rel="preconnect" href="https://fonts.googleapis.com"/>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet"/>
<style>
:root{--bg:#080e17;--surface:#0d1520;--surface2:#111c2d;--border:rgba(255,255,255,.07);--border2:rgba(255,255,255,.13);--accent:#3b82f6;--accent2:#6366f1;--green:#10b981;--red:#f43f5e;--text:#e2e8f0;--text2:#64748b;--text3:#334155;--mono:'JetBrains Mono',monospace;}
*,*::before,*::after{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Inter',system-ui,sans-serif;background:var(--bg);min-height:100vh;display:flex;flex-direction:column;align-items:center;color:var(--text);padding:32px 16px 64px;background-image:radial-gradient(ellipse 80% 50% at 20% -10%,rgba(59,130,246,.06) 0%,transparent 60%),radial-gradient(ellipse 60% 40% at 80% 110%,rgba(99,102,241,.05) 0%,transparent 60%);}
.header{text-align:center;margin-bottom:24px;animation:fadeDown .5s ease}
.eyebrow{display:inline-flex;align-items:center;gap:8px;font-size:.72rem;font-weight:600;letter-spacing:2px;text-transform:uppercase;color:var(--accent);margin-bottom:10px}
.eyebrow span{width:22px;height:1px;background:var(--accent);opacity:.4}
.header h1{font-size:1.75rem;font-weight:700;letter-spacing:-.5px}
.header p{color:var(--text2);font-size:.83rem;margin-top:5px}
.ip-pill{display:inline-flex;align-items:center;gap:8px;background:var(--surface);border:1px solid var(--border);border-radius:99px;padding:6px 16px;font-size:.8rem;color:var(--accent);font-family:var(--mono);margin-top:10px;letter-spacing:.5px}
.ip-pill .dlive{width:7px;height:7px;border-radius:50%;background:var(--green);box-shadow:0 0 0 3px rgba(16,185,129,.2);flex-shrink:0}
.conn-bar{display:inline-flex;align-items:center;gap:8px;background:var(--surface);border:1px solid var(--border);border-radius:99px;padding:5px 14px 5px 10px;font-size:.75rem;color:var(--text2);margin-bottom:24px;animation:fadeDown .5s ease}
.dot{width:7px;height:7px;border-radius:50%;background:var(--text3);transition:all .4s;flex-shrink:0}
.dot.on{background:var(--green);box-shadow:0 0 0 3px rgba(16,185,129,.2)}
.dot.off{background:var(--red);box-shadow:0 0 0 3px rgba(244,63,94,.2)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:16px;width:100%;max-width:920px}
.card{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:22px;animation:fadeUp .55s ease;transition:border-color .2s}
.card:hover{border-color:var(--border2)}
.ct{display:flex;align-items:center;gap:7px;font-size:.68rem;font-weight:700;letter-spacing:1.5px;text-transform:uppercase;color:var(--text2);margin-bottom:20px}
.lock-wrap{display:flex;justify-content:center;margin-bottom:18px}
.lock-bg{width:76px;height:76px;border-radius:18px;background:var(--surface2);border:1px solid var(--border);display:flex;align-items:center;justify-content:center;transition:all .4s}
.lock-bg.open{background:rgba(16,185,129,.08);border-color:rgba(16,185,129,.25)}
.sbadge{display:flex;align-items:center;justify-content:center;padding:9px 14px;border-radius:8px;font-size:.82rem;font-weight:600;margin-bottom:16px;font-family:var(--mono);letter-spacing:.5px;transition:all .4s}
.sbadge.lk{background:rgba(244,63,94,.08);border:1px solid rgba(244,63,94,.2);color:#fda4af}
.sbadge.ul{background:rgba(16,185,129,.08);border:1px solid rgba(16,185,129,.2);color:#6ee7b7}
.brow{display:flex;gap:10px}
.btn{flex:1;padding:10px 8px;border:1px solid var(--border);border-radius:10px;font-size:.82rem;font-weight:600;cursor:pointer;font-family:inherit;transition:all .2s;background:var(--surface2);color:var(--text)}
.btn:active{transform:scale(.97)}
.btn:disabled{opacity:.3;cursor:not-allowed}
.blk:hover:not(:disabled){background:rgba(244,63,94,.1);border-color:rgba(244,63,94,.3);color:#fda4af}
.bul:hover:not(:disabled){background:rgba(16,185,129,.1);border-color:rgba(16,185,129,.3);color:#6ee7b7}
.p-ring-w{display:flex;justify-content:center;margin-bottom:16px}
.p-ring{width:76px;height:76px;border-radius:18px;background:var(--surface2);border:1px solid var(--border);display:flex;align-items:center;justify-content:center;transition:all .5s}
.p-ring.on{background:rgba(16,185,129,.08);border-color:rgba(16,185,129,.25);box-shadow:0 0 22px rgba(16,185,129,.1);animation:glow 2.5s ease-in-out infinite}
.p-lbl{text-align:center;font-size:1rem;font-weight:600;margin-bottom:4px;transition:color .4s}
.p-lbl.on{color:#6ee7b7}.p-lbl.off{color:var(--text2)}
.p-sub{text-align:center;font-size:.78rem;color:var(--text2);margin-bottom:16px;line-height:1.5}
.div{height:1px;background:var(--border);margin:0 -22px 16px}
.ir-bar{display:flex;align-items:center;gap:9px;background:var(--surface2);border:1px solid var(--border);border-radius:8px;padding:8px 12px;font-size:.75rem;color:var(--text2);font-family:var(--mono)}
.ir-led{width:7px;height:7px;border-radius:50%;flex-shrink:0;transition:all .4s;background:var(--text3)}
.ir-led.on{background:var(--green);box-shadow:0 0 0 3px rgba(16,185,129,.2)}
.wc{width:100%;max-width:920px}
.sc{background:var(--surface);border:1px solid var(--border);border-radius:16px;padding:22px;margin-top:16px;animation:fadeUp .6s ease}
.cam-wrap{position:relative;width:100%;background:#000;border-radius:12px;overflow:hidden;border:1px solid var(--border);min-height:260px;display:flex;align-items:center;justify-content:center;margin-bottom:14px}
.cam-wrap img.lv{width:100%;height:auto;display:block;max-height:480px;object-fit:contain;border-radius:10px}
.cam-ph{display:flex;flex-direction:column;align-items:center;gap:10px;color:var(--text3);font-size:.82rem;padding:60px 20px;text-align:center;width:100%}
.cam-ph svg{opacity:.3}
.cbadge{position:absolute;top:10px;right:10px;background:rgba(0,0,0,.75);border:1px solid rgba(16,185,129,.3);border-radius:6px;padding:4px 10px;font-size:.7rem;color:var(--green);font-family:var(--mono);display:none}
.cbadge.on{display:block}
.citag{position:absolute;bottom:10px;left:10px;background:rgba(0,0,0,.75);border:1px solid var(--border);border-radius:6px;padding:4px 10px;font-size:.68rem;color:var(--text2);font-family:var(--mono);display:none}
.citag.on{display:block}
.bsec{padding:8px 16px;border:1px solid var(--border);border-radius:8px;cursor:pointer;background:var(--surface2);color:var(--text);font-weight:600;font-size:.8rem;font-family:inherit;transition:all .2s}
.bsec:hover{border-color:var(--accent);color:var(--accent)}
.snap-g{display:grid;grid-template-columns:1fr 1fr;gap:16px;align-items:start}
@media(max-width:540px){.snap-g{grid-template-columns:1fr}}
.sf{background:var(--surface2);border:1px solid var(--border);border-radius:12px;overflow:hidden;aspect-ratio:4/3;display:flex;align-items:center;justify-content:center;position:relative}
.sf img.si{width:100%;height:100%;object-fit:cover;display:block;border-radius:12px}
.sol{display:flex;flex-direction:column;align-items:center;justify-content:center;gap:8px;color:var(--text3);font-size:.78rem;text-align:center;padding:20px;width:100%;height:100%}
.sol svg{opacity:.2;margin-bottom:4px}
.smeta{display:flex;flex-direction:column;gap:10px;padding:4px 0}
.stag{display:inline-flex;align-items:center;gap:6px;border:1px solid var(--border);border-radius:7px;padding:5px 10px;font-size:.72rem;font-family:var(--mono)}
.stag .sl{width:6px;height:6px;border-radius:50%;background:currentColor;flex-shrink:0}
.stag.det{background:rgba(16,185,129,.08);border-color:rgba(16,185,129,.2);color:#6ee7b7}
.stag.wait{background:rgba(59,130,246,.06);border-color:rgba(59,130,246,.15);color:#93c5fd}
.stag.abs{background:rgba(100,116,139,.08);border-color:rgba(100,116,139,.2);color:var(--text2)}
.stm{font-size:.7rem;color:var(--text3);font-family:var(--mono)}
.sinf{font-size:.82rem;color:var(--text2);line-height:1.6}
.sinf strong{color:var(--text);font-weight:600}
.sbtns{display:flex;gap:8px;flex-wrap:wrap;margin-top:12px}
.htitle{font-size:.68rem;font-weight:700;letter-spacing:1.4px;text-transform:uppercase;color:var(--text2);margin:20px 0 12px}
.hg{display:grid;grid-template-columns:repeat(auto-fill,minmax(80px,1fr));gap:10px}
.hi{position:relative;border:1px solid var(--border);border-radius:9px;overflow:hidden;aspect-ratio:1;cursor:pointer;transition:all .25s;background:var(--surface2)}
.hi:hover{border-color:var(--accent);transform:scale(1.05);box-shadow:0 4px 14px rgba(59,130,246,.2)}
.hi img{width:100%;height:100%;object-fit:cover;display:block}
.ht{position:absolute;bottom:0;left:0;right:0;background:linear-gradient(to top,rgba(0,0,0,.85),transparent);color:var(--text);font-size:.6rem;padding:6px 5px 4px;font-family:var(--mono)}
.ll{list-style:none;display:flex;flex-direction:column;gap:3px;max-height:160px;overflow-y:auto}
.ll::-webkit-scrollbar{width:3px}.ll::-webkit-scrollbar-thumb{background:var(--border);border-radius:4px}
.li{display:flex;align-items:center;gap:9px;font-size:.75rem;color:var(--text2);padding:6px 9px;border-radius:7px;border-left:2px solid transparent;transition:background .2s}
.li:hover{background:var(--surface2)}
.li.lk{border-left-color:var(--red)}.li.ul{border-left-color:var(--green)}
.li.pk{border-left-color:var(--accent2)}.li.inf{border-left-color:var(--accent)}
.lt{color:var(--text3);font-size:.68rem;white-space:nowrap;min-width:50px;font-family:var(--mono)}
.rbar{max-width:920px;width:100%;margin-top:12px;display:flex;align-items:center;font-size:.7rem;color:var(--text3);font-family:var(--mono)}
.rt{height:1px;background:var(--border);border-radius:2px;overflow:hidden;flex:1;margin:0 12px}
.rf{height:100%;background:var(--accent);width:0%}
.rf.run{animation:rfill 2s linear infinite}
.toast{position:fixed;bottom:24px;left:50%;transform:translateX(-50%) translateY(16px);background:var(--surface);border:1px solid var(--border);border-radius:10px;padding:10px 20px;font-size:.8rem;color:var(--text);opacity:0;transition:all .3s;pointer-events:none;box-shadow:0 8px 32px rgba(0,0,0,.6);z-index:99;white-space:nowrap}
.toast.show{opacity:1;transform:translateX(-50%) translateY(0)}
.toast.ok{border-color:rgba(16,185,129,.35);color:#6ee7b7}
.toast.err{border-color:rgba(244,63,94,.35);color:#fda4af}
@keyframes fadeDown{from{opacity:0;transform:translateY(-12px)}to{opacity:1;transform:translateY(0)}}
@keyframes fadeUp{from{opacity:0;transform:translateY(12px)}to{opacity:1;transform:translateY(0)}}
@keyframes glow{0%,100%{box-shadow:0 0 22px rgba(16,185,129,.1)}50%{box-shadow:0 0 30px rgba(16,185,129,.22)}}
@keyframes rfill{from{width:0%}to{width:100%}}
@media(max-width:480px){.header h1{font-size:1.4rem}.card,.sc{padding:16px}}
</style>
</head>
<body>
<div class="header">
  <div class="eyebrow"><span></span>IoT Control Panel<span></span></div>
  <h1>Smart Parcel Locker</h1>
  <p>ESP32 Wireless Lock &amp; Parcel Detection</p>
  <div class="ip-pill"><div class="dlive"></div><span id="ipDisp">Loading...</span></div>
</div>
<div class="conn-bar"><div class="dot" id="cDot"></div><span id="cTxt">Connecting to ESP32...</span></div>
<div class="grid">
  <div class="card">
    <div class="ct"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>Lock Control</div>
    <div class="lock-wrap"><div class="lock-bg" id="lbg">
      <svg id="lsvg" width="34" height="34" viewBox="0 0 24 24" fill="none" stroke="#3b82f6" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round">
        <rect x="3" y="11" width="18" height="11" rx="2" fill="rgba(59,130,246,.1)" stroke="#3b82f6"/>
        <path id="shk" d="M7 11V7a5 5 0 0 1 10 0v4"/>
        <circle cx="12" cy="16" r="1.4" fill="#3b82f6" stroke="none"/>
      </svg>
    </div></div>
    <div class="sbadge lk" id="sbg"><span id="stx">LOCKED</span></div>
    <div class="brow">
      <button class="btn blk" id="bLk" onclick="cmd('lock')"   disabled>&#x1F512; Lock</button>
      <button class="btn bul" id="bUl" onclick="cmd('unlock')" disabled>&#x1F513; Unlock</button>
    </div>
  </div>
  <div class="card">
    <div class="ct"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/><polyline points="3.27 6.96 12 12.01 20.73 6.96"/><line x1="12" y1="22.08" x2="12" y2="12"/></svg>Parcel Detection</div>
    <div class="p-ring-w"><div class="p-ring" id="pr">
      <svg width="30" height="30" viewBox="0 0 24 24" fill="none" stroke="#64748b" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" id="pi">
        <path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>
        <polyline points="3.27 6.96 12 12.01 20.73 6.96"/><line x1="12" y1="22.08" x2="12" y2="12"/>
      </svg>
    </div></div>
    <div class="p-lbl off" id="pl">No Parcel</div>
    <div class="p-sub" id="ps">Connecting...</div>
    <div class="div"></div>
    <div class="ir-bar"><div class="ir-led" id="irl"></div><span>Ultrasonic &#8212; <span id="irt">Awaiting</span></span></div>
  </div>
</div>
<!-- LIVE CAMERA -->
<div class="wc"><div class="sc">
  <div class="ct"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/></svg>Live Camera Feed</div>
  <div class="cam-wrap" id="cw">
    <div class="cam-ph" id="cph"><svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/></svg><span id="cphtx">Starting camera...</span></div>
    <div class="cbadge" id="cb">&#9679; LIVE</div>
    <div class="citag" id="ci"></div>
  </div>
  <button class="bsec" onclick="captureNow()">&#128247; Capture Photo</button>
</div></div>
<!-- PARCEL SNAPSHOT -->
<div class="wc"><div class="sc">
  <div class="ct"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="3" width="18" height="18" rx="2"/><circle cx="8.5" cy="8.5" r="1.5"/><polyline points="21 15 16 10 5 21"/></svg>Parcel Snapshot</div>
  <div class="snap-g">
    <div class="sf" id="sf">
      <div class="sol" id="sol">
        <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.5"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/><polyline points="3.27 6.96 12 12.01 20.73 6.96"/><line x1="12" y1="22.08" x2="12" y2="12"/></svg>
        <span id="soltx">Waiting for parcel...</span>
        <span style="font-size:.7rem;color:var(--text3)">Auto-captured on detection</span>
      </div>
    </div>
    <div class="smeta">
      <div class="stag wait" id="stag"><span class="sl"></span><span id="stagtx">No parcel detected</span></div>
      <div class="stm" id="stm">Last capture: &#8212;</div>
      <div class="sinf" id="sinf">When the ultrasonic sensor detects a parcel, the camera will <strong>automatically capture</strong> a photo and show it here.</div>
      <div class="sbtns">
        <button class="bsec" onclick="captureNow()">&#128247; Manual Capture</button>
        <button class="bsec" onclick="clearSnap()">&#128465; Clear</button>
        <button class="bsec" id="bdl" onclick="dlSnap()" style="display:none">&#11015; Download</button>
      </div>
    </div>
  </div>
  <div id="hw" style="display:none"><div class="htitle">Capture History</div><div class="hg" id="hg"></div></div>
</div></div>
<!-- LOG -->
<div class="wc"><div class="card" style="margin-top:16px">
  <div class="ct"><svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/></svg>Activity Log</div>
  <ul class="ll" id="ll"><li class="li inf"><span class="lt">--:--:--</span><span>Dashboard starting...</span></li></ul>
</div></div>
<div class="rbar"><span>Auto-refresh 2s</span><div class="rt"><div class="rf" id="rf"></div></div><span id="lu">--</span></div>
<div class="toast" id="toast"></div>
<script>
var espBase=window.location.origin;
)html";

// Camera URL is injected here by handleRoot()
// Then HTML_C continues:

const char HTML_C[] PROGMEM = R"js(
var pLk=null,pPr=null,online=false,hist=[],curSnap=null;
function pad(n){return String(n).padStart(2,'0');}
function ts(){var d=new Date();return pad(d.getHours())+':'+pad(d.getMinutes())+':'+pad(d.getSeconds());}
function ds(){var d=new Date();return d.getFullYear()+'-'+pad(d.getMonth()+1)+'-'+pad(d.getDate());}
function lg(msg,cls){var ul=document.getElementById('ll');var li=document.createElement('li');li.className='li '+(cls||'');li.innerHTML='<span class="lt">'+ts()+'</span><span>'+msg+'</span>';ul.insertBefore(li,ul.firstChild);if(ul.children.length>50)ul.removeChild(ul.lastChild);}
function toast(msg,tp){var t=document.getElementById('toast');t.innerHTML=msg;t.className='toast show '+(tp||'');clearTimeout(t._t);t._t=setTimeout(function(){t.className='toast';},3000);}
function lockUI(lk){
  var b=document.getElementById('sbg'),bg=document.getElementById('lbg'),sk=document.getElementById('shk'),sv=document.getElementById('lsvg');
  b.className='sbadge '+(lk?'lk':'ul');bg.className='lock-bg'+(lk?'':' open');
  document.getElementById('stx').textContent=lk?'LOCKED':'UNLOCKED';
  var c=lk?'#3b82f6':'#10b981';sv.setAttribute('stroke',c);sk.setAttribute('stroke',c);
  sk.setAttribute('d',lk?'M7 11V7a5 5 0 0 1 10 0v4':'M7 11V5a5 5 0 0 1 9.9-1');
}
function parcelUI(p){
  document.getElementById('pr').className='p-ring '+(p?'on':'');
  document.getElementById('pi').setAttribute('stroke',p?'#10b981':'#64748b');
  var lb=document.getElementById('pl');lb.className='p-lbl '+(p?'on':'off');
  lb.textContent=p?'Parcel Detected':'No Parcel';
  document.getElementById('ps').textContent=p?'Package inside \u2014 please collect':'Waiting for delivery...';
  document.getElementById('irl').className='ir-led '+(p?'on':'');
  document.getElementById('irt').textContent=p?'Object detected':'Clear';
}
function setConn(yes){
  online=yes;
  document.getElementById('cDot').className='dot '+(yes?'on':'off');
  document.getElementById('cTxt').textContent=yes?'Connected to ESP32':'Connection lost \u2014 retrying...';
  document.getElementById('bLk').disabled=!yes;document.getElementById('bUl').disabled=!yes;
  document.getElementById('rf').className='rf '+(yes?'run':'');
}
function startCam(){
  if(!camURL)return;
  document.getElementById('ci').textContent=camURL;
  document.getElementById('ci').className='citag on';
  doRef();setInterval(doRef,1200);
}
function doRef(){
  if(!camURL)return;
  var w=document.getElementById('cw'),ph=document.getElementById('cph'),b=document.getElementById('cb');
  var url=camURL+'/shot.jpg?t='+Date.now();
  var img=new Image();
  img.style.cssText='width:100%;height:auto;max-height:480px;object-fit:contain;display:block;border-radius:10px;';
  img.onload=function(){var o=w.querySelector('img.lv');if(o)o.remove();img.className='lv';w.insertBefore(img,ph);ph.style.display='none';b.className='cbadge on';};
  img.onerror=function(){var o=w.querySelector('img.lv');if(o)o.remove();ph.style.display='flex';b.className='cbadge';document.getElementById('cphtx').textContent='Camera offline: '+camURL;};
  img.crossOrigin='anonymous';img.src=url;
}
function captureNow(){
  if(!camURL){toast('Camera not configured','err');return;}
  var url=camURL+'/shot.jpg?t='+Date.now();
  var img=new Image();img.crossOrigin='anonymous';
  img.onload=function(){
    var c=document.createElement('canvas');c.width=img.width||640;c.height=img.height||480;
    var ctx=c.getContext('2d');ctx.drawImage(img,0,0);
    try{saveSnap(c.toDataURL('image/jpeg',0.9),pPr);}
    catch(e){toast('Capture error (CORS)','err');lg('Capture failed: '+e.message,'');}
  };
  img.onerror=function(){toast('Camera unreachable','err');};
  img.src=url;
}
function saveSnap(data,parcel){
  curSnap=data;var t=ts(),d=ds();
  var entry={src:data,time:t,date:d,parcel:parcel};
  var fr=document.getElementById('sf');document.getElementById('sol').style.display='none';
  var o=fr.querySelector('img.si');if(o)o.remove();
  var si=document.createElement('img');si.className='si';si.src=data;si.style.cssText='width:100%;height:100%;object-fit:cover;display:block;border-radius:12px;';fr.appendChild(si);
  var tag=document.getElementById('stag'),tagtx=document.getElementById('stagtx');
  if(parcel){tag.className='stag det';tagtx.textContent='Parcel detected';}
  else{tag.className='stag abs';tagtx.textContent='No parcel at capture';}
  document.getElementById('stm').textContent='Last capture: '+t;
  document.getElementById('sinf').innerHTML='Captured <strong>'+t+'</strong> on <strong>'+d+'</strong> | Parcel: <strong>'+(parcel?'YES':'NO')+'</strong>';
  document.getElementById('bdl').style.display='inline-flex';
  hist.unshift(entry);if(hist.length>12)hist.pop();
  try{localStorage.setItem('sH',JSON.stringify(hist));}catch(e){}
  renderHist();lg('Snapshot | parcel:'+(parcel?'YES':'NO'),'pk');toast('Snapshot captured','ok');
  if(parcel&&Notification&&Notification.permission==='granted')
    new Notification('Parcel Arrived!',{body:'Photo at '+t,icon:'/icon.svg'});
}
function clearSnap(){
  curSnap=null;var fr=document.getElementById('sf');var o=fr.querySelector('img.si');if(o)o.remove();
  document.getElementById('sol').style.display='flex';document.getElementById('soltx').textContent='Waiting for parcel...';
  document.getElementById('stag').className='stag wait';document.getElementById('stagtx').textContent='No parcel detected';
  document.getElementById('stm').textContent='Last capture: \u2014';
  document.getElementById('sinf').innerHTML='When the ultrasonic sensor detects a parcel, the camera will <strong>automatically capture</strong> a photo and show it here.';
  document.getElementById('bdl').style.display='none';toast('Cleared','ok');
}
function dlSnap(){if(!curSnap)return;var a=document.createElement('a');a.href=curSnap;a.download='parcel_'+ds()+'_'+ts().replace(/:/g,'-')+'.jpg';a.click();}
function renderHist(){
  var g=document.getElementById('hg'),w=document.getElementById('hw');
  if(hist.length===0){w.style.display='none';return;}
  w.style.display='block';g.innerHTML='';
  hist.forEach(function(item,i){
    var d=document.createElement('div');d.className='hi';
    var img=document.createElement('img');img.src=item.src;img.alt=''+i;
    var t=document.createElement('div');t.className='ht';t.textContent=item.time;
    d.appendChild(img);d.appendChild(t);
    d.onclick=function(){
      curSnap=item.src;var fr=document.getElementById('sf');var o=fr.querySelector('img.si');if(o)o.remove();
      document.getElementById('sol').style.display='none';
      var si=document.createElement('img');si.className='si';si.src=item.src;si.style.cssText='width:100%;height:100%;object-fit:cover;display:block;border-radius:12px;';fr.appendChild(si);
      document.getElementById('stm').textContent='Viewing: '+item.time;document.getElementById('bdl').style.display='inline-flex';
    };
    g.appendChild(d);
  });
}
function poll(){
  fetch(espBase+'/status',{signal:AbortSignal.timeout(3500)})
    .then(function(r){return r.json();})
    .then(function(d){
      if(!online){setConn(true);lg('Connected to ESP32 at '+espBase,'inf');toast('Connected!','ok');}
      if(pLk!==null&&d.locked!==pLk)lg('Door '+(d.locked?'LOCKED':'UNLOCKED'),d.locked?'lk':'ul');
      if(pPr!==null&&d.parcel!==pPr){
        if(d.parcel){lg('Parcel detected \u2014 auto-capturing...','pk');toast('Package arrived! Capturing...','ok');setTimeout(captureNow,700);}
        else{lg('Parcel removed','pk');toast('Parcel removed!','err');document.getElementById('stag').className='stag abs';document.getElementById('stagtx').textContent='Parcel removed';}
      }
      lockUI(d.locked);parcelUI(d.parcel);pLk=d.locked;pPr=d.parcel;
      document.getElementById('lu').textContent='Updated '+ts();
    })
    .catch(function(){if(online)toast('Connection lost','err');setConn(false);});
}
function cmd(c){
  fetch(espBase+'/'+c,{signal:AbortSignal.timeout(3500)})
    .then(function(r){return r.json();})
    .then(function(d){lg('Manual: '+c.toUpperCase(),c==='lock'?'lk':'ul');toast(c==='lock'?'Locked':'Unlocked','ok');lockUI(d.locked);pLk=d.locked;})
    .catch(function(){toast('Command failed','err');});
}
(function(){
  document.getElementById('ipDisp').textContent=window.location.hostname;
  if(Notification&&Notification.permission==='default')Notification.requestPermission();
  try{var h=localStorage.getItem('sH');if(h){hist=JSON.parse(h);renderHist();if(hist.length>0){var last=hist[0];curSnap=last.src;var fr=document.getElementById('sf');document.getElementById('sol').style.display='none';var si=document.createElement('img');si.className='si';si.src=last.src;si.style.cssText='width:100%;height:100%;object-fit:cover;display:block;border-radius:12px;';fr.appendChild(si);document.getElementById('stm').textContent='Last capture: '+last.time;document.getElementById('bdl').style.display='inline-flex';}}}catch(e){}
  startCam();
  if('serviceWorker' in navigator)navigator.serviceWorker.register('/sw.js').catch(function(){});
  poll();setInterval(poll,2000);
})();
</script>
</body>
</html>
)js";

// (notifications removed)

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// HTTP HANDLERS
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void handleRoot() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent_P(HTML_A);
  // Inject camera URL as JS variable at runtime
  String camLine = "var camURL='";
  camLine += CAMERA_IP;
  camLine += "';\n";
  server.sendContent(camLine);
  server.sendContent_P(HTML_C);
}

bool parcelDetected() {
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, MAX_ECHO_TIME);
  if (duration == 0) return false;

  float distanceCm = duration / 58.0;
  return distanceCm > 0 && distanceCm < DISTANCE_THRESHOLD_CM;
}

void handleLock() {
  digitalWrite(RELAY_PIN, RELAY_LOCK);
  isLocked = true;
  bool parcel = parcelDetected();
  String j = "{\"locked\":true,\"parcel\":";
  j += parcel ? "true}" : "false}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", j);
  Serial.println("[LOCK] Closed");
}

void handleUnlock() {
  digitalWrite(RELAY_PIN, RELAY_UNLOCK);
  isLocked = false;
  bool parcel = parcelDetected();
  String j = "{\"locked\":false,\"parcel\":";
  j += parcel ? "true}" : "false}";
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", j);
  Serial.println("[UNLOCK] Open");
}

void handleStatus() {
  bool parcel = parcelDetected();
  String j = "{\"locked\":";
  j += isLocked ? "true" : "false";
  j += ",\"parcel\":";
  j += parcel ? "true}" : "false}";
  server.send(200, "application/json", j);
  // status endpoint returns JSON; keep Serial diagnostics in loop
}

void handleManifest() { server.send_P(200, "application/json", MANIFEST); }
void handleSW()        { server.send_P(200, "application/javascript", SW_JS); }
void handleIcon()      { server.send_P(200, "image/svg+xml", ICON_SVG); }
void handleNotFound()  { server.send(404, "text/plain", "404"); }

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// SETUP
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

void setup() {
  Serial.begin(115200);
  delay(100);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  digitalWrite(RELAY_PIN, RELAY_LOCK);   // Start locked (safe)

  Serial.println("\n╔══════════════════════════════════════════════╗");
  Serial.println("║   Smart Parcel Locker  v5.0                  ║");
  Serial.println("╚══════════════════════════════════════════════╝");
  Serial.print  ("  Camera IP    : "); Serial.println(CAMERA_IP);
  Serial.print  ("  WiFi         : "); Serial.println(WIFI_SSID);
  Serial.print  ("  WhatsApp Bot : "); Serial.println("DISABLED");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500); Serial.print("."); tries++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi failed — restarting");
    delay(2000); ESP.restart();
  }

  Serial.println("\n══════════════════════════════════════════════");
  Serial.print  ("  >>> Open browser: http://");
  Serial.println(WiFi.localIP());
  Serial.println("  (Click the IP in Serial Monitor to open)");
  Serial.println("══════════════════════════════════════════════\n");

  server.on("/",              handleRoot);
  server.on("/lock",          handleLock);
  server.on("/unlock",        handleUnlock);
  server.on("/status",        handleStatus);
  server.on("/manifest.json", handleManifest);
  server.on("/sw.js",         handleSW);
  server.on("/icon.svg",      handleIcon);
  server.onNotFound(handleNotFound);
  server.begin();
}

// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
// LOOP
// ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

unsigned long lastDiag = 0;

void loop() {
  server.handleClient();

  if (millis() - lastDiag >= 15000) {
    lastDiag = millis();
    bool parcel = parcelDetected();
    Serial.print("[DIAG] Lock:");   Serial.print(isLocked ? "CLOSED" : "OPEN");
    Serial.print(" | Parcel:");     Serial.print(parcel ? "YES" : "NO");
    Serial.print(" | IP:");         Serial.println(WiFi.localIP());
  }
}