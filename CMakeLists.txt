# 设置 CMake 最低版本要求
cmake_minimum_required(VERSION 3.21)

# 设置项目名称
project("HuYanRead")

# 禁止在源代码目录中直接构建
if (${CMAKE_CURRENT_BINARY_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
    message(FATAL_ERROR "Using build_dir same as source_dir is forbidden")
endif()

# 打印当前目录信息
message("Current build  dir: " ${CMAKE_CURRENT_BINARY_DIR})
message("Current source dir: " ${CMAKE_CURRENT_SOURCE_DIR})

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_AUTOMOC ON)  # **开启 MOC 自动处理**
set(CMAKE_AUTOUIC ON)  # **开启 UI 自动处理**
set(CMAKE_AUTORCC ON)  # **开启资源文件自动处理**

# 查找 Qt5 组件
find_package(Qt5 COMPONENTS Core Gui Widgets WebEngineWidgets PrintSupport Network Concurrent REQUIRED)

# UI 相关
set(UI_FILES src/config/SettingsDialog.ui)

# 使用 qt5_wrap_ui 处理 UI 文件（CMake 会自动放到 `CMAKE_CURRENT_BINARY_DIR`）
qt5_wrap_ui(UI_HEADERS ${UI_FILES})

# 处理 Qt 资源文件
qt5_add_resources(QRC_FILES src/core/mainwindow.qrc)

# **重要：确保生成的 UI 头文件能够被找到**
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(MOC_HEADERS
    src/core/mainwindow.h
    src/core/TextDocumentModel.h
    src/core/TextReaderManager.h
    src/ui/chapterdialog.h
    src/ui/WebEngineView.h
    src/ui/TextReaderView.h
    src/ui/NovelSearchViewEnhanced.h
    src/config/SettingsDialog.h
    src/config/NovelConfig.h
    src/config/NovelSettingsDialog.h
    src/novel/NovelSearchManager.h
)

qt5_wrap_cpp(MOC_SRCS ${MOC_HEADERS})  # 生成 MOC 文件

# 添加可执行文件 - 使用新的模块化结构
add_executable(ProtectEye WIN32
    # Core module
    src/core/main.cpp
    src/core/mainwindow.cpp
    src/core/mainwindow.h
    src/core/TextDocumentModel.cpp
    src/core/TextDocumentModel.h
    src/core/TextReaderManager.cpp
    src/core/TextReaderManager.h

    # UI module
    src/ui/chapterdialog.cpp
    src/ui/chapterdialog.h
    src/ui/WebEngineView.cpp
    src/ui/WebEngineView.h
    src/ui/TextReaderView.cpp
    src/ui/TextReaderView.h
    src/ui/NovelSearchViewEnhanced.cpp
    src/ui/NovelSearchViewEnhanced.h

    # Config module
    src/config/settings.cpp
    src/config/settings.h
    src/config/SettingsDialog.cpp
    src/config/SettingsDialog.h
    src/config/NovelConfig.cpp
    src/config/NovelConfig.h
    src/config/NovelSettingsDialog.cpp
    src/config/NovelSettingsDialog.h

    # Novel module
    src/novel/NovelSearchManager.cpp
    src/novel/NovelSearchManager.h
    src/novel/NovelModels.cpp
    src/novel/NovelModels.h
    src/novel/NovelSearcher.cpp
    src/novel/NovelSearcher.h
    src/novel/ChapterDownloader.cpp
    src/novel/ChapterDownloader.h
    src/novel/FileGenerator.cpp
    src/novel/FileGenerator.h

    # Network module
    src/network/HttpClient.cpp
    src/network/HttpClient.h

    # Parser module
    src/parser/RuleManager.cpp
    src/parser/RuleManager.h
    src/parser/ContentParser.cpp
    src/parser/ContentParser.h
    src/parser/LexborHtmlParser.cpp
    src/parser/LexborHtmlParser.h

    ${MOC_SRCS}  # **确保 MOC 生成的代码被编译**
    ${UI_HEADERS}  # **确保 UI 处理的头文件被编译**
    ${QRC_FILES}   # **资源文件**
)

set_target_properties(ProtectEye PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)


# 链接 Qt 模块
target_link_libraries(ProtectEye PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets Qt5::PrintSupport Qt5::Network Qt5::Concurrent)

# **QHotkey 处理**
set(QHOTKEY_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/QHotkey)
set(QHOTKEY_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/QHotkey)

# 添加 QHotkey 头文件路径
include_directories(${QHOTKEY_INCLUDE_DIR})

# 链接 QHotkey 库
target_link_libraries(ProtectEye PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/lib/QHotkey/qhotkey.lib)

# **Qt 部署**
get_target_property(qmake_location Qt5::qmake IMPORTED_LOCATION)
message("qmake_location: " ${qmake_location})
get_filename_component(qt5_bin_dir ${qmake_location} DIRECTORY)
message("qt5_bin_dir: " ${qt5_bin_dir})
find_program(win_deploy_qt windeployqt HINTS "${qt5_bin_dir}")

add_custom_target(
    "DeployQt"
    COMMAND "${win_deploy_qt}" ${CMAKE_CURRENT_BINARY_DIR}/bin/ProtectEye.exe
    COMMAND ${CMAKE_COMMAND} -E copy ${QHOTKEY_LIB_DIR}/qhotkey.lib ${CMAKE_CURRENT_BINARY_DIR}/bin/
    COMMAND ${CMAKE_COMMAND} -E copy ${QHOTKEY_INCLUDE_DIR}/qhotkey.h ${CMAKE_CURRENT_BINARY_DIR}/bin/
)

# **Lexbor HTML Parser 处理**
# 添加 Lexbor 子项目
add_subdirectory(lib/lexbor)

# 链接 Lexbor 库
target_link_libraries(ProtectEye PRIVATE lexbor_static)


