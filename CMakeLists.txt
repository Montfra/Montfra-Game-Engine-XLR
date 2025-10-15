cmake_minimum_required(VERSION 3.16)

project(MGE_XLR VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(USE_FETCHCONTENT "Fetch dependencies (glad/glfw) with CMake FetchContent" ON)
set(BUILD_SHARED_LIBS OFF)

find_package(OpenGL REQUIRED)

if(USE_FETCHCONTENT)
  include(FetchContent)

  # GLAD (OpenGL loader) — generate for OpenGL 3.3 core
  FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v0.1.36
  )
  set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
  set(GLAD_API "gl=3.3" CACHE STRING "" FORCE)
  set(GLAD_GENERATOR "c" CACHE STRING "" FORCE)
  set(GLAD_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glad)

  # GLFW (windowing/input)
  # First try an installed package to avoid re-downloading
  find_package(glfw3 QUIET CONFIG)
  if(NOT glfw3_FOUND)
    FetchContent_Declare(
      glfw
      GIT_REPOSITORY https://github.com/glfw/glfw.git
      GIT_TAG 3.4
    )
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    # Linux: éviter la dépendance wayland-scanner en désactivant Wayland si absent
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
  endif()
endif()

# Resolve a GLFW target name across package managers/builds
set(GLFW_TARGET "")
if(TARGET glfw)
  set(GLFW_TARGET glfw)
elseif(TARGET glfw3)
  set(GLFW_TARGET glfw3)
elseif(TARGET glfw::glfw)
  set(GLFW_TARGET glfw::glfw)
elseif(TARGET glfw3::glfw)
  set(GLFW_TARGET glfw3::glfw)
else()
  # Fallback: try to find an installed glfw3 if not using FetchContent
  if(NOT USE_FETCHCONTENT)
    find_package(glfw3 REQUIRED CONFIG)
    if(TARGET glfw)
      set(GLFW_TARGET glfw)
    elseif(TARGET glfw3)
      set(GLFW_TARGET glfw3)
    elseif(TARGET glfw::glfw)
      set(GLFW_TARGET glfw::glfw)
    elseif(TARGET glfw3::glfw)
      set(GLFW_TARGET glfw3::glfw)
    endif()
  endif()
endif()

add_executable(MGE_XLR
  src/main.cpp
  src/gui/GuiElement.h
  src/gui/GuiText.cpp
  src/gui/GuiText.h
  src/gui/GuiPanel.cpp
  src/gui/GuiPanel.h
  src/gui/GuiButton.cpp
  src/gui/GuiButton.h
  src/gui/GuiInput.h
  src/gui/GuiInput.cpp
  src/gui/GuiDraw.h
  src/gui/GuiDraw.cpp
  src/gui/GuiImage.h
  src/gui/GuiImage.cpp
  src/gui/GuiInputText.h
  src/gui/GuiInputText.cpp
  src/gui/GuiSlider.h
  src/gui/GuiSlider.cpp
  src/gui/GuiCheckbox.h
  src/gui/GuiCheckbox.cpp
  src/gui/GuiProgressBar.h
  src/gui/GuiProgressBar.cpp
  src/gui/GuiMenuBar.h
  src/gui/GuiMenuBar.cpp
  src/gui/GuiManager.h
  src/gui/GuiManager.cpp
)

# Define shader directory at compile time (absolute path, forward slashes)
file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/shaders" SHADERS_PATH)
target_compile_definitions(MGE_XLR PRIVATE SHADER_DIR="${SHADERS_PATH}" GLFW_INCLUDE_NONE)

# Link libraries
if(TARGET glad)
  target_link_libraries(MGE_XLR PRIVATE glad)
else()
  message(FATAL_ERROR "GLAD target not available. Enable USE_FETCHCONTENT or provide a glad target.")
endif()

if(GLFW_TARGET)
  target_link_libraries(MGE_XLR PRIVATE ${GLFW_TARGET})
else()
  message(FATAL_ERROR "GLFW target not available. Enable USE_FETCHCONTENT or install glfw3.")
endif()

target_link_libraries(MGE_XLR PRIVATE OpenGL::GL)

# FreeType (required for text rendering)
find_package(Freetype REQUIRED)
target_link_libraries(MGE_XLR PRIVATE Freetype::Freetype)

# On MSVC, be strict and enable parallel build
if(MSVC)
  target_compile_options(MGE_XLR PRIVATE /W4 /permissive-)
else()
  target_compile_options(MGE_XLR PRIVATE -Wall -Wextra -Wpedantic)
endif()
