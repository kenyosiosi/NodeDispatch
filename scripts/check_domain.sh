#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# NodeDispatch - Verificacion de reglas duras del dominio
#
# Uso:  bash scripts/check_domain.sh
# Sale con 0 si todo esta bien y con 1 si alguna regla se incumple.
#
# Es un chequeo estatico APROXIMADO, no un compilador ni un reemplazo de los
# tests. Lo que hace es buscar patrones sobre el codigo sin comentarios:
#
#   - No entiende comentarios de bloque (/* */): el nucleo no los usa.
#   - No distingue un identificador de una cadena de texto.
#   - La regla COD se comprueba por convencion (ver regla 3), no ejecutando
#     nada: la validacion real es el TEST_CASE que corre en CTest.
#
# Cuando el chequeo y los tests discrepen, la verdad la tienen los tests.
# -----------------------------------------------------------------------------

set -uo pipefail

fail=0
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# Archivos de codigo del nucleo, ordenados para que la salida sea estable.
core_files() {
    find include src -type f \( -name '*.hpp' -o -name '*.cpp' \) 2>/dev/null | sort
}

# Imprime "archivo:linea:contenido" de las lineas que casan el patron, quitando
# antes los comentarios de linea. Sin esto, un comentario que menciona
# std::vector o throw daba un falso positivo.
scan() {
    local pattern="$1" file
    while IFS= read -r file; do
        sed -E 's@//.*$@@' "$file" | grep -nE "$pattern" | sed "s@^@$file:@"
    done < <(core_files)
}

# --- 1. Contenedores STL prohibidos en el nucleo -----------------------------
# Tambien caza `using namespace std;`, que es la puerta habitual para escribir
# `vector<int>` sin el prefijo y saltarse esta comprobacion.
stl_pattern='\bstd::(vector|map|unordered_map|unordered_set|set|multiset|list|deque|queue|stack)[[:space:]]*<'
stl_pattern="$stl_pattern"'|using[[:space:]]+namespace[[:space:]]+std\b'
hits="$(scan "$stl_pattern")"

if [ -n "$hits" ]; then
    echo "FALLA: contenedor STL prohibido en el nucleo (usa DeSiGNAR)."
    echo "$hits"
    fail=1
fi

# --- 2. Punto flotante en importes (weightKg es la unica excepcion) ----------
# `double`/`float` ademas de literales con decimales: `auto ratio = 1.5;` no
# menciona el tipo y aun asi es punto flotante.
float_pattern='\b(double|float)\b|[0-9]+\.[0-9]+'
hits="$(scan "$float_pattern" | grep -v 'weightKg')"

if [ -n "$hits" ]; then
    echo "FALLA: punto flotante fuera de weightKg; el dinero va en int64_t."
    echo "$hits"
    fail=1
fi

# --- 3. La regla COD debe tener cobertura en tests/ --------------------------
# Se exige a partir de S3.6. Antes de esa tarjeta se avisa pero no se falla:
# un chequeo en rojo durante tres semanas se termina ignorando.
#
# Convencion: el caso cuenta solo si esta en un archivo que tiene un TEST_CASE
# y menciona COD y DELIVERED en codigo (no en un comentario). Asi, un archivo
# con la sola linea `// CASH_ON_DELIVERY se probara en S3.6` ya no lo aprueba.
cod_ok=0
while IFS= read -r file; do
    cod_refs="$(sed -E 's@//.*$@@' "$file" | grep -c 'CASH_ON_DELIVERY')"
    delivered_refs="$(sed -E 's@//.*$@@' "$file" | grep -c 'DELIVERED')"
    case_refs="$(sed -E 's@//.*$@@' "$file" | grep -c 'TEST_CASE')"

    if [ "$cod_refs" -gt 0 ] && [ "$delivered_refs" -gt 0 ] && [ "$case_refs" -gt 0 ]; then
        cod_ok=1
        echo "OK: la regla COD tiene cobertura en $file."
        break
    fi
done < <(find tests -type f -name '*.cpp' 2>/dev/null | sort)

if [ "$cod_ok" -eq 0 ]; then
    echo "PENDIENTE: la regla COD aun no tiene TEST_CASE que la cubra (obligatorio desde S3.6)."
fi

# --- 4. La API publica no lanza excepciones de negocio -----------------------
hits="$(scan '\bthrow\b')"

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
# Se acepta `struct Package` y `class Package`: la regla es donde vive el tipo,
# no con que palabra clave se declara.
if [ -f include/nodedispatch/types.hpp ]; then
    if grep -qE '\b(struct|class)[[:space:]]+Package\b' include/nodedispatch/types.hpp; then
        echo "FALLA: Package debe vivir en package.hpp, no en types.hpp."
        fail=1
    fi
fi

# --- Resultado --------------------------------------------------------------
if [ "$fail" -eq 0 ]; then
    echo "OK: reglas de dominio verificadas."
fi

exit "$fail"
