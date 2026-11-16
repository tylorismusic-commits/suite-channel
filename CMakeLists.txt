cmake_minimum_required(VERSION 3.16)
project(suite_channel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
include(GNUInstallDirs)

set(HEARTH_DSP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/hearth-dsp"
    CACHE PATH "Path to the hearth-dsp checkout")
if(NOT EXISTS "${HEARTH_DSP_DIR}/include/hdsp/hdsp.hpp")
  message(FATAL_ERROR "hearth-dsp not found at ${HEARTH_DSP_DIR}")
endif()

set(DUSK_EQ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dusk-4k"
    CACHE PATH "Path to the vendored Dusk 4K EQ core")
if(NOT EXISTS "${DUSK_EQ_DIR}/FourKEQDSP.hpp")
  message(FATAL_ERROR "Dusk 4K EQ core not found at ${DUSK_EQ_DIR}")
endif()

include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  "${HEARTH_DSP_DIR}/include"
  "${DUSK_EQ_DIR}")

enable_testing()

set(CLAP_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/clap"
    CACHE PATH "Directory where CLAP bundles are installed")
set(LV2_DIR "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lv2"
    CACHE PATH "Directory where the LV2 bundle is installed")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(LV2 REQUIRED IMPORTED_TARGET lv2)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)
pkg_check_modules(X11 REQUIRED IMPORTED_TARGET x11)

set(SUITE_UI_SOURCES src/ui/panel.cpp)
set(SUITE_DSP_SOURCES third_party/dusk-4k/FourKEQDSP.cpp)

# ---------------------------------------------------------------------------
# CLAP plugin
# ---------------------------------------------------------------------------
add_library(suite_channel MODULE
  src/clap/suite_channel_clap.cpp
  ${SUITE_UI_SOURCES}
  ${SUITE_DSP_SOURCES})
target_include_directories(suite_channel PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/third_party/clap/include"
  "${HEARTH_DSP_DIR}/include"
  "${DUSK_EQ_DIR}")
target_link_libraries(suite_channel PRIVATE
  PkgConfig::CAIRO PkgConfig::FREETYPE PkgConfig::X11
  Threads::Threads ${CMAKE_DL_LIBS})
target_compile_features(suite_channel PRIVATE cxx_std_17)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(suite_channel PRIVATE -Wall -Wextra -fvisibility=hidden)
endif()
set_target_properties(suite_channel PROPERTIES
  PREFIX ""
  OUTPUT_NAME "SuiteChannel"
  SUFFIX ".clap"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/clap")

set(SUITE_RESOURCE_FILES
  "${CMAKE_SOURCE_DIR}/assets/fonts/Jost-ExtraBold.ttf"
  "${CMAKE_SOURCE_DIR}/assets/fonts/OFL.txt")

add_custom_target(suite_channel_bundle ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/clap/SuiteChannel.resources"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SUITE_RESOURCE_FILES}
          "${CMAKE_BINARY_DIR}/clap/SuiteChannel.resources/"
  DEPENDS suite_channel ${SUITE_RESOURCE_FILES})

install(TARGETS suite_channel LIBRARY DESTINATION "${CLAP_DIR}")
install(FILES ${SUITE_RESOURCE_FILES}
        DESTINATION "${CLAP_DIR}/SuiteChannel.resources")

# ---------------------------------------------------------------------------
# LV2 bundle
# ---------------------------------------------------------------------------
set(LV2_BUNDLE_DIR "${CMAKE_BINARY_DIR}/lv2/suite-channel.lv2")

add_library(suite_channel_lv2 MODULE
  src/suite_channel_lv2.cpp
  ${SUITE_DSP_SOURCES})
target_include_directories(suite_channel_lv2 PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${HEARTH_DSP_DIR}/include"
  "${DUSK_EQ_DIR}")
target_link_libraries(suite_channel_lv2 PRIVATE PkgConfig::LV2)
target_compile_features(suite_channel_lv2 PRIVATE cxx_std_17)
set_target_properties(suite_channel_lv2 PROPERTIES
  PREFIX ""
  OUTPUT_NAME "suite_channel"
  LIBRARY_OUTPUT_DIRECTORY "${LV2_BUNDLE_DIR}")

add_library(suite_channel_lv2_ui MODULE
  src/ui/lv2_ui.cpp
  ${SUITE_UI_SOURCES})
target_include_directories(suite_channel_lv2_ui PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${HEARTH_DSP_DIR}/include"
  "${DUSK_EQ_DIR}")
target_link_libraries(suite_channel_lv2_ui PRIVATE
  PkgConfig::LV2 PkgConfig::CAIRO PkgConfig::FREETYPE PkgConfig::X11)
target_compile_features(suite_channel_lv2_ui PRIVATE cxx_std_17)
set_target_properties(suite_channel_lv2_ui PROPERTIES
  PREFIX ""
  OUTPUT_NAME "suite_channel_ui"
  LIBRARY_OUTPUT_DIRECTORY "${LV2_BUNDLE_DIR}")

add_custom_target(suite_channel_lv2_bundle ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "${LV2_BUNDLE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_SOURCE_DIR}/lv2/manifest.ttl"
          "${CMAKE_SOURCE_DIR}/lv2/suite_channel.ttl"
          "${CMAKE_SOURCE_DIR}/lv2/presets.ttl"
          "${LV2_BUNDLE_DIR}/"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SUITE_RESOURCE_FILES}
          "${LV2_BUNDLE_DIR}/"
  DEPENDS suite_channel_lv2 suite_channel_lv2_ui
          "${CMAKE_SOURCE_DIR}/lv2/manifest.ttl"
          "${CMAKE_SOURCE_DIR}/lv2/suite_channel.ttl"
          "${CMAKE_SOURCE_DIR}/lv2/presets.ttl"
          ${SUITE_RESOURCE_FILES})

install(TARGETS suite_channel_lv2 suite_channel_lv2_ui
        LIBRARY DESTINATION "${LV2_DIR}/suite-channel.lv2")
install(FILES
          lv2/manifest.ttl
          lv2/suite_channel.ttl
          lv2/presets.ttl
        ${SUITE_RESOURCE_FILES}
        DESTINATION "${LV2_DIR}/suite-channel.lv2")

# ---------------------------------------------------------------------------
# Tools
# ---------------------------------------------------------------------------
add_executable(suite_ttl_gen tools/gen_ttl.cpp)
target_include_directories(suite_ttl_gen PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_compile_features(suite_ttl_gen PRIVATE cxx_std_17)
add_test(NAME suite_ttl
  COMMAND suite_ttl_gen --check --root "${CMAKE_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
add_executable(suite_dsp_test test/test_dsp.cpp ${SUITE_DSP_SOURCES})
target_include_directories(suite_dsp_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src"
  "${HEARTH_DSP_DIR}/include"
  "${DUSK_EQ_DIR}")
target_compile_features(suite_dsp_test PRIVATE cxx_std_17)
add_test(NAME suite_dsp COMMAND suite_dsp_test)

add_executable(suite_panel_test test/test_panel.cpp ${SUITE_UI_SOURCES})
target_include_directories(suite_panel_test PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(suite_panel_test PRIVATE PkgConfig::CAIRO PkgConfig::FREETYPE)
target_compile_features(suite_panel_test PRIVATE cxx_std_17)
add_test(NAME suite_panel COMMAND suite_panel_test)

add_executable(suite_ui_preview test/ui_preview.cpp ${SUITE_UI_SOURCES})
target_include_directories(suite_ui_preview PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(suite_ui_preview PRIVATE PkgConfig::CAIRO PkgConfig::FREETYPE)
target_compile_features(suite_ui_preview PRIVATE cxx_std_17)

add_executable(suite_clap_host_test test/clap_host_test.cpp)
target_include_directories(suite_clap_host_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/third_party/clap/include")
target_link_libraries(suite_clap_host_test PRIVATE
  PkgConfig::X11 ${CMAKE_DL_LIBS})
target_compile_features(suite_clap_host_test PRIVATE cxx_std_17)
add_dependencies(suite_clap_host_test suite_channel_bundle)
find_program(XVFB_RUN xvfb-run)
if(XVFB_RUN)
  add_test(NAME suite_clap_host
    COMMAND "${XVFB_RUN}" -a -s "-screen 0 1600x1200x24"
      "$<TARGET_FILE:suite_clap_host_test>" "${CMAKE_BINARY_DIR}/clap/SuiteChannel.clap")
else()
  add_test(NAME suite_clap_host
    COMMAND suite_clap_host_test "${CMAKE_BINARY_DIR}/clap/SuiteChannel.clap")
endif()

add_executable(suite_lv2_host_test test/lv2_host_test.cpp)
target_include_directories(suite_lv2_host_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(suite_lv2_host_test PRIVATE
  PkgConfig::LV2 ${CMAKE_DL_LIBS})
target_compile_features(suite_lv2_host_test PRIVATE cxx_std_17)
add_dependencies(suite_lv2_host_test suite_channel_lv2_bundle)
add_test(NAME suite_lv2_host
  COMMAND suite_lv2_host_test "${CMAKE_BINARY_DIR}/lv2/suite-channel.lv2/suite_channel.so")

add_executable(suite_lv2_ui_host_test test/lv2_ui_host_test.cpp)
target_include_directories(suite_lv2_ui_host_test PRIVATE
  "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(suite_lv2_ui_host_test PRIVATE
  PkgConfig::X11 ${CMAKE_DL_LIBS})
target_compile_features(suite_lv2_ui_host_test PRIVATE cxx_std_17)
add_dependencies(suite_lv2_ui_host_test suite_channel_lv2_bundle)
if(XVFB_RUN)
  add_test(NAME suite_lv2_ui_host
    COMMAND "${XVFB_RUN}" -a -s "-screen 0 1600x1200x24"
      "$<TARGET_FILE:suite_lv2_ui_host_test>" "${CMAKE_BINARY_DIR}/lv2/suite-channel.lv2")
else()
  add_test(NAME suite_lv2_ui_host
    COMMAND suite_lv2_ui_host_test "${CMAKE_BINARY_DIR}/lv2/suite-channel.lv2")
endif()

# ---------------------------------------------------------------------------
# Distribution
# ---------------------------------------------------------------------------
add_custom_target(dist
  COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/dist"
  COMMAND ${CMAKE_COMMAND} -E tar czf
          "${CMAKE_BINARY_DIR}/dist/suite-channel-${PROJECT_VERSION}-linux-x86_64.tar.gz"
          lv2/suite-channel.lv2 clap/SuiteChannel.clap clap/SuiteChannel.resources
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  DEPENDS suite_channel_lv2_bundle suite_channel_bundle
  COMMENT "Packing suite-channel-${PROJECT_VERSION}")
