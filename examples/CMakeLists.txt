# Examples CMakeLists.txt
# This file contains all example applications for Gfx

cmake_minimum_required(VERSION 3.16)

# stb_image for texture loading
include(FetchContent)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(stb)

# Create an interface library for stb_image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})

# GLFW dependency for examples
if(NOT BUILD_FOR_WEB)
    find_package(glfw3 3.3 QUIET)
    
    if(NOT glfw3_FOUND)
        message(STATUS "GLFW not found, fetching via FetchContent...")
        include(FetchContent)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
            GIT_SHALLOW TRUE
        )
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)
    else()
        message(STATUS "Using system GLFW")
    endif()
endif()

# Function to compile GLSL shaders to SPIR-V using glslc
function(compile_glsl_shader target shader_source)
    if(NOT GLSLC_EXECUTABLE)
        message(WARNING "Cannot compile GLSL shader ${shader_source} - glslc not found")
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
    # Make output path unique per input file by using full source path hash or relative path
    file(RELATIVE_PATH shader_rel_path ${CMAKE_CURRENT_SOURCE_DIR} ${shader_source})
    string(REPLACE "/" "_" shader_unique_name "${shader_rel_path}")
    string(REGEX REPLACE "\\.wgsl$" "" shader_unique_base "${shader_unique_name}")
    set(shader_output "${CMAKE_CURRENT_BINARY_DIR}/${shader_unique_base}.spv")
    
    add_custom_command(
        OUTPUT ${shader_output}
        COMMAND $<TARGET_FILE:tint_cmd_tint_cmd> ${shader_source} -o ${shader_output} --format spirv --entry-point main
        DEPENDS ${shader_source} tint_cmd_tint_cmd
        COMMENT "Compiling WGSL shader: ${shader_name}${shader_ext} -> ${shader_unique_base}.spv"
        VERBATIM
    )
    
    # Add the output to the target's dependencies
    target_sources(${target} PRIVATE ${shader_output})
endfunction()

# =============================================================================
# Web Examples (Emscripten)
# =============================================================================
if(BUILD_FOR_WEB)
    # Web Cube Example (Emscripten)
    add_executable(cube_example_web ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/cube_example.c)
    target_link_libraries(cube_example_web gfx stb_image)
    
    # Emscripten-specific flags for web cube example
    target_compile_options(cube_example_web PRIVATE -DEMSCRIPTEN -sDISABLE_EXCEPTION_CATCHING=0)
    target_link_options(cube_example_web PRIVATE
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=2
        -sSTACK_OVERFLOW_CHECK=2
        -sSAFE_HEAP=1
        -sDISABLE_EXCEPTION_CATCHING=0
        -sEXPORTED_RUNTIME_METHODS=requestFullscreen
        --preload-file=${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders@shaders
        --preload-file=${CMAKE_CURRENT_SOURCE_DIR}/c/cube/textures@textures
        -g
        -O0
        --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
    )
    
    # Set output to create HTML, JS, and WASM
    set_target_properties(cube_example_web PROPERTIES
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
    )
    
    message(STATUS "Web example will be built to: ${CMAKE_CURRENT_BINARY_DIR}/web/cube_example_web.html")
    
    # Web Compute Example (Emscripten)
    add_executable(compute_example_web ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/compute_example.c)
    target_link_libraries(compute_example_web gfx)
    
    # Emscripten-specific flags for web compute example
    target_compile_options(compute_example_web PRIVATE -DEMSCRIPTEN -sDISABLE_EXCEPTION_CATCHING=0)
    target_link_options(compute_example_web PRIVATE
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=2
        -sSTACK_OVERFLOW_CHECK=2
        -sSAFE_HEAP=1
        -sDISABLE_EXCEPTION_CATCHING=0
        -sEXPORTED_RUNTIME_METHODS=requestFullscreen
        --preload-file=${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders@shaders
        -g
        -O0
        --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
    )
    
    # Set output to create HTML, JS, and WASM
    set_target_properties(compute_example_web PROPERTIES
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
    )
    
    message(STATUS "Web example will be built to: ${CMAKE_CURRENT_BINARY_DIR}/web/compute_example_web.html")
    
    # Web C++ Cube Example (Emscripten)
    if(BUILD_CPP_WRAPPER)
        add_executable(cube_example_cpp_web ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/cube_example.cpp)
        target_link_libraries(cube_example_cpp_web gfx_cpp)
    
    # Emscripten-specific flags for web C++ cube example
    target_compile_options(cube_example_cpp_web PRIVATE -DEMSCRIPTEN -sDISABLE_EXCEPTION_CATCHING=0)
    target_link_options(cube_example_cpp_web PRIVATE
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
        -sASSERTIONS=2
        -sSTACK_OVERFLOW_CHECK=2
        -sSAFE_HEAP=1
        -sDISABLE_EXCEPTION_CATCHING=0
        -sEXPORTED_RUNTIME_METHODS=requestFullscreen
        --preload-file=${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders@shaders
        -g
        -O0
        --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
    )
    
    # Set output to create HTML, JS, and WASM
    set_target_properties(cube_example_cpp_web PROPERTIES
        SUFFIX ".html"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
    )
    
    message(STATUS "Web C++ cube example will be built to: ${CMAKE_CURRENT_BINARY_DIR}/web/cube_example_cpp_web.html")
    endif()  # BUILD_CPP_WRAPPER
    
    # Web C++ Compute Example (Emscripten)
    if(BUILD_CPP_WRAPPER)
        add_executable(compute_example_cpp_web ${CMAKE_CURRENT_SOURCE_DIR}/cpp/compute/compute_example.cpp)
        target_link_libraries(compute_example_cpp_web gfx_cpp)
        
        # Emscripten-specific flags for web C++ compute example
        target_compile_options(compute_example_cpp_web PRIVATE -DEMSCRIPTEN -sDISABLE_EXCEPTION_CATCHING=0)
        target_link_options(compute_example_cpp_web PRIVATE
            --use-port=emdawnwebgpu
            -sUSE_GLFW=3
            -sASYNCIFY=1
            -sALLOW_MEMORY_GROWTH=1
            -sASSERTIONS=0
            -sDISABLE_EXCEPTION_CATCHING=0
            -sEXPORTED_RUNTIME_METHODS=requestFullscreen
            --preload-file=${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders@shaders
            -g
            -O0
            --shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html
        )
        
        # Set output to create HTML, JS, and WASM
        set_target_properties(compute_example_cpp_web PROPERTIES
            SUFFIX ".html"
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/web"
        )
        
        message(STATUS "Web C++ compute example will be built to: ${CMAKE_CURRENT_BINARY_DIR}/web/compute_example_cpp_web.html")
    endif()  # BUILD_CPP_WRAPPER

# =============================================================================
# Native Examples
# =============================================================================
else()
    # Native C Cube Example
    add_executable(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/cube_example.c)
    target_link_libraries(cube_example_c gfx glfw stb_image m)  # Add stb_image
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(cube_example_c PRIVATE -fsanitize=address)
    endif()
    
    # Compile WGSL shaders to SPIR-V for C example
    compile_wgsl_shader(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.vert.wgsl)
    compile_wgsl_shader(cube_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.frag.wgsl)
    
    # Copy WGSL shaders and compiled SPIR-V to build directory
    add_custom_command(TARGET cube_example_c POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.vert.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.vert.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.frag.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.frag.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_cube_shaders_cube_textured.vert.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_cube_shaders_cube_textured.frag.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.frag.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/textures
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/textures/vulkan.png
            ${CMAKE_CURRENT_BINARY_DIR}/textures/vulkan.png
        COMMENT "Copying WGSL, SPIR-V shaders, and textures"
    )
    
    # Native C Threaded Cube Example (uses pthreads)
    add_executable(cube_example_c_threaded ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/cube_example_threaded.c)
    target_link_libraries(cube_example_c_threaded gfx glfw stb_image m pthread)  # Add stb_image
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(cube_example_c_threaded PRIVATE -fsanitize=address)
    endif()
    
    # Compile WGSL shaders to SPIR-V for threaded C example
    compile_wgsl_shader(cube_example_c_threaded ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.vert.wgsl)
    compile_wgsl_shader(cube_example_c_threaded ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.frag.wgsl)
    
    # Copy WGSL shaders and compiled SPIR-V for threaded example
    add_custom_command(TARGET cube_example_c_threaded POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.vert.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.vert.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/shaders/cube_textured.frag.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.frag.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_cube_shaders_cube_textured.vert.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_cube_shaders_cube_textured.frag.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube_textured.frag.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/textures
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/cube/textures/vulkan.png
            ${CMAKE_CURRENT_BINARY_DIR}/textures/vulkan.png
        COMMENT "Copying WGSL, SPIR-V shaders, and textures for threaded example"
    )

    # Threaded C++ Cube Example (uses C API with C++ language features)
    add_executable(cube_example_cpp_threaded ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/cube_example_threaded.cpp)
    target_link_libraries(cube_example_cpp_threaded gfx glfw stb_image pthread)
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(cube_example_cpp_threaded PRIVATE -fsanitize=address)
    endif()
    
    # Compile WGSL shaders to SPIR-V for C++ threaded example
    compile_wgsl_shader(cube_example_cpp_threaded ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.vert.wgsl)
    compile_wgsl_shader(cube_example_cpp_threaded ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.frag.wgsl)
    
    add_custom_command(TARGET cube_example_cpp_threaded POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.vert.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.vert.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.frag.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.frag.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/cpp_cube_shaders_cube.vert.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/cpp_cube_shaders_cube.frag.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.frag.spv
        COMMENT "Copying WGSL and SPIR-V shaders for C++ threaded example"
    )

    # C++ Cube Example (uses C++ wrapper)
    if(BUILD_CPP_WRAPPER)
        add_executable(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/cube_example.cpp)
        target_link_libraries(cube_example_cpp gfx_cpp glfw stb_image)
        if(ENABLE_ASAN AND NOT MSVC)
            target_link_options(cube_example_cpp PRIVATE -fsanitize=address)
        endif()
        
        # Compile WGSL shaders to SPIR-V for C++ wrapper example (uses its own simpler shaders)
        compile_wgsl_shader(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.vert.wgsl)
        compile_wgsl_shader(cube_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.frag.wgsl)
        
        # Copy WGSL shaders and compiled SPIR-V
        add_custom_command(TARGET cube_example_cpp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.vert.wgsl
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.vert.wgsl
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/cpp/cube/shaders/cube.frag.wgsl
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.frag.wgsl
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_BINARY_DIR}/cpp_cube_shaders_cube.vert.spv
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.vert.spv
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_BINARY_DIR}/cpp_cube_shaders_cube.frag.spv
                ${CMAKE_CURRENT_BINARY_DIR}/shaders/cube.frag.spv
            COMMENT "Copying WGSL and SPIR-V shaders for C++ wrapper example"
        )
    endif()  # BUILD_CPP_WRAPPER

    # C Compute Example
    add_executable(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/compute_example.c)
    target_link_libraries(compute_example_c gfx glfw)
    if(ENABLE_ASAN AND NOT MSVC)
        target_link_options(compute_example_c PRIVATE -fsanitize=address)
    endif()
    
    # Compile WGSL shaders to SPIR-V for compute example
    compile_wgsl_shader(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders/generate.comp.wgsl)
    compile_wgsl_shader(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders/fullscreen.vert.wgsl)
    compile_wgsl_shader(compute_example_c ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders/postprocess.frag.wgsl)
    
    # Copy WGSL shaders and compiled SPIR-V to build directory
    add_custom_command(TARGET compute_example_c POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders/generate.comp.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/generate.comp.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders/fullscreen.vert.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/fullscreen.vert.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/c/compute/shaders/postprocess.frag.wgsl
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/postprocess.frag.wgsl
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_compute_shaders_generate.comp.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/generate.comp.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_compute_shaders_fullscreen.vert.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/fullscreen.vert.spv
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_BINARY_DIR}/c_compute_shaders_postprocess.frag.spv
            ${CMAKE_CURRENT_BINARY_DIR}/shaders/postprocess.frag.spv
        COMMENT "Copying WGSL and SPIR-V shaders"
    )

    # C++ Compute Example
    if(BUILD_CPP_WRAPPER)
        add_executable(compute_example_cpp ${CMAKE_CURRENT_SOURCE_DIR}/cpp/compute/compute_example.cpp)
        target_link_libraries(compute_example_cpp gfx_cpp glfw)
        if(ENABLE_ASAN AND NOT MSVC)
            target_link_options(compute_example_cpp PRIVATE -fsanitize=address)
        endif()
            
        # C++ compute example uses the same compiled shaders as C example
        add_dependencies(compute_example_cpp compute_example_c)
    endif()  # BUILD_CPP_WRAPPER
endif()  # BUILD_FOR_WEB
