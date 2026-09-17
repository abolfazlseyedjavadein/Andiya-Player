cmake_minimum_required(VERSION 3.24)

project(Andiya VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep Linux executables separate from the generated Andiya QML module directory.
if(UNIX AND NOT APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS
    Quick
    QuickControls2
    Multimedia
    Network
)

include(GNUInstallDirs)
qt_standard_project_setup(REQUIRES 6.8)

set_source_files_properties(qml/WorkspaceDialog.qml PROPERTIES QT_RESOURCE_ALIAS WorkspaceDialog.qml)
set_source_files_properties(qml/Main.qml PROPERTIES QT_RESOURCE_ALIAS Main.qml)
set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_RESOURCE_ALIAS Theme.qml
)
set_source_files_properties(qml/TimeFormat.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_RESOURCE_ALIAS TimeFormat.qml
)
set_source_files_properties(qml/SettingsDialog.qml PROPERTIES QT_RESOURCE_ALIAS SettingsDialog.qml)
set_source_files_properties(qml/AboutDialog.qml PROPERTIES QT_RESOURCE_ALIAS AboutDialog.qml)
set_source_files_properties(qml/ShortcutsDialog.qml PROPERTIES QT_RESOURCE_ALIAS ShortcutsDialog.qml)
set_source_files_properties(qml/NetworkStreamDialog.qml PROPERTIES QT_RESOURCE_ALIAS NetworkStreamDialog.qml)
set_source_files_properties(qml/components/GlassPanel.qml PROPERTIES QT_RESOURCE_ALIAS GlassPanel.qml)
set_source_files_properties(qml/components/IconButton.qml PROPERTIES QT_RESOURCE_ALIAS IconButton.qml)
set_source_files_properties(qml/components/ThemedMenu.qml PROPERTIES QT_RESOURCE_ALIAS ThemedMenu.qml)
set_source_files_properties(qml/components/ThemedMenuItem.qml PROPERTIES QT_RESOURCE_ALIAS ThemedMenuItem.qml)
set_source_files_properties(qml/components/ThemedMenuSeparator.qml PROPERTIES QT_RESOURCE_ALIAS ThemedMenuSeparator.qml)
set_source_files_properties(qml/components/MediaStage.qml PROPERTIES QT_RESOURCE_ALIAS MediaStage.qml)
set_source_files_properties(qml/components/PlaybackOverlay.qml PROPERTIES QT_RESOURCE_ALIAS PlaybackOverlay.qml)
set_source_files_properties(qml/components/CompareView.qml PROPERTIES QT_RESOURCE_ALIAS CompareView.qml)
set_source_files_properties(qml/components/TimelineControls.qml PROPERTIES QT_RESOURCE_ALIAS TimelineControls.qml)
set_source_files_properties(qml/components/DisplayAdjustments.qml PROPERTIES QT_RESOURCE_ALIAS DisplayAdjustments.qml)
set_source_files_properties(qml/components/NavRailButton.qml PROPERTIES QT_RESOURCE_ALIAS NavRailButton.qml)
set_source_files_properties(qml/panels/PluginDrawer.qml PROPERTIES QT_RESOURCE_ALIAS PluginDrawer.qml)
set_source_files_properties(qml/panels/FilterGraphDialog.qml PROPERTIES QT_RESOURCE_ALIAS FilterGraphDialog.qml)
set_source_files_properties(qml/panels/ScreenshotsPanel.qml PROPERTIES QT_RESOURCE_ALIAS ScreenshotsPanel.qml)
set_source_files_properties(qml/panels/MediaLibraryPanel.qml PROPERTIES QT_RESOURCE_ALIAS MediaLibraryPanel.qml)

qt_add_executable(Andiya WIN32
    src/SelfTests.cpp
    src/SelfTests.h
    src/main.cpp
    src/player/PlayerController.cpp
    src/player/PlayerController.h
    src/player/ThumbnailProvider.cpp
    src/player/ThumbnailProvider.h
    src/player/ScreenshotListModel.cpp
    src/player/ScreenshotListModel.h
    src/plugins/PluginListModel.cpp
    src/plugins/PluginListModel.h
    src/plugins/FilterPipeline.cpp
    src/plugins/FilterPipeline.h
    src/plugins/PluginValidation.cpp
    src/plugins/PluginValidation.h
    src/player/FrameStore.cpp
    src/player/FrameStore.h
    src/player/ExportQueue.cpp
    src/player/ExportQueue.h
    src/player/StudioController.cpp
    src/player/StudioController.h
    src/plugins/PythonImageFilterHost.cpp
    src/plugins/PythonImageFilterHost.h
    include/andiya/plugin_api.h
)

qt_add_resources(Andiya "andiya_assets" PREFIX "/" FILES assets/andiya-icon.png)

qt_add_qml_module(Andiya
    URI Andiya
    VERSION 1.0
    QML_FILES
        qml/WorkspaceDialog.qml
        qml/Main.qml
        qml/Theme.qml
        qml/TimeFormat.qml
        qml/SettingsDialog.qml
        qml/AboutDialog.qml
        qml/ShortcutsDialog.qml
        qml/NetworkStreamDialog.qml
        qml/components/GlassPanel.qml
        qml/components/IconButton.qml
        qml/components/ThemedMenu.qml
        qml/components/ThemedMenuItem.qml
        qml/components/ThemedMenuSeparator.qml
        qml/components/MediaStage.qml
        qml/components/PlaybackOverlay.qml
        qml/components/CompareView.qml
        qml/components/TimelineControls.qml
        qml/components/DisplayAdjustments.qml
        qml/components/NavRailButton.qml
        qml/panels/PluginDrawer.qml
        qml/panels/FilterGraphDialog.qml
        qml/panels/ScreenshotsPanel.qml
        qml/panels/MediaLibraryPanel.qml
)

target_include_directories(Andiya PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(Andiya PRIVATE
    Qt6::Quick
    Qt6::Network
    Qt6::QuickControls2
    Qt6::Multimedia
)

add_executable(AndiyaAudioProbe EXCLUDE_FROM_ALL
    tools/audio_probe.cpp
)

target_link_libraries(AndiyaAudioProbe PRIVATE
    Qt6::Multimedia
)

add_library(AndiyaSoftContrastPlugin SHARED
    plugins/soft-contrast/plugin.cpp
    include/andiya/plugin_api.h
)

target_link_libraries(AndiyaSoftContrastPlugin PRIVATE Qt6::Core)

target_include_directories(AndiyaSoftContrastPlugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(ANDIYA_SAMPLE_PLUGIN_DIRECTORY
    "${CMAKE_BINARY_DIR}/plugins/example.soft-contrast")
file(MAKE_DIRECTORY "${ANDIYA_SAMPLE_PLUGIN_DIRECTORY}")
configure_file(
    plugins/soft-contrast/plugin.json.in
    "${ANDIYA_SAMPLE_PLUGIN_DIRECTORY}/plugin.json"
    @ONLY
)

set_target_properties(AndiyaSoftContrastPlugin PROPERTIES
    OUTPUT_NAME "soft_contrast"
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}"
    RUNTIME_OUTPUT_DIRECTORY "$<1:${ANDIYA_SAMPLE_PLUGIN_DIRECTORY}>"
    LIBRARY_OUTPUT_DIRECTORY "$<1:${ANDIYA_SAMPLE_PLUGIN_DIRECTORY}>"
)

add_dependencies(Andiya AndiyaSoftContrastPlugin)
add_custom_command(TARGET Andiya POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Andiya>/plugins/example.soft-contrast"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:AndiyaSoftContrastPlugin>"
        "$<TARGET_FILE_DIR:Andiya>/plugins/example.soft-contrast/$<TARGET_FILE_NAME:AndiyaSoftContrastPlugin>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ANDIYA_SAMPLE_PLUGIN_DIRECTORY}/plugin.json"
        "$<TARGET_FILE_DIR:Andiya>/plugins/example.soft-contrast/plugin.json"
    VERBATIM
)

# The Python plugin bridge: the runner script Andiya spawns for every
# enabled Python plugin, plus a minimal example plugin that uses it.
add_custom_command(TARGET Andiya POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Andiya>/python_runtime"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime/python/andiya_python_host.py"
        "$<TARGET_FILE_DIR:Andiya>/python_runtime/andiya_python_host.py"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:Andiya>/plugins/example.python-grayscale"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins/python-grayscale/plugin.json"
        "$<TARGET_FILE_DIR:Andiya>/plugins/example.python-grayscale/plugin.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/plugins/python-grayscale/filter.py"
        "$<TARGET_FILE_DIR:Andiya>/plugins/example.python-grayscale/filter.py"
    VERBATIM
)

set_target_properties(Andiya PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "Andiya"
    MACOSX_BUNDLE_GUI_IDENTIFIER "io.andiya.player"
)

if(WIN32)
    target_sources(Andiya PRIVATE assets/andiya.rc)
    target_link_libraries(Andiya PRIVATE crypt32)
endif()
add_executable(AndiyaFilterWorker src/plugins/FilterWorker.cpp src/plugins/NativeImageFilterHost.cpp)
target_include_directories(AndiyaFilterWorker PRIVATE src include)
target_link_libraries(AndiyaFilterWorker PRIVATE Qt6::Gui)
add_executable(AndiyaPluginValidator tools/validate_plugin.cpp src/plugins/PluginValidation.cpp)
target_include_directories(AndiyaPluginValidator PRIVATE src)
target_link_libraries(AndiyaPluginValidator PRIVATE Qt6::Core)
if(WIN32)
    target_link_libraries(AndiyaPluginValidator PRIVATE crypt32)
endif()
add_dependencies(Andiya AndiyaFilterWorker AndiyaPluginValidator)
include(cmake/Packaging.cmake)
