# NodeDispatch — reglas del proyecto

Motor de logística en memoria (C++17) para Programación 3, Universidad de Los
Andes. El diseño de dominio vive en `docs/SDD.md`: ese documento es la fuente de
verdad y estas reglas lo resumen para trabajar rápido.

---

## 1. Entorno

- El repositorio vive en **WSL** en `~/projects/NodeDispatch`. Desde Windows se
  accede por `\\wsl.localhost\Ubuntu\home\keny\projects\NodeDispatch`.
- Ejecuta build, tests y git **siempre dentro de WSL**:

  ```bash
  wsl.exe -d Ubuntu -- bash -lc "cd ~/projects/NodeDispatch && <comando>"
  ```

- `cmake` **no está en el PATH** de WSL. Usa `~/.local/bin/cmake` o el portable
  `/tmp/cmake-3.30.5-linux-x86_64/bin/cmake` (el portable se borra al reiniciar
  WSL; si falta, reinstálalo o usa `python3 -m pip install --user cmake`).
- `gh` solo existe en Windows: `C:\Program Files\GitHub CLI\gh.exe`.
- **No hay `sudo`.** Las herramientas de usuario se instalan con
  `python3 -m pip install --user cmake clang-format gcovr`.

## 2. Comandos

| Tarea | Comando |
| :--- | :--- |
| Configurar | `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug` |
| Compilar | `cmake --build build -j"$(nproc)"` |
| Probar todo | `ctest --test-dir build --output-on-failure` |
| Solo doctest | `ctest --test-dir build -R "^unit\." --output-on-failure` |
| Un caso | `ctest --test-dir build -R "<nombre del TEST_CASE>" --output-on-failure` |
| Sanitizers | `cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"` |
| Formato | `clang-format --dry-run --Werror <archivos>` |

Para ver qué descubre CTest: `ctest --test-dir build -N`.

## 3. Reglas duras del dominio

Estas no se negocian; si un cambio las rompe, el cambio está mal.

- **C++17**, sin contenedores STL en el núcleo. Solo `DeSiGNAR`.
  Prohibido en `include/` y `src/`: `std::vector`, `std::map`, `std::unordered_map`,
  `std::set`, `std::list`, `std::deque`, `std::queue`, `std::stack`.
  Permitido: `std::string`, `std::optional`, `std::int64_t`, `std::pair`,
  `std::quoted`, `<algorithm>` sobre punteros.
- **Dinero en centavos**: `std::int64_t`. Nunca `double` ni `float` para importes.
  (`weightKg` es la única magnitud decimal permitida.)
- La **API pública no lanza excepciones de negocio**: devuelve `bool`,
  `std::optional` u `OperationResult`.
- **Nunca** se devuelven punteros ni referencias a memoria interna. Siempre copias.
- **Regla COD**: `CASH_ON_DELIVERY` no puede pasar a `DELIVERED` mientras
  `paymentStatus != PAID`.
- `ServiceLevel` vale 1/2/3 y es invariante de `DispatchCmp`. No reordenar ni
  reasignar valores.
- `registerPackage` normaliza el pago: `PREPAID => PAID`,
  `CASH_ON_DELIVERY => PENDING`. Un `Package{}` por defecto es un estado neutro
  **previo** al registro, no un paquete válido.
- La jerarquía geográfica es **insert-only** en el MVP (`MTreeNode` no permite
  borrar un hijo intermedio).

## 4. Convenciones de código

- `enum class` para todo estado; nada de enteros mágicos.
- Cabeceras en `include/nodedispatch/`, implementación en `src/`, pruebas en `tests/`.
- `#pragma once`, namespace `nodedispatch`, includes ordenados y en minúsculas.
- Todo `TEST_CASE` de doctest debe tener **nombre único**: `doctest_discover_tests`
  registra solo el nombre, sin `TEST_SUITE`, y dos homónimos colisionan en CTest.
- No tocar `include/nodedispatch/types.hpp` sin actualizar `tests/types_test.cpp`.
- Comentarios en español, identificadores en inglés.

## 5. Flujo de trabajo (GitFlow)

- Ramas: `feature/<id>-<slug>` desde `development`; `fix/<id>-<slug>` para bugs.
- Todo entra por **PR a `development`** con **merge commit** (nunca squash).
- Un PR = una tarjeta del tablero. Título: `tipo(alcance): mensaje (Sx.y)`.
- Tipos de commit: `feat`, `fix`, `test`, `docs`, `build`, `refactor`, `chore`.
- `development -> main` solo en el cierre, con tag anotado (`v1.0`).
- Un PR abierto más de 48 h sin revisión es un bloqueo: avisa en la tarjeta.

## 6. Definición de terminado

Una tarea no está terminada hasta que:

- [ ] `cmake --build build` no emite warnings en `include/` ni en `src/`
      (los de `_deps/` son de terceros y se ignoran).
- [ ] `ctest` está verde y hay al menos un caso nuevo que falla sin el cambio.
- [ ] Si toca memoria: ASan/UBSan limpio.
- [ ] Si cambió una firma pública: `docs/SDD.md` actualizado en el mismo PR.
- [ ] La tarjeta del tablero enlaza el PR o el commit.

## 7. Errores frecuentes en este repo

- Compilar desde Windows con el toolchain de Visual Studio: **no**. Todo va por WSL.
- Usar `cmake` del PATH: no existe. Usa la ruta explícita.
- Crear un `main()` nuevo para probar algo: usa `TEST_CASE` y CTest.
- Devolver `Package*` desde el tracking "para no copiar": prohibido por contrato.
- Poner `-Werror` global en CMake: rompe por warnings de DeSiGNAR. Si se usa,
  solo en el target `nodedispatch_warnings`.
- Confiar en que `/tmp/cmake-*` sigue ahí tras reiniciar WSL: no sigue.
