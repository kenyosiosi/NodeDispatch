# Reglas de dominio verificables

Resumen operativo del SDD §5. Si el código y esta tabla difieren, gana el SDD y
hay que corregir uno de los dos en el mismo PR.

## Transiciones de `PackageStatus`

| Desde | Hacia | Condición |
| :--- | :--- | :--- |
| `REGISTERED` | `IN_TRANSIT` | siempre |
| `REGISTERED` | `RETURNED` | solo cancelación |
| `IN_TRANSIT` | `AT_LOCAL_AGENCY` | siempre |
| `AT_LOCAL_AGENCY` | `OUT_FOR_DELIVERY` | solo `HOME_DELIVERY` |
| `AT_LOCAL_AGENCY` | `DELIVERED` | pago válido |
| `OUT_FOR_DELIVERY` | `DELIVERED` | pago válido (COD ⇒ `PAID`) |
| `OUT_FOR_DELIVERY` | `RETURNED` | destinatario ausente |
| `AT_LOCAL_AGENCY` | `RETURNED` | pago fallido o vencimiento |

Cualquier otra transición devuelve `INVALID_STATE`.

## Contratos de `registerPackage`

Rechaza, sin excepción:

- código de rastreo duplicado (`DUPLICATE`);
- peso `<= 0` (`INVALID_INPUT`);
- importe negativo (`INVALID_INPUT`);
- agencia destino inexistente (`NOT_FOUND`);
- `senderId` o `receiverId` no registrados (`NOT_FOUND`).

Normaliza: `PREPAID => PAID`, `CASH_ON_DELIVERY => PENDING`.

## Niveles de retorno

Los códigos de arriba son `OperationResult` a nivel de **módulo de dominio**.
La **fachada** del SDD §7 los traduce a `bool` (mutaciones) o `std::optional`
(consultas) para no exponer detalles internos. Ninguna de las dos capas lanza
excepciones de negocio.

## Finanzas

`totalCents = baseCents + (HOME_DELIVERY ? deliverySurchargeCents : 0)`.

Todo en centavos `std::int64_t`. Nunca `double`.

## Prioridad de despacho

Min-heap por `ServiceLevel` (1 = EXPRESS_24H, 2 = PRIORITY, 3 = STANDARD) y
desempate FIFO por `createdAt` ascendente.

## Auditoría rápida

```bash
bash scripts/check_domain.sh
grep -rn "CASH_ON_DELIVERY" tests/
grep -n "static_assert" tests/types_test.cpp
```
