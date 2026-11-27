# Formato de snapshot — ND1

Especificación del archivo de texto que `save()` escribe y `load()` reconstruye
(SDD §6.6). Es un formato **plano, legible y versionado**: se puede inspeccionar
con `less`, comparar con `diff` y corregir a mano sin herramienta propia.

- **Versión:** `ND1`.
- **Codificación:** UTF-8 sin BOM.
- **Fin de línea:** `\n`. Al leer se aceptan `\r\n` y se normalizan.
- **Delimitador de campo:** `|` (barra vertical).
- **Cadenas:** siempre entre comillas dobles, escapadas con `std::quoted`
  (`<iomanip>`), que duplica la comilla y la barra invertida internas.
- **Números:** decimal, sin separador de miles ni símbolo de moneda. Los importes
  son centavos enteros (`std::int64_t`). Las marcas de tiempo son segundos Unix
  en UTC (`std::int64_t`).
- **Booleanos:** no hay; se usan los enums.

## 1. Estructura del archivo

```
ND1
AGENCY|<id>|<name>|<state>|<city>|<exactAddress>|<postalCode>|<parentId>
...
```

- **Primera línea:** la cadena `ND1`, sola, sin delimitadores. Si no coincide, el
  archivo se rechaza (ver §5).
- **Una línea por registro.** No hay líneas de continuación ni comentarios.
- **Las líneas vacías se ignoran** (por ejemplo, la que deja un salto final).
- El orden de los registros dentro del archivo debe respetar §4.

### Escapado

Cada campo de texto se escribe con `std::quoted`, así que:

| Valor lógico | En el archivo |
| :--- | :--- |
| `Agencia Norte` | `"Agencia Norte"` |
| `Agencia "Norte"` | `"Agencia \"Norte\""` |
| `Calle 5 | Local 2` | `"Calle 5 | Local 2"` |
| `C:\datos\` | `"C:\\datos\\"` |
| *(vacío)* | `""` |

El delimitador `|` **nunca** aparece fuera de comillas, de modo que
`split` por `|` respetando el entrecomillado es suficiente para trocear la línea.

## 2. Registros

### 2.1 `AGENCY`

| # | Campo | Tipo | Notas |
| :--- | :--- | :--- | :--- |
| 0 | `AGENCY` | literal | Etiqueta del registro. |
| 1 | `id` | texto | Clave única. |
| 2 | `name` | texto | |
| 3 | `state` | texto | Parte de `Address`. |
| 4 | `city` | texto | |
| 5 | `exactAddress` | texto | |
| 6 | `postalCode` | texto | |
| 7 | `parentId` | texto | `""` en la raíz. Debe existir antes en el archivo. |

### 2.2 `CLIENT`

| # | Campo | Tipo | Notas |
| :--- | :--- | :--- | :--- |
| 0 | `CLIENT` | literal | |
| 1 | `nationalId` | texto | Clave única. |
| 2 | `firstName` | texto | |
| 3 | `lastName` | texto | |
| 4 | `phone` | texto | |
| 5 | `state` | texto | Parte de `Address`. |
| 6 | `city` | texto | |
| 7 | `exactAddress` | texto | |
| 8 | `postalCode` | texto | |

### 2.3 `PACKAGE`

| # | Campo | Tipo | Notas |
| :--- | :--- | :--- | :--- |
| 0 | `PACKAGE` | literal | |
| 1 | `trackingCode` | texto | Clave única. |
| 2 | `senderId` | texto | Debe existir como `CLIENT`. |
| 3 | `receiverId` | texto | Debe existir como `CLIENT`. |
| 4 | `targetAgencyId` | texto | Debe existir como `AGENCY`. |
| 5 | `weightKg` | real | Ver §3. |
| 6 | `baseCents` | entero | ≥ 0. |
| 7 | `deliverySurchargeCents` | entero | ≥ 0. |
| 8 | `paymentType` | enum | `PREPAID` \| `CASH_ON_DELIVERY`. |
| 9 | `paymentStatus` | enum | `PENDING` \| `PAID` \| `FAILED`. |
| 10 | `deliveryMethod` | enum | `AGENCY_PICKUP` \| `HOME_DELIVERY`. |
| 11 | `serviceLevel` | enum | `EXPRESS_24H` \| `PRIORITY` \| `STANDARD`. |
| 12 | `status` | enum | `REGISTERED` \| `IN_TRANSIT` \| `AT_LOCAL_AGENCY` \| `OUT_FOR_DELIVERY` \| `DELIVERED` \| `RETURNED`. |
| 13 | `returnReason` | enum | `NONE` \| `PAYMENT_FAILED` \| `RECIPIENT_ABSENT` \| `CANCELLED`. |
| 14 | `createdAt` | entero | Segundos Unix UTC. |
| 15 | `updatedAt` | entero | Segundos Unix UTC. |

### 2.4 `EVENT`

| # | Campo | Tipo | Notas |
| :--- | :--- | :--- | :--- |
| 0 | `EVENT` | literal | |
| 1 | `trackingCode` | texto | Debe existir como `PACKAGE`. |
| 2 | `status` | enum | Igual que el campo 12 de `PACKAGE`. |
| 3 | `at` | entero | Segundos Unix UTC. |
| 4 | `note` | texto | Puede ser `""`. |

Los eventos se guardan en el orden en que ocurrieron; `load()` los reproduce en
ese mismo orden dentro del `AuditLog`.

## 3. Números reales

`weightKg` se escribe con `std::ostringstream`, locale clásico
(`std::locale::classic()`) y `std::setprecision(std::numeric_limits<double>::max_digits10)`,
que garantiza ida y vuelta exacta. Al leer se usa `std::istringstream` con el
mismo locale; **nunca** `std::stod` con el locale del sistema, porque en español
interpreta la coma como separador decimal.

## 4. Orden de carga

`load()` construye el estado en este orden y **rechaza** cualquier referencia que
apunte hacia adelante:

1. `AGENCY` — padres antes que hijos. La raíz (`parentId` vacío) primero.
2. `CLIENT`.
3. `PACKAGE` — `senderId`, `receiverId` y `targetAgencyId` ya deben existir.
4. `EVENT` — `trackingCode` ya debe existir.

Las colas de despacho **no se serializan**: se reconstruyen a partir de los
paquetes cuyo `status` es `AT_LOCAL_AGENCY`, con el mismo criterio que
`dispatchNextPackage` (SDD §5.3). El inventario por agencia se reconstruye igual,
a partir de `targetAgencyId`, pero **solo con los paquetes en `AT_LOCAL_AGENCY`**:
el inventario es stock presente, no previsión por destino (SDD §6.3).

## 5. Política de error

`load()` es **todo o nada**: si una línea falla, no se publica ningún registro y
el estado previo queda intacto. Se parsea en estructuras temporales y se
intercambian al final.

Errores que se rechazan, con el número de línea en el mensaje:

| Código | Situación | Efecto |
| :--- | :--- | :--- |
| `ND_E_VERSION` | La primera línea no es `ND1`. | No se lee nada. |
| `ND_E_TAG` | Etiqueta de registro desconocida. | No se lee nada. |
| `ND_E_FIELDS` | Número de campos distinto del que define §2. | No se lee nada. |
| `ND_E_NUMBER` | Entero o real mal formado, o fuera de rango de `int64_t`. | No se lee nada. |
| `ND_E_ENUM` | Valor de enum desconocido. | No se lee nada. |
| `ND_E_DUP` | Clave repetida (`id`, `nationalId`, `trackingCode`). | No se lee nada. |
| `ND_E_REF` | Referencia a un registro que aún no existe. | No se lee nada. |
| `ND_E_INVALID` | `weightKg <= 0`, importe negativo, `DELIVERED` con cobro destino sin pagar. | No se lee nada. |

`load()` **limpia el estado** antes de reconstruirlo, pero solo después de que el
archivo haya pasado la validación completa: así un archivo corrupto no deja el
sistema a medias.

## 6. Casos límite

1. **Delimitador dentro de un texto.** `exactAddress` = `Calle 5 | Local 2` se
   escribe `"Calle 5 | Local 2"` y vuelve idéntico. Sin `std::quoted`, el `|`
   partiría el campo en dos.
2. **Comillas y barras invertidas.** `name` = `Depósito "La Esquina"` se escribe
   `"Depósito \"La Esquina\""`. El round-trip preserva las dos comillas y el
   escape.
3. **Cadena vacía frente a campo ausente.** La raíz de la jerarquía lleva
   `parentId` vacío, escrito `""`. Un campo ausente es un error de número de
   campos, no una cadena vacía: la distinción es deliberada.
4. **Agencia hija antes que el padre.** Se rechaza con `ND_E_REF` indicando la
   línea; no se reordena en silencio, porque un archivo desordenado suele delatar
   una edición manual equivocada.
5. **Archivo truncado a mitad de línea.** El último registro puede no terminar en
   `\n`; se acepta. Pero si la línea está incompleta (menos campos), se rechaza
   con `ND_E_FIELDS`. El guardado escribe primero en `<archivo>.tmp` y renombra al
   final, de modo que un corte deja el snapshot anterior intacto.

Casos adicionales que la implementación debe cubrir aunque no estén entre los
cinco: `\r\n` (se normaliza), líneas en blanco (se ignoran), BOM al inicio (se
rechaza con `ND_E_VERSION` para no adivinar) y `weightKg` con notación
exponencial (`1.5e-3`, se acepta: sale de la misma biblioteca que lo escribió).

## 7. Ejemplo

```
ND1
AGENCY|"AG-CCS"|"Agencia Caracas"|"Distrito Capital"|"Caracas"|"Av. Libertador, Torre Norte"|"1010"|""
AGENCY|"AG-CCS-CH"|"Acopio Chacao"|"Miranda"|"Chacao"|"Calle 4 con Av. 2"|"1060"|"AG-CCS"
CLIENT|"V-12345678"|"Ana"|"Pérez"|"+58-412-1234567"|"Distrito Capital"|"Caracas"|"Calle 10, Casa 3"|"1010"
PACKAGE|"ND-20261120-000001"|"V-12345678"|"V-87654321"|"AG-CCS-CH"|2.35|1500|300|PREPAID|PAID|HOME_DELIVERY|EXPRESS_24H|AT_LOCAL_AGENCY|NONE|1795000000|1795000300
EVENT|"ND-20261120-000001"|REGISTERED|1795000000|"Registrado en agencia origen"
EVENT|"ND-20261120-000001"|AT_LOCAL_AGENCY|1795000300|"Recibido en acopio"
```

## 8. Compatibilidad

- Un lector `ND1` **no** acepta un archivo de versión superior: prefiere fallar a
  malinterpretar. La versión `ND2` se reserva para cambios incompatibles
  (renombrar o quitar campos, cambiar el delimitador).
- Añadir un campo **al final** de un registro obliga a subir de versión en este
  documento, porque `ND1` exige un número exacto de campos.
- El formato no guarda tipos ni metadatos: el esquema es este documento y el
  código de `save()`/`load()` debe citarlo.

## 9. Lo que este formato no es

- **No es un WAL.** No hay registro por adelantado ni replay idempotente: el
  snapshot es una foto completa, y la escritura atómica (`tmp` + `rename`) evita
  dejar un archivo a medias. El WAL es una mejora opcional fuera del alcance
  obligatorio (SDD §6.6).
- **No serializa estructuras con `write(&obj, sizeof(obj))`**: eso arrastraría
  padding, endianness y punteros, y ataría el archivo a un compilador (SDD §6.6).
