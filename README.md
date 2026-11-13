# NodeDispatch 📦🚛

> **Core Engine de Logística y Distribución de Paquetes**
>
> *Proyecto para la asignatura Programación 3 — Universidad de Los Andes.*

## 📌 Estado del Proyecto

**Versión:** `0.1.0` — Semana 1 en progreso (build + tipos base + smoke test).

El repositorio ya cuenta con la especificación técnica corregida en
[`docs/SDD.md`](docs/SDD.md), el esqueleto de carpetas, el `.gitignore`, el
`CMakeLists.txt` con DeSiGNAR v4.0.1 vía `FetchContent` y el smoke test S1.0 que
valida las cinco estructuras (`MTreeNode`, `TreeMap`/`RbTree`, `HashMap`,
`DynHeap` y `DynArray`).

## 👥 Autores

* **Keny Rondón** — *Desarrollador / Coautor*
* **Rafael Castillo** — *Desarrollador / Coautor*

## 🎯 Visión General

**NodeDispatch** es un motor logístico en memoria escrito en **C++17** y construido
obligatoriamente sobre la biblioteca de estructuras de datos **DeSiGNAR v4.0.1**.
Gestiona la jerarquía geográfica de agencias, el inventario de encomiendas, el
rastreo global y la priorización de cargas para la flota de reparto.

Incluye un modelo financiero (Prepago vs Cobro Destino) y de entrega (Retiro en
Agencia vs Última Milla). El núcleo es independiente de la presentación: la demo se
realiza por consola y, opcionalmente, con una capa gráfica simple basada en SFML.

## 🛠️ Mapeo con DeSiGNAR

| Módulo | Responsabilidad | Estructura DeSiGNAR |
| :--- | :--- | :--- |
| **Geographic Hierarchy** | Jerarquía N-aria de agencias | `MTreeNode<Agency>` + índice `HashMap` |
| **Inventory Index** | Stock ordenado por agencia | `TreeMap` con backend `RbTree` |
| **Tracking Engine** | Rastreo global de paquetes | `HashMap<std::string, Package>` |
| **Priority Dispatch Queue** | Cola de despacho por prioridad | `DynHeap` |
| **Client Index** | Clientes normalizados | `HashMap<std::string, Client>` |
| **Audit Log** | Historial de eventos | `DLList<PackageEvent>` |
| **Reports** | Resultados de consultas | `DynArray<T>` |

> Nota: en DeSiGNAR v4.0.1, `Designar::Tree` es un **árbol binario de búsqueda no
> balanceado**, no un árbol N-ario. La jerarquía se modela con
> `Designar::MTreeNode`. Los detalles están en el [SDD](docs/SDD.md).

## 📂 Estructura del Repositorio

```
NodeDispatch/
├── assets/                 # Recursos para la capa SFML (fuentes, iconos)
├── docs/
│   └── SDD.md              # Software Design Document (Especificación v1.1)
├── include/
│   └── nodedispatch/       # Cabeceras del núcleo
│       ├── types.hpp       # Enums, OperationResult y tipos de valor
│       ├── agency.hpp      # Agency
│       ├── client.hpp      # Client
│       └── package.hpp     # Package, DispatchEntry, PackageEvent
├── src/                    # Implementación C++ (.cpp) [Próximamente]
├── tests/
│   └── smoke_test.cpp      # S1.0: validación de las 5 estructuras DeSiGNAR
├── third_party/            # Dependencias adicionales (DeSiGNAR vía FetchContent)
├── .gitignore
├── CMakeLists.txt          # DeSiGNAR v4.0.1 + targets del núcleo
└── README.md
```

## 🧰 Requisitos

* Compilador con soporte **C++17** (GCC 9+, Clang 10+ o MSVC 2019+).
* **CMake 3.20** o superior.
* **DeSiGNAR v4.0.1** (<https://github.com/R3mmurd/DeSiGNAR>), integrado como
  submódulo o mediante `FetchContent`.

## 🔨 Build

Requisitos: **CMake 3.20+** y un compilador **C++17**. `FetchContent` descarga y
compila DeSiGNAR v4.0.1 automáticamente en `build/_deps/`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
# o directamente:
./build/smoke_test
```

El **smoke test S1.0** (`tests/smoke_test.cpp`) valida las cinco estructuras del
núcleo con la API real de DeSiGNAR. La integración fija el tag `v4.0.1` y
desactiva los ejemplos y pruebas de la biblioteca:

```cmake
set(DESIGNAR_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(DESIGNAR_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
```

### 🧪 Pruebas

La suite usa **doctest** (framework de cabecera única, fijado por `FetchContent`
en `v2.5.3`, igual que DeSiGNAR). El ejecutable `nodedispatch_tests` agrupa los
archivos listados en el target `nodedispatch_tests` y `ctest` registra un caso
por cada `TEST_CASE` mediante `doctest_discover_tests`, con el prefijo `unit.`:

```bash
ctest --test-dir build --output-on-failure                 # suite completa
ctest --test-dir build -R "^unit\." --output-on-failure    # solo doctest
```

El smoke test S1.0 (`smoke_test`) se mantiene como caso independiente de CTest.

## 🔄 Flujo de Trabajo Git (GitFlow)

* **`main`**: rama estable de producción / entregas oficiales.
* **`development`**: rama principal de integración continua.
* **`feature/*`**: ramas de desarrollo modular (`feature/hierarchy`,
  `feature/inventory`, `feature/tracking`, `feature/dispatch`, etc.).

Las características se integran a `development` mediante *pull requests* y, una vez
estables, `development` se fusiona en `main`.

## 📄 Documentación Completa

La arquitectura técnica detallada, el modelo de datos en C++, la máquina de estados,
la API de la fachada, el plan de pruebas, la trazabilidad con la materia y el plan
de 4 semanas están en el
[SDD v1.1 en `docs/SDD.md`](docs/SDD.md).
