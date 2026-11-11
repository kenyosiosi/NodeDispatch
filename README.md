# NodeDispatch 📦🚛

> **Core Engine de Logística y Distribución de Paquetes**
>
> *Proyecto para la asignatura Programación 3 — Universidad de Los Andes.*

## 📌 Estado del Proyecto

**Versión:** `1.1` — fase de diseño técnico y preparación de la implementación.

El repositorio ya cuenta con el esqueleto de carpetas (`docs/`, `include/`, `src/`,
`tests/`, `assets/`, `third_party/`), el `.gitignore` y la especificación técnica
corregida en [`docs/SDD.md`](docs/SDD.md). El código ejecutable se agregará en las
próximas semanas siguiendo el plan del SDD.

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
├── include/                # Cabeceras (.hpp) [Próximamente]
├── src/                    # Implementación C++ (.cpp) [Próximamente]
├── tests/                  # Pruebas [Próximamente]
├── third_party/            # Dependencias (DeSiGNAR vía submodule/FetchContent)
├── .gitignore
├── CMakeLists.txt          # [Próximamente]
└── README.md
```

## 🧰 Requisitos

* Compilador con soporte **C++17** (GCC 9+, Clang 10+ o MSVC 2019+).
* **CMake 3.20** o superior.
* **DeSiGNAR v4.0.1** (<https://github.com/R3mmurd/DeSiGNAR>), integrado como
  submódulo o mediante `FetchContent`.

## 🔨 Build (planificado)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

La integración de DeSiGNAR se hace en el `CMakeLists.txt` con
`FetchContent` fijando el tag `v4.0.1` y desactivando sus ejemplos y pruebas:

```cmake
set(DESIGNAR_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(DESIGNAR_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
```

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
