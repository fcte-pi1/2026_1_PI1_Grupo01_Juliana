# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Reaproveita os componentes ja validados na base de navegacao (teste_navegacao).
set(EXTRA_COMPONENT_DIRS
        ../odometria/components/movimentacao
        ../odometria/components/odometria
        ../odometria/components/m_driver
        ../odometria/components/encoder
        )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(teste_floodfill)
