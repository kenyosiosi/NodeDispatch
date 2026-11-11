# Software Design Document (SDD) — NodeDispatch

| Metadato | Detalle |
| :--- | :--- |
| **Proyecto** | **NodeDispatch** (Core Engine de Logística y Distribución) |
| **Documento** | Especificación Técnica — Versión 1.1 |
| **Autores** | Keny Rondón y Rafael Castillo |
| **Institución / Cátedra** | Universidad de Los Andes — Programación 3 |
| **Biblioteca Obligatoria** | DeSiGNAR v4.0.1 (Data Structures GeNeral librARy) |
| **Estándar de C++** | C++17 |
| **Fecha de actualización** | 11 de noviembre de 2026 |
| **Repositorio GitHub** | [NodeDispatch](https://github.com/kenyosiosi/NodeDispatch.git) (`main` ← `development` ← `feature/*`) |

> **Cambios respecto a la v1.0.** Se corrigió el mapeo con la API real de DeSiGNAR
> (`MTreeNode`, `RbTree`, `TreeMap`, `DynHeap`, `HashMap`), se agregó el modelo
> `Agency`, la máquina de estados, la API completa, el plan de pruebas, la
> trazabilidad con la materia y el plan de 4 semanas. Se reclasificaron WAL y
> concurrencia como mejoras opcionales y se incorporó una capa de presentación
> simple (CLI y, opcionalmente, SFML).

---

## 1. Visión General y Objetivos

### 1.1 Contexto y Problema

Las empresas medianas de logística y distribución de encomiendas enfrentan desafíos
algorítmicos complejos directamente en memoria: control de inventarios en bodegas
locales, búsquedas de rastreo instantáneas, gestión de prioridades de carga para
flota vehicular y soporte para modalidades financieras (Prepago vs Cobro Destino) y
métodos de entrega (Retiro en Agencia vs Envío a Domicilio / Última Milla).

NodeDispatch resuelve estos problemas con un motor en memoria apoyado
obligatoriamente en estructuras de datos de la biblioteca **DeSiGNAR**. El proyecto
es una aplicación de consola (con una capa gráfica simple opcional para la
presentación) y no depende de bases de datos externas.

### 1.2 Objetivos Principales

- **Topología jerárquica geográfica.** Modelar la red de agencias y centros de
  acopio como un **árbol N-ario** mediante `Designar::MTreeNode<Agency>`, con
  recorridos en profundidad y anchura.
- **Inventario de alta frecuencia.** Gestionar el stock por bodega con complejidad
  $O(\log n)$ mediante `Designar::TreeMap` con backend `Designar::RbTree`
  (árbol rojo-negro).
- **Rastreo global.** Proveer consultas de rastreo en $O(1)$ promedio mediante
  `Designar::HashMap`.
- **Despacho priorizado.** Ordenar la carga vehicular en cada plataforma con
  `Designar::DynHeap` (montículo binario), atendiendo nivel de servicio y
  antigüedad del paquete.
- **Modelo financiero y última milla.** Control de costos base, recargos por
  delivery y validación de pago condicional antes de la entrega (`DELIVERED`).
- **Uso demostrable del contenido de la materia.** Arrays, listas, árboles,
  montículos y tablas hash de DeSiGNAR aplicados a funcionalidades reales.

### 1.3 Fuera de Alcance (Non-Goals)

- Motores de bases de datos relacionales o NoSQL externos. El núcleo opera en RAM
  y, opcionalmente, se apoya en un archivo de *snapshot* local.
- Contenedores genéricos de la STL para las **estructuras de datos núcleo**
  (reemplazados obligatoriamente por DeSiGNAR). Sí se permite `std::string` para
  texto, `std::optional` para resultados y utilidades no contenedoras.
- Write-Ahead Log (WAL) obligatorio. La persistencia mínima es un *snapshot*; el
  WAL queda como mejora opcional.
- Interfaz gráfica compleja. El núcleo es independiente de la presentación; se
  puede añadir una capa SFML simple para la demostración comercial.
- Optimización de rutas vehiculares, autenticación de usuarios y multi-tenancy.

---

## 2. Requisitos

### 2.1 Requisitos Funcionales

| ID | Requisito | Estructura principal |
| :--- | :--- | :--- |
| **RF-01** | Registrar agencias y centros de acopio dentro de una jerarquía, indicando su agencia padre. | `MTreeNode<Agency>` |
| **RF-02** | Buscar una agencia por su ID y listar sus descendientes (subárbol). | `HashMap` + `MTreeNode` |
| **RF-03** | Registrar un paquete con emisor, receptor, agencia destino, modalidad y nivel de servicio. | `HashMap` |
| **RF-04** | Rastrear un paquete por su código y devolver una copia segura de su estado. | `HashMap` |
| **RF-05** | Actualizar el estado financiero de un paquete (`PENDING`, `PAID`, `FAILED`). | `HashMap` |
| **RF-06** | Cambiar el estado operativo del paquete respetando la máquina de estados. | `HashMap` |
| **RF-07** | Impedir la entrega (`DELIVERED`) de un paquete con Cobro Destino cuyo pago no esté confirmado. | Regla de negocio |
| **RF-08** | Encolar paquetes listos en la agencia y despachar el de mayor prioridad. | `DynHeap` |
| **RF-09** | Listar el inventario de una agencia ordenado por código de rastreo. | `TreeMap`/`RbTree` |
| **RF-10** | Registrar el historial de eventos de cada paquete. | `DLList` |
| **RF-11** | Generar reportes (paquetes por agencia/estado, ingresos, entregas a domicilio). | `DynArray` |
| **RF-12** | Guardar y recuperar el estado completo del sistema desde un archivo local. | Snapshot |

### 2.2 Requisitos No Funcionales

| ID | Requisito |
| :--- | :--- |
| **RNF-01** | Compilar con C++17 y la biblioteca DeSiGNAR v4.0.1. |
| **RNF-02** | Rastreo en $O(1)$ promedio con factor de carga $\alpha \le 0.75$. |
| **RNF-03** | Operaciones de inventario en $O(\log n)$ en el peor caso. |
| **RNF-04** | Despacho con inserción y extracción en $O(\log n)$. |
| **RNF-05** | Ninguna operación pública debe exponer punteros internos de DeSiGNAR. |
| **RNF-06** | El sistema debe procesar al menos 100 000 paquetes en memoria sin degradar el rastreo. |
| **RNF-07** | La lógica de negocio debe estar separada de la presentación. |
| **RNF-08** | El código debe ser portable a Linux (GCC) y compilable con CMake. |

### 2.3 Casos de Uso Principales

- **CU-01 Registrar agencia:** el operador crea una agencia indicando su padre; el
  sistema valida unicidad del ID y existencia del padre.
- **CU-02 Registrar paquete:** el operador ingresa los datos; el sistema valida,
  genera el código de rastreo, calcula el costo y lo indexa.
- **CU-03 Rastrear paquete:** el operador consulta por código y recibe el estado.
- **CU-04 Registrar pago:** el operador marca el pago como `PAID` o `FAILED`.
- **CU-05 Despachar:** el operador solicita el siguiente paquete de una agencia y
  el sistema entrega el de mayor prioridad.
- **CU-06 Actualizar estado:** el operador avanza el paquete por la máquina de
  estados; el sistema rechaza transiciones inválidas.
- **CU-07 Reportes:** el operador consulta listados y totales.
- **CU-08 Persistencia:** el sistema guarda y recupera el estado.

---

## 3. Arquitectura de Alto Nivel

### 3.1 Diagrama de Componentes

```
                        +-----------------------------------+
                        |       NodeDispatch (Facade)       |
                        |  API pública + reglas de negocio  |
                        +-----------------------------------+
                                          |
        +-------------------+-------------+-------------+-------------------+
        |                   |                           |                   |
+---------------+   +---------------+           +---------------+   +---------------+
|  Geographic   |   |   Inventory   |           |   Tracking    |   |   Priority    |
|   Hierarchy   |   |     Index     |           |    Engine     |   |   Dispatch    |
+---------------+   +---------------+           +---------------+   +---------------+
| MTreeNode     |   | TreeMap +     |           | HashMap       |   | DynHeap +     |
| <Agency>      |   | RbTree        |           | <code,Package>|   | lazy deletion |
+---------------+   +---------------+           +---------------+   +---------------+
        |                   |                           |                   |
        +-------------------+-------------+-------------+-------------------+
                                          |
                        +-----------------------------------+
                        |  ClientIndex (HashMap)            |
                        |  AuditLog (DLList)                |
                        |  Persistence (snapshot)           |
                        +-----------------------------------+
                                          |
                        +-----------------------------------+
                        |  Presentation (CLI / SFML simple) |
                        +-----------------------------------+
```

### 3.2 Mapeo Tecnológico (corregido)

| Módulo | Responsabilidad | Estructura DeSiGNAR | Header | Complejidad |
| :--- | :--- | :--- | :--- | :--- |
| **GeographicHierarchy** | Jerarquía N-aria de agencias | `MTreeNode<Agency>` + `HashMap<std::string, MTreeNode<Agency>*>` | `<mtreenode.hpp>`, `<map.hpp>` | hijo $O(1)$; búsqueda por ID $O(1)$; recorrido $O(n)$ |
| **TrackingEngine** | Rastreo global de paquetes | `HashMap<std::string, Package>` | `<map.hpp>` | $O(1)$ promedio |
| **InventoryIndex** | Stock ordenado por agencia | `TreeMap<std::string, std::string, std::less<std::string>, RbTree>` o `RbTree<std::string>` | `<map.hpp>`, `<rbtree.hpp>` | $O(\log n)$ |
| **PriorityDispatchQueue** | Orden de carga por agencia | `DynHeap<DispatchEntry, DispatchCmp>` | `<heap.hpp>` | inserción/extracción $O(\log n)$ |
| **ClientIndex** | Clientes normalizados | `HashMap<std::string, Client>` | `<map.hpp>` | $O(1)$ promedio |
| **AuditLog** | Historial de eventos | `DLList<PackageEvent>` | `<list.hpp>` | inserción $O(1)$ |
| **Reports** | Resultados de consultas | `DynArray<T>` | `<array.hpp>` | acceso $O(1)$ |
| **Hierarchy BFS** | Recorrido por niveles | `ListQueue<MTreeNode<Agency>*>` | `<queue.hpp>` | $O(n)$ |

> **Nota sobre `Designar::Tree`.** En DeSiGNAR v4.0.1, `Designar::Tree` es un
> **árbol binario de búsqueda no balanceado** (búsqueda $O(h)$, peor caso $O(n)$) y
> **no** es un árbol N-ario. La jerarquía geográfica se modela con
> `Designar::MTreeNode`, que sí es un nodo multi-ramas con padre, hijos y hermanos.
> La v1.0 del documento mapeaba erróneamente la jerarquía a `Designar::Tree`.

> **Nota sobre `TreeMap`.** El backend por defecto de `TreeMap` es `RankedTreap`.
> Para usar realmente un árbol rojo-negro hay que indicar `Designar::RbTree` como
> cuarto parámetro de la plantilla.

> **Nota sobre `DynHeap`.** `DynHeap` con el comparador por defecto `std::less` es
> un **min-heap**: `top()`/`get()` devuelven el elemento menor. Para que un Express
> salga antes que un Estándar, el comparador `DispatchCmp` debe considerar "más
> urgente" como "menor".

### 3.3 Estructura del Repositorio

```
NodeDispatch/
├── assets/                 # Recursos para la capa SFML (fuentes, iconos)
├── docs/
│   └── SDD.md              # Este documento
├── include/                # Cabeceras (.hpp)
├── src/                    # Implementación (.cpp)
├── tests/                  # Pruebas
├── third_party/            # Dependencias (DeSiGNAR vía submodule/FetchContent)
├── .gitignore
├── CMakeLists.txt
└── README.md
```

---

## 4. Modelo de Dominio y Datos

### 4.1 Enumeraciones

```cpp
#pragma once

#include <cstdint>
#include <string>

namespace nodedispatch {

enum class PaymentType {
    PREPAID,           // Pagado en origen por el emisor
    CASH_ON_DELIVERY   // Cobro destino
};

enum class PaymentStatus {
    PENDING,           // Pendiente de cobro
    PAID,              // Pagado y verificado
    FAILED             // Rechazado o fallido
};

enum class DeliveryMethod {
    AGENCY_PICKUP,     // Retiro en agencia
    HOME_DELIVERY      // Última milla (aplica recargo)
};

enum class ServiceLevel {
    EXPRESS_24H = 1,
    PRIORITY    = 2,
    STANDARD    = 3
};

enum class PackageStatus {
    REGISTERED,        // Registrado en el sistema
    IN_TRANSIT,        // En traslado entre nodos
    AT_LOCAL_AGENCY,   // Disponible en la agencia destino
    OUT_FOR_DELIVERY,  // En reparto (solo HOME_DELIVERY)
    DELIVERED,         // Entregado
    RETURNED           // Devuelto
};

enum class ReturnReason {
    NONE,
    PAYMENT_FAILED,
    RECIPIENT_ABSENT,
    CANCELLED
};

} // namespace nodedispatch
```

### 4.2 Estructuras

```cpp
struct Address {
    std::string state;
    std::string city;
    std::string exactAddress;
    std::string postalCode;
};

struct Client {
    std::string nationalId;   // Clave única
    std::string firstName;
    std::string lastName;
    std::string phone;
    Address address;
};

struct Package {
    std::string trackingCode;    // Clave única de rastreo
    std::string senderId;        // Referencia a ClientIndex
    std::string receiverId;      // Referencia a ClientIndex
    std::string targetAgencyId;  // Agencia destino

    double weightKg;

    std::int64_t baseCents;              // Costo base en centavos
    std::int64_t deliverySurchargeCents; // Recargo de delivery en centavos
    PaymentType paymentType;
    PaymentStatus paymentStatus;

    DeliveryMethod deliveryMethod;
    ServiceLevel serviceLevel;
    PackageStatus status;
    ReturnReason returnReason;

    std::int64_t createdAt;   // Marca de tiempo Unix de admisión
    std::int64_t updatedAt;   // Última modificación
};

struct Agency {
    std::string id;        // Clave única
    std::string name;
    Address location;
    std::string parentId;  // Vacío para la raíz
};

struct DispatchEntry {
    std::string trackingCode;
    std::string agencyId;
    ServiceLevel level;
    std::int64_t createdAt;
};

struct PackageEvent {
    std::string trackingCode;
    PackageStatus status;
    std::int64_t at;
    std::string note;
};
```

### 4.3 Decisiones de Diseño de Datos

- **Texto:** se usa `std::string` en lugar de `char[N]`. Los arrays de caracteres
  como clave de `HashMap` comparan direcciones (`std::equal_to<char[N]>`) y hashean
  bytes no inicializados (`super_fast_hash`), por lo que producen búsquedas
  incorrectas.
- **Dinero:** se usan enteros de centavos (`std::int64_t`) para evitar errores de
  punto flotante al comparar o acumular importes.
- **Normalización de clientes:** `Package` guarda `senderId`/`receiverId` en lugar
  de copiar dos `Client` completos. Los clientes viven en `ClientIndex`
  (`HashMap<std::string, Client>`). Esto reduce el tamaño de cada paquete y evita
  duplicar datos cuando un cliente envía varios paquetes.
- **Fuente única de verdad:** `TrackingEngine` es el dueño canónico de cada
  `Package`. El inventario por agencia y la cola de despacho guardan **códigos de
  rastreo**, no copias completas.
- **Niveles de servicio:** `enum class` en lugar de enteros mágicos.

---

## 5. Máquina de Estados y Reglas de Negocio

### 5.1 Transiciones de `PackageStatus`

| Desde | Hacia | Condición |
| :--- | :--- | :--- |
| `REGISTERED` | `IN_TRANSIT` | El operador confirma la salida del paquete. |
| `REGISTERED` | `RETURNED` | Cancelación antes del envío. |
| `IN_TRANSIT` | `AT_LOCAL_AGENCY` | Llega a la agencia destino. |
| `AT_LOCAL_AGENCY` | `OUT_FOR_DELIVERY` | Solo si `deliveryMethod == HOME_DELIVERY`. |
| `AT_LOCAL_AGENCY` | `DELIVERED` | Retiro en agencia y pago válido. |
| `OUT_FOR_DELIVERY` | `DELIVERED` | Pago válido (ver 5.2). |
| `OUT_FOR_DELIVERY` | `RETURNED` | Destinatario ausente. |
| `AT_LOCAL_AGENCY` | `RETURNED` | Pago fallido o vencimiento de retiro. |

Cualquier transición no listada se rechaza con un error de dominio.

### 5.2 Reglas Financieras

- Al registrar un paquete:
  - `PREPAID` ⇒ `paymentStatus = PAID`.
  - `CASH_ON_DELIVERY` ⇒ `paymentStatus = PENDING`.
- Costo total:
  $$
  \text{totalCents} = \text{baseCents} +
  \left(\text{deliveryMethod} = \text{HOME\_DELIVERY} ? \text{deliverySurchargeCents} : 0\right)
  $$
- **Regla de entrega:** si `paymentType == CASH_ON_DELIVERY`, la transición a
  `DELIVERED` se rechaza mientras `paymentStatus != PAID`.
- `FAILED` en un paquete con Cobro Destino lo deja elegible para `RETURNED`.
- Los importes se calculan y comparan en centavos enteros.

### 5.3 Prioridad de Despacho

La cola de cada agencia ordena por:

1. `ServiceLevel`: `EXPRESS_24H` (1) antes que `PRIORITY` (2) antes que `STANDARD` (3).
2. `createdAt`: el paquete más antiguo primero (desempate FIFO).

```cpp
struct DispatchCmp {
    // En un min-heap, "menor" significa "más urgente".
    bool operator()(const DispatchEntry& a, const DispatchEntry& b) const {
        if (a.level != b.level)
            return static_cast<int>(a.level) < static_cast<int>(b.level);
        return a.createdAt < b.createdAt;
    }
};
```

Como `DynHeap` no permite eliminar un elemento arbitrario ni actualizar su
prioridad, se usa **eliminación perezosa**: al extraer, se descartan las entradas
cuyo paquete ya no esté `AT_LOCAL_AGENCY` o no pertenezca a la agencia solicitada.

---

## 6. Diseño Detallado por Módulo

### 6.1 GeographicHierarchy

- **Estructura:** `Designar::MTreeNode<Agency>` para el árbol y
  `Designar::HashMap<std::string, Designar::MTreeNode<Agency>*>` como índice de
  búsqueda $O(1)$.
- **Operaciones:**
  - `addAgency(agency, parentId)`: valida ID único y padre existente; crea el nodo
    con `new` y lo enlaza con `append_child`.
  - `findAgency(id)`: busca en el índice y devuelve una copia del `Agency`.
  - `listDescendants(id)`: recorrido DFS con `for_each_child` o BFS con
    `Designar::ListQueue`.
  - `destroy()`: llama a `MTreeNode<Agency>::destroy_tree(root)` y limpia el índice.
- **Limitación conocida:** `MTreeNode` solo expone `remove_first_child` y
  `remove_last_child`; no hay eliminación arbitraria de un hijo intermedio. Para el
  MVP la jerarquía es de solo inserción; si se necesita eliminar, se restringe a
  hojas o se reconstruye el subárbol.
- **RAII:** una clase `AgencyTree` envuelve el nodo raíz y libera la memoria en su
  destructor para evitar fugas.

### 6.2 TrackingEngine

- **Estructura:** `Designar::HashMap<std::string, Package>`.
- **Operaciones:**
  - `register(pkg)`: `insert` devuelve `nullptr` si el código ya existe, lo que se
    usa para rechazar duplicados.
  - `track(code)`: devuelve `std::optional<Package>` (copia), nunca un puntero
    interno.
  - `updatePayment`, `updateStatus`: buscan el paquete, validan la transición y
    modifican el valor.
- **Rehashing:** automático en DeSiGNAR al superar $\alpha = 0.75$. No se
  implementa rehashing propio.

### 6.3 InventoryIndex

- **Estructura:** por cada agencia, un `TreeMap` con backend `RbTree` (o un
  `RbTree<std::string>` si solo se almacenan códigos).
- **Operaciones:** `add(code)`, `remove(code)`, `contains(code)`, `listSorted()`.
- **Uso:** el inventario guarda los códigos de los paquetes cuya agencia destino es
  esa agencia. El estado real del paquete se consulta en `TrackingEngine`.
- **Orden:** el recorrido in-order del árbol entrega el inventario ordenado por
  código de rastreo.

### 6.4 PriorityDispatchQueue

- **Estructura:** `Designar::HashMap<std::string, Designar::DynHeap<DispatchEntry, DispatchCmp>>`
  (una cola por agencia).
- **Operaciones:** `enqueue(entry)`, `dispatch(agencyId)`.
- **Extracción:** `dispatch` hace `get()` en el heap y descarta entradas obsoletas
  hasta encontrar un paquete despachable; marca el paquete y registra el evento.
- **Sin actualización de prioridad:** si un paquete cambia de nivel de servicio
  antes de salir, se encola una nueva entrada y la anterior se descarta por
  eliminación perezosa.

### 6.5 AuditLog

- **Estructura:** `Designar::DLList<PackageEvent>` global, más una lista por
  paquete si se requiere consulta rápida.
- **Uso:** registra cada transición de estado con marca de tiempo y nota.
- **Complejidad:** inserción al final $O(1)$; recorrido $O(n)$.

### 6.6 Persistence (Snapshot)

- **Formato:** archivo de texto con un registro por línea, campos separados por un
  delimitador y cadenas escapadas con `std::quoted` (`<iomanip>`).
- **Registros:** `AGENCY`, `CLIENT`, `PACKAGE`, `EVENT`.
- **Orden de carga:** primero agencias (padres antes que hijos), luego clientes,
  luego paquetes, luego eventos. Las colas de despacho se reconstruyen a partir de
  los paquetes en estado `AT_LOCAL_AGENCY`.
- **Reglas:** no se serializan estructuras con `write(&obj, sizeof(obj))` (padding,
  endianness y versionado). `load()` limpia el estado antes de reconstruir.
- **WAL:** fuera del alcance obligatorio; queda documentado como mejora opcional.

### 6.7 Presentation Layer

- **CLI (obligatoria para la demo):** menú de comandos
  (`agency add`, `agency tree`, `package register`, `package track`,
  `package pay`, `package dispatch`, `report`, `save`, `load`).
- **SFML (opcional):** ventana simple que dibuja el árbol de agencias y el estado
  de los paquetes. Consume la misma fachada; no accede a las estructuras internas.
  Si el tiempo aprieta, la demo por CLI es suficiente.

---

## 7. API del Núcleo (Fachada)

```cpp
namespace nodedispatch {

class NodeDispatch {
public:
    // --- Jerarquía geográfica ---
    bool addAgency(const Agency& agency, const std::string& parentId);
    std::optional<Agency> findAgency(const std::string& id) const;
    DynArray<Agency> listDescendants(const std::string& id) const;

    // --- Clientes ---
    bool registerClient(const Client& client);

    // --- Paquetes ---
    bool registerPackage(const Package& pkg);
    std::optional<Package> trackPackage(const std::string& trackingCode) const;
    bool updatePaymentStatus(const std::string& trackingCode, PaymentStatus status);
    bool updatePackageStatus(const std::string& trackingCode, PackageStatus status);
    bool cancelPackage(const std::string& trackingCode);

    // --- Despacho ---
    std::optional<Package> dispatchNextPackage(const std::string& agencyId);

    // --- Consultas y reportes ---
    DynArray<Package> packagesAtAgency(const std::string& agencyId) const;
    DynArray<Package> packagesByStatus(PackageStatus status) const;
    std::int64_t totalRevenueCents() const;

    // --- Persistencia ---
    bool save(const std::string& path) const;
    bool load(const std::string& path);
};

} // namespace nodedispatch
```

**Política de errores:** la API pública no lanza excepciones de negocio. Devuelve
`bool` o `std::optional`. Las excepciones de DeSiGNAR (por ejemplo, `find` sobre una
clave inexistente) se capturan en la fachada y se traducen a un resultado seguro.

**Contratos:**

- `registerPackage`: rechaza códigos duplicados, peso $\le 0$, importes negativos,
  agencia destino inexistente o cliente no registrado.
- `updatePackageStatus`: valida la transición según la tabla 5.1 y la regla 5.2.
- `dispatchNextPackage`: solo extrae paquetes `AT_LOCAL_AGENCY` de la agencia
  indicada; actualiza el estado y el historial antes de devolver la copia.
- Ninguna función devuelve punteros o referencias a memoria interna.

---

## 8. Concurrencia (Mejora Opcional)

No es un requisito de la cátedra. Si se implementa:

- Un único `std::shared_mutex` en la fachada para simplificar; lecturas con
  `std::shared_lock` y escrituras con `std::unique_lock`.
- No se devuelven punteros internos: las lecturas copian el valor mientras el lock
  está tomado.
- El rehashing del `HashMap` ocurre bajo lock exclusivo.
- DeSiGNAR ofrece `Designar::ConcurrentMap`, pero usa `std::mutex` (sin lectores
  concurrentes); solo se usaría si se prefiere simplicidad sobre paralelismo.

---

## 9. Estrategia de Pruebas

| Área | Casos |
| :--- | :--- |
| Agencias | ID duplicado, padre inexistente, recorrido DFS/BFS, subárbol correcto |
| Rastreo | Código inexistente, registro duplicado, 100 000 inserciones y búsquedas |
| Inventario | Orden in-order, inserción/eliminación, agencia sin stock |
| Despacho | Orden por nivel, desempate por antigüedad, entradas obsoletas (lazy) |
| Pagos | `PREPAID` nace `PAID`; COD no se entrega sin `PAID`; transición inválida |
| Estados | Cada transición permitida y al menos una prohibida por origen |
| Persistencia | Guardar/cargar sin pérdida, archivo corrupto, carga idempotente |
| Memoria | Ejecución bajo AddressSanitizer sin fugas (árbol y listas) |

Herramientas: pruebas con `assert` (estilo DeSiGNAR) o un framework de cabecera
única; compilación con `-fsanitize=address,undefined` en modo Debug.

---

## 10. Build, Dependencias y Despliegue

- **Estándar:** C++17.
- **DeSiGNAR:** no es *header-only*; es una librería estática con target
  `Designar::Designar`. Se integra con `FetchContent` o submódulo, fijando el tag
  `v4.0.1`, y se desactivan sus pruebas y ejemplos:

```cmake
include(FetchContent)
set(DESIGNAR_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(DESIGNAR_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Designar
    GIT_REPOSITORY https://github.com/R3mmurd/DeSiGNAR.git
    GIT_TAG        v4.0.1)
FetchContent_MakeAvailable(Designar)

target_link_libraries(nodedispatch PRIVATE Designar::Designar)
```

- **Licencia:** DeSiGNAR es MIT; se conserva la atribución.
- **GitFlow:** `main` (entregas) ← `development` (integración) ← `feature/*`
  (trabajo modular). `main` y `development` ya existen en el repositorio.
- **.gitignore:** excluye `build/`, `bin/`, `obj/`, `lib/`, binarios, archivos de
  CMake generados y snapshots de datos.

---

## 11. Trazabilidad con el Contenido de la Materia

| Tema | Estructura DeSiGNAR | Módulo | Operación demostrable |
| :--- | :--- | :--- | :--- |
| **Arrays** | `DynArray<T>` | Reportes, resultados de consultas, importación masiva | `append`, `operator[]`, recorrido |
| **Listas** | `DLList<PackageEvent>` | Historial de eventos del paquete | `append`, iteración, `remove` |
| **Árboles** | `MTreeNode<Agency>` | Jerarquía de agencias (N-ario) | `append_child`, `for_each_child`, recorrido |
| **Árboles** | `RbTree` / `TreeMap` | Inventario ordenado por agencia | `insert`, `search`, `remove`, in-order |
| **Tablas hash** | `HashMap` | Rastreo global e índice de agencias/clientes | `insert`, `search`, `remove`, factor de carga |
| **Montículos** | `DynHeap` | Cola de despacho por prioridad | `insert`, `top`, `get` |
| **Colas** (apoyo) | `ListQueue` | Recorrido BFS de la jerarquía | `put`, `get` |

---

## 12. Plan de 4 Semanas y Roles

| Semana | Foco | Entregable |
| :--- | :--- | :--- |
| **1** | CMake + DeSiGNAR, modelos, `AgencyTree`, `TrackingEngine`, esqueleto CLI | Agregar agencias; registrar y rastrear paquetes |
| **2** | Inventario RB, máquina de estados, pagos, `DynHeap` con lazy deletion | Pagar, despachar y cambiar estado |
| **3** | Reportes, snapshot, integración, suite de pruebas | Flujo completo + persistencia |
| **4** | Hardening, documentación, benchmarks, presentación, colchón | Demo final + SDD v1.1 + README |

**Reparto sugerido (2 personas):**

- **A:** núcleo de estructuras (jerarquía, inventario, despacho).
- **B:** dominio, estados/pagos, persistencia, CLI y pruebas.
- **Ambos:** integración, demo y documentación.

---

## 13. Riesgos

| Riesgo | Impacto | Mitigación |
| :--- | :--- | :--- |
| Curva de aprendizaje de DeSiGNAR | Alto | Semana 1 dedicada a ejemplos (`samples/src`) y pruebas pequeñas |
| `MTreeNode` no permite eliminar hijos intermedios | Medio | Jerarquía de solo inserción en el MVP; eliminación solo de hojas |
| `DynHeap` no permite eliminar/actualizar | Medio | Eliminación perezosa documentada |
| Alcance excesivo | Alto | MoSCoW: WAL, concurrencia y SFML fuera del camino crítico |
| SFML consume tiempo | Medio | CLI primero; SFML solo si sobra la semana 4 |
| Datos de texto y serialización | Medio | `std::string` + `std::quoted`; nunca volcar structs crudos |

---

## 14. Glosario y Referencias

- **DeSiGNAR v4.0.1:** <https://github.com/R3mmurd/DeSiGNAR>
- **`MTreeNode`:** nodo de árbol multi-ramas (`include/mtreenode.hpp`).
- **`RbTree`:** árbol rojo-negro como conjunto ordenado (`include/rbtree.hpp`).
- **`TreeMap`:** mapa ordenado; backend configurable, por defecto `RankedTreap`
  (`include/map.hpp`).
- **`HashMap`:** tabla hash con encadenamiento separado y rehashing automático
  (`include/map.hpp`, `include/chainedhash.hpp`).
- **`DynHeap`:** montículo binario dinámico, min-heap con `std::less`
  (`include/heap.hpp`).
- **WAL (Write-Ahead Log):** registro de transacciones previo a la escritura;
  mejora opcional en este proyecto.
- **Snapshot:** volcado completo del estado a un archivo local.
