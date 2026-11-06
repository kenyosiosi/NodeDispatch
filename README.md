# NodeDispatch 📦🚛

> **Core Engine de Logística y Distribución de Paquetes**
>
> *Proyecto para la asignatura Programación 3 — Universidad de Los Andes.*

## 📌 Estado del Proyecto

**Versión Actual:** `1.0-draft`

El proyecto se encuentra en la **fase inicial de diseño arquitectónico y especificación técnica**. Actualmente no incluye código ejecutable; las bases estructurales y contratos del sistema están definidos en el [Documento de Diseño de Software (SDD)](docs/SDD.md).

## 👥 Autores

* **Keny Rondón** — *Desarrollador / Coautor*

* **Rafael Castillo** — *Desarrollador / Coautor*

## 🎯 Visión General

**NodeDispatch** es un motor logístico de alto rendimiento en memoria diseñado en C++17/C++20. Su objetivo principal es gestionar eficientemente la jerarquía geográfica de agencias, inventarios locales de encomiendas, rastreo global en tiempo real y priorización de cargas para transporte.

El proyecto está construido obligatoriamente sobre la biblioteca de estructuras de datos **DeSiGNAR**.

## 🛠️ Ecosistema Técnico y Estructuras (Propuestas)

| Módulo del Sistema | Responsabilidad | Estructura / Abstracción DeSiGNAR | 
| ----- | ----- | ----- | 
| **Geographic Hierarchy** | Modelado jerárquico de agencias y centros | `Designar::Tree` (Árbol N-ario) | 
| **Inventory Index** | Control de stock y bodega por agencia | `Designar::RedBlackTree` | 
| **Tracking Engine** | Consulta global de paquetes en $O(1)$ | `Designar::HashMap` | 
| **Priority Dispatch Queue** | Cola priorizada para despacho de unidades | `Designar::Heap` | 

## 📂 Estructura del Repositorio (Proyectada)

```
NodeDispatch/
├── docs/
│   └── SDD.md              # Software Design Document (Especificación v1.0)
├── include/                # Archivos de cabecera (.hpp) [Próximamente]
├── src/                    # Implementación C++ (.cpp) [Próximamente]
├── tests/                  # Pruebas unitarias [Próximamente]
├── .gitignore              # Configuración de exclusiones Git
└── README.md               # Este archivo

```

## 🔄 Flujo de Trabajo Git (GitFlow)

El desarrollo seguirá estrictamente la siguiente jerarquía de ramas:

* **`main`**: Rama estable de producción / entregas oficiales.

* **`development`**: Rama principal de integración continua.

* **`feature/*`**: Ramas de desarrollo para módulos específicos (`feature/inventory`, `feature/tracking`, etc.).

## 📄 Documentación Completa

Para consultar la arquitectura técnica detallada, modelos de datos en C++, análisis de concurrencia (`std::shared_mutex`), persistencia Write-Ahead Log (WAL) y decisiones de diseño, revisa el [SDD v1.0 en `docs/SDD.md`](docs/SDD.md).
