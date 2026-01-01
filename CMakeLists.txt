cmake_minimum_required(VERSION 3.16)
project(GfxWrapper VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detect Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building for Emscripten/WebAssembly")
    set(BUILD_FOR_WEB TRUE)
    # Force options for web builds
    set(BUILD_VULKAN_BACKEND OFF CACHE BOOL "Vulkan not available on web" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static libraries for web" FORCE)
else()
    set(BUILD_FOR_WEB FALSE)
endif()

# Options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_VULKAN_BACKEND "Build Vulkan backend" ON)
option(BUILD_WEBGPU_BACKEND "Build WebGPU backend" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)

# Find required packages
find_package(PkgConfig REQUIRED)

# GLFW for examples
if(BUILD_EXAMPLES AND NOT BUILD_FOR_WEB)
    find_package(glfw3 REQUIRED)
endif()

# Vulkan
if(BUILD_VULKAN_BACKEND AND NOT BUILD_FOR_WEB)
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

# Function to compile WGSL shaders to SPIR-V using tint
function(compile_wgsl_shader target shader_source)
    if(NOT BUILD_WEBGPU_BACKEND)
        message(WARNING "Cannot compile WGSL shader ${shader_source} - WebGPU backend not enabled")
        return()
    endif()
    
    get_filename_component(shader_name ${shader_source} NAME_WE)
    get_filename_component(shader_ext ${shader_source} EXT)
    # Remove .wgsl extension and add .spv
    string(REGEX REPLACE "\\.wgsl$" "" shader_base "${shader_name}${shader_ext}")
    set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/${shader_base}.spv")
    
    add_custom_command(
        OUTPUT ${shader_output}
        COMMAND ${CMAKE_BINARY_DIR}/dawn/tint ${shader_source} -o ${shader_output} --format spirv --entry-point main
        DEPENDS ${shader_source} tint
        COMMENT "Compiling WGSL shader: ${shader_name}${shader_ext} -> ${shader_base}.spv"
        VERBATIM
    )
    
    # Add the output to the target's dependencies
    target_sources(${target} PRIVATE ${shader_output})
endfunction()

# WebGPU (Dawn)
if(BUILD_WEBGPU_BACKEND)
    # Use Dawn from specific location as subdirectory
    set(DAWN_ROOT "/home/lhelcl/Dev/webgpu-cross-platform-app/dawn")
    
    if(EXISTS "${DAWN_ROOT}/CMakeLists.txt")
        message(STATUS "Adding Dawn from: ${DAWN_ROOT}")
        
        # Save our BUILD_SHARED_LIBS setting
        set(GFXWRAPPER_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
        
        # Dawn build options - Dawn requires BUILD_SHARED_LIBS=OFF
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "" FORCE)
        set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
        set(DAWN_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        
        # Platform-specific Dawn options
        if(BUILD_FOR_WEB)
            set(DAWN_USE_X11 OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_VULKAN OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_METAL OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_D3D11 OFF CACHE BOOL "" FORCE)
        else()
            set(DAWN_USE_X11 ON CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_VULKAN ON CACHE BOOL "" FORCE)  # Enable Vulkan for native rendering
            set(DAWN_ENABLE_METAL OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_D3D11 OFF CACHE BOOL "" FORCE)
        endif()
        set(DAWN_ENABLE_D3D12 OFF CACHE BOOL "" FORCE)
        set(TINT_BUILD_CMD_TOOLS ON CACHE BOOL "" FORCE)  # Build tint command-line tool
        
        add_subdirectory(${DAWN_ROOT} ${CMAKE_BINARY_DIR}/dawn EXCLUDE_FROM_ALL)
        
        # Restore our BUILD_SHARED_LIBS setting
        set(BUILD_SHARED_LIBS ${GFXWRAPPER_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
        
        set(WEBGPU_INCLUDE_DIR "${DAWN_ROOT}/include")
        set(WEBGPU_FOUND TRUE)
        message(STATUS "WebGPU enabled via Dawn subdirectory")
        message(STATUS "WebGPU include dir: ${WEBGPU_INCLUDE_DIR}")
    else()
        message(WARNING "Dawn directory not found at ${DAWN_ROOT} - WebGPU backend will be disabled")
        set(BUILD_WEBGPU_BACKEND OFF)
    endif()
endif()

# Platform-specific libraries
if(BUILD_FOR_WEB)
    # Web builds don't need platform libraries
    set(PLATFORM_LIBS "")
elseif(WIN32)
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
else()
    message(ERROR "Unknown configuration.")
endif()

# C API Library (implemented in C++)
set(GFX_C_SOURCES
    gfx/include/gfx/gfx.h
    gfx/src/backend/IBackend.h
    gfx/src/backend/BackendFactory.cpp
    gfx/src/api/GfxDispatcher.cpp
)

if(BUILD_VULKAN_BACKEND)
    list(APPEND GFX_C_SOURCES gfx/src/backend/vulkan/VulkanBackend.cpp)
endif()

if(BUILD_WEBGPU_BACKEND)
    list(APPEND GFX_C_SOURCES gfx/src/backend/webgpu/WebGPUBackend.cpp)
endif()

# Create library based on BUILD_SHARED_LIBS option
if(BUILD_SHARED_LIBS)
    add_library(gfx SHARED ${GFX_C_SOURCES})
    # Set version information for shared library
    set_target_properties(gfx PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    # Define GFX_BUILDING_DLL when building the shared library
    target_compile_definitions(gfx PRIVATE GFX_BUILDING_DLL)
else()
    add_library(gfx STATIC ${GFX_C_SOURCES})
endif()

target_include_directories(gfx 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gfx/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gfx/src
)

# Link Vulkan if available
if(BUILD_VULKAN_BACKEND)
    target_link_libraries(gfx PUBLIC Vulkan::Vulkan)
    target_compile_definitions(gfx PUBLIC GFX_ENABLE_VULKAN=1)
    # Don't include bundled headers - use system Vulkan headers instead
endif()

# Link WebGPU if available
if(BUILD_WEBGPU_BACKEND)
    # For native builds, link to webgpu_dawn; for web builds, use Emscripten port
    if(NOT BUILD_FOR_WEB)
        target_link_libraries(gfx PUBLIC webgpu_dawn)
    endif()
    
    # Add Emscripten-specific generated headers BEFORE source headers
    if(BUILD_FOR_WEB)
        target_include_directories(gfx PUBLIC 
            ${CMAKE_BINARY_DIR}/dawn/gen/src/emdawnwebgpu/include  # Generated headers first
            ${WEBGPU_INCLUDE_DIR}                                  # Source headers second
        )
    else()
        target_include_directories(gfx PUBLIC 
            ${CMAKE_BINARY_DIR}/dawn/gen/include
            ${WEBGPU_INCLUDE_DIR}
        )
    endif()
    
    target_include_directories(gfx PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/include)
    target_compile_definitions(gfx PUBLIC GFX_ENABLE_WEBGPU=1)
    
    # Ensure Dawn's generated headers are built first
    if(TARGET webgpu_headers_gen)
        add_dependencies(gfx webgpu_headers_gen)
    endif()
    if(TARGET emdawnwebgpu_headers_gen)
        add_dependencies(gfx emdawnwebgpu_headers_gen)
    endif()
endif()

# Link platform libraries
target_link_libraries(gfx PUBLIC ${PLATFORM_LIBS})

# C++ API Library
set(GFX_CPP_SOURCES
    gfx_cpp/include/gfx_cpp/Gfx.hpp
    gfx_cpp/src/GfxImpl.cpp  # Single unified C++ wrapper for all backends
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

target_include_directories(gfx_cpp 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gfx_cpp/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/gfx_cpp/src
)
target_link_libraries(gfx_cpp PUBLIC gfx)  # Link to C API library

# Link Vulkan if available
if(BUILD_VULKAN_BACKEND)
    target_link_libraries(gfx_cpp PUBLIC Vulkan::Vulkan)
    target_compile_definitions(gfx_cpp PUBLIC GFX_ENABLE_VULKAN=1)
endif()

# Link WebGPU if available
if(BUILD_WEBGPU_BACKEND)
    target_link_libraries(gfx_cpp PUBLIC ${WEBGPU_LIBRARY})
    
    # Add Emscripten-specific generated headers BEFORE source headers
    if(BUILD_FOR_WEB)
        target_include_directories(gfx_cpp PUBLIC 
            ${CMAKE_BINARY_DIR}/dawn/gen/src/emdawnwebgpu/include  # Generated headers first
            ${WEBGPU_INCLUDE_DIR}                                   # Source headers second
        )
    else()
        target_include_directories(gfx_cpp PUBLIC 
            ${CMAKE_BINARY_DIR}/dawn/gen/include
            ${WEBGPU_INCLUDE_DIR}
        )
    endif()
    
    target_compile_definitions(gfx_cpp PUBLIC GFX_ENABLE_WEBGPU=1)
    
    # Ensure Dawn's generated headers are built first
    if(TARGET webgpu_headers_gen)
        add_dependencies(gfx_cpp webgpu_headers_gen)
    endif()
    if(TARGET emdawnwebgpu_headers_gen)
        add_dependencies(gfx_cpp emdawnwebgpu_headers_gen)
    endif()
endif()

# Link platform libraries
target_link_libraries(gfx_cpp PUBLIC ${PLATFORM_LIBS})

# Emscripten-specific configuration
if(BUILD_FOR_WEB)
    target_compile_options(gfx PRIVATE 
        # -sUSE_GLFW=3
        # -sASYNCIFY=1
    )
    target_compile_options(gfx_cpp PRIVATE 
        # -sUSE_GLFW=3
        # -sASYNCIFY=1
    )
    target_link_options(gfx PUBLIC
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
    )
    target_link_options(gfx_cpp PUBLIC
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
    )
endif()

# Compiler flags
if(MSVC)
    target_compile_options(gfx PRIVATE /W4)
    target_compile_options(gfx_cpp PRIVATE /W4)
else()
    target_compile_options(gfx PRIVATE -Wall -Wextra -Wpedantic -g)
    target_compile_options(gfx_cpp PRIVATE -Wall -Wextra -Wpedantic -g)
    
    # Address Sanitizer (optional)
    if(ENABLE_ASAN)
        target_compile_options(gfx PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_compile_options(gfx_cpp PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(gfx PRIVATE -fsanitize=address)
        target_link_options(gfx_cpp PRIVATE -fsanitize=address)
    endif()
endif()

# Examples
if(BUILD_EXAMPLES)
    if(BUILD_FOR_WEB)
        # Web Cube Example (Emscripten)
        add_executable(cube_example_web examples/c/cube/cube_example.c)
        target_link_libraries(cube_example_web gfx)
        
        # Emscripten-specific flags for web example
        target_compile_options(cube_example_web PRIVATE -DEMSCRIPTEN)
        target_link_options(cube_example_web PRIVATE
            --use-port=emdawnwebgpu
            -sUSE_GLFW=3
            -sASYNCIFY=1
            -sALLOW_MEMORY_GROWTH=1
            -sASSERTIONS=2
            -sSTACK_OVERFLOW_CHECK=2
            -sSAFE_HEAP=1
            -sEXPORTED_RUNTIME_METHODS=requestFullscreen
            --preload-file=${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders@shaders
            -g
            -O0
            --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/examples/web/shell.html
        )
        
        # Set output to create HTML, JS, and WASM
        set_target_properties(cube_example_web PROPERTIES
            SUFFIX ".html"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
        )
        
        message(STATUS "Web example will be built to: ${CMAKE_CURRENT_BINARY_DIR}/web/cube_example_web.html")
        
    else()
        # Native C Cube Example
        add_executable(cube_example_c examples/c/cube/cube_example.c)
        target_link_libraries(cube_example_c gfx glfw m)  # Add math library
        if(ENABLE_ASAN AND NOT MSVC)
            target_link_options(cube_example_c PRIVATE -fsanitize=address)
        endif()
        
        # Compile WGSL shaders to SPIR-V for C example
        compile_wgsl_shader(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.vert.wgsl)
        compile_wgsl_shader(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.frag.wgsl)
        
        # Copy WGSL shaders to build directory for WebGPU runtime loading
        add_custom_command(TARGET cube_example_c POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.vert.wgsl
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.vert.wgsl
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.frag.wgsl
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.frag.wgsl
        COMMENT "Copying WGSL shaders for WebGPU runtime"
    )
    endif()  # BUILD_FOR_WEB
    
    # C++ and Compute examples only for native builds (for now)
    if(NOT BUILD_FOR_WEB)
    # C++ Cube Example
    add_executable(cube_example_cpp examples/cpp/cube/cube_example.cpp)
    target_link_libraries(cube_example_cpp gfx_cpp glfw)
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(cube_example_cpp PRIVATE -fsanitize=address)
    endif()
    
    # Compile WGSL shaders to SPIR-V for C++ example (shares with C example)
    compile_wgsl_shader(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.vert.wgsl)
    compile_wgsl_shader(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.frag.wgsl)
    
    # Copy WGSL shaders for WebGPU runtime loading (same command as C example)
    add_custom_command(TARGET cube_example_cpp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.vert.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.vert.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/cube/shaders/cube.frag.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.frag.wgsl
        COMMENT "Copying WGSL shaders for WebGPU runtime"
    )
    
    # C Compute Example
    add_executable(compute_example_c examples/c/compute/compute_example.c)
    target_link_libraries(compute_example_c gfx glfw m)
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(compute_example_c PRIVATE -fsanitize=address)
    endif()
    
    # Compile WGSL shaders to SPIR-V for compute example
    compile_wgsl_shader(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/compute/shaders/generate.comp.wgsl)
    compile_wgsl_shader(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/compute/shaders/fullscreen.vert.wgsl)
    compile_wgsl_shader(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/compute/shaders/postprocess.frag.wgsl)
    
    # Copy WGSL shaders to build directory for WebGPU runtime loading
    add_custom_command(TARGET compute_example_c POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/compute/shaders/generate.comp.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/generate.comp.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/compute/shaders/fullscreen.vert.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/fullscreen.vert.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/examples/c/compute/shaders/postprocess.frag.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/postprocess.frag.wgsl
        COMMENT "Copying WGSL shaders for WebGPU runtime"
    )
    
    # C++ Compute Example
    add_executable(compute_example_cpp examples/cpp/compute/compute_example.cpp)
    target_link_libraries(compute_example_cpp gfx_cpp glfw)
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(compute_example_cpp PRIVATE -fsanitize=address)
    endif()
    
    # C++ compute example uses the same compiled shaders as C example
    add_dependencies(compute_example_cpp compute_example_c)
    endif()  # NOT BUILD_FOR_WEB
endif()  # BUILD_EXAMPLES

# Install targets
install(TARGETS gfx gfx_cpp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES gfx/include/gfx/gfx.h DESTINATION include/gfx)
install(FILES gfx_cpp/include/gfx_cpp/Gfx.hpp DESTINATION include/gfx_cpp)

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
message(STATUS "  Address Sanitizer: ${ENABLE_ASAN}")
message(STATUS "  Vulkan backend: ${BUILD_VULKAN_BACKEND}")
message(STATUS "  WebGPU backend: ${BUILD_WEBGPU_BACKEND}")
if(BUILD_VULKAN_BACKEND)
    message(STATUS "  Vulkan library: ${Vulkan_LIBRARIES}")
endif()
if(BUILD_WEBGPU_BACKEND AND WEBGPU_FOUND)
    message(STATUS "  WebGPU library: ${WEBGPU_LIBRARY}")
endif()
message(STATUS "")