cmake_minimum_required(VERSION 3.20)

project(NodeDispatch
    VERSION 0.1.0
    DESCRIPTION "Core Engine de Logistica y Distribucion de Paquetes"
    LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Estandar y configuracion general
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Tipo de build" FORCE)
endif()

# -----------------------------------------------------------------------------
# Dependencia obligatoria: DeSiGNAR v4.0.1 (C++17)
#
# DeSiGNAR no es header-only: se compila como libreria estatica con el target
# Designar::Designar. Se desactivan sus pruebas y ejemplos para no arrastrar
# tiempo de build ni dependencias ajenas al proyecto.
# -----------------------------------------------------------------------------
include(FetchContent)

set(DESIGNAR_BUILD_TESTS        OFF CACHE BOOL "" FORCE)
set(DESIGNAR_BUILD_SAMPLES      OFF CACHE BOOL "" FORCE)
set(DESIGNAR_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(Designar
    GIT_REPOSITORY https://github.com/R3mmurd/DeSiGNAR.git
    GIT_TAG        v4.0.1
    GIT_PROGRESS   TRUE)

FetchContent_MakeAvailable(Designar)

# -----------------------------------------------------------------------------
# SFML opcional (S3.10): esqueleto de presentacion para la demo.
# OFF por defecto: el nucleo no depende de SFML y el CI no lo requiere.
# Si se activa, se descarga la version fijada igual que DeSiGNAR.
# -----------------------------------------------------------------------------
option(BUILD_SFML "Construir demo SFML (opcional)" OFF)

if(BUILD_SFML)
    message(STATUS "SFML habilitado: descargando version fijada...")
    # Solo necesitamos graphics, window, system. Audio requiere OpenAL y no lo usamos.
    set(SFML_BUILD_AUDIO        OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_EXAMPLES     OFF CACHE BOOL "" FORCE)
    set(SFML_BUILD_DOC          OFF CACHE BOOL "" FORCE)
    set(SFML_INSTALL_PKGCONFIG  OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG        2.6.1
        GIT_PROGRESS   TRUE)

    FetchContent_MakeAvailable(sfml)

    # Tratar cabeceras de SFML como sistema para no ensuciar warnings propios.
    if(TARGET sfml-graphics)
        set_target_properties(sfml-graphics sfml-window sfml-system PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${sfml_SOURCE_DIR}/include")
    endif()
endif()

# -----------------------------------------------------------------------------
# Nucleo de NodeDispatch
#
# Libreria estatica con los modulos implementados. Las cabeceras publicas se
# exponen a traves de `include/`.
# -----------------------------------------------------------------------------

# DeSiGNAR es codigo de terceros: sus cabeceras se tratan como "sistema" para
# que sus propias advertencias no ensucien el build de NodeDispatch.
if(DEFINED designar_SOURCE_DIR)
    set_target_properties(Designar PROPERTIES
        INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${designar_SOURCE_DIR}/include")
endif()

add_library(nodedispatch_warnings INTERFACE)
if(MSVC)
    target_compile_options(nodedispatch_warnings INTERFACE /W4)
else()
    target_compile_options(nodedispatch_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_library(nodedispatch_core STATIC
    src/audit_log.cpp
    src/client_index.cpp
    src/dispatch_cmp.cpp
    src/dispatch_queue.cpp
    src/geographic_hierarchy.cpp
    src/inventory_index.cpp
    src/tracking_code.cpp
    src/tracking_engine.cpp)
target_include_directories(nodedispatch_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
target_compile_features(nodedispatch_core PUBLIC cxx_std_17)
target_link_libraries(nodedispatch_core PUBLIC Designar::Designar)
target_link_libraries(nodedispatch_core PRIVATE nodedispatch_warnings)

# -----------------------------------------------------------------------------
# Pruebas
# -----------------------------------------------------------------------------
include(CTest)

if(BUILD_TESTING)
    # doctest: framework de cabecera unica para las pruebas del nucleo (SDD 9).
    # Se fija el tag, igual que DeSiGNAR, para que el build sea reproducible.
    set(DOCTEST_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(DOCTEST_WITH_MAIN_IN_STATIC_LIB OFF CACHE BOOL "" FORCE)
    set(DOCTEST_NO_INSTALL ON CACHE BOOL "" FORCE)

    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.5.3
        GIT_PROGRESS   TRUE)

    FetchContent_MakeAvailable(doctest)

    # doctest es codigo de terceros: sus cabeceras se tratan como "sistema" para
    # que sus propias advertencias no ensucien el build de NodeDispatch.
    if(TARGET doctest)
        set_target_properties(doctest PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${doctest_SOURCE_DIR}")
    endif()

    # Suite principal: un solo ejecutable con todos los TEST_CASE del dominio.
    add_executable(nodedispatch_tests
        tests/audit_log_test.cpp
        tests/client_index_test.cpp
        tests/doctest_main.cpp
        tests/dispatch_cmp_test.cpp
        tests/dispatch_queue_test.cpp
        tests/facade_test.cpp
        tests/framework_test.cpp
        tests/inventory_test.cpp
        tests/tracking_code_test.cpp
        tests/tracking_engine_test.cpp
        tests/types_test.cpp)
    target_link_libraries(nodedispatch_tests PRIVATE
        nodedispatch_core nodedispatch_warnings doctest::doctest)

    # Un caso de CTest por cada TEST_CASE, con el prefijo "unit.".
    include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
    doctest_discover_tests(nodedispatch_tests TEST_PREFIX "unit.")

    # S1.0: smoke test de las cinco estructuras de DeSiGNAR.
    add_executable(smoke_test tests/smoke_test.cpp)
    target_link_libraries(smoke_test PRIVATE nodedispatch_core nodedispatch_warnings)
    add_test(NAME smoke_test COMMAND smoke_test)

    add_executable(hierarchy_test tests/hierarchy_test.cpp)
    target_link_libraries(hierarchy_test PRIVATE nodedispatch_core nodedispatch_warnings)
    add_test(NAME hierarchy_test COMMAND hierarchy_test)
endif()

# -----------------------------------------------------------------------------
# Demo SFML (S3.10): solo si BUILD_SFML=ON.
# Vive en demo/ para no meter SFML ni punto flotante en el nucleo.
# -----------------------------------------------------------------------------
if(BUILD_SFML)
    add_executable(nodedispatch_gui demo/sfml_demo.cpp)
    target_link_libraries(nodedispatch_gui PRIVATE
        nodedispatch_core nodedispatch_warnings
        sfml-graphics sfml-window sfml-system)
endif()
