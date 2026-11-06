# Software Design Document (SDD) — NodeDispatch

| Metadato | Detalle |
| :--- | :--- |
| **Proyecto** | **NodeDispatch** (Core Engine de Logística y Distribución) |
| **Documento** | Especificación Técnica Base — Versión 1.0 (Draft Activo) |
| **Autores** | Keny Rondón y Rafael Castillo |
| **Institución / Cátedra** | Universidad de Los Andes — Programación 3 |
| **Framework Obligatorio** | DeSiGNAR (Data Structures GeNeral librARy — C++17 / C++20) |
| **Fecha de actualización** | 6 de noviembre de 2026 |
| **Repositorio GitHub** | [NodeDispatch Repo](https://github.com/kenyosiosi/NodeDispatch.git) (`main` $\leftarrow$ `development` $\leftarrow$ `feature/*`) |

---

## 1. Visión General y Objetivos (Overview & Goals)

### 1.1 Contexto y Problema

Las empresas medianas de logística y distribución de encomiendas enfrentan desafíos algorítmicos complejos directamente en memoria: control masivo de inventarios en bodegas locales, búsquedas de rastreo instantáneas, gestión de prioridades de carga para flota vehicular y soporte riguroso para modalidades financieras (Prepago vs Cobro Destino) y métodos de entrega (Retiro en Agencia vs Envío a Domicilio / Última Milla). NodeDispatch resuelve estos problemas mediante un motor en memoria optimizado con estructuras de datos avanzadas.

### 1.2 Objetivos Principales (Goals)

* **Topología Jerárquica Geográfica:** Modelar la estructura organizativa de agencias y centros de acopio mediante un Árbol General / N-ario (`Designar::Tree`) para reflejar la subordinación administrativa y operativa.

* **Inventario de Alta Frecuencia:** Gestionar inserciones y búsquedas masivas en bodegas en complejidad $O(\log n)$ utilizando Árboles Rojo-Negro (`Designar::RedBlackTree`), minimizando el costo de rotaciones frente a árboles AVL bajo ráfagas de escritura.

* **Rastreo Instantáneo $O(1)$:** Proveer un motor de consulta global mediante tablas hash personalizadas con encadenamiento separado (`Designar::HashMap`).

* **Despacho Priorizado:** Automatizar el orden de carga vehicular en plataformas de salida mediante montículos avanzados (`Designar::Heap`).

* **Modelo Financiero y Última Milla:** Control nativo de costos base, recargos por delivery y validaciones de pago condicionales para la entrega final (`DELIVERED`).

* **Concurrencia Segura y Durabilidad:** Garantizar operaciones multi-hilo seguras mediante cerrojos de lectores-escritores (`std::shared_mutex`) y persistencia ante fallos mediante un sistema de registro de transacciones (*Write-Ahead Log*).

### 1.3 Fuera de Alcance (Non-Goals)

* Uso de motores de bases de datos relacionales o NoSQL externos pesados (el núcleo opera en RAM, apoyándose únicamente en archivos locales livianos para persistencia WAL).

* Uso de contenedores genéricos de la STL para las estructuras de datos núcleo (reemplazados obligatoriamente por la biblioteca DeSiGNAR).

* Desarrollo de interfaces gráficas complejas dentro del núcleo backend (la capa visual interactiva se delegará externamente mediante SFML).

## 2. Arquitectura de Alto Nivel (High-Level Architecture)

### 2.1 Diagrama de Componentes y Mapeo con DeSiGNAR

```
                        +-----------------------------------+
                        |      NodeDispatch (Facade)        |
                        +-----------------------------------+
                                          |
        +-------------------+-------------+-------------+-------------------+
        |                   |                           |                   |
+---------------+   +---------------+           +---------------+   +---------------+
|  Geographic   |   |   Inventory   |           |   Tracking    |   |PriorityDispatch|
|   Hierarchy   |   |     Index     |           |    Engine     |   |     Queue     |
+---------------+   +---------------+           +---------------+   +---------------+
|Designar::Tree |   |Designar::     |           |Designar::     |   |Designar::Heap |
| (Árbol N-ario)|   |RedBlackTree   |           |HashMap        |   | (Binary Heap) |
+---------------+   +---------------+           +---------------+   +---------------+
```

### 2.2 Tabla de Mapeo Tecnológico

| Módulo del Sistema | Responsabilidad Operativa | Abstracción DeSiGNAR Aplicada | Complejidad Temporal | 
| ----- | ----- | ----- | ----- | 
| **GeographicHierarchy** | Jerarquía de agencias y centros | `Designar::Tree` (Árbol General) | $O(n)$ recorrido / $O(\log n)$ búsq. | 
| **InventoryIndex** | Control de stock por bodega | `Designar::RedBlackTree<Key, Package>` | $O(\log n)$ | 
| **TrackingEngine** | Rastreo global de envíos | `Designar::HashMap` / Tablas Hash | $O(1)$ promedio | 
| **PriorityDispatchQueue** | Urgencia y orden de despacho | `Designar::Heap` | $O(\log n)$ extracción | 

## 3. Diseño Detallado (Detailed Technical Design)

### 3.1 Modelos de Datos y Esquemas en C++

```cpp
#pragma once
#include <types.hpp>

enum class PaymentType { 
    PREPAID,           // Pagado en origen por el emisor
    CASH_ON_DELIVERY   // Cobro destino (pagado por el receptor al recibir)
};

enum class PaymentStatus { 
    PENDING,           // Pendiente de cobro financiero
    PAID,              // Pagado y verificado en firme
    FAILED             // Transacción rechazada o fallida
};

enum class DeliveryMethod { 
    AGENCY_PICKUP,     // Retiro en agencia u oficina (tarifa base estándar)
    HOME_DELIVERY      // Envío a domicilio / última milla (aplica recargo)
};

enum class PackageStatus { 
    REGISTERED,        // Registrado en el sistema central
    IN_TRANSIT,        // En traslado entre nodos de la red
    AT_LOCAL_AGENCY,   // Disponible en la bodega de destino final
    OUT_FOR_DELIVERY,  // En ruta de reparto a domicilio (exclusivo delivery)
    DELIVERED,         // Entregado satisfactoriamente al receptor
    RETURNED           // Devuelto por falta de pago o destinatario ausente
};

struct Address {
    char state[64];         // Estado / Provincia
    char city[64];          // Ciudad
    char exactAddress[128]; // Dirección exacta
    char postalCode[16];    // Código postal opcional
};

struct Client {
    char nationalId[32];    // Cédula / ID único
    char firstName[64];     // Nombre
    char lastName[64];      // Apellido
    char phone[32];         // Teléfono de contacto
    Address address;        // Dirección asociada al cliente
};

struct Package {
    char trackingHash[64];       // Clave única de rastreo (Indexación O(1))
    Client sender;               // Datos completos del emisor
    Client receiver;             // Datos completos del receptor
    char targetAgencyNodeId[64]; // ID de la agencia o bodega de destino final
    double weightKg;             // Peso físico en kilogramos
    
    // Estructura Financiera y Tarifaria
    double baseShippingCost;     // Costo base por flete y peso
    double deliverySurcharge;    // Recargo por delivery (0.0 si es AGENCY_PICKUP)
    PaymentType paymentType;     // Prepago vs Cobro Destino
    PaymentStatus paymentStatus; // Estado financiero del pago
    
    // Modalidad Operativa
    DeliveryMethod deliveryMethod; // Retiro en oficina vs Envío a domicilio
    int serviceLevel;              // 1: Express 24h, 2: Prioritario, 3: Estándar
    PackageStatus currentStatus;   // Estado operativo actual en la red
    Designar::int_t timestampCreation; // Marca de tiempo Unix de admisión
};
```

### 3.2 Contratos de API / Interfaces del Núcleo

* `bool registerPackage(const Package& pkg);` — Registra un envío, escribe en el WAL de persistencia, y lo asigna al inventario y motor de rastreo.

* `Package trackPackageSafe(const char* trackingHash);` — Retorna una copia segura del paquete en $O(1)$ promedio evitando condiciones de carrera en punteros.

* `bool updatePaymentStatus(const char* trackingHash, PaymentStatus status);` — Actualiza el estado financiero del envío.

* `bool dispatchNextPackage(const char* agencyId);` — Extrae el paquete con mayor prioridad desde el Heap de despacho de la agencia.

### 3.3 Lógica de Negocio y Algoritmos

* **Cálculo Financiero:**

  $$
  \text{Total} = \text{baseShippingCost} + \left(\text{deliveryMethod} == \text{HOME\_DELIVERY} ? \text{deliverySurcharge} : 0.0\right)
  $$

* **Regla de Validación de Entrega:** Si `paymentType == CASH_ON_DELIVERY`, la transición a `DELIVERED` queda estrictamente denegada si `paymentStatus != PAID`.

## 4. Alternativas Consideradas y Trade-Offs

| Componente | Opción Descartada | Opción Seleccionada | Justificación y Pros/Contras | 
| ----- | ----- | ----- | ----- | 
| **Inventario en Bodega** | Árbol AVL | Árbol Rojo-Negro (`Designar::RedBlackTree`) | El AVL es excesivamente estricto ($h \le 1.44 \log_2 n$) y penaliza inserciones masivas. El Rojo-Negro relaja el balance ($h \le 2 \log_2 n$). | 
| **Estructuras Núcleo** | Contenedores genéricos STL | Biblioteca DeSiGNAR (Obligatorio) | Garantiza cumplimiento estricto de las directrices académicas de la asignatura. | 

## 5. Requisitos No Funcionales y Mitigación de Riesgos

### 5.1 Concurrencia y Thread-Safety

* Cada bodega e índice operativo encapsulará un `std::shared_mutex`.

* **Lecturas concurrentes:** Las consultas de rastreo utilizarán bloqueos compartidos (`std::shared_lock`), permitiendo lecturas en paralelo.

* **Escrituras exclusivas:** Las inserciones y actualizaciones financieras emplearán bloqueos exclusivos (`std::unique_lock`).

### 5.2 Persistencia y Recuperación ante Fallos (WAL)

* Implementación de un sistema de registro de transacciones (*Write-Ahead Log*) en archivo plano local (`std::ofstream`) para reconstrucción de estado tras reinicios inesperados.

### 5.3 Dinámica de la Tabla Hash

* Monitoreo constante del factor de carga ($\alpha > 0.75$) para activar redimensionamiento automático (*rehashing*) y preservar búsquedas en $O(1)$ promedio.

## 6. Estrategia de Observabilidad y Despliegue

### 6.1 Monitoreo y Métricas

* Registro de trazas de ejecución en consola y archivos de log para auditoría de estados.

* Validación de aserciones (`assert`) internas para verificar invariantes estructurales de DeSiGNAR.

### 6.2 Plan de Despliegue y Repositorio Git (`NodeDispatch`)

* **Estructura de Ramas (Git Flow):**

  * `main`: Rama estable de producción (actualizada únicamente mediante merges desde `development`).

  * `development`: Rama de integración de características.

  * `feature/*`: Ramas de desarrollo modular individual.

* **Exclusiones:** Exclusión estricta de binarios, carpetas `/bin`, `/obj`, `.exe`, `.o` mediante `.gitignore`.
