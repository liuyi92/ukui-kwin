# KI18N Translation Domain for this library
add_definitions(-DTRANSLATION_DOMAIN=\"kcmkwincommon\")

include_directories(${UKUI_KWIN_SOURCE_DIR}/effects)

set(kcmkwincommon_SRC
    effectsmodel.cpp
)

qt5_add_dbus_interface(kcmkwincommon_SRC
    ${UKUI_KWIN_SOURCE_DIR}/org.ukui.kwin.Effects.xml kwin_effects_interface
)

add_library(kcmukuikwincommon SHARED ${kcmkwincommon_SRC})

target_link_libraries(kcmukuikwincommon
    Qt5::Core
    Qt5::DBus
    KF5::CoreAddons
    KF5::ConfigCore
    KF5::I18n
    KF5::Package
    KF5::KCMUtils
    kwin4_effect_builtins
)

set_target_properties(kcmukuikwincommon PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

install(TARGETS kcmukuikwincommon ${INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
