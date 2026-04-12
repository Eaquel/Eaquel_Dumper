<div align="center">

```
███████╗ █████╗  ██████╗ ██╗   ██╗███████╗██╗
██╔════╝██╔══██╗██╔═══██╗██║   ██║██╔════╝██║
█████╗  ███████║██║   ██║██║   ██║█████╗  ██║
██╔══╝  ██╔══██║██║▄▄ ██║██║   ██║██╔══╝  ██║
███████╗██║  ██║╚██████╔╝╚██████╔╝███████╗███████╗
╚══════╝╚═╝  ╚═╝ ╚══▀▀═╝  ╚═════╝ ╚══════╝╚══════╝
         D U M P E R   ·   v 2 . 0
```

**IL2CPP Binary Resolver & Runtime Forensics Module**

[![Build](https://github.com/yourusername/Eaquel_Dumper/actions/workflows/build.yml/badge.svg)](https://github.com/yourusername/Eaquel_Dumper/actions)
![Android](https://img.shields.io/badge/Android-11--16-green?logo=android)
![API](https://img.shields.io/badge/API-30--36-blue)
![NDK](https://img.shields.io/badge/NDK-r29-orange)
![C++](https://img.shields.io/badge/C%2B%2B-20-purple)
![License](https://img.shields.io/badge/License-Educational-red)

</div>

---

## 🌍 Language / Dil / Язык / 语言 / Lingua / Sprache / Langue / Idioma / 言語 / 언어

| # | Language | Jump |
|---|----------|------|
| 01 | 🇹🇷 Türkçe      | [→ TR](#-türkçe)       |
| 02 | 🇬🇧 English      | [→ EN](#-english)      |
| 03 | 🇷🇺 Русский      | [→ RU](#-русский)      |
| 04 | 🇨🇳 中文          | [→ ZH](#-中文)          |
| 05 | 🇩🇪 Deutsch      | [→ DE](#-deutsch)      |
| 06 | 🇫🇷 Français     | [→ FR](#-français)     |
| 07 | 🇪🇸 Español      | [→ ES](#-español)      |
| 08 | 🇮🇳 हिन्दी        | [→ HI](#-हिन्दी)        |
| 09 | 🇯🇵 日本語         | [→ JA](#-日本語)         |
| 10 | 🇰🇷 한국어         | [→ KO](#-한국어)         |

---

## 🇹🇷 Türkçe

### Nedir?

**Eaquel_Dumper**, Android 11–16 ekosisteminde çalışan Unity/IL2CPP tabanlı uygulamaların çalışma zamanı bellek yapısını analiz eden, ELF ikili dosyalarını parse eden ve IL2CPP sembol tablolarını (`Dynsym` / `Symtab`) dump eden modern bir **Zygisk** modülüdür. Tüm işlemler kullanıcının kendi cihazında, **root yetkisiyle** gerçekleştirilir.

### Mimari

Proje tek bir header (`Core.h`) ve tek bir kaynak dosyasından (`Dumper.cpp`) oluşur:

**`Core.h` — Birleşik Başlık**
Orijinal IL2CPP tip tanımları, attribute sabitleri ve DO_API makro tablosuna ek olarak iki namespace barındırır:
- `scanner::` — ARM64 prologue imza tabanlı pattern scanner. Sembolleri silinmiş binary dosyalarda xdl_sym başarısız olduğunda devreye girer.
- `config::` — `/data/local/tmp/eaquel_config.json` dosyasını runtime'da okuyarak hedef paket adını, çıktı dizinini ve üretim seçeneklerini çözümler. Proje artık `Target.h` içindeki hardcoded sabit kullanmaz.

**`Dumper.cpp` — Birleşik Kaynak**
`Main.cpp` (Zygisk entry, EaquelDumperModule), `Hack.cpp` (exponential backoff ile libil2cpp.so bulucu, x86 NativeBridge köprüsü) ve orijinal `Dumper.cpp` (FunctionTable, dump motoru, çıktı yazıcılar) tek bir translation unit'te birleştirilmiştir.

### Çıktılar

| Dosya | İçerik |
|-------|--------|
| `dump.cs` | C# sınıf tanımları, erişim belirleyiciler, RVA offsetleri |
| `dump.h` | Offset'li C++ struct header dosyası |
| `dump.js` | Her metod için Frida `Interceptor.attach` taslakları |

### Yapılandırma (JSON)

`/data/local/tmp/eaquel_config.json` dosyasını cihaza gönderin:

```json
{
  "target_package"        : "com.hedef.uygulama",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

Veya Kitsune / Magisk WebUI üzerindeki **CONFIG** sekmesinden tek tıkla ayarlayın.

### Proje Yapısı

```
Eaquel_Dumper/
├── .github/
│   └── workflows/
│       └── build.yml                  ← CI/CD (package name gerektirmez)
├── Dumper/
│   ├── build.gradle.kts               ← AGP 9.1.0 · Gradle 9.4.1
│   └── Source/Main/
│       ├── AndroidManifest.xml
│       └── Native/
│           ├── CMakeLists.txt         ← Tek kaynak: Dumper.cpp
│           ├── Core.h                 ← Birleşik header (IL2CPP + Scanner + Config)
│           ├── Dumper.cpp             ← Birleşik kaynak (Main + Hack + Dumper)
│           └── Zygisk.hpp             ← Zygisk API v4
├── Base/
│   └── module.prop
├── Gradle/
├── out/
├── eaquel_config.json                 ← Runtime config şablonu
├── build.gradle.kts
├── settings.gradle.kts
├── module.gradle.kts
├── gradle.properties
├── gradlew  /  gradlew.bat
└── README.md
```

### Kurulum

```bash
# 1. Repo'yu fork'la ve Actions'ta Build'i çalıştır
# 2. Artifacts'tan zip'i indir
# 3. Magisk / KernelSU / APatch üzerinden modülü yükle
# 4. Config'i gönder:
adb push eaquel_config.json /data/local/tmp/
# 5. Hedef uygulamayı başlat
```

---

## 🇬🇧 English

### What Is It?

**Eaquel_Dumper** is a modern **Zygisk** module that analyses the runtime memory layout of Unity/IL2CPP applications running on Android 11–16, parses ELF binaries and dumps IL2CPP symbol tables (`Dynsym` / `Symtab`). All operations are performed on the user's own device under **root authority**.

### Architecture

The project is reduced to a single header (`Core.h`) and a single source file (`Dumper.cpp`):

**`Core.h` — Unified Header**
Houses the original IL2CPP type definitions, attribute constants and the DO_API macro table, plus two namespaces:
- `scanner::` — ARM64 prologue-signature pattern scanner. Activates as a fallback when `xdl_sym` fails on stripped binaries.
- `config::` — Reads `/data/local/tmp/eaquel_config.json` at runtime to resolve target package, output directory and generation flags. The project no longer uses the hardcoded `Target.h` constant.

**`Dumper.cpp` — Unified Source**
`Main.cpp` (Zygisk entry, EaquelDumperModule), `Hack.cpp` (exponential-backoff `libil2cpp.so` finder, x86 NativeBridge path) and the original `Dumper.cpp` (FunctionTable, dump engine, output writers) are merged into a single translation unit.

### Outputs

| File | Content |
|------|---------|
| `dump.cs` | C# class definitions, access modifiers, RVA offsets |
| `dump.h` | C++ struct header with field offsets |
| `dump.js` | Frida `Interceptor.attach` stubs for every method |

### Configuration (JSON)

Push `/data/local/tmp/eaquel_config.json` to the device:

```json
{
  "target_package"        : "com.your.target",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

Or configure with a single tap from the **CONFIG** tab in the Kitsune / Magisk WebUI.

### Project Structure

```
Eaquel_Dumper/
├── .github/workflows/build.yml        ← CI/CD (no package name required)
├── Dumper/
│   ├── build.gradle.kts               ← AGP 9.1.0 · Gradle 9.4.1
│   └── Source/Main/Native/
│       ├── CMakeLists.txt             ← Single source: Dumper.cpp
│       ├── Core.h                     ← Unified header (IL2CPP + Scanner + Config)
│       ├── Dumper.cpp                 ← Unified source (Main + Hack + Dumper)
│       └── Zygisk.hpp
├── Base/module.prop
├── eaquel_config.json                 ← Runtime config template
└── README.md
```

### Installation

```bash
# 1. Fork the repo and run Build in Actions
# 2. Download the zip from Artifacts
# 3. Install module via Magisk / KernelSU / APatch
# 4. Push config:
adb push eaquel_config.json /data/local/tmp/
# 5. Launch target app
```

### Tech Stack

| Component | Version |
|-----------|---------|
| AGP | 9.1.0 |
| Gradle | 9.4.1 |
| NDK | r29 · 29.0.14206865 |
| CMake | 3.22.1+ |
| Java | 21 Temurin LTS |
| C++ Standard | C++20 |
| xDL | 2.3.0 |
| Min SDK | 30 (Android 11) |
| Target SDK | 36 (Android 16) |
| Page Size | 16 KB compatible |

---

## 🇷🇺 Русский

### Что это такое?

**Eaquel_Dumper** — современный **Zygisk**-модуль для анализа структуры памяти Unity/IL2CPP-приложений на Android 11–16. Парсит ELF-бинарники и дампит таблицы символов IL2CPP. Все операции выполняются на устройстве пользователя с **правами root**.

### Архитектура

Проект сведён к одному заголовочному файлу (`Core.h`) и одному исходному файлу (`Dumper.cpp`):

**`Core.h`** — объединённый заголовок с типами IL2CPP, константами атрибутов, DO_API-таблицей и двумя пространствами имён:
- `scanner::` — ARM64 pattern scanner по прологу функций (fallback при strip-бинарниках).
- `config::` — читает `/data/local/tmp/eaquel_config.json` во время выполнения.

**`Dumper.cpp`** — объединённый источник: `Main.cpp` + `Hack.cpp` + оригинальный `Dumper.cpp` в одной единице трансляции.

### Выходные файлы

| Файл | Содержимое |
|------|-----------|
| `dump.cs` | C#-классы с RVA-смещениями |
| `dump.h` | C++ struct-заголовок с оффсетами полей |
| `dump.js` | Frida `Interceptor.attach` шаблоны |

### Конфигурация (JSON)

Скопируйте на устройство `/data/local/tmp/eaquel_config.json`:

```json
{
  "target_package"        : "com.your.game",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

### Установка

```bash
adb push eaquel_config.json /data/local/tmp/
# Установите ZIP через Magisk / KernelSU, затем запустите целевое приложение
```

---

## 🇨🇳 中文

### 这是什么？

**Eaquel_Dumper** 是一个现代 **Zygisk** 模块，用于分析运行在 Android 11–16 上的 Unity/IL2CPP 应用程序的运行时内存结构、解析 ELF 二进制文件并转储 IL2CPP 符号表。所有操作均在用户自己的设备上以 **root 权限**执行。

### 架构

项目精简为一个头文件（`Core.h`）和一个源文件（`Dumper.cpp`）：

**`Core.h`** — 统一头文件，包含 IL2CPP 类型定义、属性常量、DO_API 宏表以及两个命名空间：
- `scanner::` — 基于 ARM64 函数序言签名的模式扫描器（stripped 二进制文件的回退方案）。
- `config::` — 运行时读取 `/data/local/tmp/eaquel_config.json`。

**`Dumper.cpp`** — 统一源文件，将 `Main.cpp`、`Hack.cpp` 和原始 `Dumper.cpp` 合并为单一翻译单元。

### 输出文件

| 文件 | 内容 |
|------|------|
| `dump.cs` | 带 RVA 偏移的 C# 类定义 |
| `dump.h` | 带字段偏移的 C++ 结构头文件 |
| `dump.js` | 每个方法的 Frida `Interceptor.attach` 模板 |

### 配置（JSON）

将以下内容推送至设备 `/data/local/tmp/eaquel_config.json`：

```json
{
  "target_package"        : "com.your.game",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

### 安装

```bash
adb push eaquel_config.json /data/local/tmp/
# 通过 Magisk / KernelSU 安装 ZIP，然后启动目标应用
```

---

## 🇩🇪 Deutsch

### Was ist das?

**Eaquel_Dumper** ist ein modernes **Zygisk**-Modul zur Laufzeit-Speicheranalyse von Unity/IL2CPP-Anwendungen auf Android 11–16. Es parst ELF-Binärdateien und dumpt IL2CPP-Symboltabellen. Alle Operationen werden auf dem eigenen Gerät des Nutzers mit **Root-Rechten** durchgeführt.

### Architektur

Das Projekt besteht aus einer einzigen Header-Datei (`Core.h`) und einer einzigen Quelldatei (`Dumper.cpp`):

**`Core.h`** — Vereinheitlichter Header mit IL2CPP-Typdeklarationen, Attributkonstanten, DO_API-Makrotabelle und zwei Namespaces:
- `scanner::` — ARM64-Pattern-Scanner basierend auf Funktionsprologen (Fallback für gestrippte Binärdateien).
- `config::` — Liest `/data/local/tmp/eaquel_config.json` zur Laufzeit.

**`Dumper.cpp`** — Vereinheitlichte Quelldatei: `Main.cpp` + `Hack.cpp` + originales `Dumper.cpp` in einer einzigen Übersetzungseinheit.

### Ausgabedateien

| Datei | Inhalt |
|-------|--------|
| `dump.cs` | C#-Klassendefinitionen mit RVA-Offsets |
| `dump.h` | C++ Struct-Header mit Feld-Offsets |
| `dump.js` | Frida `Interceptor.attach`-Vorlagen |

### Konfiguration (JSON)

```json
{
  "target_package"        : "com.dein.spiel",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

```bash
adb push eaquel_config.json /data/local/tmp/
```

---

## 🇫🇷 Français

### Qu'est-ce que c'est ?

**Eaquel_Dumper** est un module **Zygisk** moderne qui analyse la structure mémoire à l'exécution des applications Unity/IL2CPP sur Android 11–16, parse les binaires ELF et extrait les tables de symboles IL2CPP. Toutes les opérations s'effectuent sur le propre appareil de l'utilisateur avec des **droits root**.

### Architecture

Le projet se résume à un seul en-tête (`Core.h`) et un seul fichier source (`Dumper.cpp`):

**`Core.h`** — En-tête unifié avec les types IL2CPP, les constantes d'attributs, la table de macros DO_API et deux espaces de noms :
- `scanner::` — Scanner de patterns ARM64 basé sur les prologues de fonctions (repli pour les binaires strippés).
- `config::` — Lit `/data/local/tmp/eaquel_config.json` à l'exécution.

**`Dumper.cpp`** — Source unifiée : `Main.cpp` + `Hack.cpp` + `Dumper.cpp` original en une seule unité de traduction.

### Fichiers de sortie

| Fichier | Contenu |
|---------|---------|
| `dump.cs` | Définitions de classes C# avec offsets RVA |
| `dump.h` | En-tête struct C++ avec offsets de champs |
| `dump.js` | Modèles Frida `Interceptor.attach` |

### Configuration (JSON)

```json
{
  "target_package"        : "com.votre.jeu",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

```bash
adb push eaquel_config.json /data/local/tmp/
```

---

## 🇪🇸 Español

### ¿Qué es?

**Eaquel_Dumper** es un módulo **Zygisk** moderno que analiza la estructura de memoria en tiempo de ejecución de aplicaciones Unity/IL2CPP en Android 11–16, analiza binarios ELF y vuelca tablas de símbolos IL2CPP. Todas las operaciones se realizan en el propio dispositivo del usuario con **permisos root**.

### Arquitectura

El proyecto se reduce a un único encabezado (`Core.h`) y un único archivo fuente (`Dumper.cpp`):

**`Core.h`** — Encabezado unificado con tipos IL2CPP, constantes de atributos, tabla de macros DO_API y dos espacios de nombres:
- `scanner::` — Escáner de patrones ARM64 basado en prólogos de funciones (respaldo para binarios despojados).
- `config::` — Lee `/data/local/tmp/eaquel_config.json` en tiempo de ejecución.

**`Dumper.cpp`** — Fuente unificada: `Main.cpp` + `Hack.cpp` + `Dumper.cpp` original en una sola unidad de traducción.

### Archivos de salida

| Archivo | Contenido |
|---------|-----------|
| `dump.cs` | Definiciones de clases C# con offsets RVA |
| `dump.h` | Encabezado struct C++ con offsets de campos |
| `dump.js` | Plantillas Frida `Interceptor.attach` |

### Configuración (JSON)

```json
{
  "target_package"        : "com.tu.juego",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

```bash
adb push eaquel_config.json /data/local/tmp/
```

---

## 🇮🇳 हिन्दी

### यह क्या है?

**Eaquel_Dumper** एक आधुनिक **Zygisk** मॉड्यूल है जो Android 11–16 पर चलने वाले Unity/IL2CPP अनुप्रयोगों की रनटाइम मेमोरी संरचना का विश्लेषण करता है, ELF बाइनरी पार्स करता है और IL2CPP सिम्बल तालिकाएँ डंप करता है। सभी ऑपरेशन उपयोगकर्ता के अपने डिवाइस पर **root अनुमति** के साथ किए जाते हैं।

### आर्किटेक्चर

प्रोजेक्ट एक एकल हेडर (`Core.h`) और एक एकल सोर्स फ़ाइल (`Dumper.cpp`) तक सीमित है:

**`Core.h`** — एकीकृत हेडर जिसमें IL2CPP टाइप परिभाषाएँ, attribute constants, DO_API macro table और दो namespaces हैं:
- `scanner::` — ARM64 function prologue आधारित pattern scanner (stripped बाइनरी के लिए fallback)।
- `config::` — रनटाइम पर `/data/local/tmp/eaquel_config.json` पढ़ता है।

**`Dumper.cpp`** — एकीकृत सोर्स: `Main.cpp` + `Hack.cpp` + मूल `Dumper.cpp` एकल translation unit में।

### कॉन्फ़िगरेशन (JSON)

```json
{
  "target_package"        : "com.aapka.game",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

```bash
adb push eaquel_config.json /data/local/tmp/
```

---

## 🇯🇵 日本語

### 概要

**Eaquel_Dumper** は、Android 11–16 上で動作する Unity/IL2CPP アプリケーションのランタイムメモリ構造を解析し、ELF バイナリをパースして IL2CPP シンボルテーブルをダンプする現代的な **Zygisk** モジュールです。すべての操作はユーザー自身のデバイスで **root 権限** のもと実行されます。

### アーキテクチャ

プロジェクトは単一のヘッダー (`Core.h`) と単一のソースファイル (`Dumper.cpp`) に集約されています:

**`Core.h`** — IL2CPP 型定義、属性定数、DO_API マクロテーブル、および2つの名前空間を含む統合ヘッダー:
- `scanner::` — ARM64 関数プロローグシグネチャベースのパターンスキャナー（stripped バイナリのフォールバック）。
- `config::` — ランタイムに `/data/local/tmp/eaquel_config.json` を読み込む。

**`Dumper.cpp`** — `Main.cpp` + `Hack.cpp` + 元の `Dumper.cpp` を単一翻訳単位に統合したソース。

### 出力ファイル

| ファイル | 内容 |
|---------|------|
| `dump.cs` | RVA オフセット付き C# クラス定義 |
| `dump.h` | フィールドオフセット付き C++ 構造体ヘッダー |
| `dump.js` | 全メソッドの Frida `Interceptor.attach` テンプレート |

### 設定 (JSON)

```json
{
  "target_package"        : "com.your.game",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

```bash
adb push eaquel_config.json /data/local/tmp/
```

---

## 🇰🇷 한국어

### 소개

**Eaquel_Dumper** 는 Android 11–16에서 실행되는 Unity/IL2CPP 애플리케이션의 런타임 메모리 구조를 분석하고, ELF 바이너리를 파싱하며 IL2CPP 심볼 테이블을 덤프하는 현대적인 **Zygisk** 모듈입니다. 모든 작업은 사용자 자신의 기기에서 **root 권한**으로 수행됩니다.

### 아키텍처

프로젝트는 단일 헤더 (`Core.h`)와 단일 소스 파일 (`Dumper.cpp`)로 구성됩니다:

**`Core.h`** — IL2CPP 타입 정의, 속성 상수, DO_API 매크로 테이블과 두 개의 네임스페이스를 포함하는 통합 헤더:
- `scanner::` — ARM64 함수 프롤로그 시그니처 기반 패턴 스캐너 (stripped 바이너리 폴백).
- `config::` — 런타임에 `/data/local/tmp/eaquel_config.json`을 읽음.

**`Dumper.cpp`** — `Main.cpp` + `Hack.cpp` + 원본 `Dumper.cpp`를 단일 번역 단위로 통합한 소스.

### 출력 파일

| 파일 | 내용 |
|------|------|
| `dump.cs` | RVA 오프셋 포함 C# 클래스 정의 |
| `dump.h` | 필드 오프셋 포함 C++ 구조체 헤더 |
| `dump.js` | 모든 메서드의 Frida `Interceptor.attach` 템플릿 |

### 설정 (JSON)

```json
{
  "target_package"        : "com.your.game",
  "output_dir"            : "/sdcard/Eaquel",
  "generate_cpp_header"   : true,
  "generate_frida_script" : true
}
```

```bash
adb push eaquel_config.json /data/local/tmp/
```

---

<div align="center">

## ⚙️ Kitsune / Magisk WebUI Config Panel

The module ships with a built-in **WebUI** accessible directly from the Magisk / KernelSU / APatch module manager.

```
┌─────────────────────────────────────────────┐
│  EAQUEL.DUMP  ·  IL2CPP Binary Resolver     │
├─────────────────────────────────────────────┤
│  [TARGET]  [CONFIG]  [EXTRACT]  [CONSOLE]   │
├─────────────────────────────────────────────┤
│                                             │
│  ⚙  RUNTIME CONFIGURATION                  │
│  ─────────────────────────────────────────  │
│  Target Package  [ com.your.game         ]  │
│  Output Dir      [ /sdcard/Eaquel        ]  │
│  C++ Header      [  ON ●──────────────  ]   │
│  Frida Script    [  ON ●──────────────  ]   │
│                                             │
│  [  SAVE & APPLY CONFIG  ]                  │
│                                             │
│  ✔ Config saved → /data/local/tmp/          │
│    eaquel_config.json                       │
└─────────────────────────────────────────────┘
```

No recompilation required. Change target at any time.

---

## 🛡️ Ethics & Disclaimer

This project is designed for **forensic analysis**, **reverse engineering education**, and **system-level research**. All operations are performed by the user on their own device with self-granted root access. Unauthorized use against third-party applications is the sole responsibility of the user.

---

*Eaquel_Dumper · 2026 · C++20 · Zygisk API v4 · 16 KB Page Size Compatible*

</div>
