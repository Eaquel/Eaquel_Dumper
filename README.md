# Eaquel_Dumper

**Eaquel_Dumper**, Android 11–16 (API 30–36) ekosisteminde çalışan uygulamaların çalışma zamanı bellek yapısını analiz eden, ELF ikili dosyalarını parse eden ve IL2CPP sembol tablolarını (`Dynsym` / `Symtab`) dump eden modern bir **Zygisk** modülüdür.

Magisk'in Zygisk altyapısı üzerinde çalışır. Hedef uygulama başlatıldığında modül devreye girer, `libil2cpp.so` kütüphanesini bellekte bulur, sembol tablosunu çözümler ve çıktıyı cihaza yazar. Tüm işlemler kullanıcı tarafından sağlanan **root yetkisiyle** ve **kontrollü bir ortamda** gerçekleştirilir.

---

## Teknik Yığın

| Bileşen | Sürüm |
|---|---|
| Android Gradle Plugin | 9.1.0 |
| Gradle | 9.4.1 |
| NDK | r29 · 29.0.14206865 |
| CMake | 3.22.1+ |
| Java | 21 Temurin LTS |
| C++ Standardı | C++20 |
| xDL | 2.3.0 (Maven Central · Prefab) |
| Min SDK | 30 · Android 11 |
| Target SDK | 36 · Android 16 |
| 16KB Page Size | Etkin |
| Magisk | v26.0+ |

---

## Mimari

Proje üç ana katmandan oluşur:

**Binary Resolver** — `libil2cpp.so` ve `libunity.so` gibi kütüphaneleri çalışma zamanında analiz eder. RVA (Relative Virtual Address) offsetlerini hesaplar, `Dynsym` ve `Symtab` bölümlerini parse eder.

**Pattern Matcher** — Sembolleri silinmiş (stripped) fonksiyonları, fonksiyon başlangıç imzalarından (prologue) tespit eden algoritmadır. `libil2cpp.so` gibi sıkıştırılmış veya şifrelenmiş binary dosyalarında sembol olmadan da adres tespiti yapar.

**Root Bridge** — Zygisk API üzerinden hedef uygulamaya enjekte edilen katmandır. `process_vm_readv()` sistem çağrısını kullanarak ayrıcalıklı bellek okuma işlemlerini gerçekleştirir.

---

## Proje Yapısı

```
Eaquel_Dumper/
├── .github/
│   └── build.yml                     ← GitHub Actions CI/CD iş akışı
├── Dumper/
│   ├── build.gradle.kts              ← Modül build yapılandırması (AGP 9.1.0)
│   └── Source/Main/
│       ├── AndroidManifest.xml       ← Android manifest
│       └── Native/
│           ├── CMakeLists.txt        ← NDK build sistemi (CMake 3.22.1+)
│           ├── Main.cpp              ← Zygisk giriş noktası
│           ├── Hack.cpp              ← Bellek okuma ve RVA hesaplama motoru
│           └── Dumper.cpp            ← IL2CPP sembol dump motoru
├── Base/
│   └── module.prop                   ← Magisk modül metadata şablonu
├── Gradle/
│   ├── gradle-wrapper.jar            ← Gradle wrapper çalıştırıcı
│   ├── gradle-wrapper.properties     ← Gradle 9.4.1 dağıtım URL'i
│   └── libs.versions.toml            ← Merkezi versiyon kataloğu
├── out/                              ← Derleme çıktıları (otomatik oluşur)
├── build.gradle.kts                  ← Kök build yapılandırması
├── settings.gradle.kts               ← Modül tanımı ve repo yönetimi
├── module.gradle.kts                 ← Modül metadata değişkenleri
├── gradle.properties                 ← JVM ve AGP bayrakları
├── gradlew                           ← Unix build betiği
├── gradlew.bat                       ← Windows build betiği
└── README.md
```

---

## Gereksinimler

- **Magisk** v26.0 veya üzeri (Zygisk etkin)
- **Android Studio** Otter 3 Feature Drop veya üzeri
- **Java** 21 (Temurin LTS)
- **Android NDK** r29
- **CMake** 3.22.1+

---

## GitHub Actions ile Derleme

1. Bu repoyu **Fork** la
2. **Actions → Build → Run workflow** yolunu izle
3. Hedef uygulamanın paket adını gir (örnek: `com.example.game`)
4. Derleme varyantını seç (`release` / `debug`)
5. İş akışı tamamlanınca **Artifacts** bölümünden zip dosyasını indir

---

## Android Studio ile Derleme

1. `Dumper/Source/Main/Native/` klasöründeki `game.h` dosyasını aç
2. `GamePackageName` değerini hedef uygulamanın paket adıyla değiştir
3. Gradle görevini çalıştır:
   ```
   :Dumper:assembleRelease
   ```
4. Çıktı zip dosyası `out/` klasöründe oluşur

---

## Kurulum

1. Magisk Manager'ı aç
2. **Modüller → Depodan Yükle** veya **zip'ten Yükle** seçeneğini kullan
3. Oluşturulan `.zip` dosyasını seç ve yükle
4. Cihazı yeniden başlat
5. Hedef uygulamayı başlat
6. Dump çıktısı şu konuma yazılır:
   ```
   /data/data/<paket_adı>/files/dump.cs
   ```

---

## Bağımlılıklar

| Kütüphane | Sürüm | Açıklama |
|---|---|---|
| [xDL](https://github.com/hexhacking/xDL) | 2.3.0 | Android linker namespace kısıtlamalarını aşan gelişmiş `dlopen` / `dlsym` implementasyonu. `.dynsym` ve `.symtab` sembol tablosu çözümleme desteği sunar. Android 4.1–16 (API 16–36) tam uyumlu. |

xDL, **Maven Central** üzerinden **Prefab** formatında sağlanır. Kaynak kodu projeye dahil edilmez; Gradle derleme sırasında otomatik olarak indirir.

---

## Güvenlik ve Etik

Bu proje **adli bilişim (forensics)**, **tersine mühendislik eğitimi** ve **sistem analizi** amacıyla geliştirilmiştir. Tüm işlemler kullanıcı tarafından sağlanan root yetkisiyle, kullanıcının kendi cihazında gerçekleştirilir. Üçüncü taraf uygulamalara yetkisiz erişim için kullanılması kullanıcının sorumluluğundadır.

---