# CMakeLists.txt for Boing Ball Screensaver (macOS)
cmake_minimum_required(VERSION 3.15)
project(BoingBallSaver VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core library (physics and rendering)
add_library(BoingCore STATIC
    src/core/BoingPhysics.cpp
    src/core/BoingPhysics.h
    src/core/BoingRenderer.cpp
    src/core/BoingRenderer.h
    src/core/BoingConfig.h
    src/core/Platform.h
)

target_include_directories(BoingCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# macOS Screensaver (.saver bundle)
add_library(BoingBallSaver MODULE
    src/MacBoingBallView.mm
    src/MacBoingBallView.h
    src/MacPlatform.mm
    src/MacPlatform.h
)

target_link_libraries(BoingBallSaver PRIVATE
    BoingCore
    "-framework ScreenSaver"
    "-framework Cocoa"
    "-framework OpenGL"
    "-framework AppKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-Wl,-ObjC"
)

target_include_directories(BoingBallSaver PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Silence OpenGL deprecation warnings on macOS
target_compile_definitions(BoingBallSaver PRIVATE GL_SILENCE_DEPRECATION)

# Set bundle properties
set_target_properties(BoingBallSaver PROPERTIES
    BUNDLE TRUE
    BUNDLE_EXTENSION "saver"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/Info.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "Boing Ball"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.adamb3ll.BoingBallSaver"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.3.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.3.0"
    OUTPUT_NAME "BoingBallSaver"
)

# Copy sound files and thumbnail to Resources folder in bundle
add_custom_command(TARGET BoingBallSaver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory 
        "$<TARGET_BUNDLE_DIR:BoingBallSaver>/Contents/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds/BoingBallF.wav"
        "$<TARGET_BUNDLE_DIR:BoingBallSaver>/Contents/Resources/BoingBallF.wav"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/sounds/BoingBallW.wav"
        "$<TARGET_BUNDLE_DIR:BoingBallSaver>/Contents/Resources/BoingBallW.wav"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/resources/thumbnail.tiff"
        "$<TARGET_BUNDLE_DIR:BoingBallSaver>/Contents/Resources/thumbnail.tiff"
)

# Standalone test app for performance testing (as .app bundle)
add_executable(BoingBallTestApp MACOSX_BUNDLE
    src/TestApp/main.mm
    src/MacBoingBallView.mm
    src/MacPlatform.mm
)

target_link_libraries(BoingBallTestApp PRIVATE
    BoingCore
    "-framework ScreenSaver"
    "-framework Cocoa"
    "-framework OpenGL"
    "-framework AppKit"
    "-framework Foundation"
    "-framework QuartzCore"
    "-framework IOKit"
    "-Wl,-ObjC"
)

target_include_directories(BoingBallTestApp PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

target_compile_definitions(BoingBallTestApp PRIVATE GL_SILENCE_DEPRECATION)

# Configure as .app bundle
set_target_properties(BoingBallTestApp PROPERTIES
    BUNDLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/TestApp/Info.plist"
    MACOSX_BUNDLE_BUNDLE_NAME "Boing Ball Test"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.adamb3ll.BoingBallTestApp"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    OUTPUT_NAME "BoingBallTestApp"
)

# Install screensaver to user's Screen Savers folder
install(TARGETS BoingBallSaver
    LIBRARY DESTINATION "$ENV{HOME}/Library/Screen Savers"
)

