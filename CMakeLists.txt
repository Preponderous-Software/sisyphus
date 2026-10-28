cmake_minimum_required(VERSION 3.16)
project(sisyphus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(SISYPHUS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(SISYPHUS_BUILD_TESTS "Build the test suite" ON)
option(SISYPHUS_WITH_STRATA "Build the playable game on the Strata engine" ON)
option(SISYPHUS_WEB "Build the browser game (Emscripten + Strata's WebGL 2 backend)" OFF)

# Emscripten: the rules and tests build as they are and the tests run under
# Node; SISYPHUS_WEB adds the browser game. Strata compiles C++ exceptions as
# wasm exceptions, and everything in one .wasm must match.
if(EMSCRIPTEN)
  cmake_minimum_required(VERSION 3.24)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fwasm-exceptions>")
  add_link_options(-fwasm-exceptions -sALLOW_MEMORY_GROWTH=1)
  if(SISYPHUS_WITH_STRATA)
    message(FATAL_ERROR "SISYPHUS_WITH_STRATA builds the desktop game: use -DSISYPHUS_WITH_STRATA=OFF -DSISYPHUS_WEB=ON")
  endif()
endif()
if(SISYPHUS_WEB AND NOT EMSCRIPTEN)
  message(FATAL_ERROR "SISYPHUS_WEB needs Emscripten (emcmake cmake ...)")
endif()

function(sisyphus_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(SISYPHUS_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
    if(SISYPHUS_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# The simulation: the hill, the boulder, the modules and their rot, the
# autopilot. No engine, no GPU. Never fuse multiply-adds: the rules must give
# bit-identical results on every compiler.
add_library(sisyphus_sim STATIC src/sim/autopilot.cpp src/sim/hill.cpp src/sim/world.cpp)
target_include_directories(sisyphus_sim PUBLIC src)
sisyphus_warnings(sisyphus_sim)
if(MSVC)
  target_compile_options(sisyphus_sim PRIVATE /fp:precise)
else()
  target_compile_options(sisyphus_sim PRIVATE -ffp-contract=off)
endif()

# The hill, the town and the boulder as triangle meshes. Still no engine.
add_library(sisyphus_geometry STATIC src/bridge/geometry.cpp)
target_include_directories(sisyphus_geometry PUBLIC src)
target_link_libraries(sisyphus_geometry PUBLIC sisyphus_sim)
sisyphus_warnings(sisyphus_geometry)

# Saves: the World as a checksummed file, written atomically, with the
# previous save kept (Kindling's and Atman's format; see ARCHITECTURE.md).
add_library(sisyphus_save STATIC src/save/save.cpp src/save/sha256.cpp)
target_include_directories(sisyphus_save PUBLIC src)
target_link_libraries(sisyphus_save PUBLIC sisyphus_sim)
sisyphus_warnings(sisyphus_save)

# The sounds, worked out from the world and synthesized (no recordings). No
# device here: the sound card is opened only by the game (below).
add_library(sisyphus_audio STATIC src/audio/cues.cpp src/audio/synth.cpp)
target_include_directories(sisyphus_audio PUBLIC src)
target_link_libraries(sisyphus_audio PUBLIC sisyphus_sim)
sisyphus_warnings(sisyphus_audio)

if(SISYPHUS_BUILD_TESTS)
  enable_testing()
  foreach(t hill rules autopilot determinism geometry saves audio)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE sisyphus_sim sisyphus_geometry sisyphus_save sisyphus_audio)
    target_include_directories(test_${t} PRIVATE tests)
    sisyphus_warnings(test_${t})
    if(EMSCRIPTEN)
      # Under Node, the tests read and write the real filesystem.
      target_link_options(test_${t} PRIVATE -sNODERAWFS=1)
    endif()
    add_test(NAME ${t} COMMAND test_${t} "${CMAKE_CURRENT_BINARY_DIR}")
  endforeach()
  # The saves test loads the committed fixtures (tests/fixtures).
  target_compile_definitions(test_saves PRIVATE SISYPHUS_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
endif()

# ---------------------------------------------------------------- the game on Strata
# Strata is fetched at SISYPHUS_STRATA_REF (the v0.6.0 tag: ABI 1.12), or taken
# from a local checkout with -DFETCHCONTENT_SOURCE_DIR_STRATA=<path>. The
# desktop game and the browser game share everything but their main().
if(SISYPHUS_WITH_STRATA OR SISYPHUS_WEB)
  cmake_minimum_required(VERSION 3.24)
  include(FetchContent)
  set(SISYPHUS_STRATA_REF "c2d7ee4c1102726c654c90d8361383d9ba3590b3" CACHE STRING "Strata git ref (a full commit SHA)")
  set(STRATA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(STRATA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(STRATA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(STRATA_WITH_USD OFF CACHE BOOL "" FORCE)
  set(STRATA_WITH_SIM OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(strata
    GIT_REPOSITORY https://github.com/Preponderous-Software/strata.git
    GIT_TAG ${SISYPHUS_STRATA_REF})
  FetchContent_MakeAvailable(strata)

  # The sound card, through miniaudio (public domain or MIT-0, third_party/miniaudio/),
  # built with its own warnings.
  find_package(Threads REQUIRED)
  add_library(sisyphus_miniaudio STATIC src/audio/miniaudio.cpp)
  target_include_directories(sisyphus_miniaudio SYSTEM PUBLIC third_party/miniaudio)
  target_link_libraries(sisyphus_miniaudio PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
  add_library(sisyphus_sound STATIC src/audio/device.cpp)
  target_link_libraries(sisyphus_sound PUBLIC sisyphus_audio PRIVATE sisyphus_miniaudio)
  sisyphus_warnings(sisyphus_sound)

  add_library(sisyphus_play STATIC src/bridge/scene.cpp src/play/session.cpp src/play/view.cpp)
  target_include_directories(sisyphus_play PUBLIC src)
  target_link_libraries(sisyphus_play PUBLIC sisyphus_geometry sisyphus_save sisyphus_sound strata::static)
  sisyphus_warnings(sisyphus_play)
endif()

if(SISYPHUS_WITH_STRATA)
  add_executable(sisyphus app/main.cpp)
  target_link_libraries(sisyphus PRIVATE sisyphus_play)
  target_compile_definitions(sisyphus PRIVATE SISYPHUS_VERSION="${PROJECT_VERSION}")
  sisyphus_warnings(sisyphus)
endif()

# emcmake cmake -S . -B build-web -DSISYPHUS_WITH_STRATA=OFF -DSISYPHUS_WEB=ON:
# the page is sisyphus.html + .js + .wasm; saves go to IndexedDB (docs/WEB.md).
if(SISYPHUS_WEB)
  add_executable(sisyphus-web app/web_main.cpp)
  target_link_libraries(sisyphus-web PRIVATE sisyphus_play)
  sisyphus_warnings(sisyphus-web)
  set_target_properties(sisyphus-web PROPERTIES OUTPUT_NAME sisyphus SUFFIX ".html")
  target_link_options(sisyphus-web PRIVATE
    -sSTACK_SIZE=1MB
    -lidbfs.js
    "-sEXPORTED_FUNCTIONS=_main,_sisyphus_look,_sisyphus_stick,_sisyphus_hold,_sisyphus_press,_sisyphus_captured,_sisyphus_help,_sisyphus_sound,_sisyphus_save,_sisyphus_restart,_sisyphus_resize,_sisyphus_status"
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,FS,IDBFS,addRunDependency,removeRunDependency
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
  set_property(TARGET sisyphus-web APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
endif()
