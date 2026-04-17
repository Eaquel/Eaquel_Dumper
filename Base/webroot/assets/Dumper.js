/* ═══════════════════════════════════════════════════════════════
   Eaquel Dumper — Dumper.js
   Author: Kitsune
   ═══════════════════════════════════════════════════════════════ */

"use strict";

/* ── BRIDGE ───────────────────────────────────────────────────── */
const Bridge = (() => {
  let _type = null;

  function detect() {
    if (_type) return _type;
    if (window.kitsune  && typeof window.kitsune.exec      === "function") { _type = "kitsune";   return _type; }
    if (window.ksu      && typeof window.ksu.exec          === "function") { _type = "ksu";       return _type; }
    if (window.kernelsu && typeof window.kernelsu.exec     === "function") { _type = "kernelsu";  return _type; }
    if (window.mmrl     && typeof window.mmrl.exec         === "function") { _type = "mmrl";      return _type; }
    if (window.magisk   && typeof window.magisk.exec       === "function") { _type = "magisk";    return _type; }
    if (window.magisk   && typeof window.magisk.runCommand === "function") { _type = "magisk_rc"; return _type; }
    if (window.apatch   && typeof window.apatch.exec       === "function") { _type = "apatch";    return _type; }
    if (typeof window.exec === "function")                                  { _type = "global";    return _type; }
    return null;
  }

  function friendlyName() {
    const t = detect();
    const map = {
      kitsune: "Kitsune Magisk", ksu: "KernelSU", kernelsu: "KernelSU",
      mmrl: "MMRL", magisk: "Magisk", magisk_rc: "Magisk",
      apatch: "APatch", global: "Su Bridge",
    };
    return t ? (map[t] || t) : null;
  }

  async function godRun(cmd) {
    const t = detect();
    if (!t) return "";
    try {
      if (t === "kitsune"  ) return (await window.kitsune.exec(cmd)).stdout   || "";
      if (t === "ksu"      ) return (await window.ksu.exec(cmd)).stdout       || "";
      if (t === "kernelsu" ) return (await window.kernelsu.exec(cmd)).stdout  || "";
      if (t === "mmrl"     ) return await window.mmrl.exec(cmd)               || "";
      if (t === "magisk"   ) return (await window.magisk.exec(cmd)).stdout    || "";
      if (t === "magisk_rc") return await window.magisk.runCommand(cmd)       || "";
      if (t === "apatch"   ) return (await window.apatch.exec(cmd)).stdout    || "";
      if (t === "global"   ) return await window.exec(cmd)                    || "";
    } catch (e) {
      return `[err] ${e.message || e}`;
    }
    return "";
  }

  async function diagnose() {
    const lines = [];
    const checks = [
      ["window.kitsune",  !!window.kitsune],
      ["window.ksu",      !!window.ksu],
      ["window.kernelsu", !!window.kernelsu],
      ["window.mmrl",     !!window.mmrl],
      ["window.magisk",   !!window.magisk],
      ["window.apatch",   !!window.apatch],
      ["window.exec",     typeof window.exec === "function"],
    ];
    checks.forEach(([k, v]) => lines.push(`${k}: ${v ? "✓" : "✗"}`));
    return lines.join(" | ");
  }

  return { detect, friendlyName, godRun, diagnose, get type() { return _type; } };
})();

/* ── TOAST ────────────────────────────────────────────────────── */
const Toast = (() => {
  function show(msg, type = "", dur = 2200) {
    const ct = document.getElementById("toast-container");
    if (!ct) return;
    const t = document.createElement("div");
    t.className = `toast ${type}`;
    t.textContent = msg;
    ct.appendChild(t);
    setTimeout(() => {
      t.classList.add("out");
      setTimeout(() => t.remove(), 260);
    }, dur);
  }
  return { show };
})();

/* ── TERMINAL ─────────────────────────────────────────────────── */
const Terminal = (() => {
  let _el    = null;
  let _lines = 0;
  const MAX  = 500;

  function init() {
    _el    = document.getElementById("terminal-output");
    _lines = 0;
    if (_el) _el.innerHTML = "";
  }

  function log(text, type = "data") {
    if (!_el) _el = document.getElementById("terminal-output");
    if (!_el) return;

    if (_lines >= MAX) {
      // remove oldest 50
      const children = _el.children;
      for (let i = 0; i < 50 && children.length > 0; i++) children[0].remove();
      _lines -= 50;
    }

    const now  = new Date();
    const time = `${String(now.getHours()).padStart(2,"0")}:${String(now.getMinutes()).padStart(2,"0")}:${String(now.getSeconds()).padStart(2,"0")}`;
    const row  = document.createElement("div");
    row.className = "term-row";

    const cls = { sys:"t-sys", ok:"t-ok", err:"t-err", zg:"t-zg", data:"t-data", cfg:"t-cfg" }[type] || "t-data";

    row.innerHTML = `<span class="t-time">${time}</span><span class="${cls}">${_escape(text)}</span>`;
    _el.appendChild(row);
    _el.scrollTop = _el.scrollHeight;
    _lines++;
  }

  function clear() {
    if (!_el) return;
    _el.innerHTML = "";
    _lines = 0;
    log(Language.get("term_clear"), "sys");
  }

  function _escape(s) {
    return String(s)
      .replace(/&/g,"&amp;")
      .replace(/</g,"&lt;")
      .replace(/>/g,"&gt;");
  }

  return { init, log, clear };
})();

/* ── CONFIG ───────────────────────────────────────────────────── */
const Config = (() => {
  const FILE    = "/data/adb/modules/eaqueldumper/config.json";
  const DEFCONF = {
    outputDir: "/sdcard/EaquelDumps",
    filename:  "dump.cs",
    genCpp:    true,
    genFrida:  true,
    il2cppScan: true,
    autoKill:  false,
    darkCRT:   true,
  };

  let _cfg = { ...DEFCONF };

  async function load() {
    const raw = await Bridge.godRun(`cat "${FILE}" 2>/dev/null`);
    if (raw && raw.startsWith("{")) {
      try { _cfg = { ...DEFCONF, ...JSON.parse(raw) }; } catch (e) { /* ignore */ }
    }
    _syncUI();
    Terminal.log("Config loaded.", "cfg");
    return _cfg;
  }

  async function save() {
    _readUI();
    const json    = JSON.stringify(_cfg, null, 2);
    const escaped = json.replace(/'/g, "'\\''");
    await Bridge.godRun(`echo '${escaped}' > "${FILE}"`);
    Terminal.log("Config saved → " + FILE, "cfg");
    Toast.show("✓ " + Language.get("btn_save_config"), "green");
  }

  function _syncUI() {
    _setVal("cfg-outdir",   _cfg.outputDir);
    _setVal("cfg-filename", _cfg.filename);
    _setChk("cfg-cpp",      _cfg.genCpp);
    _setChk("cfg-frida",    _cfg.genFrida);
    _setChk("cfg-il2cpp",   _cfg.il2cppScan);
    _setChk("cfg-autokill", _cfg.autoKill);
  }

  function _readUI() {
    _cfg.outputDir  = _getVal("cfg-outdir")   || DEFCONF.outputDir;
    _cfg.filename   = _getVal("cfg-filename") || DEFCONF.filename;
    _cfg.genCpp     = _getChk("cfg-cpp");
    _cfg.genFrida   = _getChk("cfg-frida");
    _cfg.il2cppScan = _getChk("cfg-il2cpp");
    _cfg.autoKill   = _getChk("cfg-autokill");
  }

  function _getVal(id)       { const el = document.getElementById(id); return el ? el.value.trim() : ""; }
  function _setVal(id, val)  { const el = document.getElementById(id); if (el) el.value = val ?? ""; }
  function _getChk(id)       { const el = document.getElementById(id); return el ? el.checked : false; }
  function _setChk(id, val)  { const el = document.getElementById(id); if (el) el.checked = !!val; }

  return { load, save, get: key => _cfg[key], all: () => ({ ..._cfg }) };
})();

/* ── APP STATE ────────────────────────────────────────────────── */
const AppState = {
  root:   false,
  apps:   [],       // { pkg, name, icon, isGame, isUnity, isIL2CPP, baseAddr }
  target: null,
  filter: "all",
  query:  "",
  tab:    "target",
};

/* ── SPLASH ───────────────────────────────────────────────────── */
const Splash = (() => {
  function setProgress(pct) {
    const bar = document.getElementById("splash-progress-bar");
    if (bar) bar.style.width = Math.min(100, pct) + "%";
  }
  function setStatus(txt) {
    const el = document.getElementById("splash-status");
    if (el) el.textContent = txt;
  }
  function setAttempt(txt) {
    const el = document.getElementById("splash-attempt");
    if (el) el.textContent = txt;
  }
  function _stepEl(id) { return document.getElementById("step-" + id); }
  function stepActive(id) {
    const el = _stepEl(id);
    if (!el) return;
    el.classList.remove("done","fail"); el.classList.add("active");
    el.querySelector(".step-icon").innerHTML = '<i class="fa-solid fa-circle-notch fa-spin"></i>';
  }
  function stepDone(id, val) {
    const el = _stepEl(id);
    if (!el) return;
    el.classList.remove("active","fail"); el.classList.add("done");
    el.querySelector(".step-icon").innerHTML = '<i class="fa-solid fa-check"></i>';
    if (val) el.querySelector(".step-status").textContent = val;
  }
  function stepFail(id, val) {
    const el = _stepEl(id);
    if (!el) return;
    el.classList.remove("active","done"); el.classList.add("fail");
    el.querySelector(".step-icon").innerHTML = '<i class="fa-solid fa-xmark"></i>';
    if (val) el.querySelector(".step-status").textContent = val;
  }
  function showError() {
    const btn  = document.getElementById("splash-retry-btn");
    const det  = document.getElementById("splash-err-detail");
    const stat = document.getElementById("splash-status");
    if (btn)  btn.style.display  = "inline-flex";
    if (det)  det.style.display  = "block";
    if (stat) stat.style.color   = "#ef4444";
    setStatus("ROOT BRIDGE NOT FOUND");
  }
  function dismiss() {
    const el = document.getElementById("splash");
    if (!el) return;
    el.classList.add("fade-out");
    setTimeout(() => { el.style.display = "none"; }, 520);
  }
  return { setProgress, setStatus, setAttempt, stepActive, stepDone, stepFail, showError, dismiss };
})();

/* ── CORE ─────────────────────────────────────────────────────── */
const Core = (() => {

  /* ── Tab switch ─────────────────────────────────────────────── */
  function switchTab(id) {
    AppState.tab = id;
    document.querySelectorAll(".tab-pane").forEach(el => el.classList.remove("active"));
    document.querySelectorAll(".nav-btn").forEach(el => el.classList.remove("active"));
    const pane = document.getElementById("tab-" + id);
    const btn  = document.getElementById("nav-" + id);
    if (pane) pane.classList.add("active");
    if (btn)  btn.classList.add("active");
  }

  /* ── Scan apps ──────────────────────────────────────────────── */
  async function scanApps() {
    if (!AppState.root) { Toast.show("No root access", "red"); return; }
    switchTab("target");
    Terminal.log(Language.get("scan_start"), "sys");

    const btn  = document.getElementById("scan-btn");
    const icon = document.getElementById("scan-icon");
    if (btn)  btn.disabled = true;
    if (icon) { icon.className = "fa-solid fa-circle-notch fa-spin"; }

    try {
      const cfg     = Config.all();
      let   pkgList = [];

      // detect il2cpp games via pm list packages
      const pmOut = await Bridge.godRun("pm list packages -f 2>/dev/null | head -n 300");
      const lines = pmOut.split("\n").filter(l => l.includes("package:"));

      Terminal.log(`Found ${lines.length} packages, checking IL2CPP...`, "sys");

      const results = [];
      // batch check — look for libil2cpp.so
      const allPkgs = lines.map(l => {
        const m = l.match(/package:(.+)=(.+)/);
        return m ? { apkPath: m[1], pkg: m[2] } : null;
      }).filter(Boolean);

      // check in parallel-ish groups of 20
      const BATCH = 20;
      for (let i = 0; i < allPkgs.length; i += BATCH) {
        const group = allPkgs.slice(i, i + BATCH);
        const cmds  = group.map(({ apkPath, pkg }) =>
          `unzip -l "${apkPath}" 2>/dev/null | grep -q libil2cpp && echo "IL2:${pkg}"`
        ).join("; ");
        const out = await Bridge.godRun(cmds + " 2>/dev/null");
        const hits = out.split("\n").filter(l => l.startsWith("IL2:")).map(l => l.slice(4).trim());
        results.push(...hits);

        const pct = Math.round((i / allPkgs.length) * 100);
        Splash.setProgress?.(pct);
        document.getElementById("stat-total").textContent = results.length;
      }

      AppState.apps = [];
      for (const pkg of results) {
        const isGame  = pkg.includes("game") || pkg.includes("play");
        const isUnity = true; // all il2cpp games are unity-based
        AppState.apps.push({ pkg, name: pkg.split(".").pop(), icon: null, isGame, isUnity, isIL2CPP: true, baseAddr: null });
      }

      // update stats
      document.getElementById("stat-total").textContent = AppState.apps.length;
      document.getElementById("stat-unity").textContent = AppState.apps.filter(a => a.isUnity).length;
      document.getElementById("stat-game").textContent  = AppState.apps.filter(a => a.isGame).length;

      Terminal.log(Language.get("scan_found", AppState.apps.length), AppState.apps.length > 0 ? "ok" : "data");
      renderApps();

    } catch (e) {
      Terminal.log("[ERR] scan: " + e.message, "err");
    } finally {
      if (btn)  btn.disabled = false;
      if (icon) icon.className = "fa-solid fa-radar";
    }
  }

  /* ── Render app list ────────────────────────────────────────── */
  function renderApps() {
    const grid = document.getElementById("app-grid");
    if (!grid) return;
    grid.innerHTML = "";

    const q    = AppState.query.toLowerCase();
    const filt = AppState.filter;

    let apps = AppState.apps.filter(a => {
      if (q && !a.pkg.toLowerCase().includes(q) && !a.name.toLowerCase().includes(q)) return false;
      if (filt === "game"  && !a.isGame)   return false;
      if (filt === "unity" && !a.isUnity)  return false;
      return true;
    });

    if (apps.length === 0) {
      grid.innerHTML = `
        <div class="empty-state">
          <i class="fa-solid fa-radar"></i>
          <span>${AppState.apps.length === 0
            ? `Tap <strong>SCAN</strong> to detect IL2CPP games`
            : Language.get("no_match")}</span>
        </div>`;
      return;
    }

    apps.forEach((app, idx) => {
      const card = document.createElement("div");
      card.className = "app-card" + (AppState.target === app.pkg ? " selected" : "");
      card.style.animationDelay = (idx * 18) + "ms";

      const tags = [];
      if (app.isGame)  tags.push(`<span class="tag tag-game">GAME</span>`);
      if (app.isUnity) tags.push(`<span class="tag tag-unity">UNITY</span>`);
      tags.push(`<span class="tag tag-il2cpp">IL2CPP</span>`);
      if (app.baseAddr) tags.push(`<span class="tag tag-on">LIVE</span>`);

      card.innerHTML = `
        <div class="app-icon-wrap">
          ${app.icon
            ? `<img src="${app.icon}" onerror="this.parentNode.innerHTML='<i class=\\'fa-solid fa-ghost\\'></i>'">`
            : `<i class="fa-solid fa-ghost" style="color:#ff4444;font-size:0.9rem;opacity:0.6;"></i>`}
        </div>
        <div class="app-info min-w-0">
          <div class="app-name">${_esc(app.name)}</div>
          <div class="app-pkg">${_esc(app.pkg)}</div>
          <div class="flex gap-1" style="margin-top:5px;flex-wrap:wrap;">${tags.join("")}</div>
        </div>
        <div class="app-right">
          <div class="app-sel-dot"></div>
        </div>`;

      card.onclick = () => selectTarget(app.pkg, app.name, app.icon);
      grid.appendChild(card);
    });
  }

  function _esc(s) {
    return String(s).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");
  }

  /* ── Select target ──────────────────────────────────────────── */
  function selectTarget(pkg, name, icon) {
    AppState.target = pkg;
    renderApps();

    const bar  = document.getElementById("locked-bar");
    const lname = document.getElementById("locked-name");
    const lpkg  = document.getElementById("locked-pkg");
    const licon = document.getElementById("locked-icon");
    if (bar)  bar.classList.remove("hide");
    if (lname) lname.textContent = name || pkg.split(".").pop();
    if (lpkg)  lpkg.textContent  = pkg;
    if (licon) licon.innerHTML   = icon
      ? `<img src="${icon}" onerror="this.parentNode.innerHTML='<i class=\\'fa-solid fa-crosshairs text-red-500\\'></i>'">`
      : `<i class="fa-solid fa-crosshairs" style="color:#ff4444;"></i>`;

    Terminal.log(`Target locked: ${pkg}`, "cfg");
  }

  function clearTarget() {
    AppState.target = null;
    renderApps();
    const bar = document.getElementById("locked-bar");
    if (bar) bar.classList.add("hide");
    Terminal.log("Target cleared.", "sys");
  }

  /* ── Filter / search ────────────────────────────────────────── */
  function setFilter(f) {
    AppState.filter = f;
    ["all","game","unity"].forEach(id => {
      const btn = document.getElementById("f-" + id);
      if (!btn) return;
      btn.className = id === f ? "btn-primary px-3 py-1 text-xs font-bold flex-shrink-0" : "btn-ghost px-3 py-1 text-xs font-bold flex-shrink-0";
    });
    renderApps();
  }

  function filterApps(q) {
    AppState.query = q;
    renderApps();
  }

  /* ── Inject ─────────────────────────────────────────────────── */
  async function inject() {
    if (!AppState.target) { switchTab("target"); Toast.show("Select a target first", "red"); return; }
    switchTab("term");
    const pkg     = AppState.target;
    const outDir  = Config.get("outputDir") || "/sdcard/EaquelDumps";
    const genCpp  = Config.get("genCpp");
    const genFrida= Config.get("genFrida");

    Terminal.log(Language.get("inject_preparing"), "zg");
    Terminal.log(`Target: ${pkg}`, "cfg");
    Terminal.log(`Output: ${outDir}`, "cfg");

    // Write config for Zygisk
    const cfgPath = `/data/adb/modules/eaqueldumper/target.cfg`;
    const cfgData = `pkg=${pkg}\noutdir=${outDir}\ngen_cpp=${genCpp ? 1 : 0}\ngen_frida=${genFrida ? 1 : 0}`;
    await Bridge.godRun(`mkdir -p /data/adb/modules/eaqueldumper && printf '${cfgData}' > "${cfgPath}"`);
    Terminal.log(Language.get("inject_config_written") + ` → ${cfgPath}`, "ok");

    // Force-stop + launch
    await Bridge.godRun(`am force-stop ${pkg}`);
    await new Promise(r => setTimeout(r, 600));
    const launch = await Bridge.godRun(`monkey -p ${pkg} -c android.intent.category.LAUNCHER 1 2>&1`);
    Terminal.log(Language.get("inject_launched"), "ok");

    // Poll for dump.cs
    Terminal.log(Language.get("inject_waiting"), "sys");
    let found = false;
    for (let i = 0; i < 60; i++) {
      await new Promise(r => setTimeout(r, 1000));
      const chk = await Bridge.godRun(`ls "${outDir}/dump.cs" 2>/dev/null`);
      if (chk.includes("dump.cs")) {
        Terminal.log(Language.get("inject_found"), "ok");
        Toast.show("✓ dump.cs ready!", "green");
        found = true;
        // notify extract tab badge
        const badge = document.getElementById("nav-badge-extract");
        if (badge) badge.classList.add("show");
        break;
      }
    }
    if (!found) {
      Terminal.log(Language.get("inject_wait"), "err");
    }
  }

  /* ── Kill target ────────────────────────────────────────────── */
  async function killTarget() {
    if (!AppState.target) return;
    await Bridge.godRun(`am force-stop ${AppState.target}`);
    Terminal.log(`[KILL] ${AppState.target}`, "err");
    Terminal.log("Process terminated.", "sys");
    Toast.show("Killed: " + AppState.target, "red");
  }

  /* ── Pull file ──────────────────────────────────────────────── */
  async function pull(fileName) {
    if (!AppState.target) { switchTab("target"); Toast.show("Select a target first", "red"); return; }
    switchTab("term");
    const pkg  = AppState.target;
    const dest = "/sdcard/EaquelDumps";
    const dirs = [
      dest,
      `/sdcard/Android/data/${pkg}`,
      `/data/data/${pkg}`,
      `/data/local/tmp`,
    ];

    Terminal.log(Language.get("pull_looking") + `: ${fileName}`, "sys");
    let found = false;
    for (const dir of dirs) {
      const chk = await Bridge.godRun(`ls "${dir}/${fileName}" 2>/dev/null`);
      if (chk.includes(fileName)) {
        const dst = `/sdcard/EaquelDumps/${pkg}_${fileName}`;
        await Bridge.godRun(`mkdir -p /sdcard/EaquelDumps && cp "${dir}/${fileName}" "${dst}" && chmod 644 "${dst}"`);
        Terminal.log(`[OK] → ${dst}`, "ok");
        Toast.show("✓ Pulled: " + fileName, "green");
        found = true;
        break;
      }
    }
    if (!found) {
      Terminal.log(Language.get("pull_not_found"), "err");
      Terminal.log(Language.get("pull_checked") + ": " + dirs.join(", "), "data");
      Terminal.log(Language.get("pull_inject_first"), "sys");
    }
  }

  async function pullAll() {
    for (const f of ["dump.cs","dump.h","dump.js"]) await pull(f);
  }

  async function pullApk() {
    if (!AppState.target) { switchTab("target"); Toast.show("Select a target first", "red"); return; }
    switchTab("term");
    const pkg     = AppState.target;
    const pathOut = await Bridge.godRun(`pm path ${pkg}`);
    const line    = pathOut.split("\n").find(l => l.includes("base.apk"));
    if (line) {
      const apkPath = line.replace(/package:/g, "").trim();
      const dst     = `/sdcard/EaquelDumps/${pkg}_base.apk`;
      await Bridge.godRun(`mkdir -p /sdcard/EaquelDumps && cp "${apkPath}" "${dst}" && chmod 644 "${dst}"`);
      Terminal.log(`[OK] → ${dst}`, "ok");
      Toast.show("✓ APK pulled!", "green");
    } else {
      Terminal.log(`[ERR] base.apk not found for ${pkg}`, "err");
    }
  }

  /* ── Terminal run command ───────────────────────────────────── */
  async function runCmd() {
    const inp = document.getElementById("term-cmd");
    const cmd = inp ? inp.value.trim() : "";
    if (!cmd) return;
    if (inp) inp.value = "";
    Terminal.log(`$ ${cmd}`, "sys");
    const out = await Bridge.godRun(cmd);
    if (out) out.trim().split("\n").forEach(l => Terminal.log(l, "data"));
  }

  /* ── Update base addresses ──────────────────────────────────── */
  async function refreshBaseAddresses() {
    if (!AppState.root || AppState.apps.length === 0) return;
    for (const app of AppState.apps) {
      const maps = await Bridge.godRun(`cat /proc/$(pidof ${app.pkg} 2>/dev/null)/maps 2>/dev/null | grep libil2cpp | head -1`);
      app.baseAddr = maps ? maps.split("-")[0] : null;
    }
    const running = AppState.apps.filter(a => a.baseAddr).length;
    const el = document.getElementById("stat-il2cpp");
    if (el) el.textContent = running;
    renderApps();
  }

  /* ── Boot sequence ──────────────────────────────────────────── */
  async function doBoot() {
    Terminal.init();

    Splash.stepActive("bridge");
    Splash.setStatus("WAITING FOR ROOT BRIDGE...");
    Splash.setProgress(5);

    const MAX_ATTEMPTS = 150;
    const INTERVAL_MS  = 200;
    let   attempts     = 0;
    let   bridgeFound  = false;

    const hasBridge = () => Bridge.detect() !== null;

    while (attempts < MAX_ATTEMPTS) {
      if (hasBridge()) { bridgeFound = true; break; }
      attempts++;
      const elapsed = ((attempts * INTERVAL_MS) / 1000).toFixed(1);
      const pct     = Math.min(5 + (attempts / MAX_ATTEMPTS) * 30, 35);
      Splash.setProgress(pct);
      Splash.setAttempt(`Attempt ${attempts}/${MAX_ATTEMPTS} · ${elapsed}s`);
      if (attempts === 25) Splash.setStatus("STILL WAITING — Initializing...");
      if (attempts === 75) Splash.setStatus("TAKING LONGER THAN USUAL...");
      await new Promise(r => setTimeout(r, INTERVAL_MS));
    }

    Bridge.detect(); // re-detect to set _type
    const bridgeName = Bridge.friendlyName();
    Splash.setAttempt("");

    if (!bridgeFound) {
      Splash.stepFail("bridge", "N/A");
      Splash.stepFail("root",   "NO BRIDGE");
      Splash.stepFail("cfg",    "SKIP");
      Splash.stepFail("ready",  "SKIP");
      Splash.setProgress(100);
      Splash.showError();
      return;
    }

    Splash.stepDone("bridge", bridgeName || "SU");
    Splash.setProgress(40);

    Splash.stepActive("root");
    Splash.setStatus("VERIFYING ROOT ACCESS...");
    const idCheck = await Bridge.godRun("id");
    const hasRoot = idCheck.includes("uid=0") || idCheck.includes("root");

    if (hasRoot) {
      Splash.stepDone("root", "uid=0");
      Splash.setProgress(65);

      Splash.stepActive("cfg");
      Splash.setStatus("LOADING CONFIGURATION...");
      await Config.load();
      Splash.stepDone("cfg", "OK");
      Splash.setProgress(85);

      Splash.stepActive("ready");
      Splash.setStatus("SYSTEM READY");
      await new Promise(r => setTimeout(r, 300));
      Splash.stepDone("ready", "✓");
      Splash.setProgress(100);

      AppState.root = true;
      const badge = document.getElementById("root-badge");
      if (badge) {
        badge.className = "badge-root-on";
        badge.innerHTML = `<i class="fa-solid fa-shield-check"></i> ROOT · ${bridgeName || "SU"}`;
      }

      Terminal.log(Language.get("term_ready"), "ok");
      Terminal.log(`Bridge: ${bridgeName || "unknown"} — uid=0 confirmed.`, "ok");

      // device info
      const model  = await Bridge.godRun("getprop ro.product.model");
      const sdk    = await Bridge.godRun("getprop ro.build.version.sdk");
      const kernel = await Bridge.godRun("uname -r");
      if (model)  Terminal.log(`Device: ${model.trim()}`, "cfg");
      if (sdk)    Terminal.log(`SDK: ${sdk.trim()}`, "cfg");
      if (kernel) Terminal.log(`Kernel: ${kernel.trim()}`, "cfg");

      _updateAbout(model, sdk, kernel, bridgeName);

      await new Promise(r => setTimeout(r, 400));
      Splash.dismiss();

    } else {
      Splash.stepFail("root", "DENIED");
      Splash.setProgress(100);
      Terminal.log("Root check failed — id=" + (idCheck || "empty"), "err");
      Splash.showError();
    }
  }

  function _updateAbout(model, sdk, kernel, bridge) {
    _setAbout("about-device",  model  ? model.trim()  : "Unknown");
    _setAbout("about-sdk",     sdk    ? "API " + sdk.trim() : "Unknown");
    _setAbout("about-kernel",  kernel ? kernel.trim() : "Unknown");
    _setAbout("about-bridge",  bridge || "Unknown");
  }
  function _setAbout(id, val) {
    const el = document.getElementById(id);
    if (el) el.textContent = val;
  }

  async function retryBoot() {
    const btn  = document.getElementById("splash-retry-btn");
    const det  = document.getElementById("splash-err-detail");
    const stat = document.getElementById("splash-status");
    if (btn)  btn.style.display = "none";
    if (det)  det.style.display = "none";
    if (stat) { stat.style.color = ""; stat.classList.add("pulse-red"); }

    ["bridge","root","cfg","ready"].forEach(id => {
      const el = document.getElementById("step-" + id);
      if (!el) return;
      el.classList.remove("done","active","fail");
      el.querySelector(".step-icon").innerHTML = '<i class="fa-solid fa-minus"></i>';
      el.querySelector(".step-status").textContent = "—";
    });
    Splash.setProgress(0);
    Splash.setStatus("RETRYING ROOT BRIDGE...");
    await doBoot();
  }

  async function boot() {
    await doBoot();
  }

  return {
    boot, retryBoot,
    switchTab, scanApps, renderApps,
    selectTarget, clearTarget,
    setFilter, filterApps,
    inject, killTarget,
    pull, pullAll, pullApk,
    runCmd, refreshBaseAddresses,
  };
})();

/* ── LOGCAT POLLER ────────────────────────────────────────────── */
let _lastLogLines = new Set();
setInterval(async () => {
  if (!AppState.root) return;
  const out = await Bridge.godRun("logcat -d -s Eaquel:* 2>/dev/null | tail -n 20");
  if (!out || out.length < 5 || out.includes("not found")) return;
  const lines = out.trim().split("\n").filter(l => l.includes("Eaquel"));
  lines.forEach(line => {
    const trimmed = line.trim();
    if (!trimmed || _lastLogLines.has(trimmed)) return;
    _lastLogLines.add(trimmed);
    if (_lastLogLines.size > 200) _lastLogLines = new Set([..._lastLogLines].slice(-100));
    const level = trimmed.includes("E Eaquel") ? "err" : trimmed.includes("W Eaquel") ? "sys" : "zg";
    Terminal.log(`[ZYGISK] ${trimmed}`, level);
    if (trimmed.includes("dump.cs written")) {
      Terminal.log(Language.get("dump_complete"), "ok");
      const badge = document.getElementById("nav-badge-extract");
      if (badge) badge.classList.add("show");
    }
  });
  const running = AppState.apps.filter(a => a.baseAddr).length;
  const el = document.getElementById("stat-il2cpp");
  if (el) el.textContent = running;
}, 2000);

setInterval(async () => {
  if (!AppState.root || AppState.apps.length === 0) return;
  await Core.refreshBaseAddresses();
}, 8000);

/* ── INIT ─────────────────────────────────────────────────────── */
document.addEventListener("DOMContentLoaded", async () => {
  await Language.init();
  setTimeout(() => Core.boot(), 120);
});

window.Core     = Core;
window.Bridge   = Bridge;
window.Config   = Config;
window.Terminal = Terminal;
window.Toast    = Toast;
window.Splash   = Splash;
window.AppState = AppState;
