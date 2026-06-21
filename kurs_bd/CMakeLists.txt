cmake_minimum_required(VERSION 3.16)
project(kurs_bd VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Включение автоматической генерации метаобъектов Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Добавление путей поиска Qt для Windows (например, MSYS2 MinGW)
if(NOT CMAKE_PREFIX_PATH AND EXISTS "C:/msys64/mingw64")
    list(APPEND CMAKE_PREFIX_PATH "C:/msys64/mingw64")
endif()

# Поиск пакетов Qt6, если не найдены - пробуем Qt5
find_package(Qt6 COMPONENTS Core Widgets Sql QUIET)
if(Qt6_FOUND)
    message(STATUS "Found Qt6")
    set(QT_LIBRARIES Qt6::Core Qt6::Widgets Qt6::Sql)
else()
    find_package(Qt5 COMPONENTS Core Widgets Sql REQUIRED)
    message(STATUS "Found Qt5")
    set(QT_LIBRARIES Qt5::Core Qt5::Widgets Qt5::Sql)
endif()

# Описание исполняемого файла и исходников
add_executable(kurs_bd
    main.cpp
    models.h
    dbmanager.h
    dbmanager.cpp
    loginwindow.h
    loginwindow.cpp
    mainwindow.h
    mainwindow.cpp
    style.qss
)

# Линковка библиотек Qt
target_link_libraries(kurs_bd PRIVATE ${QT_LIBRARIES})

# Копирование файла стилей в директорию сборки для удобства запуска
add_custom_command(TARGET kurs_bd POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/style.qss"
    "$<TARGET_FILE_DIR:kurs_bd>/style.qss"
)
