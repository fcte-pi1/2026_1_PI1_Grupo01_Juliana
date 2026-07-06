# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS
        ../odometria/components/movimentacao
        ../odometria/components/odometria
        ../odometria/components/m_driver
        ../odometria/components/encoder
        ../odometria/components/infrared
        ../odometria/components/power_module
        ../odometria/components/telemetry_data_nvs
        ../odometria/components/seletor_modo
        ../odometria/components/telemetria
        )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(teste_navegacao)
