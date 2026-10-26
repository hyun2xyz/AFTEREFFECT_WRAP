cmake_minimum_required(VERSION 3.20)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.4" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
project(EdgeLightWrap VERSION 2.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(AE_SDK_ROOT "" CACHE PATH "Extracted Adobe After Effects SDK root; SDK is not redistributed")
option(ELW_BUILD_PLUGIN "Build the After Effects plug-in" ON)
option(ELW_BUILD_TESTS "Build native rendering tests" ON)

add_library(WrapCore STATIC native/WrapCore.cpp)
target_include_directories(WrapCore PUBLIC native)
target_compile_options(WrapCore PRIVATE -Wall -Wextra -Wpedantic)
set_target_properties(WrapCore PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(WIN32)
  # MinGW-w64 build: no runtime DLLs beside the .aex (AE loads it from MediaCore).
  set(ELW_STATIC_RUNTIME -static -static-libgcc -static-libstdc++)
  # Same platform defines as the SDK's Visual Studio projects.
  set(ELW_WIN_DEFINES MSWindows WIN32 _WINDOWS)
endif()

if(ELW_BUILD_PLUGIN)
  if(NOT EXISTS "${AE_SDK_ROOT}/Examples/Headers/AE_Effect.h")
    message(FATAL_ERROR "Set AE_SDK_ROOT to your locally installed Adobe After Effects SDK.")
  endif()
  set(ELW_SDK_INCLUDES "${AE_SDK_ROOT}/Examples/Headers" "${AE_SDK_ROOT}/Examples/Headers/SP"
                       "${AE_SDK_ROOT}/Examples/Util" "${AE_SDK_ROOT}/Examples/Resources")
  if(APPLE)
    add_library(EdgeLightWrap MODULE native/EdgeLightWrap.cpp)
    target_include_directories(EdgeLightWrap SYSTEM PRIVATE ${ELW_SDK_INCLUDES})
    target_link_libraries(EdgeLightWrap PRIVATE WrapCore "-framework Cocoa")
    target_compile_options(EdgeLightWrap PRIVATE -Wall -Wextra)
    set_target_properties(EdgeLightWrap PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin PREFIX ""
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/native/mac/Info.plist")
    find_program(REZ_TOOL Rez REQUIRED)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo" "eFKTFXTC")
    add_custom_command(TARGET EdgeLightWrap POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:EdgeLightWrap>/Resources"
        COMMAND "${REZ_TOOL}" -d __MACH__=1
                -i "${AE_SDK_ROOT}/Examples/Headers" -i "${AE_SDK_ROOT}/Examples/Resources"
                -useDF -o "$<TARGET_BUNDLE_CONTENT_DIR:EdgeLightWrap>/Resources/EdgeLightWrap.rsrc"
                "${CMAKE_CURRENT_SOURCE_DIR}/native/mac/EdgeLightWrapPiPL.r"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_SOURCE_DIR}/native/mac/Info.plist"
                "$<TARGET_BUNDLE_CONTENT_DIR:EdgeLightWrap>/Info.plist"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_CURRENT_BINARY_DIR}/PkgInfo"
                "$<TARGET_BUNDLE_CONTENT_DIR:EdgeLightWrap>/PkgInfo"
        COMMAND /usr/bin/codesign --force --sign - --timestamp=none "$<TARGET_BUNDLE_DIR:EdgeLightWrap>"
        VERBATIM)
  elseif(WIN32)
    # Windows has no Rez; scripts/pipl.py writes the same PiPL (checked against Rez on macOS).
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(ELW_PIPL_RC "${CMAKE_CURRENT_BINARY_DIR}/EdgeLightWrapPiPL.rc")
    add_custom_command(OUTPUT "${ELW_PIPL_RC}"
        COMMAND Python3::Interpreter -B "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pipl.py"
                --sdk "${AE_SDK_ROOT}" --rc "${ELW_PIPL_RC}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/scripts/pipl.py" VERBATIM)
    enable_language(RC)
    add_library(EdgeLightWrap MODULE native/EdgeLightWrap.cpp "${ELW_PIPL_RC}")
    target_include_directories(EdgeLightWrap SYSTEM PRIVATE ${ELW_SDK_INCLUDES})
    target_compile_definitions(EdgeLightWrap PRIVATE ${ELW_WIN_DEFINES})
    target_link_libraries(EdgeLightWrap PRIVATE WrapCore ${ELW_STATIC_RUNTIME})
    target_compile_options(EdgeLightWrap PRIVATE -Wall -Wextra -Wno-multichar)
    # Stripped and without a link timestamp, so a rebuild of the same source gives the same file.
    target_link_options(EdgeLightWrap PRIVATE -s -Wl,--no-insert-timestamp)
    set_target_properties(EdgeLightWrap PROPERTIES PREFIX "" SUFFIX ".aex"
        CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
  else()
    message(FATAL_ERROR "The plug-in builds for macOS or Windows (MinGW-w64).")
  endif()
endif()

if(ELW_BUILD_TESTS)
  enable_testing()
  add_executable(wrap-core-tests tests/WrapCoreTests.cpp)
  target_link_libraries(wrap-core-tests PRIVATE WrapCore ${ELW_STATIC_RUNTIME})
  add_test(NAME wrap-core COMMAND wrap-core-tests)
  if(ELW_BUILD_PLUGIN)
    add_executable(ae-host-tests tests/AEHostTests.cpp)
    target_include_directories(ae-host-tests SYSTEM PRIVATE ${ELW_SDK_INCLUDES})
    if(APPLE)
      target_link_libraries(ae-host-tests PRIVATE WrapCore "-framework Cocoa")
    else()
      target_compile_definitions(ae-host-tests PRIVATE ${ELW_WIN_DEFINES})
      target_link_libraries(ae-host-tests PRIVATE WrapCore ${ELW_STATIC_RUNTIME})
      # Shipped as tests/windows/ae-host-tests.exe so CI can load the real .aex on Windows without the SDK.
      target_link_options(ae-host-tests PRIVATE -s -Wl,--no-insert-timestamp)
    endif()
    add_dependencies(ae-host-tests EdgeLightWrap)
    add_test(NAME ae-host COMMAND ae-host-tests "$<TARGET_FILE:EdgeLightWrap>")
  endif()
endif()
