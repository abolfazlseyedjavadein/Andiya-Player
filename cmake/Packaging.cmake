# Native desktop install layout. Keep generated dependencies outside the source tree.
option(ANDIYA_DEPLOY_RUNTIME "Bundle Qt and media runtime dependencies on install" OFF)

if(APPLE)
    set(ANDIYA_RUNTIME_DIR "Andiya.app/Contents/MacOS")
    set(ANDIYA_DATA_DIR "Andiya.app/Contents/Resources")
    set_source_files_properties(${CMAKE_SOURCE_DIR}/assets/andiya.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    target_sources(Andiya PRIVATE ${CMAKE_SOURCE_DIR}/assets/andiya.icns)
    set_target_properties(Andiya PROPERTIES
        MACOSX_BUNDLE_ICON_FILE andiya.icns
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set_target_properties(AndiyaFilterWorker AndiyaPluginValidator PROPERTIES
        INSTALL_RPATH "@loader_path/../Frameworks")
    set_target_properties(AndiyaSoftContrastPlugin PROPERTIES
        INSTALL_RPATH "@loader_path/../../../Frameworks")
    add_custom_command(TARGET Andiya POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:AndiyaFilterWorker>" "$<TARGET_FILE_DIR:Andiya>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:AndiyaPluginValidator>" "$<TARGET_FILE_DIR:Andiya>"
        VERBATIM)
else()
    set(ANDIYA_RUNTIME_DIR "${CMAKE_INSTALL_BINDIR}")
    set(ANDIYA_DATA_DIR "${CMAKE_INSTALL_DATADIR}/andiya")
    if(UNIX)
        set_target_properties(Andiya AndiyaFilterWorker AndiyaPluginValidator PROPERTIES
            INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
        set_target_properties(AndiyaSoftContrastPlugin PROPERTIES
            INSTALL_RPATH "$ORIGIN/../../../${CMAKE_INSTALL_LIBDIR}")
        install(FILES packaging/linux/io.andiya.player.desktop
            DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
        install(FILES assets/andiya-256.png
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps"
            RENAME io.andiya.player.png)
    endif()
endif()

install(TARGETS Andiya BUNDLE DESTINATION . RUNTIME DESTINATION "${ANDIYA_RUNTIME_DIR}")
install(TARGETS AndiyaFilterWorker AndiyaPluginValidator RUNTIME DESTINATION "${ANDIYA_RUNTIME_DIR}")
install(TARGETS AndiyaSoftContrastPlugin
    RUNTIME DESTINATION "${ANDIYA_RUNTIME_DIR}/plugins/example.soft-contrast"
    LIBRARY DESTINATION "${ANDIYA_RUNTIME_DIR}/plugins/example.soft-contrast")
install(FILES "${ANDIYA_SAMPLE_PLUGIN_DIRECTORY}/plugin.json"
    DESTINATION "${ANDIYA_RUNTIME_DIR}/plugins/example.soft-contrast")
install(FILES runtime/python/andiya_python_host.py DESTINATION "${ANDIYA_RUNTIME_DIR}/python_runtime")
install(FILES plugins/python-grayscale/plugin.json plugins/python-grayscale/filter.py
    DESTINATION "${ANDIYA_RUNTIME_DIR}/plugins/example.python-grayscale")
install(FILES include/andiya/plugin_api.h DESTINATION "${ANDIYA_DATA_DIR}/sdk/include/andiya")
install(FILES tools/package_plugin.py DESTINATION "${ANDIYA_DATA_DIR}/sdk/tools")
install(FILES assets/andiya-icon.png assets/andiya.ico DESTINATION "${ANDIYA_DATA_DIR}/icons")
install(FILES LICENSE README.md PLUGIN_DEVELOPMENT.md THIRD_PARTY.md DESTINATION "${ANDIYA_DATA_DIR}")
install(DIRECTORY docs/ DESTINATION "${ANDIYA_DATA_DIR}/docs")

if(ANDIYA_DEPLOY_RUNTIME)
    if(WIN32)
        set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
        set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${ANDIYA_RUNTIME_DIR}")
        include(InstallRequiredSystemLibraries)
        qt_generate_deploy_qml_app_script(TARGET Andiya OUTPUT_SCRIPT deploy_script
            NO_COMPILER_RUNTIME)
    else()
        qt_generate_deploy_qml_app_script(TARGET Andiya OUTPUT_SCRIPT deploy_script)
    endif()
    install(SCRIPT "${deploy_script}")
endif()

set(CPACK_PACKAGE_NAME Andiya)
set(CPACK_PACKAGE_VENDOR "Andiya contributors")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open-source media player with frame review and extensible filters")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/packages")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_MONOLITHIC_INSTALL ON)
set(CPACK_PACKAGE_INSTALL_DIRECTORY Andiya)
set(CPACK_VERBATIM_VARIABLES YES)
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ANDIYA_PACKAGE_ARCH)
if(WIN32)
    set(ANDIYA_PACKAGE_ARCH "x64")
    set(CPACK_GENERATOR ZIP)
elseif(APPLE)
    set(CPACK_GENERATOR DragNDrop)
    set(CPACK_DMG_VOLUME_NAME Andiya)
    set(CPACK_DMG_FORMAT UDZO)
else()
    set(ANDIYA_PACKAGE_ARCH "x86_64")
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/andiya")
    set(CPACK_DEBIAN_PACKAGE_NAME andiya)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Andiya contributors")
    set(CPACK_DEBIAN_PACKAGE_SECTION video)
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python3")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libc6 (>= 2.35), libstdc++6, libgl1, libegl1, libxkbcommon0, libxkbcommon-x11-0, libxcb-cursor0, libxcb-icccm4, libxcb-keysyms1, libxcb-shape0, libxcb-xinerama0, libxcb-randr0, libxcb-render-util0, libxcb-image0, libxcb-xfixes0, libfontconfig1, libdbus-1-3, libpulse0")
endif()
set(CPACK_PACKAGE_FILE_NAME "Andiya-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${ANDIYA_PACKAGE_ARCH}")
include(CPack)
