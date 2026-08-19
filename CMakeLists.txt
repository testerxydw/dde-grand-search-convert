cmake_minimum_required(VERSION 3.10)
project(convert-search-plugin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# ── Qt 版本自动探测（Qt6 优先，回退 Qt5）──
if (NOT DEFINED QT_VERSION_MAJOR)
    find_package(Qt6 QUIET COMPONENTS Core Gui DBus Network)
    if (Qt6_FOUND)
        set(QT_VERSION_MAJOR 6)
        message(" >>> Auto detected Qt version: 6")
    else()
        find_package(Qt5 QUIET COMPONENTS Core Gui DBus Network)
        if (Qt5_FOUND)
            set(QT_VERSION_MAJOR 5)
            message(" >>> Auto detected Qt version: 5")
        else()
            message(FATAL_ERROR "Neither Qt6 nor Qt5 found!")
        endif()
    endif()
else()
    message(" >>> Using specified Qt version: ${QT_VERSION_MAJOR}")
endif()
set(QT_NS Qt${QT_VERSION_MAJOR})
find_package(${QT_NS} REQUIRED COMPONENTS Core Gui DBus Network)

# ── 头文件包含路径（根目录，供子目录引用 resultbuilder.h 等）──
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ── 源文件 ──
set(PLUGIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/convertsearch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/convertsearch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/queryparser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/queryparser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/resultbuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/resultbuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/unitprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/unitprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/timeprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/timeprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/currencyprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/currencyprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/calcprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/calcprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/programprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/programprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/datetimeprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/datetimeprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/colorprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/colorprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/energyprovider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/providers/energyprovider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/i18n.h
    ${CMAKE_CURRENT_SOURCE_DIR}/i18n.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/uosai.h
    ${CMAKE_CURRENT_SOURCE_DIR}/uosai.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/searchpluginadaptor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/searchpluginadaptor.cpp
)

add_executable(${PROJECT_NAME} ${PLUGIN_SOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_NS}::Core ${QT_NS}::Gui ${QT_NS}::DBus ${QT_NS}::Network)

# ── 安装 ──
include(GNUInstallDirs)
if(NOT DEFINED CONVERT_PLUGIN_DIR)
    set(CONVERT_PLUGIN_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/dde-grand-search-daemon/plugins/searcher)
endif()
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CONVERT_PLUGIN_DIR})

# 由 .in 模板生成配置文件
set(CONVERT_PLUGIN_EXEC "${CONVERT_PLUGIN_DIR}/${PROJECT_NAME}")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/convert-search.conf.in
    ${CMAKE_CURRENT_BINARY_DIR}/convert-search.conf
    @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/convert-search.conf DESTINATION ${CONVERT_PLUGIN_DIR})
