


cmake_minimum_required(VERSION 3.16)
project(SnipOCR LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6
set(CMAKE_PREFIX_PATH "D:/Qt/6.8.3/msvc2022_64")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg)

# nlohmann/json (header-only)
include_directories(${CMAKE_SOURCE_DIR}/third_party)
# Allow includes like "core/xxx.h" / "ui/xxx.h" / "snap_ocr_app.h" from anywhere
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/core ${CMAKE_SOURCE_DIR}/ui)


# ONNX Runtime 1.18.1
set(ORT_DIR ${CMAKE_SOURCE_DIR}/third_party/onnxruntime-win-x64-1.18.1)
include_directories(${ORT_DIR}/include)
link_directories(${ORT_DIR}/lib)

# OpenCV 4.10.0 (lite: core + imgproc + imgcodecs only, ~9.5MB DLL set)
set(OPENCV_DIR ${CMAKE_SOURCE_DIR}/third_party/opencv/build_lite/install)
include_directories(${OPENCV_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/opencv/sources/3rdparty/quirc/include)
link_directories(${OPENCV_DIR}/x64/vc16/lib)

# zlib (from OpenCV build) - used by minizip
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/opencv/sources/3rdparty/zlib)
set(ZLIB_INCLUDE_DIR2 ${CMAKE_SOURCE_DIR}/third_party/opencv/build_lite/3rdparty/zlib)
set(ZLIB_LIB ${CMAKE_SOURCE_DIR}/third_party/opencv/build_lite/3rdparty/lib/Release/zlib.lib)

# minizip (for updater zip extraction)
set(MINIZIP_DIR ${CMAKE_SOURCE_DIR}/third_party/minizip)

set(SOURCES
    main.cpp
    snap_ocr_app.cpp
    core/crypto.cpp
    core/resource_helper.cpp
    core/config_manager.cpp
    core/i18n.cpp
    core/locale_res_loader.cpp
    core/img_res_loader.cpp
    core/ui_theme.cpp
    core/sys_env_helper.cpp
    core/region_router.cpp
    core/model_pack_reader.cpp
    core/ocr_engine.cpp
    core/ocr_worker.cpp
    core/screen_capture.cpp
    core/sticker_window.cpp
    core/hotkey_manager.cpp
    core/system_tray.cpp
    core/delay_capture.cpp
    ui/ocr_result_dialog.cpp
    ui/settings_dialog.cpp
    ui/about_dialog.cpp
    ui/license_dialog.cpp
    core/updater.cpp
    ui/update_dialog.cpp
    third_party/opencv/sources/3rdparty/quirc/src/quirc.c
    third_party/opencv/sources/3rdparty/quirc/src/decode.c
    third_party/opencv/sources/3rdparty/quirc/src/version_db.c
    third_party/opencv/sources/3rdparty/quirc/src/identify.c
)

set(HEADERS
    snap_ocr_app.h
    core/constants.h
    core/crypto.h
    core/resource_helper.h
    core/config_manager.h
    core/i18n.h
    core/locale_res_loader.h
    core/img_res_loader.h
    core/ui_theme.h
    core/sys_env_helper.h
    core/region_router.h
    core/model_pack_reader.h
    core/ocr_engine.h
    core/ocr_worker.h
    core/screen_capture.h
    core/sticker_window.h
    core/hotkey_manager.h
    core/system_tray.h
    core/delay_capture.h
    ui/ocr_result_dialog.h
    ui/settings_dialog.h
    ui/about_dialog.h
    ui/license_dialog.h
    core/updater.h
    ui/update_dialog.h
)

add_executable(SnipOCR ${SOURCES} ${HEADERS} SnipOCR.rc)


# ONNX Runtime
target_link_libraries(SnipOCR PRIVATE onnxruntime)

# OpenCV (lite build: core + imgproc + imgcodecs only, ~9.5MB total)
# All configs link release libs; force release CRT below.
target_link_libraries(SnipOCR PRIVATE
    opencv_core4100
    opencv_imgproc4100
    opencv_imgcodecs4100
)

target_link_libraries(SnipOCR PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Svg
    bcrypt user32 gdi32 shell32 ole32 uuid
)

if(WIN32)
    set_target_properties(SnipOCR PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()

# Post-build: copy DLLs to output dir
add_custom_command(TARGET SnipOCR POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORT_DIR}/lib/onnxruntime.dll"
        $<TARGET_FILE_DIR:SnipOCR>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ORT_DIR}/lib/onnxruntime_providers_shared.dll"
        $<TARGET_FILE_DIR:SnipOCR>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DIR}/x64/vc16/bin/opencv_core4100.dll"
        $<TARGET_FILE_DIR:SnipOCR>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DIR}/x64/vc16/bin/opencv_imgproc4100.dll"
        $<TARGET_FILE_DIR:SnipOCR>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${OPENCV_DIR}/x64/vc16/bin/opencv_imgcodecs4100.dll"
        $<TARGET_FILE_DIR:SnipOCR>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/.version.json"
        $<TARGET_FILE_DIR:SnipOCR>
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:SnipOCR>/locales"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/de.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/de.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/en_US.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/en_US.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/es.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/es.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/fr.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/fr.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/ja.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/ja.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/ko.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/ko.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/locales_res.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/locales_res.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/pt.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/pt.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/ru.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/ru.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/zh_CN.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/zh_CN.bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/locales/zh_TW.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/locales/zh_TW.bin"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:SnipOCR>/img"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/img/img_res.bin"
        "$<TARGET_FILE_DIR:SnipOCR>/img"
)

# ─── Standalone updater executable (minizip-based) ───
# Only build if minizip source files exist; otherwise skip gracefully.
if(EXISTS "${MINIZIP_DIR}/ioapi.c" AND EXISTS "${MINIZIP_DIR}/unzip.c")
    add_executable(updater
        updater_main.cpp
        updater.rc
        ${MINIZIP_DIR}/ioapi.c
        ${MINIZIP_DIR}/unzip.c
    )
    target_include_directories(updater PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${MINIZIP_DIR}
        ${ZLIB_INCLUDE_DIR}
        ${ZLIB_INCLUDE_DIR2}
    )
    target_link_libraries(updater PRIVATE
        ${ZLIB_LIB}
        user32 shell32
    )
    set_target_properties(updater PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    # Post-build: copy updater.exe to SnipOCR output dir
    add_custom_command(TARGET updater POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:updater>
            $<TARGET_FILE_DIR:SnipOCR>
    )
else()
    message(WARNING "minizip not found at ${MINIZIP_DIR} - skipping updater build")
endif()

