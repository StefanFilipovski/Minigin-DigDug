cmake_minimum_required(VERSION 3.26)
project(DigDugProject)

if(EMSCRIPTEN)
  message("Using Emscripten!")
endif()

# enable c++20 features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Statically link the MSVC runtime (/MT) so the executable runs on any PC
# without the Visual C++ redistributable installed.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ============================================================
# Dependencies (FetchContent)
# ============================================================
include(FetchContent)

# --- glm ---
find_package(glm CONFIG QUIET)
if(NOT glm_FOUND)
  FetchContent_Declare(
    glm
    URL https://github.com/g-truc/glm/releases/download/1.0.3/glm-1.0.3.zip
    DOWNLOAD_NO_PROGRESS ON
    DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
  )
  FetchContent_MakeAvailable(glm)
endif()

# --- SDL3 ---
find_package(SDL3 3.4 CONFIG QUIET)
if(NOT SDL3_FOUND)
  if (WIN32)
    FetchContent_Declare(
      SDL3
      URL https://www.libsdl.org/release/SDL3-devel-3.4.0-VC.zip
      DOWNLOAD_NO_PROGRESS ON
      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
    )
    FetchContent_MakeAvailable(SDL3)
    list(PREPEND CMAKE_PREFIX_PATH "${sdl3_SOURCE_DIR}")
    find_package(SDL3 CONFIG REQUIRED)
  else()
    FetchContent_Declare(
      SDL3
      GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
      GIT_TAG release-3.4.0
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(SDL3)
  endif()
endif()

# --- SDL3_ttf ---
find_package(SDL3_ttf 3.2.2 CONFIG QUIET)
if(NOT SDL3_ttf_FOUND)
  if (WIN32)
    FetchContent_Declare(
      SDL3_ttf
      URL https://www.libsdl.org/projects/SDL_ttf/release/SDL3_ttf-devel-3.2.2-VC.zip
      DOWNLOAD_NO_PROGRESS ON
      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
    )
    FetchContent_MakeAvailable(SDL3_ttf)
    list(PREPEND CMAKE_PREFIX_PATH "${sdl3_ttf_SOURCE_DIR}")
    find_package(SDL3_ttf CONFIG REQUIRED)
  else()
    FetchContent_Declare(
      SDL3_ttf
      GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
      GIT_TAG release-3.2.2
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
    )
    set(SDL3TTF_INSTALL OFF)
    if(EMSCRIPTEN)
      list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/emscripten")
      set(SDL3TTF_USE_SYSTEM_FREETYPE ON)
      set(SDL3TTF_VENDORED OFF)
      set(SDLTTF_PLUTOSVG OFF)
      add_compile_options(-sUSE_FREETYPE=1 -sUSE_HARFBUZZ=1)
      add_link_options(-sUSE_FREETYPE=1 -sUSE_HARFBUZZ=1)
    endif()
    FetchContent_MakeAvailable(SDL3_ttf)
  endif()
endif()

# --- SDL3_mixer ---
find_package(SDL3_mixer CONFIG QUIET)
if(NOT SDL3_mixer_FOUND)
  if (WIN32)
    FetchContent_Declare(
      SDL3_mixer
      URL https://github.com/libsdl-org/SDL_mixer/releases/download/release-3.2.0/SDL3_mixer-devel-3.2.0-VC.zip
      DOWNLOAD_NO_PROGRESS ON
      DOWNLOAD_DIR ${CMAKE_BINARY_DIR}/downloads
    )
    FetchContent_MakeAvailable(SDL3_mixer)
    list(PREPEND CMAKE_PREFIX_PATH "${sdl3_mixer_SOURCE_DIR}")
    find_package(SDL3_mixer CONFIG REQUIRED)
  else()
    FetchContent_Declare(
      SDL3_mixer
      GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
      GIT_TAG release-3.2.0
      GIT_SHALLOW TRUE
      GIT_PROGRESS TRUE
    )
    FetchContent_MakeAvailable(SDL3_mixer)
  endif()
endif()

# --- VLD (optional) ---
if (WIN32)
  # Look for VLD next to the project (common student setup)
  set(VLD_SEARCH_PATHS
    "${CMAKE_SOURCE_DIR}/../Visual Leak Detector"
    "C:/Program Files (x86)/Visual Leak Detector"
    "C:/Program Files/Visual Leak Detector"
  )
  foreach(vld_path IN LISTS VLD_SEARCH_PATHS)
    if(EXISTS "${vld_path}/cmake/VLDConfig.cmake")
      set(VLD_DIR "${vld_path}/cmake" CACHE PATH "VLD cmake config directory" FORCE)
      break()
    endif()
  endforeach()
  find_package(VLD CONFIG QUIET)
  if(VLD_FOUND)
    message(STATUS "VLD found: ${VLD_INCLUDE_DIR}")
  else()
    message(STATUS "VLD NOT found - leak detection disabled")
  endif()
endif()

# --- Steamworks (optional) ---
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/steam")
find_package(Steamworks QUIET)
if(Steamworks_FOUND)
  message(STATUS "Steamworks SDK found!")
else()
  message(STATUS "Steamworks SDK NOT found - Steam features disabled")
endif()

# --- Dear ImGui ---
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG v1.92.5-docking
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PUBLIC SDL3::SDL3)
target_compile_features(imgui PUBLIC cxx_std_20)

# ============================================================
# Minigin Engine (static library)
# ============================================================
add_library(Minigin STATIC
  Minigin/Minigin.cpp
  Minigin/Renderer.cpp
  Minigin/ResourceManager.cpp
  Minigin/SceneManager.cpp
  Minigin/Scene.cpp
  Minigin/GameObject.cpp
  Minigin/Font.cpp
  Minigin/Texture2D.cpp
  Minigin/Transform.cpp
  Minigin/TransformComponent.cpp
  Minigin/RenderComponent.cpp
  Minigin/TextComponent.cpp
  Minigin/FPSComponent.cpp
  Minigin/InputManager.cpp
  Minigin/Controller.cpp
  Minigin/Subject.cpp
  Minigin/ServiceLocator.cpp
  Minigin/SDLSoundService.cpp
  Minigin/SpriteAnimatorComponent.cpp
)

target_compile_features(Minigin PUBLIC cxx_std_20)

target_include_directories(Minigin PUBLIC
  ${CMAKE_SOURCE_DIR}/Minigin/
  ${CMAKE_SOURCE_DIR}/cmake/nlohmann/
)

target_link_libraries(Minigin PUBLIC
  SDL3::SDL3
  SDL3_ttf::SDL3_ttf
  SDL3_mixer::SDL3_mixer
  glm::glm
  imgui
)

if(VLD_FOUND)
  target_include_directories(Minigin PUBLIC ${VLD_INCLUDE_DIR})
  target_link_libraries(Minigin PUBLIC ${VLD_LIBRARIES})
endif()

if(Steamworks_FOUND)
  target_link_libraries(Minigin PUBLIC Steamworks::Steamworks)
  target_compile_definitions(Minigin PUBLIC USE_STEAMWORKS)
endif()

if(MSVC)
  target_compile_options(Minigin PRIVATE /W4 /WX)
else()
  target_compile_options(Minigin PRIVATE -Wall -Wextra -Werror)
endif()

# ============================================================
# Dig Dug Game (executable)
# ============================================================
add_executable(DigDug
  Game/Main.cpp
  Game/GameSession.cpp
  Game/MenuState.cpp
  Game/PlayingState.cpp
  Game/GameOverState.cpp
  Game/HighScoreState.cpp
  Game/LevelLoader.cpp
  Game/GridComponent.cpp
  Game/GridMovementComponent.cpp
  Game/PlayerAnimController.cpp
  Game/PlayerCollisionComponent.cpp
  Game/EnemyComponent.cpp
  Game/EnemyStates.cpp
  Game/EnemyTypeInfo.cpp
  Game/PumpComponent.cpp
  Game/RockComponent.cpp
  Game/HealthComponent.cpp
  Game/ScoreComponent.cpp
  Game/FloatingScoreComponent.cpp
  Game/LivesDisplayComponent.cpp
  Game/PointsDisplayComponent.cpp
)

target_compile_features(DigDug PUBLIC cxx_std_20)

target_include_directories(DigDug PRIVATE
  ${CMAKE_SOURCE_DIR}/Game/
)

# Link against Minigin — transitively provides all engine includes and libraries
target_link_libraries(DigDug PRIVATE Minigin)

if(MSVC)
  target_compile_options(DigDug PRIVATE /W4 /WX)
else()
  target_compile_options(DigDug PRIVATE -Wall -Wextra -Werror)
endif()

# ============================================================
# Post-build steps
# ============================================================
if(EMSCRIPTEN)
  set(EMSCRIPTEN_COMPILE_FLAGS "${EMSCRIPTEN_LINK_FLAGS} -g")
  set(EMSCRIPTEN_LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS} -sUSE_FREETYPE=1 -sUSE_HARFBUZZ=1 -g -sALLOW_MEMORY_GROWTH --preload-file \"${CMAKE_SOURCE_DIR}/Data@/\"")

  set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmscriptenToolchain.cmake)

  set_target_properties(DigDug PROPERTIES
      COMPILE_FLAGS "${EMSCRIPTEN_COMPILE_FLAGS}"
      LINK_FLAGS "${EMSCRIPTEN_LINK_FLAGS}"
  )

  add_custom_command(TARGET DigDug POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/web" "$<TARGET_FILE_DIR:DigDug>/"
  )

  set(CMAKE_EXECUTABLE_SUFFIX ".html")

elseif(WIN32)
  # Set DigDug as the default startup project
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DigDug)
  # Set working directory to output folder
  set_target_properties(DigDug PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:DigDug>")

  # Copy SDL DLLs to output folder
  set(SDL_RUNTIME_DLLS
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_mixer::SDL3_mixer
  )

  foreach(dllTarget IN LISTS SDL_RUNTIME_DLLS)
    add_custom_command(
      TARGET DigDug POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "$<TARGET_FILE:${dllTarget}>"
          "$<TARGET_FILE_DIR:DigDug>"
      VERBATIM
    )
  endforeach()

  if(Steamworks_FOUND)
    add_custom_command(TARGET DigDug POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy "${Steamworks_RUNTIME_DLL}" "$<TARGET_FILE_DIR:DigDug>")
    add_custom_command(TARGET DigDug POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "480" > "$<TARGET_FILE_DIR:DigDug>/steam_appid.txt"
      COMMENT "Generating steam_appid.txt"
    )
  endif()

  # Copy VLD runtime DLLs to output folder (leak detection)
  if(VLD_FOUND)
    set(VLD_BIN_DIR "${VLD_DIR}/../bin/Win64")
    set(VLD_RUNTIME_FILES
      "${VLD_BIN_DIR}/vld_x64.dll"
      "${VLD_BIN_DIR}/dbghelp.dll"
      "${VLD_BIN_DIR}/symsrv.dll"
      "${VLD_BIN_DIR}/Microsoft.DTfW.DHL.manifest"
    )
    foreach(vldFile IN LISTS VLD_RUNTIME_FILES)
      add_custom_command(TARGET DigDug POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${vldFile}"
            "$<TARGET_FILE_DIR:DigDug>"
        VERBATIM
      )
    endforeach()
  endif()

  # Copy Data folder to output directory
  add_custom_command(TARGET DigDug POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:DigDug>/Data"
  )

elseif(APPLE)
  set_target_properties(DigDug PROPERTIES
    MACOSX_BUNDLE TRUE
    INSTALL_RPATH "@executable_path/../Frameworks"
    BUILD_WITH_INSTALL_RPATH TRUE
  )
  add_custom_command(TARGET DigDug POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:DigDug>/Contents/Frameworks"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3::SDL3-shared>" "$<TARGET_BUNDLE_DIR:DigDug>/Contents/Frameworks/"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL3_ttf::SDL3_ttf-shared>" "$<TARGET_BUNDLE_DIR:DigDug>/Contents/Frameworks/"
    COMMAND install_name_tool -change "@rpath/libSDL3_ttf.0.dylib" "@executable_path/../Frameworks/libSDL3_ttf.0.2.2.dylib" "$<TARGET_FILE:DigDug>"
    COMMAND install_name_tool -change "@rpath/libSDL3.0.dylib" "@executable_path/../Frameworks/libSDL3.0.dylib" "$<TARGET_FILE:DigDug>"
  )

  if(Steamworks_FOUND)
    add_custom_command(TARGET DigDug POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${Steamworks_LIBRARIES}" "$<TARGET_BUNDLE_DIR:DigDug>/Contents/Frameworks/"
    )
    add_custom_command(TARGET DigDug POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "480" > "$<TARGET_FILE_DIR:DigDug>/steam_appid.txt"
      COMMENT "Generating steam_appid.txt"
    )
  endif()

  add_custom_command(TARGET DigDug POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:DigDug>/Data"
  )

else()
  # Linux
  if(Steamworks_FOUND)
    add_custom_command(TARGET DigDug POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${Steamworks_LIBRARIES}" "$<TARGET_FILE_DIR:DigDug>"
    )
    add_custom_command(TARGET DigDug POST_BUILD
      COMMAND "${CMAKE_COMMAND}" -E echo "480" > "$<TARGET_FILE_DIR:DigDug>/steam_appid.txt"
      COMMENT "Generating steam_appid.txt"
    )
  endif()

  add_custom_command(TARGET DigDug POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/Data" "$<TARGET_FILE_DIR:DigDug>/Data"
  )
endif()
