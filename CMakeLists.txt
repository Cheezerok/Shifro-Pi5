cmake_minimum_required(VERSION 3.10)
project(Shifro VERSION 1.0 LANGUAGES C CXX)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)


find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED libgpiod)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)


add_library(kuznechik_lib STATIC
    src/kuznechik.c
)

add_library(cmac_lib STATIC
    src/cmac.cpp
)

target_link_libraries(cmac_lib PRIVATE kuznechik_lib)

add_executable(shifro
    src/main.cpp
    src/encryption_app.cpp
    src/display_pi.cpp
    src/keyboard.cpp
    src/kuznechik.c
    src/cmac.cpp
    src/counter_mode.cpp
    src/spi_pi.cpp
)


target_link_libraries(shifro PRIVATE
    kuznechik_lib
    cmac_lib
    Qt5::Core
    Qt5::Widgets
    ${GPIOD_LIBRARIES}
    pthread
)

target_include_directories(shifro PRIVATE
    ${GPIOD_INCLUDE_DIRS}
)

install(TARGETS shifro DESTINATION bin)


set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-Wall -Wextra) 