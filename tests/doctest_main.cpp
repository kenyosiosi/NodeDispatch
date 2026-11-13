// -----------------------------------------------------------------------------
// NodeDispatch - Punto de entrada de la suite de pruebas
//
// Un unico ejecutable agrupa todos los TEST_CASE del proyecto. doctest define
// main() aqui; los demas archivos de prueba solo incluyen <doctest/doctest.h>.
//
// Compilar y ejecutar:
//   cmake -S . -B build
//   cmake --build build
//   ctest --test-dir build --output-on-failure
// -----------------------------------------------------------------------------

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
