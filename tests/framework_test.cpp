// -----------------------------------------------------------------------------
// NodeDispatch - S1.2 Verificacion de la infraestructura de pruebas
//
// Estos casos comprueban que doctest quedo correctamente integrado con CMake y
// CTest: si fallan, el problema esta en el runner y no en el codigo de dominio.
// -----------------------------------------------------------------------------

#include <stdexcept>
#include <vector>

#include <doctest/doctest.h>

TEST_SUITE("framework") {

TEST_CASE("doctest se ejecuta bajo el runner de pruebas")
{
    CHECK(doctest::is_running_in_test);
}

TEST_CASE("las aserciones basicas distinguen exito de fallo")
{
    CHECK(1 + 1 == 2);
    CHECK_EQ(2 * 3, 6);
    CHECK_FALSE(false);
    CHECK_UNARY(true);
    CHECK_THROWS_AS(throw std::runtime_error("boom"), std::runtime_error);
}

TEST_CASE("las utilidades de la STL siguen disponibles")
{
    const std::vector<int> values{3, 1, 2};

    REQUIRE(values.size() == 3);
    CHECK(values[0] == 3);
    CHECK(values[2] == 2);
}

} // TEST_SUITE
