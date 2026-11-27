# Cómo compilar y ejecutar NodeDispatch

Guía rápida para Linux/WSL, Windows y macOS. El proyecto es CMake + C++17; la
fuente de verdad del dominio es `docs/SDD.md`.

## Requisitos

- CMake >= 3.20 y compilador con C++17 (GCC, Clang, AppleClang o MSVC).
- Git y conexión: la primera configuración descarga por `FetchContent`:
  DeSiGNAR v4.0.1, doctest v2.5.3 y, solo si se activa, SFML 2.6.1.
- No se necesita `sudo` ni permisos especiales.

---

## 1. Linux / WSL (flujo canónico)

En la máquina de Keny `cmake` no siempre está en el `PATH`; se usa el portable.
En otras máquinas basta `cmake`.

```bash
cd ~/projects/NodeDispatch

# Elegir CMake: primero el portable, luego el del PATH
CMAKE="$(command -v cmake)"
for c in "$HOME/.local/bin/cmake" /tmp/cmake-3.30.5-linux-x86_64/bin/cmake; do
  [ -x "$c" ] && CMAKE="$c" && break
done

"$CMAKE" -S . -B build -DCMAKE_BUILD_TYPE=Debug
"$CMAKE" --build build --parallel 4        # 4 o 6: ver nota de recursos
"$CMAKE" --build build --parallel 6        # alternativa
ctest --test-dir build --output-on-failure
```

**Paralelismo:** con 28 cores y ~7.7 GiB de RAM, `--parallel 4` o `--parallel 6`
deja margen para las unidades de doctest. `--parallel 28` puede agotar memoria.
No borres `build/`: se pierden los `_deps` y la reconfiguración tarda ~30 s.

**Solo warnings propios** (los de `_deps/` son de terceros):

```bash
cmake --build build --parallel 4 2>&1 | tee /tmp/nd-build.log
grep -E "warning:" /tmp/nd-build.log | grep -v "_deps" || echo "sin warnings propios"
```

**Sanitizers:**

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake --build build-asan --parallel 4
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-asan --output-on-failure
```

---

## 2. Windows nativo

### Con MSVC (Visual Studio Build Tools)

Clona el repo en el filesystem de Windows, no en la ruta UNC `\\wsl.localhost`.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_CXX_FLAGS="/utf-8"
cmake --build build --config Debug --parallel 4
ctest --test-dir build -C Debug --output-on-failure
```

`/utf-8` evita el warning C4819 con los comentarios en español.

### Con MinGW-w64 (MSYS2 UCRT64)

```powershell
$env:PATH = 'C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B build -G Ninja `
  "-DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe" `
  "-DCMAKE_C_COMPILER=C:/msys64/ucrt64/bin/gcc.exe" `
  "-DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe" `
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

---

## 3. macOS

```bash
xcode-select --install          # si hace falta
brew install cmake              # si hace falta

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

---

## 4. Ejecutar la CLI

La CLI es la presentación obligatoria (SDD 6.7). Modo interactivo:

```bash
./build/nodedispatch_cli
help
quit
```

Modo guion (transcript reproducible, el que usa CTest):

```bash
./build/nodedispatch_cli --script scripts/cli_demo.txt
./build/nodedispatch_cli --script scripts/cli_edge_cases.txt
```

`cli_demo` recorre el flujo completo: clientes, jerarquía de agencias, paquete
prepago entregado, paquete COD bloqueado hasta pago, recargo a domicilio y
reportes. `cli_edge_cases` añade los rechazos del contrato, tokens inválidos,
transiciones ilegales y los stubs de persistencia (`save`/`load`, pendiente S4).

Órdenes principales:

```
agency add <id> <padre|-> <nombre>      agency tree <id>
client add <cedula> <nombre> <apellido>
package register <codigo> <agencia> <remitente> <destinatario> \
                 <baseCents> <PREPAID|COD> <HOME|AGENCY> [<nivel>]
package track|pay|status|cancel <codigo> ...
package dispatch <agencia>
report revenue | status <ESTADO> | agency <agencia>
save <ruta> / load <ruta>               # pendiente S4
```

Pruebas relacionadas:

```bash
ctest --test-dir build -R '^cli_' --output-on-failure
ctest --test-dir build -R '^unit\.' --output-on-failure
ctest --test-dir build -N             # lista todo lo que descubre CTest
```

---

## 5. Demo SFML (opcional)

Vive en `demo/`, fuera del núcleo. El CI la compila en Ubuntu (`build-gui`).

```bash
cmake -S . -B build-sfml -DCMAKE_BUILD_TYPE=Debug -DBUILD_SFML=ON
cmake --build build-sfml --target nodedispatch_gui --parallel 4
./build-sfml/nodedispatch_gui
```

Teclas: `1` Registrar, `2` Rastrear, `3` Despachar, `Tab` cicla, `ESC` sale.
La pantalla Despachar dibuja la jerarquía real. Los formularios están en estado
vacío (pendiente de conectar a la fachada).

Aviso: la fuente TTF se busca en rutas de Linux (`/usr/share/fonts/...`). En
Windows/macOS la ventana abre pero el texto no se dibuja; es una mejora pendiente,
no afecta al núcleo ni a la CLI.

---

## Problemas frecuentes

- **Warnings de `_deps/`:** se ignoran; solo cuentan `include/` y `src/`.
- **Falla la primera configuración:** normalmente es red/Git para `FetchContent`; reintenta.
- **CTest no ve un caso nuevo:** `doctest_discover_tests` usa solo el nombre del
  `TEST_CASE` (sin `TEST_SUITE`) y dos homónimos colisionan.
- **No añadas `-Werror` global:** solo existe en el target `nodedispatch_warnings`.
- **CMake no encontrado tras reiniciar WSL:** `/tmp/cmake-*` se borra; reinstala
  con `python3 -m pip install --user cmake` o usa `~/.local/bin/cmake`.
