add_subdirectory(icons)

########### next target ###############
add_executable(ukui_kwin5_update_default_rules update_default_rules.cpp)
target_link_libraries(ukui_kwin5_update_default_rules
    KF5::ConfigCore
    Qt5::Core
    Qt5::DBus
)
install(TARGETS ukui_kwin5_update_default_rules DESTINATION ${LIB_INSTALL_DIR}/kconf_update_bin/)

########### install files ###############

install(FILES org_ukui_kwin.categories DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR})
install(FILES ukui-kwin.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/share/applications)
install(FILES ukui-kwinrc DESTINATION /etc/xdg)
