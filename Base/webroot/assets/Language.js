/* ═══════════════════════════════════════════════════════════════
   Eaquel Dumper — Language.js
   Author: Kitsune
   ═══════════════════════════════════════════════════════════════ */

"use strict";

const Language = (() => {

  /* ── State ──────────────────────────────────────────────────── */
  const STORAGE_KEY = "eaquel_lang";
  const FALLBACK    = "en";

  const LANGS = [
    { code: "en", name: "English",    flag: "🇬🇧" },
    { code: "tr", name: "Türkçe",     flag: "🇹🇷" },
    { code: "de", name: "Deutsch",    flag: "🇩🇪" },
    { code: "fr", name: "Français",   flag: "🇫🇷" },
    { code: "es", name: "Español",    flag: "🇪🇸" },
    { code: "ar", name: "العربية",    flag: "🇸🇦" },
    { code: "zh", name: "中文",        flag: "🇨🇳" },
    { code: "ja", name: "日本語",      flag: "🇯🇵" },
    { code: "ru", name: "Русский",    flag: "🇷🇺" },
    { code: "pt", name: "Português",  flag: "🇧🇷" },
  ];

  let _strings  = {};     // loaded strings map
  let _current  = FALLBACK;
  let _loaded   = false;

  /* ── XML Parser ─────────────────────────────────────────────── */
  function _parseXML(xmlText) {
    const parser = new DOMParser();
    const doc    = parser.parseFromString(xmlText, "text/xml");
    const map    = {};
    doc.querySelectorAll("string").forEach(el => {
      const key = el.getAttribute("name");
      if (key) map[key] = el.textContent.trim();
    });
    return map;
  }

  /* ── Load a lang file ───────────────────────────────────────── */
  async function _loadFile(code) {
    try {
      const base = _getBase();
      const url  = `${base}langs/${code}.xml`;
      const resp = await fetch(url);
      if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
      const text = await resp.text();
      return _parseXML(text);
    } catch (e) {
      console.warn(`[Language] Failed to load ${code}:`, e);
      return null;
    }
  }

  /* Resolve base path relative to index.html */
  function _getBase() {
    const scripts = Array.from(document.querySelectorAll("script[src]"));
    const me      = scripts.find(s => s.src.includes("Language.js"));
    if (me) {
      return me.src.replace("assets/Language.js", "");
    }
    return "./";
  }

  /* ── Init ───────────────────────────────────────────────────── */
  async function init() {
    const saved  = localStorage.getItem(STORAGE_KEY) || FALLBACK;
    const exists = LANGS.find(l => l.code === saved);
    const code   = exists ? saved : FALLBACK;
    return await switchTo(code, false);
  }

  /* ── Switch language ────────────────────────────────────────── */
  async function switchTo(code, save = true) {
    const data = await _loadFile(code);
    if (!data) {
      // fallback to en
      if (code !== FALLBACK) return await switchTo(FALLBACK, false);
      console.error("[Language] Cannot load fallback en.xml");
      return false;
    }

    _strings  = data;
    _current  = code;
    _loaded   = true;
    if (save) localStorage.setItem(STORAGE_KEY, code);

    // RTL support
    document.documentElement.setAttribute("lang", code);
    document.documentElement.dir = code === "ar" ? "rtl" : "ltr";

    _applyAll();
    _updatePickerUI();
    return true;
  }

  /* ── Get a string ───────────────────────────────────────────── */
  function get(key, ...args) {
    let str = _strings[key] || key;
    // simple printf-style %d/%s substitution
    let i = 0;
    str = str.replace(/%[ds]/g, () => args[i++] ?? "");
    return str;
  }

  /* ── Apply strings to DOM ───────────────────────────────────── */
  function _applyAll() {
    // elements with data-i18n="key"
    document.querySelectorAll("[data-i18n]").forEach(el => {
      const key = el.getAttribute("data-i18n");
      el.textContent = get(key);
    });
    // placeholders with data-i18n-placeholder="key"
    document.querySelectorAll("[data-i18n-placeholder]").forEach(el => {
      const key = el.getAttribute("data-i18n-placeholder");
      el.placeholder = get(key);
    });
    // title attributes
    document.querySelectorAll("[data-i18n-title]").forEach(el => {
      const key = el.getAttribute("data-i18n-title");
      el.title = get(key);
    });
  }

  /* ── Update lang picker button ──────────────────────────────── */
  function _updatePickerUI() {
    const lang  = LANGS.find(l => l.code === _current);
    const btn   = document.getElementById("lang-btn");
    if (btn && lang) {
      btn.innerHTML = `${lang.flag} <span style="font-size:0.55rem;">${lang.code.toUpperCase()}</span>`;
    }
    // active class in grid
    document.querySelectorAll(".lang-item").forEach(el => {
      el.classList.toggle("active", el.dataset.code === _current);
    });
  }

  /* ── Build lang picker grid ─────────────────────────────────── */
  function buildPicker() {
    const grid = document.getElementById("lang-grid");
    if (!grid) return;
    grid.innerHTML = "";
    LANGS.forEach(lang => {
      const div = document.createElement("div");
      div.className = "lang-item" + (lang.code === _current ? " active" : "");
      div.dataset.code = lang.code;
      div.innerHTML = `<span class="flag">${lang.flag}</span>${lang.name}`;
      div.onclick = async () => {
        await switchTo(lang.code);
        closePicker();
      };
      grid.appendChild(div);
    });
  }

  /* ── Picker open/close ──────────────────────────────────────── */
  function openPicker() {
    buildPicker();
    const overlay = document.getElementById("lang-overlay");
    if (overlay) overlay.classList.add("open");
  }

  function closePicker() {
    const overlay = document.getElementById("lang-overlay");
    if (overlay) overlay.classList.remove("open");
  }

  /* ── Public API ─────────────────────────────────────────────── */
  return {
    init,
    switchTo,
    get,
    apply: _applyAll,
    openPicker,
    closePicker,
    get current() { return _current; },
    get loaded()  { return _loaded;  },
    get langs()   { return LANGS;    },
  };

})();

window.Language = Language;
