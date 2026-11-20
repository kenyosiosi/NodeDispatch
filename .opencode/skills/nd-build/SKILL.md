---
name: NodeDispatch Build
description: Configura y compila NodeDispatch con CMake dentro de WSL, sin sudo, y reporta solo los warnings del código propio
---

# Build de NodeDispatch

## Reglas

- Todo se ejecuta en WSL: `wsl.exe -d Ubuntu -- bash -lc "cd ~/projects/NodeDispatch && ..."`.
- Nunca compiles desde Windows con MSVC: el proyecto se valida con GCC en WSL.
- Los warnings de `_deps/` (DeSiGNAR, doctest) **no cuentan**. Los de `include/`
  y `src/` sí: son deuda del proyecto y hay que reportarlos.

## Pasos

1. Localiza CMake en este orden y usa el primero que exista:

   ```bash
   for c in "$HOME/.local/bin/cmake" /tmp/cmake-3.30.5-linux-x86_64/bin/cmake; do
     [ -x "$c" ] && echo "CMAKE=$c" && break
   done
   ```

   Si no hay ninguno: `python3 -m pip install --user cmake`.

2. Configura y compila:

   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
   cmake --build build --parallel 2>&1 | tee /tmp/nd-build.log
   ```

3. Filtra los warnings propios:

   ```bash
   grep -E "warning:" /tmp/nd-build.log | grep -v "_deps" || echo "sin warnings propios"
   ```

4. Reporta: resultado del build, ruta del compilador, warnings propios con
   `archivo:línea`, y el primer error si falló.

## No hagas

- No borres `build/` sin avisar: se pierden los `_deps` y la reconfiguración
  tarda ~30 s.
- No añadas `-Werror` global: solo en `nodedispatch_warnings`. Las cabeceras de
  DeSiGNAR se marcan como sistema, así que hoy no romperían la build; si esa
  propiedad se pierde, cualquier warning ajeno pasa a ser un error.
