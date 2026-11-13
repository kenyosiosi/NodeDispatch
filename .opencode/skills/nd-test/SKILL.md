---
name: NodeDispatch Test
description: Ejecuta CTest y los sanitizers de NodeDispatch, resume los fallos con su TEST_CASE y nunca reporta verde sobre un build viejo
---

# Pruebas de NodeDispatch

## Regla de oro

Nunca reportes "pasa" si no recompilaste después del último cambio. Primero
`cmake --build build`, después `ctest`.

## Pasos

1. Suite completa:

   ```bash
   ctest --test-dir build --output-on-failure
   ```

2. Solo doctest (excluye `smoke_test` y `hierarchy_test`):

   ```bash
   ctest --test-dir build -R "^unit\." --output-on-failure
   ```

3. Un caso concreto (el nombre es el del `TEST_CASE`, sin `TEST_SUITE`):

   ```bash
   ctest --test-dir build -N            # lista lo que descubre CTest
   ctest --test-dir build -R "Package nace" --output-on-failure
   ```

4. Sanitizers:

   ```bash
   cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
     -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
   cmake --build build-asan -j"$(nproc)"
   ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
   UBSAN_OPTIONS=halt_on_error=1 \
     ctest --test-dir build-asan --output-on-failure
   ```

5. Reporta: `N/N` pasando, y si falla, el bloque `FAILED` completo con
   `archivo:línea` y una hipótesis de causa.

## Trampas conocidas

- `doctest_discover_tests` usa **solo** el nombre del `TEST_CASE`. Dos casos con
  el mismo nombre en archivos distintos colisionan en CTest.
- `tests/hierarchy_test.cpp` es un `main` manual con `CHECK`, no doctest. Si un
  caso "no aparece" en `ctest -N`, es por eso.
- Un test que pasa en `build/` pero no en `build-asan/` suele ser un uso después
  de liberar o una lectura fuera de rango.
