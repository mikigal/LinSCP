# OpenSCP — plan portu WinSCP na Linuksa (Wayland)

Status: propozycja do akceptacji · 2026-10-24

## 1. Cel i założenia

- Natywna aplikacja na Linuksa z funkcjonalnością WinSCP: SFTP, SCP, FTP/FTPS, WebDAV, S3,
  site manager, interfejs Commander/Explorer, kolejka transferów, synchronizacja, edycja
  plików (wewnętrzna i zewnętrzna), skrypty (`winscp.com`-kompatybilny CLI), konsola poleceń.
- Pierwszorzędna obsługa Waylanda (KDE Plasma/KWin jako platforma referencyjna, ale bez
  twardej zależności od KDE — ma działać też na GNOME/wlroots).
- Poprawna integracja z pulpitem: drag & drop w obie strony z menedżerem plików, schowek
  plików, portale XDG, powiadomienia, obsługa URL-i `sftp://` itd.
- Z tego repozytorium **nigdy** nie będzie budowana wersja Windows → kod może przestać być
  kompilowalny w C++Builderze; nie utrzymujemy `#ifdef`-ów dla Windows.
- Repozytorium jest forkiem 1:1 upstreamu. Zakładam, że chcemy dalej wciągać poprawki
  z upstreamu (bezpieczeństwo, protokoły) — to mocno wpływa na strategię (sekcja 3.2).

## 2. Wyniki analizy

| Katalog | Rozmiar | Co to jest | Zależności | Decyzja |
|---|---|---|---|---|
| `source/core` | 69 plików, ~76k linii | logika protokołów, sesje, kolejka, skrypty, synchronizacja | RTL Delphi (`UnicodeString`, `TStrings`, `TDateTime`, …), dialekt C++Builder, trochę WinAPI | **port** (shim RTL + konwersja dialektu) |
| `source/putty` | ~83k linii | PuTTY 0.85 z modyfikacjami WinSCP (51× `MPEXT`) | tylko część `windows/` jest platformowa | **port**: kod wspólny bez zmian, `windows/` → `unix/` z upstreamu PuTTY 0.85 |
| `source/filezilla` | ~19k linii | silnik FTP z FileZilla 2 | MFC (`CString`, `CTime`, `CFile`), `CAsyncSocketEx` na ukrytych oknach i `WSAAsyncSelect` | **port**: własny mini-shim MFC + reaktor na `poll()` zamiast pętli komunikatów |
| `libs/neon`, `libs/libs3` | — | WebDAV/HTTP i S3, **zmodyfikowane przez WinSCP** (31 i 22 pliki z markerami) | OpenSSL, expat — dostępne w systemie | **budujemy wersje z repo** (nie systemowe), CMake |
| `libs/openssl`, `libs/expat` | — | kopie bibliotek | — | systemowe (`openssl` 3.6, `expat` 2.9) |
| `libs/mfc` | — | wycinek źródeł MFC | Win32 | **porzucamy** (zastępuje go shim) |
| `source/windows` | ~29k linii | spoiwo GUI: konfiguracja, `TerminalManager`, `QueueController`, `EditorManager`, `ConsoleRunner`, `WinInterface` | VCL + WinAPI | **częściowo port** (logika), reszta przepisana pod Qt |
| `source/forms` | 48 formularzy `.dfm`, ~47k linii | wszystkie okna i dialogi (`CustomScpExplorer.cpp` sam ma 12,6k linii) | VCL | **przepisanie w Qt 6**, `.dfm` jako specyfikacja |
| `source/components`, `source/packages` | ~125k linii | DirView/DriveView, Toolbar2000/TBX, DragDrop, JCL, PNG | VCL, Pascal | **porzucamy** — odpowiedniki w Qt |
| `source/dragext` | <1k linii | rozszerzenie powłoki Explorera do D&D | COM | **porzucamy** — zastępuje go mechanizm z sekcji 5 |
| `source/console` | ~1k linii | `winscp.com` (cienki klient do `WinSCP.exe /console`) | WinAPI IPC | **zastępujemy** jednym binarnym CLI |
| `dotnet/` | — | assembly .NET sterujące `winscp.com` | P/Invoke Win32 (eventy, file mapping) | **poza zakresem** na start (ew. później) |
| `deployment/` | — | instalator Inno Setup | — | **zastępujemy** PKGBUILD / Flatpak / AppImage |
| `translations/` | 3,7k linii/język | tłumaczenia `.ini` kluczowane `Formularz.dfm` → `Komponent.Caption` | — | **reużywamy** (sekcja 3.6) |

Najważniejsze obserwacje:

1. **Core jest czysto oddzielony od GUI.** Komunikacja przez `core/Interface.h` (callbacki typu
   `QueryUser`, `PromptUser`, `ShowExtendedException`, `ProcessGUI`) i zdarzenia `TTerminal`.
   Bezpośredniego WinAPI w core jest mało i jest skupione w kilku plikach (`Terminal.cpp` —
   lokalne operacje na plikach, `Common.cpp`, `HierarchicalStorage.cpp` — rejestr,
   `SecureShell.cpp` — zdarzenia socketów, `PuttyIntf.cpp`, `FileBuffer.cpp`).
2. **Główną barierą jest dialekt i RTL, nie WinAPI.** W core: 613× `__property`, 45× typy
   `__closure` (+ przypisania metod jako zdarzeń), 181× `try/__finally`, 5277× `__fastcall`,
   6249× `UnicodeString`, ~700× `TStrings/TStringList`, 266× `TDateTime`, ~700× formatowanie
   w stylu Delphi (`FORMAT`/`FMTLOAD` → `Format(..., ARRAYOFCONST(...))`). Żaden kompilator
   poza C++Builderem tego nie skompiluje bez warstwy zgodności.
3. **Ścieżki lokalne są „windowsowe”.** 343 wywołania `IncludeTrailingBackslash`/`ExtractFilePath`
   itp. i 47 literałów `\\` w core. Ścieżki zdalne już używają `/` (`UnixIncludeTrailingBackslash`).
4. Konfiguracja domyślnie w rejestrze, ale WinSCP natywnie obsługuje też plik INI
   (`TIniFileStorage`) — na Linuksie używamy tylko INI, w formacie zgodnym z `WinSCP.ini`
   (import sesji z Windows przez skopiowanie pliku).

## 3. Decyzje architektoniczne

### 3.1 Port, nie przepisywanie od zera

Core + PuTTY + neon + libs3 + silnik FileZilla to ~180k linii dojrzałego, przetestowanego
w boju kodu z obsługą setek dziwactw serwerów. Przepisanie (np. w Rust/Qt z WinSCP jako
specyfikacją) to utrata tej wiedzy i lata pracy. Wine odpada — nie spełnia wymagań co do
Waylanda i D&D. **Przenosimy cały kod nie-wizualny, GUI piszemy od nowa w Qt.**

### 3.2 Dialekt C++Builder: shim RTL + powtarzalna konwersja źródeł

- **Kompilator: Clang** (jest 23.1) z `-fdeclspec`. Clang obsługuje
  `__declspec(property(get=…, put=…))`, także indeksowane (`T Items[]`), więc **miejsca
  użycia właściwości (tysiące) zostają bez zmian** — konwertujemy tylko deklaracje.
  GCC tego nie potrafi, więc projekt jest „Clang-only” (świadomie).
- **`source/rtl/`** — nowa biblioteka z podzbiorem RTL/VCL używanym przez core, w standardowym
  C++20: `UnicodeString` (indeksowanie od 1, API Delphi), `AnsiString`/`RawByteString`/`UTF8String`
  (strony kodowe przez iconv), `TObject`/`TPersistent`, `TList`/`TObjectList`, `TStrings`/
  `TStringList` (z `CommaText`, `Names/Values`, sortowaniem), strumienie (`THandleStream` na fd),
  `TDateTime` + `DateUtils`/`FormatDateTime`, `Format` w semantyce Delphi (indeksy `%0:s`,
  `*`, szerokości), hierarchia `Exception`, `TMemIniFile`, `TCriticalSection` (rekurencyjny),
  zdarzenia (eventfd/condvar), `TSearchRec`/`FindFirst`, `TPath`/`TFile` (std::filesystem),
  Base64, mały `IXMLNode` na expat. Wzorzec: Far-NetBox (port core WinSCP do MSVC) — jako
  inspiracja, nie kopia (jest wciąż Windows-only).
- **`UnicodeString` na `wchar_t`** (na Linuksie UTF-32). Dzięki temu literały `L"…"` i API
  `wchar_t` w core działają bez zmian. UTF-8 tylko na granicach (system plików, sieć).
  Nazwy plików niebędące poprawnym UTF-8 mapujemy odwracalnie (jak `surrogateescape` w Pythonie).
- **`tools/bcb2clang/`** — skrypt (Python, tokenizer z dopasowaniem nawiasów) robiący
  deterministyczną konwersję:
  - `__property T X = { read = FX, write = SetX };` → `__declspec(property(...)) T X;`
    (+ wygenerowane akcesory dla pól),
  - `typedef void __fastcall (__closure *TEvt)(Args)` → `typedef TMethodPtr<void(Args)> TEvt;`
    (delegat obiekt+metoda, porównywalny jak Delphi `TMethod`),
  - `try { A } __finally { B }` → blok ze strażnikiem zakresu (B wykonuje się przy wyjściu,
    także przy wyjątku; strażnik nie zgubi wyjątku z B podczas odwijania),
  - przypisania metod jako zdarzeń (`OnX = Metoda;`) — naprawiane w pętli sterowanej
    diagnostykami Clanga (kompilator podaje dokładną lokalizację, skrypt aplikuje poprawkę).
  - `__fastcall`, `__published`, `PACKAGE`, `DYNAMIC` → puste makra; `__int64` → `long long`.
- **Workflow z upstreamem („vendor branch”):** gałąź `upstream` = czysty WinSCP; gałąź
  `upstream-converted` = wynik `bcb2clang` na `upstream`. Nowe wydanie WinSCP:
  aktualizujemy `upstream`, puszczamy konwersję, commit na `upstream-converted`, merge do
  `master`. Konflikty pojawią się tylko tam, gdzie ręcznie zmienialiśmy kod — nie w tysiącach
  mechanicznie skonwertowanych linii. Dlatego: ręczne zmiany w `core/` minimalne, cała nowa
  logika w nowych katalogach (`rtl/`, `linux/`, `qt/`), a stare katalogi Windows zostają
  nietknięte jako referencja (zero konfliktów przy merge'ach).

### 3.3 Warstwa platformowa (`source/linux/`)

- Lokalne operacje na plikach w `Terminal.cpp`/`FileBuffer.cpp` → POSIX (`openat`, `fstatat`,
  `utimensat`, `rename`, xattry opcjonalnie). Atrybuty Windows (`faHidden` itd.) mapowane
  (ukryty = kropka na początku nazwy). Separator lokalny `/` w shimie (`IncludeTrailingBackslash`
  działa jak `IncludeTrailingPathDelimiter`), 47 literałów `\\` audytujemy ręcznie.
- Lokalny system plików jest **wrażliwy na wielkość liter** — audyt porównań `SameText` na
  ścieżkach lokalnych (szczególnie synchronizacja i porównywanie katalogów).
- Sanityzacja lokalnych nazw (`\/:*?"<>|` → `%XX`) zmienia znaczenie: na Linuksie niedozwolone
  są tylko `/` i NUL, ale **ochrona przed path traversal musi zostać** (patrz ostatni commit
  upstreamu, issue 2477). Osobny przegląd bezpieczeństwa tego obszaru.
- Wątki/zdarzenia: `TSimpleThread` → `std::thread`, eventy → `eventfd`, `WaitForMultipleObjects`
  na socketach → `poll()`.
- PuTTY: dokładamy z upstreamu PuTTY 0.85 pliki `unix/` (sieć, `uxsel`, agent `ssh-agent`
  przez `SSH_AUTH_SOCK`, GSSAPI przez libgssapi_krb5, lokalny proxy przez fork/exec, noise).
  `SecureShell.cpp` przestawiamy z `WSAEventSelect` na zbiór fd z `uxsel` + `poll()`.
- FileZilla: `CAsyncSocketEx` + ukryte okno komunikatów → reaktor `poll()` per wątek
  z kolejką komunikatów (`PostMessage` → kolejka + `eventfd`). Mini-shim `CString`/`CTime`/`CFile`
  na bazie `rtl/`.
- Konfiguracja: `$XDG_CONFIG_HOME/openscp/openscp.ini` (format `WinSCP.ini`). Hasła jak w
  WinSCP (opcjonalne hasło główne); później opcjonalnie Secret Service (KWallet/libsecret)
  przez QtKeychain.
- Teksty zasobów (`TextsCore*.rc`, `TextsWin*.rc`, `TextsFileZilla.rc`) → generowane w czasie
  budowania tabele C++ dla `LoadStr(id)`.

### 3.4 GUI: Qt 6 Widgets

- **Dlaczego Qt 6 Widgets:** core jest w C++ (bezpośrednie linkowanie, bez FFI); WinSCP to gęsty,
  klasyczny interfejs (drzewa, listy z kolumnami, paski narzędzi, ~45 dialogów) — Widgets mapują
  się 1:1 na koncepcje VCL; Wayland jest platformą pierwszej klasy (skalowanie ułamkowe,
  D&D, schowek, xdg-activation, dekoracje serwerowe KWin); na Plasmie wygląd i dialogi plików
  są natywne. Odrzucone: GTK4 (C, gorsze dopasowanie do C++ core, słabsza integracja z KDE),
  QML (zły wybór dla gęstych list plików i masy dialogów), Lazarus/LCL (Pascal, nie C++).
- Opcjonalne integracje KF6 za flagą CMake (KNotifications, KIO `KFilePlacesModel` dla
  „dysków” w panelu lokalnym) — aplikacja musi też budować się i działać bez KDE.
- Model wątków jak w WinSCP: operacje `TTerminal` wywoływane synchronicznie z wątku GUI,
  `ProcessGUI()` → `QCoreApplication::processEvents()`; kolejka transferów w tle na własnych
  wątkach (`TTerminalQueue`) — bez zmian.
- Struktura: `source/qt/app` (aplikacja, odpowiednik `TerminalManager`, implementacja
  `Interface.h`), `models` (`RemoteDirModel` nad `TRemoteFileList`, model lokalny),
  `views` (panele, drzewa, kolejka), `dialogs` (jeden na `.dfm`).
- **Nazwy obiektów Qt = nazwy komponentów z `.dfm`** — dzięki temu tłumaczenia z
  `translations/*.ini` (`Copy.dfm` → `OkButton.Caption=…`) da się wczytać istniejącym
  formatem, a testy E2E mogą adresować kontrolki stabilnymi nazwami.
- Ikony: motyw ikon freedesktop (Breeze itd.) z fallbackiem na glify WinSCP wyciągnięte
  z `Glyphs*.dfm` (PNG w kilku skalach).

### 3.5 CLI

Jeden binarny `openscp-cli` (odpowiednik `winscp.com`), oparty na porcie `ConsoleRunner.cpp`
i `core/Script.cpp`: ten sam język skryptów, przełączniki, kody wyjścia, log XML. To pierwszy
w pełni działający produkt i główne narzędzie testów protokołów.

### 3.6 Nazewnictwo

Produkt/binaria: `openscp`, `openscp-cli` (nazwa „WinSCP” zostaje tylko w historii i
atrybucji licencyjnej).

## 4. Integracje Windows → Linux/Wayland

| Windows (WinSCP) | Linux (OpenSCP) |
|---|---|
| Rejestr | plik INI w `$XDG_CONFIG_HOME/openscp/` |
| Pageant | `ssh-agent` (`SSH_AUTH_SOCK`), klucze PuTTY i OpenSSH jak dotąd |
| PuTTYgen (GUI) | `puttygen` (Unix PuTTY) / `ssh-keygen`; konwersja kluczy w core zostaje |
| „Otwórz w PuTTY” | emulator terminala (`xdg-terminal-exec`/konfigurowalny) + `ssh` lub `putty` |
| SSPI/Kerberos | GSSAPI (krb5) |
| Magazyn certyfikatów Windows | systemowy bundle CA OpenSSL |
| `ShellExecute`, „Otwórz za pomocą” | `QDesktopServices`/portal `OpenURI`, dialog „Open with” (KDE/portal) |
| Obserwacja pliku edytowanego zewnętrznie | `QFileSystemWatcher` (inotify) |
| Keep remote directory up to date | inotify (rekurencyjnie) |
| DragExt (D&D do Explorera) | patrz sekcja 5 |
| Kopiuj/wklej pliki z Explorerem | schowek `text/uri-list` + `x-kde-cutselection` / `x-special/gnome-copied-files` |
| Pasek zadań: postęp, jump listy | `com.canonical.Unity.LauncherEntry` (Plasma, Dash to Dock), akcje w `.desktop` |
| Ikona w zasobniku | `QSystemTrayIcon` (StatusNotifierItem) |
| Powiadomienia po transferze | `org.freedesktop.Notifications` / KNotification |
| Pojedyncza instancja, przekazywanie sesji | D-Bus (`org.openscp.OpenSCP`) + token xdg-activation |
| Rejestracja `sftp://`, `scp://`… | `x-scheme-handler/*` w pliku `.desktop` |
| „Wyślij do” w Explorerze | Dolphin service menu (+ akcja Nautilusa opcjonalnie) |
| Panel „dysków” (DriveView) | miejsca: `$HOME`, punkty montowania (`QStorageInfo`), zakładki/`KFilePlacesModel` |
| Import sesji (PuTTY z rejestru, FileZilla, OpenSSH) | `~/.putty/sessions`, `~/.config/filezilla/sitemanager.xml`, `~/.ssh/config`, `known_hosts`, `WinSCP.ini` |
| Instalator, aktualizacje, „Uruchom jako administrator” | pakiety (PKGBUILD/Flatpak/AppImage); bez auto-aktualizacji i elewacji |
| Zapamiętywanie pozycji okien | tylko rozmiar/maksymalizacja (Wayland nie pozwala klientom ustawiać pozycji) |
| Uprawnienia lokalnych plików (brak na Windows) | **nowość**: opcja zachowania lokalnego `mode` przy uploadzie |

## 5. Wayland: drag & drop i schowek

- **Do aplikacji** (z Dolphina/Nautilusa): `text/uri-list` → upload przez standardowy dialog
  kopiowania/kolejkę. Obsługa także danych bez pliku (np. obraz z przeglądarki) jako zapis
  do pliku tymczasowego.
- **Z aplikacji do menedżera plików** — problem znany z Windows (WinSCP rozwiązuje go
  rozszerzeniem powłoki albo katalogiem tymczasowym). Na Waylandzie nie ma XDS
  (`XdndDirectSave` jest tylko w X11). Dwa tryby, analogicznie do WinSCP:
  1. **KDE**: protokół, którego używa Ark — typy MIME `application/x-kde-ark-dndextract-service`
     i `…-path`; Dolphin po upuszczeniu woła po D-Bus metodę z katalogiem docelowym →
     pobieramy prosto do celu przez kolejkę, bez blokowania. Do zweryfikowania na Plasma 6.8.
  2. **Uniwersalny**: leniwe `QMimeData::retrieveData()` — dopiero gdy cel zażąda danych,
     pobieramy do katalogu tymczasowego (z oknem postępu) i oddajemy `file://` URI.
- D&D wewnątrz aplikacji (panel ↔ panel, na drzewo katalogów) — standardowo w Qt.
- Testy D&D tylko z prawdziwym wejściem z kompozytora (sekcja 6.4), bo na Waylandzie
  przeciąganie wymaga serialu prawdziwego zdarzenia wskaźnika.

## 6. Testowanie i praca autonomiczna

### 6.1 Zasady bezpieczeństwa (twarde)

- GUI uruchamiam **wyłącznie** w osobnym, bezgłowym KWinie — nigdy na Twoim `WAYLAND_DISPLAY`.
- Każdy przebieg testów ma izolowane `HOME`, `XDG_CONFIG_HOME`, `XDG_RUNTIME_DIR` i własną
  sesję D-Bus (`dbus-run-session`). Nie dotykam `~/.config`, `~/.ssh`, agenta SSH.
- Lokalne serwery testowe tylko na `127.0.0.1`, porty wysokie, bez `sudo`. Root mam tylko na
  testowej VM (sekcja 6.3), która nie przechowuje niczego cennego.

### 6.2 Warstwy testów

| Warstwa | Narzędzia | Co sprawdza |
|---|---|---|
| L1 unit | GoogleTest 1.18 (jest), ASan+UBSan | semantyka shimu RTL 1:1 z Delphi (indeksy od 1, `Format`, daty, `TStringList`), czysta logika core (maski plików, parsowanie listingów, URL-e sesji, `CopyParam`) |
| L2 protokoły | `openscp-cli` + pytest + lokalne serwery | open/ls/get/put/rm/mv/mkdir/chmod, resume, synchronize, sumy kontrolne, znaczniki czasu, uprawnienia, klucze hosta, uwierzytelnianie — macierz po protokołach |
| L3 komponenty GUI | QTest, `QT_QPA_PLATFORM=offscreen` | logika dialogów i modeli, szybko i deterministycznie |
| L4 E2E GUI | bezgłowy KWin + harness | pełne scenariusze w prawdziwym kompozytorze, zrzuty ekranu oglądane przeze mnie, D&D ze Dolphinem |

Sanitizery są tu szczególnie ważne: typowe błędy portu (rozmiar `wchar_t`, czas życia
obiektów przy zmianie modelu wyjątków) wychodzą w ASan/UBSan od razu.

### 6.3 Serwery testowe (`tools/testenv/`)

- **SSH/SFTP/SCP — sprawdzone, działa:** nieuprzywilejowany `/usr/bin/sshd` na
  `127.0.0.1:22222`, własne klucze hosta, uwierzytelnianie kluczem, `sftp-server`, wiele połączeń
  (trzeba podawać absolutną ścieżkę configu). Ograniczenie: bez `root` nie ma uwierzytelniania
  hasłem/PAM.
- **Testowa VM (dostęp po SSH, root)** — ustalone 2026-10-24. Na niej wszystko, czego nie da
  się sensownie uruchomić lokalnie bez roota: OpenSSH z hasłami/keyboard-interactive/zmianą
  hasła/banerami/chrootem, vsftpd + proftpd + pure-ftpd (różne formaty listingów — sedno
  silnika FTP), FTPS explicit/implicit, Apache `mod_dav` i nginx (WebDAV), MinIO (S3),
  w razie potrzeby kontenery. Konfiguracja serwerów wyłącznie przez idempotentny skrypt
  w repo (`tools/testenv/vm/`), żeby VM dało się odtworzyć od zera; testy L2 dostają
  adres VM ze zmiennej środowiskowej i pomijają się, gdy VM jest niedostępna.
  Dodatkowa wartość: prawdziwa sieć (opóźnienia, duże pliki, zrywanie połączeń).
- Testy L1/L3/L4 i szybka iteracja SFTP zostają lokalnie (lokalny sshd jak wyżej).

### 6.4 Harness GUI na Waylandzie (`tools/wlharness/`) — sprawdzone, działa

Zweryfikowałem dziś: `kwin_wayland --virtual` w `dbus-run-session` z izolowanym środowiskiem
startuje bez dostępu do Twojej sesji, aplikacje Qt się w nim uruchamiają, a `spectacle -b -n -f`
robi zrzut ekranu wirtualnego wyjścia (KWin rysuje dekoracje serwerowe — widać realne
zachowanie Twojego WM). Uwaga techniczna: ścieżka gniazda musi mieścić się w 108 bajtach,
więc `XDG_RUNTIME_DIR` w krótkiej ścieżce.

Elementy harnessu:

1. `wlh-session` — start/stop bezgłowego KWina (rozdzielczość, skala np. 1.25 do testów HiDPI).
2. `wlh-shot` — zrzut ekranu (spectacle); oglądam PNG i porównuję z oczekiwaniami.
3. `wlh-input` — mały klient w C protokołu `org_kde_kwin_fake_input` (XML jest w
   `plasma-wayland-protocols`, `wayland-scanner` jest) — prawdziwe zdarzenia myszy/klawiatury
   od kompozytora, potrzebne do D&D i schowka między aplikacjami. Autoryzacja przez plik
   `.desktop` z `X-KDE-Wayland-Interfaces` w izolowanym `XDG_DATA_HOME`.
4. `wlh-kwin` — skrypty KWin przez D-Bus: deterministyczne rozmieszczenie okien (np. OpenSCP
   po lewej, Dolphin po prawej) i odczyt ich geometrii, bo na Waylandzie klient nie zna
   swojej pozycji na ekranie.
5. **Hook automatyzacji w aplikacji** (tylko w buildach testowych, flaga CMake): gniazdo
   Unix z JSON-owym API — drzewo widżetów, kliknięcie/wywołanie akcji po `objectName`,
   wpisanie tekstu, odczyt stanu (np. kolejki), „czekaj aż bezczynna”, zrzut pojedynczego
   okna. Dodatkowo AT-SPI (jest `gi.repository.Atspi`) jako niezależna kontrola dostępności.

Przykładowy test D&D: harness startuje KWin + OpenSCP + Dolphin (izolowany HOME, lokalny
sshd), rozmieszcza okna, `wlh-input` przeciąga plik z panelu zdalnego do Dolphina,
test sprawdza, że plik pojawił się w katalogu docelowym z poprawną zawartością.

### 6.5 Jak pracuję autonomicznie

- Każdy kamień milowy ma **mierzalne kryteria wyjścia** (sekcja 7) i własny zestaw testów;
  kamień jest „zrobiony”, gdy testy przechodzą, a nie gdy kod się kompiluje.
- Pętla: mała zmiana → build (Ninja, ccache) → testy L1/L2 → przy GUI: E2E + zrzut ekranu
  obejrzany przeze mnie → commit z opisem. Małe, logiczne commity bezpośrednio na `master`
  (ustalone 2026-10-24); `master` ma zawsze się budować i przechodzić testy.
- `CLAUDE.md` w repo z poleceniami budowania/testów i zasadami z 6.1, żeby każda kolejna
  sesja startowała z tym samym kontekstem; `docs/port/` na decyzje i dziennik postępu.
- Rzeczy wymagające Twojej decyzji lub `sudo` zbieram i pytam o nie w jednym miejscu,
  zamiast blokować pracę.

## 7. Kamienie milowe

| # | Zakres | Kryterium wyjścia |
|---|---|---|
| M0 | Infrastruktura: CMake+Ninja (Clang), `CLAUDE.md`, `tools/testenv`, `tools/wlharness`, szkielet testów, vendor branch | `cmake --build` przechodzi; harness uruchamia KWin + testową aplikację Qt i robi zrzut; sshd testowy startuje ze skryptu |
| M1 | Shim RTL (`source/rtl`) + testy | testy L1 shimu zielone pod ASan/UBSan; nagłówki core się parsują |
| M2 | `bcb2clang` + kompilacja core (UI-callbacki jako stuby), generator tekstów `.rc` | `libopenscp-core` linkuje się; testy L1 logiki core zielone |
| M3 | PuTTY `unix/`, `SecureShell` na `poll()`, `openscp-cli` (port `ConsoleRunner`); skrypt provisioningu testowej VM | testy L2 dla SFTP i SCP przechodzą lokalnie i na VM (w tym hasła, resume, synchronize, klucze hosta, agent) |
| M4 | FTP/FTPS (port FileZilla), WebDAV (neon), S3 (libs3) | ta sama macierz L2 zielona dla każdego protokołu na serwerach z VM |
| M5 | Szkielet GUI: aplikacja, site manager (Login), okno Commander z panelami, nawigacja, podstawowe operacje, postęp, monity uwierzytelniania | E2E: połączenie z site managera, przeglądanie, upload/download; zrzuty OK |
| M6 | Kolejka, dialog kopiowania z opcjami, właściwości/chmod, zmiana nazwy, szukanie plików, synchronizacja + checklist, keep-up-to-date, zakładki/profile, polecenia własne, konsola, edytor + edycja zewnętrzna | E2E dla każdej funkcji |
| M7 | Integracja z pulpitem: D&D (oba tryby), schowek plików, URL-handlery, pojedyncza instancja, powiadomienia, tray, postęp na pasku zadań, `.desktop`, service menu Dolphina, terminal | E2E D&D i schowek ze Dolphinem w bezgłowym KWinie |
| M8 | Preferencje, interfejs Explorer, karty/workspace'y, import sesji, tłumaczenia, pakiety (PKGBUILD, potem Flatpak/AppImage) | aplikacja kompletna funkcjonalnie wg listy z sekcji 1 |

Kolejność daje działający, testowalny produkt (CLI) już po M3 — dalsze kroki budują GUI
na sprawdzonym rdzeniu.

## 8. Ryzyka

| Ryzyko | Mitygacja |
|---|---|
| Subtelne różnice semantyki shimu vs Delphi RTL (indeksy, `Format`, daty, sortowanie) | gęste testy L1 oparte na dokumentacji Delphi; sanitizery |
| Kod zakładający `sizeof(wchar_t) == 2` (UTF-16) | audyt (w core tylko 2 jawne `sizeof(wchar_t)`), testy na nazwach z emoji/znakami spoza BMP |
| Strefy czasowe/DST (core używa `TIME_ZONE_INFORMATION`) | reimplementacja na `localtime_r`/`timegm`, testy w kilku `TZ` |
| Lokalne ścieżki: wielkość liter, `\` jako legalny znak, path traversal | audyt + testy bezpieczeństwa (nazwy z `..`, `\`, `/` od serwera) |
| Port silnika FTP (asynchroniczne sockety oparte na komunikatach okien) | reaktor `poll()` z tą samą semantyką zdarzeń; testy na kilku serwerach FTP |
| Reentrancja `processEvents()` w modelu synchronicznym | ten sam model co w VCL (`ProcessMessages`), więc te same ograniczenia; testy E2E anulowania operacji |
| Konflikty przy merge'ach upstreamu | vendor branch + minimalne ręczne zmiany w `core/` |
| D&D na zewnątrz zależne od menedżera plików | dwa tryby (KDE D-Bus + leniwe pobieranie), testy z Dolphinem |

## 9. Otwarte decyzje

Rozstrzygnięte 2026-10-24: serwery testowe na testowej VM po SSH (potrzebna od M3);
commity bezpośrednio na `master`.

1. **Upstream:** potwierdzenie, że chcemy wciągać przyszłe wydania WinSCP (vendor branch).
   Jeśli nie — konwersję można zrobić jednorazowo i swobodniej refaktorować core.
2. **.NET assembly i Flatpak** — poza zakresem pierwszej wersji?
