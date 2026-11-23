// -----------------------------------------------------------------------------
// NodeDispatch - Pruebas de ClientIndex (N1)
//
// Cubre el alta, el rechazo de duplicados y de identificaciones vacias, la
// busqueda como copia segura, la baja y el vaciado. No prueba el rehashing
// interno de DeSiGNAR: eso es de la biblioteca, no del dominio.
// -----------------------------------------------------------------------------

#include <doctest/doctest.h>

#include "nodedispatch/client_index.hpp"

using namespace nodedispatch;

namespace {

Client makeClient(const std::string& nationalId, const std::string& firstName)
{
    Client client;
    client.nationalId = nationalId;
    client.firstName = firstName;
    client.lastName = "Perez";
    client.phone = "+58-412-1234567";
    client.address.state = "Distrito Capital";
    client.address.city = "Caracas";
    client.address.exactAddress = "Calle 10, Casa 3";
    client.address.postalCode = "1010";
    return client;
}

} // namespace

TEST_SUITE("dominio/client_index") {

TEST_CASE("ClientIndex da de alta y encuentra por identificacion (N1)")
{
    ClientIndex index;

    CHECK(index.size() == 0);
    CHECK(index.add(makeClient("V-12345678", "Ana")) == OperationResult::OK);
    CHECK(index.add(makeClient("V-87654321", "Luis")) == OperationResult::OK);
    CHECK(index.size() == 2);

    const auto found = index.find("V-12345678");
    REQUIRE(found.has_value());
    CHECK(found->firstName == "Ana");
    CHECK(found->address.city == "Caracas");

    CHECK_FALSE(index.find("V-00000000").has_value());
    CHECK(index.contains("V-87654321"));
    CHECK_FALSE(index.contains("V-00000000"));
}

TEST_CASE("ClientIndex rechaza identificacion vacia y cliente repetido (N1)")
{
    ClientIndex index;

    CHECK(index.add(makeClient("", "Sin cedula")) == OperationResult::INVALID_INPUT);
    CHECK(index.size() == 0);

    CHECK(index.add(makeClient("V-12345678", "Ana")) == OperationResult::OK);
    // El mismo id con otro nombre no reemplaza: se rechaza.
    CHECK(index.add(makeClient("V-12345678", "Ana Maria")) == OperationResult::DUPLICATE);
    CHECK(index.size() == 1);

    const auto found = index.find("V-12345678");
    REQUIRE(found.has_value());
    CHECK(found->firstName == "Ana");
    CHECK_FALSE(index.find("").has_value());
    CHECK_FALSE(index.contains(""));
}

TEST_CASE("la copia devuelta no modifica el indice (N1)")
{
    ClientIndex index;
    index.add(makeClient("V-12345678", "Ana"));

    auto copy = index.find("V-12345678");
    REQUIRE(copy.has_value());
    copy->phone = "+58-414-0000000";

    const auto again = index.find("V-12345678");
    REQUIRE(again.has_value());
    CHECK(again->phone == "+58-412-1234567");
}

TEST_CASE("ClientIndex elimina, mide y se vacia (N1)")
{
    ClientIndex index;
    index.add(makeClient("V-1", "Uno"));
    index.add(makeClient("V-2", "Dos"));

    CHECK(index.remove("V-1"));
    CHECK_FALSE(index.remove("V-1")); // ya no esta
    CHECK_FALSE(index.remove("V-404"));
    CHECK(index.size() == 1);
    CHECK_FALSE(index.contains("V-1"));

    index.clear();
    CHECK(index.size() == 0);
    CHECK_FALSE(index.find("V-2").has_value());

    // Sigue utilizable despues de vaciarse.
    CHECK(index.add(makeClient("V-3", "Tres")) == OperationResult::OK);
    CHECK(index.size() == 1);
}

TEST_CASE("ClientIndex sostiene mil clientes (N1)")
{
    ClientIndex index;

    for (int i = 0; i < 1000; ++i)
    {
        const std::string id = "V-" + std::to_string(10000000 + i);
        REQUIRE(index.add(makeClient(id, "Cliente " + std::to_string(i))) == OperationResult::OK);
    }

    CHECK(index.size() == 1000);
    for (int i = 0; i < 1000; i += 97)
    {
        const auto found = index.find("V-" + std::to_string(10000000 + i));
        REQUIRE(found.has_value());
        CHECK(found->firstName == "Cliente " + std::to_string(i));
    }
}

} // TEST_SUITE
