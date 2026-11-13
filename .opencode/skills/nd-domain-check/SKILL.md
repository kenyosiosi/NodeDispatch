---
name: NodeDispatch Domain Check
description: Verifica las reglas duras del dominio antes de un PR (COD, enums, no-STL, dinero en centavos, API sin excepciones) y bloquea el merge si algo falla
---

# Verificación de reglas de dominio

Se ejecuta **antes de pedir revisión** de cualquier PR que toque `include/` o
`src/`. Es la salvaguarda de la nota de dominio.

## Pasos

1. Ejecuta el chequeo automático y muestra la salida completa:

   ```bash
   bash scripts/check_domain.sh
   ```

2. Comprueba que la regla COD tenga cobertura real:

   ```bash
   grep -rn "CASH_ON_DELIVERY" tests/
   ```

   Debe existir un test que intente entregar un paquete COD impago y espere
   `INVALID_STATE`.

3. Comprueba los invariantes de enum:

   ```bash
   grep -n "static_assert" tests/types_test.cpp
   ```

4. Contrasta la tabla de estados del código con `.opencode/rules/dominio.md`.
   Si difieren, gana el SDD §5 y hay que corregir el código o el SDD en el
   mismo PR.

5. Si algo falla: **no propongas merge**. Lista `archivo:línea`, la regla
   incumplida y la corrección mínima.

## Criterio de aprobación

| Chequeo | Obligatorio |
| :--- | :--- |
| Sin contenedores STL en `include/` y `src/` | sí |
| Sin `double`/`float` fuera de `weightKg` | sí |
| Sin `throw` en código de dominio | sí |
| Test que cubre la regla COD | sí |
| `static_assert` de `ServiceLevel` intacto | sí |
| Tabla de estados alineada con el SDD | sí |
