cmake_minimum_required(VERSION 3.1.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")

if(IS_PSIPLUS)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.psi-plus")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2001-2019 Psi IM and Psi+ developers")
else(IS_PSIPLUS)
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.psi-im")
    set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2001-2019 Psi IM developers")
endif(IS_PSIPLUS)

set(MACOSX_ICON "${PROJECT_SOURCE_DIR}/mac/application.icns")
set(MACOSX_BUNDLE_LONG_VERSION_STRING ${CLIENT_NAME} ${APP_VERSION})
set(MACOSX_BUNDLE_BUNDLE_NAME ${CLIENT_NAME})
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${APP_VERSION})
set(MACOSX_BUNDLE_BUNDLE_VERSION ${APP_VERSION})

if(${_qt5Core_install_prefix} STREQUAL "")
    message(WARNING "Warning! \"_qt5Core_install_prefix\" is not set, macdeployqt found in PATH will be used")
    set (MACDEPLOYQT "macdeployqt")
else(${_qt5Core_install_prefix} STREQUAL "")
    set (MACDEPLOYQT "${_qt5Core_install_prefix}/bin/macdeployqt")
endif(${_qt5Core_install_prefix} STREQUAL "")

if(CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CPACK_PACKAGE_FILE_NAME "${CLIENT_NAME}-${APP_VERSION}-macOS${CMAKE_OSX_DEPLOYMENT_TARGET}-${CMAKE_SYSTEM_PROCESSOR}")
else(CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CPACK_PACKAGE_FILE_NAME "${CLIENT_NAME}-${APP_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")
endif(CMAKE_OSX_DEPLOYMENT_TARGET)

set(CPACK_PACKAGE_ICON ${MACOSX_ICON})
set(CPACK_BUNDLE_ICON ${MACOSX_ICON})
set(CPACK_BUNDLE_NAME ${CLIENT_NAME})
set(CPACK_BINARY_DRAGNDROP ON)

include(CPack)
