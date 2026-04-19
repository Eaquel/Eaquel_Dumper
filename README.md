<div align="center">

# Eaquel Dumper

**IL2CPP Runtime Symbol Resolver & Memory Forensics Module**

[![Build](https://github.com/yourusername/Eaquel_Dumper/actions/workflows/build.yml/badge.svg)](https://github.com/yourusername/Eaquel_Dumper/actions)
![Android](https://img.shields.io/badge/Android-11--16-brightgreen?logo=android)
![API](https://img.shields.io/badge/API-30--36-blue)
![NDK](https://img.shields.io/badge/NDK-r29-orange)
![C++](https://img.shields.io/badge/C%2B%2B-23-purple)
![ReZygisk](https://img.shields.io/badge/ReZygisk-API%20v4-red)
![License](https://img.shields.io/badge/License-Educational-lightgrey)

</div>

---

## Language / Dil

| # | Language | Section |
|---|----------|---------|
| 01 | 🇹🇷 Türkçe | [→ TR](#türkçe) |
| 02 | 🇬🇧 English | [→ EN](#english) |
| 03 | 🇷🇺 Русский | [→ RU](#русский) |
| 04 | 🇨🇳 中文 | [→ ZH](#中文) |
| 05 | 🇩🇪 Deutsch | [→ DE](#deutsch) |
| 06 | 🇫🇷 Français | [→ FR](#français) |
| 07 | 🇪🇸 Español | [→ ES](#español) |
| 08 | 🇯🇵 日本語 | [→ JA](#日本語) |
| 09 | 🇰🇷 한국어 | [→ KO](#한국어) |

---

## Türkçe

### Nedir?

**Eaquel Dumper**, Android 11–16 üzerinde çalışan Unity/IL2CPP tabanlı uygulamaların çalışma zamanı bellek yapısını analiz eden, sembol tablolarını çözen ve insan tarafından okunabilir çıktılar üreten gelişmiş bir **ReZygisk** modülüdür. Tüm işlemler kullanıcının kendi cihazında, root yetkisiyle gerçekleştirilir; hiçbir sunucu bağlantısı kurulmaz.

---

### Mimari

Proje iki header, bir kaynak dosyasından oluşur ve sıfır harici bağımlılık içerir:

```
Eaquel_Dumper/
├── .github/workflows/build.yml
├── Dumper/
│   ├── build.gradle.kts
│   └── Source/Main/Native/
│       ├── CMakeLists.txt
│       ├── ReZygisk.hpp          ← ReZygisk API v4 (Zygisk drop-in replacement)
│       ├── Core.hpp              ← IL2CPP tipleri + Adaptive Scanner + Hot-Reload Config
│       └── Dumper.cpp            ← Hook Engine + Dump Motoru + ReZygisk Entry
├── Base/module.prop
├── Eaquel_Config.json
└── README.md
```

#### `ReZygisk.hpp`

Zygisk API'sinin tamamen ReZygisk mimarisine dönüştürülmüş hali. `rezygisk::` birincil namespace, `zygisk::` geriye dönük uyumluluk için alias olarak korunur. Mevcut `REGISTER_ZYGISK_MODULE` makrosu otomatik yönlendirilir.

#### `Core.hpp`

Üç katmandan oluşur:

1. **IL2CPP Runtime Tanımları** — Unity'nin tüm iç tipler, attribute sabitleri ve `DO_API` makro tablosu. `il2cpp_init`'ten `il2cpp_method_get_param_name`'e kadar ~70 fonksiyon imzası.

2. **`scanner::` — Adaptive Pattern Scanner** — IL2CPP ikili dosyalarında sembolleri bulmak için üç katmanlı strateji zinciri:

   | Strateji | Açıklama | Ne Zaman Devreye Girer |
   |----------|----------|------------------------|
   | Strict Scan | Tam bayt eşleşmesi | Her zaman ilk denenir |
   | Fuzzy Scan | Sadece ilk 4 byte sabit, geri kalan wildcard | Obfuscator sonrası |
   | Hash-Lattice | 16 byte'lık pencerelerin FNV-1a hash fingerprint'i | Prologue tamamen değiştiğinde |

   Yeni hash parmak izleri `known_hashes::` namespace'ine kaynak değişikliği olmadan eklenebilir. ARM64 ve ARM32 için ayrı prologue tabloları mevcuttur.

3. **`config::` — Hot-Reload Config Sistemi** — `inotify` + `eventfd` tabanlı izleyici; `Eaquel_Config.json` dosyasındaki herhangi bir değer değiştiğinde modül yeniden başlatılmadan, canlı olarak güncellenir. Mimari (32/64-bit) derleme zamanında otomatik algılanır; JSON'da tanımlanması gerekmez.

#### `Dumper.cpp`

- **Hook Engine** — Inline hook ve GOT patch motoru. RWX sayfa oluşturmaz; hedef fonksiyon için yalnızca patch süresi kadar dar bir RW penceresi açılır, hemen ardından RX'e dönülür.
- **Metadata Hook** — `Protected_Breaker` aktifken şifrelenmiş `global-metadata.dat` dosyasını intercept eder; XOR anahtar keşfi uygular, plaintext olarak diske yazar.
- **Async Waiter** — `libil2cpp.so` yüklenene kadar `eventfd` + `poll()` döngüsüyle bekler. `sleep_for` çağrısı yoktur; thread davranışı sistem log'larında anomali olarak görünmez.
- **Dump Motoru** — Assembly → Image → Class → Method/Field/Property hiyerarşisini yürür; üç farklı formatta çıktı üretir.

---

### Güvenlik Mimarisi

| Tehdit | Eski Yaklaşım | Eaquel v2 Yaklaşımı |
|--------|---------------|---------------------|
| RWX sayfa tespiti | Sayfa sonsuza kadar RWX kalıyor | Dar RW penceresi → hemen RX; RWX hiç oluşmuyor |
| Pattern obfuscation | Statik bayt dizisi kırılıyor | Strict → Fuzzy → Hash-Lattice zinciri |
| Thread anomalisi | `sleep_for` ile bekleme | `eventfd` + `poll()` ile görünmez bekleme |
| Config değişikliği | Yeniden başlatma gerekiyor | `inotify` hot-reload; anında güncelleme |
| Bit genişliği | JSON'dan okunuyor | Derleme zamanında `sizeof(void*)` ile otomatik |

---

### Yapılandırma

`/sdcard/Eaquel_Config.json` dosyasını düzenleyin. Değişiklikler **anında** geçerli olur, modülü veya uygulamayı yeniden başlatmanız gerekmez.

```json
{
  "Target_Game":      "com.hedef.oyun",
  "Output":           "/sdcard/Eaquel_Dumps",
  "Cpp_Header":       true,
  "Frida_Script":     true,
  "include_generic":  true,
  "Protected_Breaker": true
}
```

| Alan | Tip | Açıklama |
|------|-----|----------|
| `Target_Game` | string | Hedef uygulamanın paket adı |
| `Output` | string | Dump dosyalarının yazılacağı dizin |
| `Cpp_Header` | bool | `dump.h` C++ struct header'ı üret |
| `Frida_Script` | bool | `dump.js` Frida stub'larını üret |
| `include_generic` | bool | `List<T>` gibi generic sınıfları dahil et |
| `Protected_Breaker` | bool | Şifrelenmiş metadata'yı intercept et ve çöz |

> `force_32bit_mode` ve `auto_detect_bit` alanları **kaldırıldı**. Bit genişliği artık derleme zamanında otomatik olarak belirlenir.

---

### Çıktılar

| Dosya | Format | İçerik |
|-------|--------|--------|
| `dump.cs` | C# | Tüm sınıflar, erişim belirleyiciler, RVA/VA offsetleri |
| `dump.h` | C++ | Struct tanımları, field offsetleri, method RVA yorumları |
| `dump.js` | JavaScript | Her metod için hazır Frida `Interceptor.attach` şablonları |
| `global-metadata.dat` | Binary | Şifrelenmiş metadata varsa çözülmüş plaintext hali |

---

### Kurulum

```bash
# 1. GitHub Actions'ta workflow_dispatch ile build'i tetikle
# 2. Artifacts'tan zip dosyasını indir
# 3. Magisk / KernelSU / APatch üzerinden yükle
# 4. Config dosyasını düzenle:
adb push Eaquel_Config.json /sdcard/Eaquel_Config.json
# 5. Hedef uygulamayı başlat — dump otomatik gerçekleşir
```

---

### Neden Eaquel?

Piyasada birçok IL2CPP dumper bulunmaktadır. Eaquel'i farklı kılan şeyleri teknik olarak karşılaştıralım:

| Özellik | Il2CppDumper | Il2cppdumper (Perfare) | Zygisk-Il2CppDumper | **Eaquel v2** |
|---------|:---:|:---:|:---:|:---:|
| Cihaz üzerinde çalışma | Hayır | Hayır | Evet | **Evet** |
| ReZygisk desteği | Hayır | Hayır | Kısmi | **Tam** |
| RWX sayfa oluşturmaz | — | — | Hayır | **Evet** |
| Adaptive pattern scanner | — | — | Hayır | **Evet (3 katman)** |
| `sleep()` kullanmaz | — | — | Hayır | **Evet** |
| Hot-reload config | Hayır | Hayır | Hayır | **Evet (inotify)** |
| Şifreli metadata desteği | Hayır | Kısmi | Kısmi | **Evet (XOR keşif)** |
| Frida script üretimi | Hayır | Hayır | Hayır | **Evet** |
| Harici bağımlılık | Gerekli | Gerekli | Kısmi | **Sıfır** |
| ARM32 + ARM64 | Her ikisi | Her ikisi | ARM64 | **Her ikisi** |

---

### Tech Stack

| Bileşen | Versiyon |
|---------|---------|
| C++ Standardı | C++23 |
| NDK | r29 · 29.0.14206865 |
| CMake | 4.3.1+ |
| AGP | 9.1.0+ |
| Java | 21 Temurin LTS |
| Min SDK | 30 (Android 11) |
| Compile SDK | 36 (Android 16) |
| Sayfa Boyutu | 16 KB uyumlu |
| Mimari | arm64-v8a, armeabi-v7a |
| ReZygisk API | v4 |

---

## English

### What Is It?

**Eaquel Dumper** is an advanced **ReZygisk** module that analyses the runtime memory layout of Unity/IL2CPP applications on Android 11–16, resolves symbol tables and produces human-readable output files. All operations are performed on-device under root authority. No server connection is made.

---

### Architecture

The project consists of two headers and one source file with zero external dependencies:

#### `ReZygisk.hpp`

A full conversion of the Zygisk API to the ReZygisk architecture. `rezygisk::` is the primary namespace; `zygisk::` is kept as a backward-compatibility alias. The existing `REGISTER_ZYGISK_MODULE` macro is automatically forwarded.

#### `Core.hpp`

Three layers:

1. **IL2CPP Runtime Definitions** — All Unity internal types, attribute constants and the `DO_API` macro table. ~70 function signatures from `il2cpp_init` to `il2cpp_method_get_param_name`.

2. **`scanner::` — Adaptive Pattern Scanner** — Three-strategy chain for finding symbols in IL2CPP binaries:

   | Strategy | Description | When It Activates |
   |----------|-------------|------------------|
   | Strict Scan | Full byte match | Always tried first |
   | Fuzzy Scan | Only first 4 bytes fixed, rest wildcard | Post-obfuscator |
   | Hash-Lattice | FNV-1a hash fingerprint of 16-byte windows | When prologue is fully changed |

3. **`config::` — Hot-Reload Config** — `inotify` + `eventfd` based watcher. Any value change in `Eaquel_Config.json` is applied live without restarting the module or app. Architecture (32/64-bit) is auto-detected at compile time; no JSON field needed.

#### `Dumper.cpp`

- **Hook Engine** — Inline hook and GOT patch engine. Never creates RWX pages; only a narrow RW window is opened for the duration of the patch, immediately restored to RX.
- **Metadata Hook** — When `Protected_Breaker` is active, intercepts encrypted `global-metadata.dat`, performs XOR key discovery, writes plaintext to disk.
- **Async Waiter** — Waits for `libil2cpp.so` using `eventfd` + `poll()`. No `sleep_for` calls; thread behaviour does not appear as anomalous in system logs.
- **Dump Engine** — Traverses Assembly → Image → Class → Method/Field/Property hierarchy; produces three output formats.

---

### Security Architecture

| Threat | Old Approach | Eaquel v2 Approach |
|--------|-------------|-------------------|
| RWX page detection | Page stays RWX indefinitely | Narrow RW window → immediately RX; RWX never exists |
| Pattern obfuscation | Static byte array breaks | Strict → Fuzzy → Hash-Lattice chain |
| Thread anomaly | `sleep_for` blocking | Invisible wait via `eventfd` + `poll()` |
| Config change | Requires restart | `inotify` hot-reload; instant update |
| Bit width | Read from JSON | Auto at compile time via `sizeof(void*)` |

---

### Configuration

Edit `/sdcard/Eaquel_Config.json`. Changes take effect **instantly** — no restart required.

```json
{
  "Target_Game":       "com.your.game",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

| Field | Type | Description |
|-------|------|-------------|
| `Target_Game` | string | Target application package name |
| `Output` | string | Directory where dump files are written |
| `Cpp_Header` | bool | Generate `dump.h` C++ struct header |
| `Frida_Script` | bool | Generate `dump.js` Frida stubs |
| `include_generic` | bool | Include generic classes like `List<T>` |
| `Protected_Breaker` | bool | Intercept and decrypt encrypted metadata |

> `force_32bit_mode` and `auto_detect_bit` fields have been **removed**. Bit width is now determined automatically at compile time.

---

### Outputs

| File | Format | Content |
|------|--------|---------|
| `dump.cs` | C# | All classes, access modifiers, RVA/VA offsets |
| `dump.h` | C++ | Struct definitions, field offsets, method RVA comments |
| `dump.js` | JavaScript | Ready-to-use Frida `Interceptor.attach` stubs for every method |
| `global-metadata.dat` | Binary | Decrypted plaintext if encrypted metadata was detected |

---

### Installation

```bash
# 1. Trigger the build via workflow_dispatch in GitHub Actions
# 2. Download the zip from Artifacts
# 3. Install via Magisk / KernelSU / APatch
# 4. Edit config:
adb push Eaquel_Config.json /sdcard/Eaquel_Config.json
# 5. Launch the target app — dump happens automatically
```

---

### Why Eaquel?

There are many IL2CPP dumpers available. Here is a technical comparison of what sets Eaquel apart:

| Feature | Il2CppDumper | Perfare Dumper | Zygisk-Il2CppDumper | **Eaquel v2** |
|---------|:---:|:---:|:---:|:---:|
| On-device operation | No | No | Yes | **Yes** |
| ReZygisk support | No | No | Partial | **Full** |
| No RWX pages | — | — | No | **Yes** |
| Adaptive pattern scanner | — | — | No | **Yes (3 layers)** |
| No `sleep()` calls | — | — | No | **Yes** |
| Hot-reload config | No | No | No | **Yes (inotify)** |
| Encrypted metadata support | No | Partial | Partial | **Yes (XOR discovery)** |
| Frida script generation | No | No | No | **Yes** |
| External dependencies | Required | Required | Partial | **Zero** |
| ARM32 + ARM64 | Both | Both | ARM64 only | **Both** |

---

### Tech Stack

| Component | Version |
|-----------|---------|
| C++ Standard | C++23 |
| NDK | r29 · 29.0.14206865 |
| CMake | 4.3.1+ |
| AGP | 9.1.0+ |
| Java | 21 Temurin LTS |
| Min SDK | 30 (Android 11) |
| Compile SDK | 36 (Android 16) |
| Page Size | 16 KB compatible |
| Architectures | arm64-v8a, armeabi-v7a |
| ReZygisk API | v4 |

---

## Русский

### Что это такое?

**Eaquel Dumper** — продвинутый **ReZygisk**-модуль для анализа структуры памяти Unity/IL2CPP приложений на Android 11–16, разрешения таблиц символов и генерации читаемых выходных файлов. Все операции выполняются на устройстве пользователя с root-доступом без серверного соединения.

---

### Архитектура

#### `ReZygisk.hpp`

Полноценная замена Zygisk API архитектурой ReZygisk. `rezygisk::` — основное пространство имён; `zygisk::` сохранён как псевдоним для обратной совместимости.

#### `Core.hpp`

1. **IL2CPP типы** — ~70 сигнатур функций, все внутренние типы и константы атрибутов Unity.
2. **`scanner::` — Адаптивный сканер паттернов** — Три стратегии поиска символов: строгое совпадение → нечёткое → хеш-решётка (FNV-1a fingerprint окон по 16 байт).
3. **`config::` — Горячая перезагрузка** — `inotify` + `eventfd`: любое изменение `Eaquel_Config.json` применяется мгновенно без перезапуска.

#### `Dumper.cpp`

Движок хуков (без RWX страниц), перехватчик метаданных, асинхронный ожидатель (без `sleep()`), движок дампа.

---

### Конфигурация

```json
{
  "Target_Game":       "com.ваша.игра",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### Выходные файлы

| Файл | Содержимое |
|------|-----------|
| `dump.cs` | C#-классы с RVA-смещениями |
| `dump.h` | C++ struct-заголовок с оффсетами |
| `dump.js` | Frida `Interceptor.attach` шаблоны |
| `global-metadata.dat` | Расшифрованные метаданные (если были зашифрованы) |

### Почему Eaquel?

| Функция | Стандартные думперы | **Eaquel v2** |
|---------|---------------------|----------------|
| ReZygisk | Нет | **Да** |
| Нет RWX-страниц | Нет | **Да** |
| Адаптивный сканер | Нет | **Да (3 стратегии)** |
| Нет `sleep()` | Нет | **Да** |
| Горячая перезагрузка | Нет | **Да** |
| Зашифрованные метаданные | Частично | **Да** |

---

## 中文

### 这是什么？

**Eaquel Dumper** 是一个先进的 **ReZygisk** 模块，用于分析 Android 11–16 上运行的 Unity/IL2CPP 应用程序的运行时内存布局，解析符号表并生成可读输出文件。所有操作均在用户自己的设备上以 root 权限执行，无需服务器连接。

---

### 架构

#### `ReZygisk.hpp`

Zygisk API 完整迁移至 ReZygisk 架构。`rezygisk::` 为主命名空间，`zygisk::` 保留为向后兼容别名。

#### `Core.hpp`

1. **IL2CPP 类型定义** — ~70 个函数签名，涵盖所有 Unity 内部类型和属性常量。
2. **`scanner::` — 自适应模式扫描器** — 三层策略链：严格匹配 → 模糊匹配 → 哈希晶格（16字节窗口的 FNV-1a 指纹）。
3. **`config::` — 热重载配置** — 基于 `inotify` + `eventfd`：`Eaquel_Config.json` 的任何更改立即生效，无需重启。

#### `Dumper.cpp`

- 钩子引擎（无 RWX 页面）
- 元数据拦截器（XOR 密钥发现）
- 异步等待器（无 `sleep()` 调用）
- 转储引擎（三种输出格式）

---

### 配置

```json
{
  "Target_Game":       "com.your.game",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### 输出文件

| 文件 | 内容 |
|------|------|
| `dump.cs` | 带 RVA 偏移的 C# 类定义 |
| `dump.h` | 带字段偏移的 C++ 结构体头文件 |
| `dump.js` | Frida `Interceptor.attach` 模板 |
| `global-metadata.dat` | 解密后的元数据（如果已加密） |

### 为什么选择 Eaquel？

| 功能 | 普通转储器 | **Eaquel v2** |
|------|:---:|:---:|
| ReZygisk 支持 | 否 | **是** |
| 无 RWX 页面 | 否 | **是** |
| 自适应扫描器 | 否 | **是（3层）** |
| 无 `sleep()` | 否 | **是** |
| 热重载配置 | 否 | **是** |
| 加密元数据支持 | 部分 | **是** |

---

## Deutsch

### Was ist das?

**Eaquel Dumper** ist ein fortschrittliches **ReZygisk**-Modul zur Analyse der Laufzeit-Speicherstruktur von Unity/IL2CPP-Anwendungen auf Android 11–16. Es löst Symboltabellen auf und erzeugt lesbare Ausgabedateien. Alle Operationen werden auf dem eigenen Gerät des Benutzers mit Root-Rechten durchgeführt.

---

### Architektur

#### `ReZygisk.hpp`

Vollständige Migration der Zygisk-API zur ReZygisk-Architektur. `rezygisk::` ist der primäre Namespace; `zygisk::` bleibt als Rückwärtskompatibilitäts-Alias erhalten.

#### `Core.hpp`

1. **IL2CPP-Typdefinitionen** — ~70 Funktionssignaturen, alle internen Unity-Typen und Attributkonstanten.
2. **`scanner::` — Adaptiver Pattern-Scanner** — Drei-Stufen-Strategie: Strikte Übereinstimmung → Fuzzy → Hash-Gitter (FNV-1a-Fingerprint von 16-Byte-Fenstern).
3. **`config::` — Hot-Reload-Konfiguration** — `inotify` + `eventfd`: Änderungen an `Eaquel_Config.json` werden sofort übernommen.

#### `Dumper.cpp`

Hook-Engine (ohne RWX-Seiten), Metadaten-Interceptor, asynchroner Warteprozess (ohne `sleep()`), Dump-Engine.

---

### Konfiguration

```json
{
  "Target_Game":       "com.dein.spiel",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### Ausgabedateien

| Datei | Inhalt |
|-------|--------|
| `dump.cs` | C#-Klassendefinitionen mit RVA-Offsets |
| `dump.h` | C++-Struct-Header mit Feld-Offsets |
| `dump.js` | Frida `Interceptor.attach`-Vorlagen |
| `global-metadata.dat` | Entschlüsselte Metadaten (falls verschlüsselt) |

---

## Français

### Qu'est-ce que c'est ?

**Eaquel Dumper** est un module **ReZygisk** avancé qui analyse la structure mémoire en temps d'exécution des applications Unity/IL2CPP sur Android 11–16, résout les tables de symboles et génère des fichiers de sortie lisibles. Toutes les opérations sont effectuées sur l'appareil de l'utilisateur avec des droits root, sans connexion serveur.

---

### Architecture

#### `ReZygisk.hpp`

Migration complète de l'API Zygisk vers l'architecture ReZygisk. `rezygisk::` est l'espace de noms principal ; `zygisk::` est conservé comme alias de compatibilité ascendante.

#### `Core.hpp`

1. **Définitions de types IL2CPP** — ~70 signatures de fonctions, tous les types internes et constantes d'attributs Unity.
2. **`scanner::` — Scanner adaptatif** — Chaîne à trois stratégies : correspondance stricte → floue → lattice de hachage (empreinte FNV-1a de fenêtres de 16 octets).
3. **`config::` — Rechargement à chaud** — `inotify` + `eventfd` : tout changement dans `Eaquel_Config.json` est appliqué instantanément.

#### `Dumper.cpp`

Moteur de hooks (sans pages RWX), intercepteur de métadonnées, attente asynchrone (sans `sleep()`), moteur de dump.

---

### Configuration

```json
{
  "Target_Game":       "com.votre.jeu",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### Fichiers de sortie

| Fichier | Contenu |
|---------|---------|
| `dump.cs` | Définitions de classes C# avec offsets RVA |
| `dump.h` | En-tête struct C++ avec offsets de champs |
| `dump.js` | Modèles Frida `Interceptor.attach` |
| `global-metadata.dat` | Métadonnées déchiffrées (si chiffrées) |

---

## Español

### ¿Qué es?

**Eaquel Dumper** es un módulo **ReZygisk** avanzado que analiza la estructura de memoria en tiempo de ejecución de aplicaciones Unity/IL2CPP en Android 11–16, resuelve tablas de símbolos y genera archivos de salida legibles. Todas las operaciones se realizan en el dispositivo del usuario con permisos root, sin conexión a servidores.

---

### Arquitectura

#### `ReZygisk.hpp`

Migración completa de la API Zygisk a la arquitectura ReZygisk. `rezygisk::` es el espacio de nombres principal; `zygisk::` se mantiene como alias de compatibilidad hacia atrás.

#### `Core.hpp`

1. **Definiciones de tipos IL2CPP** — ~70 firmas de funciones, todos los tipos internos y constantes de atributos de Unity.
2. **`scanner::` — Escáner adaptativo** — Cadena de tres estrategias: coincidencia estricta → difusa → hash-lattice (huella FNV-1a de ventanas de 16 bytes).
3. **`config::` — Recarga en caliente** — `inotify` + `eventfd`: cualquier cambio en `Eaquel_Config.json` se aplica instantáneamente.

#### `Dumper.cpp`

Motor de hooks (sin páginas RWX), interceptor de metadatos, espera asíncrona (sin `sleep()`), motor de volcado.

---

### Configuración

```json
{
  "Target_Game":       "com.tu.juego",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### Archivos de salida

| Archivo | Contenido |
|---------|-----------|
| `dump.cs` | Definiciones de clases C# con offsets RVA |
| `dump.h` | Encabezado struct C++ con offsets de campos |
| `dump.js` | Plantillas Frida `Interceptor.attach` |
| `global-metadata.dat` | Metadatos descifrados (si estaban cifrados) |

---

## 日本語

### 概要

**Eaquel Dumper** は、Android 11–16 上で動作する Unity/IL2CPP アプリケーションのランタイムメモリ構造を解析し、シンボルテーブルを解決して可読な出力ファイルを生成する高度な **ReZygisk** モジュールです。すべての操作はユーザー自身のデバイスで root 権限のもと実行され、サーバー接続は行いません。

---

### アーキテクチャ

#### `ReZygisk.hpp`

Zygisk API を ReZygisk アーキテクチャに完全移行。`rezygisk::` が主名前空間；`zygisk::` は後方互換エイリアスとして保持。

#### `Core.hpp`

1. **IL2CPP 型定義** — ~70 の関数シグネチャ、すべての Unity 内部型と属性定数。
2. **`scanner::` — アダプティブパターンスキャナー** — 3層戦略チェーン：厳密一致 → ファジー → ハッシュ格子（16バイトウィンドウの FNV-1a フィンガープリント）。
3. **`config::` — ホットリロード設定** — `inotify` + `eventfd`：`Eaquel_Config.json` の変更は即座に反映。

#### `Dumper.cpp`

フックエンジン（RWX ページなし）、メタデータインターセプター、非同期ウェイター（`sleep()` なし）、ダンプエンジン。

---

### 設定

```json
{
  "Target_Game":       "com.your.game",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### 出力ファイル

| ファイル | 内容 |
|---------|------|
| `dump.cs` | RVA オフセット付き C# クラス定義 |
| `dump.h` | フィールドオフセット付き C++ 構造体ヘッダー |
| `dump.js` | 全メソッドの Frida `Interceptor.attach` テンプレート |
| `global-metadata.dat` | 復号化されたメタデータ（暗号化されていた場合） |

---

## 한국어

### 소개

**Eaquel Dumper**는 Android 11–16에서 실행되는 Unity/IL2CPP 애플리케이션의 런타임 메모리 구조를 분석하고, 심볼 테이블을 해석하며 읽을 수 있는 출력 파일을 생성하는 고급 **ReZygisk** 모듈입니다. 모든 작업은 사용자 자신의 기기에서 root 권한으로 수행되며 서버 연결은 없습니다.

---

### 아키텍처

#### `ReZygisk.hpp`

Zygisk API를 ReZygisk 아키텍처로 완전 전환. `rezygisk::`가 기본 네임스페이스; `zygisk::`는 하위 호환성 별칭으로 유지.

#### `Core.hpp`

1. **IL2CPP 타입 정의** — ~70개의 함수 시그니처, 모든 Unity 내부 타입과 속성 상수.
2. **`scanner::` — 적응형 패턴 스캐너** — 3단계 전략 체인: 엄격 매칭 → 퍼지 → 해시 격자 (16바이트 창의 FNV-1a 지문).
3. **`config::` — 핫 리로드 설정** — `inotify` + `eventfd`: `Eaquel_Config.json`의 변경이 즉시 반영.

#### `Dumper.cpp`

훅 엔진(RWX 페이지 없음), 메타데이터 인터셉터, 비동기 대기자(`sleep()` 없음), 덤프 엔진.

---

### 설정

```json
{
  "Target_Game":       "com.your.game",
  "Output":            "/sdcard/Eaquel_Dumps",
  "Cpp_Header":        true,
  "Frida_Script":      true,
  "include_generic":   true,
  "Protected_Breaker": true
}
```

### 출력 파일

| 파일 | 내용 |
|------|------|
| `dump.cs` | RVA 오프셋 포함 C# 클래스 정의 |
| `dump.h` | 필드 오프셋 포함 C++ 구조체 헤더 |
| `dump.js` | 모든 메서드의 Frida `Interceptor.attach` 템플릿 |
| `global-metadata.dat` | 복호화된 메타데이터(암호화되어 있었을 경우) |

---

<div align="center">

## Ethics & Disclaimer

This project is designed for **forensic analysis**, **reverse engineering education**, and **system-level research**. All operations are performed by the user on their own device with self-granted root access. Unauthorized use against third-party applications without their consent is the sole responsibility of the user.

---

*Eaquel Dumper · 2026 · C++23 · ReZygisk API v4 · 16 KB Page Size Compatible · Zero External Dependencies*

</div>
