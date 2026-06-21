QT       += core gui sql widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Разрешить использование устаревших API, если таковые появятся
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    main.cpp \
    dbmanager.cpp \
    loginwindow.cpp \
    mainwindow.cpp

HEADERS += \
    models.h \
    dbmanager.h \
    loginwindow.h \
    mainwindow.h

# Файлы разметки и ресурсов
DISTFILES += \
    style.qss

# Имя исполняемого файла
TARGET = kurs_bd
TEMPLATE = app

# Настройки для сборщика MSVC или MinGW
win32: {
    # Для Windows добавляем поддержку консольного вывода при отладке (опционально)
    # CONFIG += console
}
