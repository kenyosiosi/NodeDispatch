#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# NodeDispatch - Verificacion de reglas duras del dominio
#
# Uso:  bash scripts/check_domain.sh
# Sale con 0 si todo esta bien y con 1 si alguna regla se incumple.
#
# Es un chequeo estatico rapido, no reemplaza a los tests: la regla COD se
# valida de verdad en tests/. Aqui solo se comprueba que exista cobertura.
# -----------------------------------------------------------------------------

set -uo pipefail

fail=0
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# --- 1. Contenedores STL prohibidos en el nucleo -----------------------------
hits="$(grep -rnE \
    'std::(vector|map|unordered_map|unordered_set|set|multiset|list|deque|queue|stack)\b' \
    include src 2>/dev/null | grep -vE '^[^:]+:[0-9]+:[[:space:]]*//')"

if [ -n "$hits" ]; then
    echo "FALLA: contenedor STL prohibido en el nucleo (usa DeSiGNAR)."
    echo "$hits"
    fail=1
fi

# --- 2. Punto flotante en importes (weightKg es la unica excepcion) ----------
hits="$(grep -rnE '\b(double|float)\b' include src 2>/dev/null \
    | grep -vE 'weightKg' \
    | grep -vE '^[^:]+:[0-9]+:[[:space:]]*(//|\*)')"

if [ -n "$hits" ]; then
    echo "FALLA: punto flotante fuera de weightKg; el dinero va en int64_t."
    echo "$hits"
    fail=1
fi

# --- 3. La regla COD debe tener cobertura en tests/ --------------------------
# Se exige a partir de S3.6. Antes de esa tarjeta se avisa pero no se falla:
# un chequeo en rojo durante tres semanas se termina ignorando.
if ! grep -rq 'CASH_ON_DELIVERY' tests 2>/dev/null; then
    echo "PENDIENTE: la regla COD aun no tiene test (obligatorio desde S3.6)."
fi

# --- 4. La API publica no lanza excepciones de negocio -----------------------
hits="$(grep -rnE '\bthrow\b' include src 2>/dev/null \
    | grep -vE '^[^:]+:[0-9]+:[[:space:]]*//')"

if [ -n "$hits" ]; then
    echo "FALLA: throw en codigo de dominio; devuelve OperationResult."
    echo "$hits"
    fail=1
fi

# --- 5. Invariantes de ServiceLevel ------------------------------------------
if [ -f tests/types_test.cpp ]; then
    if ! grep -q 'static_assert' tests/types_test.cpp; then
        echo "FALLA: tests/types_test.cpp perdio los static_assert de los enums."
        fail=1
    fi
fi

# --- 6. La cabecera de tipos no debe volverse un cajon de sastre -------------
if [ -f include/nodedispatch/types.hpp ]; then
    if grep -q 'struct Package' include/nodedispatch/types.hpp; then
        echo "FALLA: Package debe vivir en package.hpp, no en types.hpp."
        fail=1
    fi
fi

# --- Resultado --------------------------------------------------------------
if [ "$fail" -eq 0 ]; then
    echo "OK: reglas de dominio verificadas."
fi

exit "$fail"
