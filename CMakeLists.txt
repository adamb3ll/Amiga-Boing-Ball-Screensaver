# CMakeLists.txt for Boing Ball Screensaver (Cross-Platform)
cmake_minimum_required(VERSION 3.15)
project(BoingBallSaver VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core library (platform-independent)
add_library(BoingCore STATIC
    src/core/BoingPhysics.cpp
    src/core/BoingPhysics.h
    src/core/BoingRenderer.cpp
    src/core/BoingRenderer.h
    src/core/BoingConfig.h
    src/core/Platform.h
)

target_include_directories(BoingCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Platform-specific builds
if(WIN32)
    # Windows Screensaver (.scr)
    add_executable(BoingBallSaver WIN32
        src/BoingBallSaver.cpp
        src/platform/windows/WindowsPlatform.cpp
        src/platform/windows/WindowsPlatform.h
        src/BoingBallSaver.rc
        src/resource.h
    )
    
    target_include_directories(BoingBallSaver PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    target_link_libraries(BoingBallSaver PRIVATE
        BoingCore
        opengl32
        glu32
        gdi32
        winmm
    )
    
    # Enable Unicode for Windows API calls
    target_compile_definitions(BoingBallSaver PRIVATE UNICODE _UNICODE)
    
    # Set output to .scr extension
    set_target_properties(BoingBallSaver PROPERTIES
        OUTPUT_NAME "BoingBallSaver"
        SUFFIX ".scr"
    )
    
    target_include_directories(BoingBallSaver PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

elseif(APPLE)
    # macOS Screensaver with OpenGL (.saver bundle)
    # Note: This is the OpenGL version. For Metal version, see ../mac-metal/
    add_library(BoingBallSaver MODULE
        src/platform/macos/MacBoingBallView.mm
        src/platform/macos/MacBoingBallView.h
        src/platform/macos/MacPlatform.mm
        src/platform/macos/MacPlatform.h
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
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/macos/Info.plist"
        MACOSX_BUNDLE_BUNDLE_NAME "Boing Ball"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.sinphaltimus.BoingBallSaver"
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
    
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

# Optional: Install targets
if(WIN32)
    install(TARGETS BoingBallSaver
        RUNTIME DESTINATION .
    )
elseif(APPLE)
    install(TARGETS BoingBallSaver
        LIBRARY DESTINATION "$ENV{HOME}/Library/Screen Savers"
    )
endif()
