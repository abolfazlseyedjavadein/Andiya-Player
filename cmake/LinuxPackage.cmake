# CPack-only desktop integration for the /opt installation.
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/../packaging/linux/io.andiya.player.desktop"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/applications")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/../assets/andiya-icon.png"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/usr/share/icons/hicolor/256x256/apps"
    RENAME io.andiya.player.png)
