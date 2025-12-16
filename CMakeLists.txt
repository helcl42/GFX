cmake_minimum_required(VERSION 3.16)
project(GfxWrapper VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_VULKAN_BACKEND "Build Vulkan backend" ON)
option(BUILD_WEBGPU_BACKEND "Build WebGPU backend" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" OFF)

# Find required packages
find_package(PkgConfig REQUIRED)

# GLFW for examples
if(BUILD_EXAMPLES)
    find_package(glfw3 REQUIRED)
endif()

# Vulkan
if(BUILD_VULKAN_BACKEND)
    find_package(Vulkan REQUIRED)
    
    # Find glslc shader compiler (comes with Vulkan SDK)
    find_program(GLSLC_EXECUTABLE
        NAMES glslc
        HINTS
            ${Vulkan_GLSLC_EXECUTABLE}
            ENV VULKAN_SDK
        PATH_SUFFIXES bin
    )
    
    if(GLSLC_EXECUTABLE)
        message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
    else()
        message(WARNING "glslc not found - shaders will not be compiled automatically")
    endif()
endif()

# Function to compile GLSL shaders to SPIR-V
function(compile_shader target shader_source)
    if(NOT GLSLC_EXECUTABLE)
        message(WARNING "Cannot compile shader ${shader_source} - glslc not found")
        return()
    endif()
    
    get_filename_component(shader_name ${shader_source} NAME)
    set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/${shader_name}.spv")
    
    add_custom_command(
        OUTPUT ${shader_output}
        COMMAND ${GLSLC_EXECUTABLE} ${shader_source} -o ${shader_output}
        DEPENDS ${shader_source}
        COMMENT "Compiling shader: ${shader_name} -> ${shader_name}.spv"
        VERBATIM
    )
    
    # Add the output to the target's dependencies
    target_sources(${target} PRIVATE ${shader_output})
endfunction()

# WebGPU (Dawn)
if(BUILD_WEBGPU_BACKEND)
    # Try to find WebGPU/Dawn
    find_path(WEBGPU_INCLUDE_DIR webgpu.h
        PATHS
            /usr/local/include
            /usr/include
            /opt/homebrew/include
    )
    
    find_library(WEBGPU_LIBRARY
        NAMES webgpu dawn_native dawn_proc webgpu_dawn
        PATHS
            /usr/local/lib
            /usr/lib
            /opt/homebrew/lib
    )
    
    if(WEBGPU_INCLUDE_DIR AND WEBGPU_LIBRARY)
        message(STATUS "Found WebGPU: ${WEBGPU_LIBRARY}")
        set(WEBGPU_FOUND TRUE)
    else()
        message(WARNING "WebGPU/Dawn not found - WebGPU backend will be disabled")
        set(BUILD_WEBGPU_BACKEND OFF)
    endif()
endif()

# Platform-specific libraries
if(WIN32)
    set(PLATFORM_LIBS user32 gdi32)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(IOKIT_LIBRARY IOKit)
    find_library(COREVIDEO_LIBRARY CoreVideo)
    set(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
elseif(UNIX)
    find_package(X11 REQUIRED)
    pkg_check_modules(XCB REQUIRED xcb)
    pkg_check_modules(WAYLAND_CLIENT wayland-client)
    set(PLATFORM_LIBS ${X11_LIBRARIES} ${XCB_LIBRARIES})
    if(WAYLAND_CLIENT_FOUND)
        list(APPEND PLATFORM_LIBS ${WAYLAND_CLIENT_LIBRARIES})
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# C API Library
set(GFX_C_SOURCES
    gfx/GfxApi.h
    gfx/GfxBackend.h
    gfx/GfxDispatcher.c
)

if(BUILD_VULKAN_BACKEND)
    list(APPEND GFX_C_SOURCES gfx/GfxVulkan.c)
endif()

if(BUILD_WEBGPU_BACKEND)
    list(APPEND GFX_C_SOURCES gfx/GfxWebGPU.c)
endif()

# Create library based on BUILD_SHARED_LIBS option
if(BUILD_SHARED_LIBS)
    add_library(gfx_c SHARED ${GFX_C_SOURCES})
    # Set version information for shared library
    set_target_properties(gfx_c PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    # Define GFX_BUILDING_DLL when building the shared library
    target_compile_definitions(gfx_c PRIVATE GFX_BUILDING_DLL)
else()
    add_library(gfx_c STATIC ${GFX_C_SOURCES})
endif()

target_include_directories(gfx_c PUBLIC gfx/)

# Link Vulkan if available
if(BUILD_VULKAN_BACKEND)
    target_link_libraries(gfx_c PUBLIC Vulkan::Vulkan)
    target_compile_definitions(gfx_c PUBLIC GFX_ENABLE_VULKAN=1)
    # Don't include bundled headers - use system Vulkan headers instead
endif()

# Link WebGPU if available
if(BUILD_WEBGPU_BACKEND)
    target_link_libraries(gfx_c PUBLIC ${WEBGPU_LIBRARY})
    target_include_directories(gfx_c PUBLIC ${WEBGPU_INCLUDE_DIR})
    target_include_directories(gfx_c PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/include)
    target_compile_definitions(gfx_c PUBLIC GFX_ENABLE_WEBGPU=1)
endif()

# Link platform libraries
target_link_libraries(gfx_c PUBLIC ${PLATFORM_LIBS})

# C++ API Library
set(GFX_CPP_SOURCES
    gfx_cpp/GfxApi.hpp
    gfx_cpp/GfxApiImpl.cpp  # Single unified C++ wrapper for all backends
)

# Create library based on BUILD_SHARED_LIBS option
if(BUILD_SHARED_LIBS)
    add_library(gfx_cpp SHARED ${GFX_CPP_SOURCES})
    # Set version information for shared library
    set_target_properties(gfx_cpp PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
else()
    add_library(gfx_cpp STATIC ${GFX_CPP_SOURCES})
endif()

target_include_directories(gfx_cpp PUBLIC gfx_cpp/)
target_link_libraries(gfx_cpp PUBLIC gfx_c)  # Link to C API library

# Link Vulkan if available
if(BUILD_VULKAN_BACKEND)
    target_link_libraries(gfx_cpp PUBLIC Vulkan::Vulkan)
    target_compile_definitions(gfx_cpp PUBLIC GFX_ENABLE_VULKAN=1)
    target_include_directories(gfx_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# Link WebGPU if available
if(BUILD_WEBGPU_BACKEND)
    target_link_libraries(gfx_cpp PUBLIC ${WEBGPU_LIBRARY})
    target_include_directories(gfx_cpp PUBLIC ${WEBGPU_INCLUDE_DIR})
    target_compile_definitions(gfx_cpp PUBLIC GFX_ENABLE_WEBGPU=1)
endif()

# Link platform libraries
target_link_libraries(gfx_cpp PUBLIC ${PLATFORM_LIBS})

# Compiler flags
if(MSVC)
    target_compile_options(gfx_c PRIVATE /W4)
    target_compile_options(gfx_cpp PRIVATE /W4)
else()
    # target_compile_options(gfx_c PRIVATE -Wall -Wextra -Wpedantic -g -fsanitize=address -fno-omit-frame-pointer)
    # target_compile_options(gfx_cpp PRIVATE -Wall -Wextra -Wpedantic -g -fsanitize=address -fno-omit-frame-pointer)
    # target_link_options(gfx_c PRIVATE -fsanitize=address)
    # target_link_options(gfx_cpp PRIVATE -fsanitize=address)

    target_compile_options(gfx_c PRIVATE -Wall -Wextra -Wpedantic -g)
    target_compile_options(gfx_cpp PRIVATE -Wall -Wextra -Wpedantic -g)
endif()

# Examples
if(BUILD_EXAMPLES)
    # C Cube Example
    add_executable(cube_example_c examples/c/cube_example.c)
    target_link_libraries(cube_example_c gfx_c glfw m)  # Add math library
    
    # Compile shaders for C example
    compile_shader(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/shaders/cube.vert)
    compile_shader(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/shaders/cube.frag)
    
    # C++ Cube Example
    add_executable(cube_example_cpp examples/cpp/cube_example.cpp)
    target_link_libraries(cube_example_cpp gfx_cpp glfw)
    
    # Compile shaders for C++ example (uses same shaders as C example)
    compile_shader(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/shaders/cube.vert)
    compile_shader(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/shaders/cube.frag)
endif()

# Install targets
install(TARGETS gfx_c gfx_cpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES gfx/GfxApi.h DESTINATION include/gfx)
install(FILES gfx_cpp/GfxApi.hpp DESTINATION include/gfx)

# Print configuration summary
message(STATUS "")
message(STATUS "GfxWrapper Configuration Summary:")
message(STATUS "  Library type: ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "  Building SHARED libraries")
else()
    message(STATUS "  Building STATIC libraries")
endif()
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Vulkan backend: ${BUILD_VULKAN_BACKEND}")
message(STATUS "  WebGPU backend: ${BUILD_WEBGPU_BACKEND}")
if(BUILD_VULKAN_BACKEND)
    message(STATUS "  Vulkan library: ${Vulkan_LIBRARIES}")
endif()
if(BUILD_WEBGPU_BACKEND AND WEBGPU_FOUND)
    message(STATUS "  WebGPU library: ${WEBGPU_LIBRARY}")
endif()
message(STATUS "")