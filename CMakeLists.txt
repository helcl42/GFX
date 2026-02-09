cmake_minimum_required(VERSION 3.16)
project(Gfx VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect Emscripten
if(EMSCRIPTEN)
    message(STATUS "Building for Emscripten/WebAssembly")
    set(BUILD_FOR_WEB TRUE)
    # Force options for web builds
    set(BUILD_VULKAN_BACKEND OFF CACHE BOOL "Vulkan not available on web" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static libraries for web" FORCE)
    set(ENABLE_ASAN OFF CACHE BOOL "ASan not well supported on web" FORCE)
else()
    set(BUILD_FOR_WEB FALSE)
endif()

# Options
option(BUILD_EXAMPLES "Build example applications" ON)
option(BUILD_TESTS "Build test suite" ON)
option(BUILD_CPP_WRAPPER "Build C++ wrapper library" ON)
option(BUILD_VULKAN_BACKEND "Build Vulkan backend" ON)
option(BUILD_WEBGPU_BACKEND "Build WebGPU backend" ON)
option(BUILD_SHARED_LIBS "Build shared libraries instead of static" ON)
option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(BUILD_HEADLESS "Build without windowing system support (no surface creation)" OFF)

# Platform-specific libraries
if(BUILD_HEADLESS)
    # Headless builds don't need windowing libraries
    set(PLATFORM_LIBS "")
    message(STATUS "Headless build: skipping windowing system libraries")
elseif(BUILD_FOR_WEB)
    # Web builds don't need platform libraries
    set(PLATFORM_LIBS "")
    set(GFX_HAS_EMSCRIPTEN ON)
elseif(ANDROID)
    # Android-specific libraries
    find_library(ANDROID_LIBRARY android)
    find_library(LOG_LIBRARY log)
    set(PLATFORM_LIBS ${ANDROID_LIBRARY} ${LOG_LIBRARY})
    set(GFX_HAS_ANDROID ON)
elseif(WIN32)
    set(PLATFORM_LIBS user32 gdi32)
    set(GFX_HAS_WIN32 ON)
elseif(APPLE)
    if(IOS)
        # iOS-specific frameworks
        find_library(UIKIT_LIBRARY UIKit)
        find_library(FOUNDATION_LIBRARY Foundation)
        find_library(QUARTZCORE_LIBRARY QuartzCore)
        set(PLATFORM_LIBS ${UIKIT_LIBRARY} ${FOUNDATION_LIBRARY} ${QUARTZCORE_LIBRARY})
        set(GFX_HAS_UIKIT ON)
    else()
        # macOS-specific frameworks
        find_library(COCOA_LIBRARY Cocoa)
        find_library(IOKIT_LIBRARY IOKit)
        find_library(COREVIDEO_LIBRARY CoreVideo)
        set(PLATFORM_LIBS ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY})
        set(GFX_HAS_COCOA ON)
    endif()
elseif(UNIX)
    # Try to find available windowing systems (all optional)
    find_package(X11)
    
    # Only use pkg-config if available (for XCB and Wayland detection)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(XCB xcb)
        pkg_check_modules(WAYLAND_CLIENT wayland-client)
    endif()
    
    # Build list of platform libraries based on what's available
    set(PLATFORM_LIBS "")
    if(X11_FOUND)
        list(APPEND PLATFORM_LIBS ${X11_LIBRARIES})
        set(GFX_HAS_X11 ON)
        message(STATUS "Found X11 windowing support")
    endif()
    if(XCB_FOUND)
        list(APPEND PLATFORM_LIBS ${XCB_LIBRARIES})
        set(GFX_HAS_XCB ON)
        message(STATUS "Found XCB windowing support")
    endif()
    if(WAYLAND_CLIENT_FOUND)
        list(APPEND PLATFORM_LIBS ${WAYLAND_CLIENT_LIBRARIES})
        set(GFX_HAS_WAYLAND ON)
        message(STATUS "Found Wayland windowing support")
    endif()
    
    # Warn if no windowing system found
    if(NOT X11_FOUND AND NOT XCB_FOUND AND NOT WAYLAND_CLIENT_FOUND)
        message(WARNING "No windowing system found (X11, XCB, or Wayland). Consider using -DBUILD_HEADLESS=ON")
    endif()
else()
    message(ERROR "Unknown configuration.")
endif()

# Vulkan
if(BUILD_VULKAN_BACKEND AND NOT BUILD_FOR_WEB)
    # Don't use find_package(Vulkan) - we'll use Dawn's bundled Vulkan headers
    # Just find the Vulkan loader library and glslc compiler
    
    # Find Vulkan loader library manually
    find_library(Vulkan_LIBRARY
        NAMES vulkan vulkan-1
        HINTS
            ENV VULKAN_SDK
        PATH_SUFFIXES lib
    )
    
    if(NOT Vulkan_LIBRARY)
        message(FATAL_ERROR "Vulkan loader library not found. Please install Vulkan SDK.")
    endif()
    
    # Find glslc shader compiler (comes with Vulkan SDK)
    find_program(GLSLC_EXECUTABLE
        NAMES glslc
        HINTS
            ENV VULKAN_SDK
        PATH_SUFFIXES bin
    )
    
    if(GLSLC_EXECUTABLE)
        message(STATUS "Found glslc: ${GLSLC_EXECUTABLE}")
    else()
        message(WARNING "glslc not found - shaders will not be compiled automatically")
    endif()
endif()

# WebGPU (Dawn)
if(BUILD_WEBGPU_BACKEND)
    # Check for Dawn in multiple locations (submodule, environment, or custom path)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dawn/CMakeLists.txt")
        set(DAWN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dawn")
        message(STATUS "Using Dawn from submodule: ${DAWN_ROOT}")
    elseif(DEFINED ENV{DAWN_ROOT} AND EXISTS "$ENV{DAWN_ROOT}/CMakeLists.txt")
        set(DAWN_ROOT "$ENV{DAWN_ROOT}")
        message(STATUS "Using Dawn from environment variable: ${DAWN_ROOT}")
    elseif(DEFINED DAWN_ROOT AND EXISTS "${DAWN_ROOT}/CMakeLists.txt")
        message(STATUS "Using Dawn from CMake variable: ${DAWN_ROOT}")
    else()
        message(FATAL_ERROR "Dawn not found. Please either:\n"
                            "  1. Initialize submodule: git submodule update --init\n"
                            "  2. Set DAWN_ROOT environment variable\n"
                            "  3. Pass -DDAWN_ROOT=/path/to/dawn to CMake")
    endif()
    
    if(EXISTS "${DAWN_ROOT}/CMakeLists.txt")
        message(STATUS "Adding Dawn from: ${DAWN_ROOT}")
        
        # Save our BUILD_SHARED_LIBS setting
        set(GFX_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
        
        # Dawn build options - Dawn requires BUILD_SHARED_LIBS=OFF
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(DAWN_FETCH_DEPENDENCIES ON CACHE BOOL "" FORCE)
        set(DAWN_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
        set(DAWN_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        
        # Enable SPIR-V support in Tint (required for native WebGPU)
        if(NOT BUILD_FOR_WEB)
            set(TINT_BUILD_SPV_READER ON CACHE BOOL "Enable SPIR-V reader for WebGPU backend" FORCE)
            set(TINT_BUILD_SPV_WRITER ON CACHE BOOL "Enable SPIR-V writer for WebGPU backend" FORCE)
        endif()
        
        # Platform-specific Dawn options
        if(BUILD_FOR_WEB)
            set(DAWN_ENABLE_VULKAN OFF CACHE BOOL "" FORCE)
            set(DAWN_USE_X11 OFF CACHE BOOL "" FORCE)
            set(DAWN_USE_WAYLAND OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_METAL OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_D3D11 OFF CACHE BOOL "" FORCE)
            set(DAWN_ENABLE_D3D12 OFF CACHE BOOL "" FORCE)
        else()
            set(DAWN_ENABLE_VULKAN ON CACHE BOOL "" FORCE)  # Enable Vulkan for native rendering
            if(GFX_HAS_X11 OR GFX_HAS_XCB OR GFX_HAS_WAYLAND)
                set(DAWN_USE_X11 ON CACHE BOOL "" FORCE)
            endif()
            if(GFX_HAS_WAYLAND)
                set(DAWN_USE_WAYLAND ON CACHE BOOL "" FORCE)
            endif()
            if(GFX_HAS_COCOA OR GFX_HAS_UIKIT)
                set(DAWN_ENABLE_METAL ON CACHE BOOL "" FORCE)
            endif()
            if(GFX_HAS_WIN32)
                set(DAWN_ENABLE_D3D11 ON CACHE BOOL "" FORCE)
                set(DAWN_ENABLE_D3D12 ON CACHE BOOL "" FORCE)
            endif()
        endif()
        set(TINT_BUILD_CMD_TOOLS ON CACHE BOOL "" FORCE)  # Build tint command-line tool
        
        # Suppress warnings from third-party code (Dawn and its dependencies)
        set(GFX_SAVED_C_FLAGS "${CMAKE_C_FLAGS}")
        set(GFX_SAVED_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        if(NOT MSVC)
            # Suppress all warnings including compiler flag warnings
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -Wno-unused-command-line-argument")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -Wno-unused-command-line-argument")
        else()
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
        endif()
        
        add_subdirectory(${DAWN_ROOT} ${CMAKE_BINARY_DIR}/dawn EXCLUDE_FROM_ALL)
        
        # Restore warning flags for our code
        set(CMAKE_C_FLAGS "${GFX_SAVED_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${GFX_SAVED_CXX_FLAGS}")
        
        # Restore our BUILD_SHARED_LIBS setting
        set(BUILD_SHARED_LIBS ${GFX_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
        
        set(WEBGPU_INCLUDE_DIR "${DAWN_ROOT}/include")
        set(WEBGPU_FOUND TRUE)
        message(STATUS "WebGPU enabled via Dawn subdirectory")
        message(STATUS "WebGPU include dir: ${WEBGPU_INCLUDE_DIR}")
    else()
        message(WARNING "Dawn directory not found at ${DAWN_ROOT} - WebGPU backend will be disabled")
        set(BUILD_WEBGPU_BACKEND OFF)
    endif()
endif()

# Google Test
if(BUILD_TESTS)
    if(BUILD_WEBGPU_BACKEND AND WEBGPU_FOUND)
        # Use GoogleTest from Dawn's third_party directory
        set(GTEST_INCLUDE_DIR "${DAWN_ROOT}/third_party/googletest/googletest/include")
        set(GMOCK_INCLUDE_DIR "${DAWN_ROOT}/third_party/googletest/googlemock/include")
        message(STATUS "Using GoogleTest from Dawn: ${GTEST_INCLUDE_DIR}")
    else()
        # Fetch GoogleTest if Dawn is not available
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.14.0
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    enable_testing()
    include(GoogleTest)
endif()

# C API Library (implemented in C++)
set(GFX_C_SOURCES
    gfx/include/gfx/gfx.h
    gfx/src/GfxImpl.cpp
    gfx/src/backend/IBackend.h
    gfx/src/backend/Factory.cpp
    gfx/src/backend/Manager.cpp
    gfx/src/common/Logger.cpp
    gfx/src/util/Utils.cpp
)

if(BUILD_VULKAN_BACKEND)
    list(APPEND GFX_C_SOURCES 
        gfx/src/backend/vulkan/Backend.cpp
        # Components
        gfx/src/backend/vulkan/component/SystemComponent.cpp
        gfx/src/backend/vulkan/component/PresentationComponent.cpp
        gfx/src/backend/vulkan/component/ResourceComponent.cpp
        gfx/src/backend/vulkan/component/RenderComponent.cpp
        gfx/src/backend/vulkan/component/ComputeComponent.cpp
        gfx/src/backend/vulkan/component/CommandComponent.cpp
        gfx/src/backend/vulkan/component/SyncComponent.cpp
        gfx/src/backend/vulkan/component/QueryComponent.cpp
        # Converters and validators
        gfx/src/backend/vulkan/converter/Conversions.cpp
        gfx/src/backend/vulkan/validator/Validations.cpp
        # System
        gfx/src/backend/vulkan/core/system/Instance.cpp
        gfx/src/backend/vulkan/core/system/Adapter.cpp
        gfx/src/backend/vulkan/core/system/Device.cpp
        gfx/src/backend/vulkan/core/system/Queue.cpp
        # Resource
        gfx/src/backend/vulkan/core/resource/Buffer.cpp
        gfx/src/backend/vulkan/core/resource/Texture.cpp
        gfx/src/backend/vulkan/core/resource/TextureView.cpp
        gfx/src/backend/vulkan/core/resource/Sampler.cpp
        gfx/src/backend/vulkan/core/resource/Shader.cpp
        gfx/src/backend/vulkan/core/resource/BindGroup.cpp
        gfx/src/backend/vulkan/core/resource/BindGroupLayout.cpp
        # Render
        gfx/src/backend/vulkan/core/render/RenderPipeline.cpp
        gfx/src/backend/vulkan/core/render/RenderPass.cpp
        gfx/src/backend/vulkan/core/render/Framebuffer.cpp
        # Compute
        gfx/src/backend/vulkan/core/compute/ComputePipeline.cpp
        # Command
        gfx/src/backend/vulkan/core/command/CommandEncoder.cpp
        gfx/src/backend/vulkan/core/command/RenderPassEncoder.cpp
        gfx/src/backend/vulkan/core/command/ComputePassEncoder.cpp
        # Sync
        gfx/src/backend/vulkan/core/sync/Semaphore.cpp
        gfx/src/backend/vulkan/core/sync/Fence.cpp
        # Query
        gfx/src/backend/vulkan/core/query/QuerySet.cpp
        # Presentation
        gfx/src/backend/vulkan/core/presentation/Swapchain.cpp
        gfx/src/backend/vulkan/core/presentation/Surface.cpp
        # Util
        gfx/src/backend/vulkan/core/util/CommandExecutor.cpp
        gfx/src/backend/vulkan/core/util/Utils.cpp
    )
endif()

if(BUILD_WEBGPU_BACKEND)
    list(APPEND GFX_C_SOURCES 
        gfx/src/backend/webgpu/Backend.cpp
        gfx/src/backend/webgpu/converter/Conversions.cpp
        gfx/src/backend/webgpu/validator/Validations.cpp
        # Components
        gfx/src/backend/webgpu/component/SystemComponent.cpp
        gfx/src/backend/webgpu/component/PresentationComponent.cpp
        gfx/src/backend/webgpu/component/ResourceComponent.cpp
        gfx/src/backend/webgpu/component/RenderComponent.cpp
        gfx/src/backend/webgpu/component/ComputeComponent.cpp
        gfx/src/backend/webgpu/component/CommandComponent.cpp
        gfx/src/backend/webgpu/component/SyncComponent.cpp
        gfx/src/backend/webgpu/component/QueryComponent.cpp
        # System
        gfx/src/backend/webgpu/core/system/Instance.cpp
        gfx/src/backend/webgpu/core/system/Adapter.cpp
        gfx/src/backend/webgpu/core/system/Device.cpp
        gfx/src/backend/webgpu/core/system/Queue.cpp
        # Resources
        gfx/src/backend/webgpu/core/resource/Buffer.cpp
        gfx/src/backend/webgpu/core/resource/Texture.cpp
        gfx/src/backend/webgpu/core/resource/TextureView.cpp
        gfx/src/backend/webgpu/core/resource/Sampler.cpp
        gfx/src/backend/webgpu/core/resource/Shader.cpp
        gfx/src/backend/webgpu/core/resource/BindGroup.cpp
        gfx/src/backend/webgpu/core/resource/BindGroupLayout.cpp
        # Render
        gfx/src/backend/webgpu/core/render/RenderPipeline.cpp
        gfx/src/backend/webgpu/core/render/RenderPass.cpp
        gfx/src/backend/webgpu/core/render/Framebuffer.cpp
        # Compute
        gfx/src/backend/webgpu/core/compute/ComputePipeline.cpp
        # Command
        gfx/src/backend/webgpu/core/command/CommandEncoder.cpp
        gfx/src/backend/webgpu/core/command/RenderPassEncoder.cpp
        gfx/src/backend/webgpu/core/command/ComputePassEncoder.cpp
        # Sync
        gfx/src/backend/webgpu/core/sync/Fence.cpp
        gfx/src/backend/webgpu/core/sync/Semaphore.cpp
        # Query
        gfx/src/backend/webgpu/core/query/QuerySet.cpp
        # Presentation
        gfx/src/backend/webgpu/core/presentation/Surface.cpp
        gfx/src/backend/webgpu/core/presentation/Swapchain.cpp
        # Util
        gfx/src/backend/webgpu/core/util/Blit.cpp
        gfx/src/backend/webgpu/core/util/Utils.cpp
    )
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
    target_link_libraries(gfx PUBLIC ${Vulkan_LIBRARY})
    target_compile_definitions(gfx PUBLIC GFX_ENABLE_VULKAN=1)
    # Use Dawn's bundled Vulkan headers if WebGPU backend is enabled to avoid version conflicts
    if(BUILD_WEBGPU_BACKEND AND TARGET Vulkan::Headers)
        target_link_libraries(gfx PUBLIC Vulkan::Headers)
    else()
        # Fall back to system Vulkan headers if Dawn is not available
        find_path(SYSTEM_VULKAN_INCLUDE_DIR vulkan/vulkan.h
            HINTS ENV VULKAN_SDK
            PATH_SUFFIXES include
        )
        if(SYSTEM_VULKAN_INCLUDE_DIR)
            target_include_directories(gfx PUBLIC ${SYSTEM_VULKAN_INCLUDE_DIR})
        else()
            message(FATAL_ERROR "Vulkan headers not found. Please install Vulkan SDK.")
        endif()
    endif()
endif()

# Link WebGPU if available
if(BUILD_WEBGPU_BACKEND)
    # Configure WebGPU for native vs web builds
    if(BUILD_FOR_WEB)
        # For web builds, use emdawnwebgpu port
        target_link_options(gfx PUBLIC --use-port=emdawnwebgpu)
        target_include_directories(gfx PUBLIC 
            ${CMAKE_BINARY_DIR}/dawn/gen/src/emdawnwebgpu/include  # Generated headers first
            ${WEBGPU_INCLUDE_DIR}                                  # Source headers second
        )
    else()
        # For native builds, link to webgpu_dawn
        target_link_libraries(gfx PUBLIC webgpu_dawn)
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

# Add headless build definition
if(BUILD_HEADLESS)
    target_compile_definitions(gfx PUBLIC GFX_HEADLESS_BUILD=1)
endif()

# Add windowing system definitions
if(GFX_HAS_WIN32)
    target_compile_definitions(gfx PUBLIC GFX_HAS_WIN32=1)
endif()
if(GFX_HAS_ANDROID)
    target_compile_definitions(gfx PUBLIC GFX_HAS_ANDROID=1)
endif()
if(GFX_HAS_COCOA)
    target_compile_definitions(gfx PUBLIC GFX_HAS_COCOA=1)
endif()
if(GFX_HAS_UIKIT)
    target_compile_definitions(gfx PUBLIC GFX_HAS_UIKIT=1)
endif()
if(GFX_HAS_X11)
    target_compile_definitions(gfx PUBLIC GFX_HAS_X11=1)
endif()
if(GFX_HAS_XCB)
    target_compile_definitions(gfx PUBLIC GFX_HAS_XCB=1)
endif()
if(GFX_HAS_WAYLAND)
    target_compile_definitions(gfx PUBLIC GFX_HAS_WAYLAND=1)
endif()
if(GFX_HAS_EMSCRIPTEN)
    target_compile_definitions(gfx PUBLIC GFX_HAS_EMSCRIPTEN=1)
endif()

# Link platform libraries
target_link_libraries(gfx PUBLIC ${PLATFORM_LIBS})

# C++ API Library (optional)
if(BUILD_CPP_WRAPPER)
    set(GFX_CPP_SOURCES
        gfx_cpp/include/gfx_cpp/gfx.hpp
        gfx_cpp/src/GfxImpl.cpp
        # Converter
        gfx_cpp/src/converter/Conversions.h
        gfx_cpp/src/converter/Conversions.cpp
        # Utilities
        gfx_cpp/src/core/util/HandleExtractor.h
        gfx_cpp/src/core/util/HandleExtractor.cpp
        gfx_cpp/src/core/util/Utils.h
        gfx_cpp/src/core/util/Utils.cpp
        # Sync
        gfx_cpp/src/core/sync/Fence.h
        gfx_cpp/src/core/sync/Fence.cpp
        gfx_cpp/src/core/sync/Semaphore.h
        gfx_cpp/src/core/sync/Semaphore.cpp
        # Query
        gfx_cpp/src/core/query/QuerySet.h
        gfx_cpp/src/core/query/QuerySet.cpp
        # Resources
        gfx_cpp/src/core/resource/Buffer.h
        gfx_cpp/src/core/resource/Buffer.cpp
        gfx_cpp/src/core/resource/Texture.h
        gfx_cpp/src/core/resource/Texture.cpp
        gfx_cpp/src/core/resource/TextureView.h
        gfx_cpp/src/core/resource/TextureView.cpp
        gfx_cpp/src/core/resource/Sampler.h
        gfx_cpp/src/core/resource/Sampler.cpp
        gfx_cpp/src/core/resource/Shader.h
        gfx_cpp/src/core/resource/Shader.cpp
        gfx_cpp/src/core/resource/BindGroup.h
        gfx_cpp/src/core/resource/BindGroup.cpp
        gfx_cpp/src/core/resource/BindGroupLayout.h
        gfx_cpp/src/core/resource/BindGroupLayout.cpp
        # Command
        gfx_cpp/src/core/command/CommandEncoder.h
        gfx_cpp/src/core/command/CommandEncoder.cpp
        gfx_cpp/src/core/command/RenderPassEncoder.h
        gfx_cpp/src/core/command/RenderPassEncoder.cpp
        gfx_cpp/src/core/command/ComputePassEncoder.h
        gfx_cpp/src/core/command/ComputePassEncoder.cpp
        # Render
        gfx_cpp/src/core/render/RenderPipeline.h
        gfx_cpp/src/core/render/RenderPipeline.cpp
        gfx_cpp/src/core/render/RenderPass.h
        gfx_cpp/src/core/render/RenderPass.cpp
        gfx_cpp/src/core/render/Framebuffer.h
        gfx_cpp/src/core/render/Framebuffer.cpp
        # Compute
        gfx_cpp/src/core/compute/ComputePipeline.h
        gfx_cpp/src/core/compute/ComputePipeline.cpp
        # Presentation
        gfx_cpp/src/core/presentation/Surface.h
        gfx_cpp/src/core/presentation/Surface.cpp
        gfx_cpp/src/core/presentation/Swapchain.h
        gfx_cpp/src/core/presentation/Swapchain.cpp
        # System
        gfx_cpp/src/core/system/Queue.h
        gfx_cpp/src/core/system/Queue.cpp
        gfx_cpp/src/core/system/Device.h
        gfx_cpp/src/core/system/Device.cpp
        gfx_cpp/src/core/system/Adapter.h
        gfx_cpp/src/core/system/Adapter.cpp
        gfx_cpp/src/core/system/Instance.h
        gfx_cpp/src/core/system/Instance.cpp
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
        target_link_libraries(gfx_cpp PUBLIC ${Vulkan_LIBRARY})
        target_compile_definitions(gfx_cpp PUBLIC GFX_ENABLE_VULKAN=1)
        # Use Dawn's bundled Vulkan headers if WebGPU backend is enabled to avoid version conflicts
        if(BUILD_WEBGPU_BACKEND AND TARGET Vulkan::Headers)
            target_link_libraries(gfx_cpp PUBLIC Vulkan::Headers)
        endif()
    endif()

    # Link WebGPU if available
    if(BUILD_WEBGPU_BACKEND)
        target_link_libraries(gfx_cpp PUBLIC ${WEBGPU_LIBRARY})
        
        # Configure WebGPU headers for native vs web builds
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
        target_compile_options(gfx_cpp PRIVATE 
            -sDISABLE_EXCEPTION_CATCHING=0
            # -sUSE_GLFW=3
            # -sASYNCIFY=1
        )
        target_link_options(gfx_cpp PUBLIC
            --use-port=emdawnwebgpu
            -sUSE_GLFW=3
            -sASYNCIFY=1
            -sALLOW_MEMORY_GROWTH=1
            -sDISABLE_EXCEPTION_CATCHING=0
        )
    endif()

    # Compiler flags
    if(MSVC)
        target_compile_options(gfx_cpp PRIVATE /W4)
    else()
        target_compile_options(gfx_cpp PRIVATE -Wall -Wextra -Wpedantic -g)
        
        # Address Sanitizer (optional)
        if(ENABLE_ASAN)
            target_compile_options(gfx_cpp PRIVATE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(gfx_cpp PRIVATE -fsanitize=address)
        endif()
    endif()
endif()  # BUILD_CPP_WRAPPER

# C API library compiler flags
if(MSVC)
    target_compile_options(gfx PRIVATE /W4)
else()
    target_compile_options(gfx PRIVATE -Wall -Wextra -Wpedantic -g)
    
    # Address Sanitizer (optional)
    if(ENABLE_ASAN)
        target_compile_options(gfx PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(gfx PRIVATE -fsanitize=address)
    endif()
endif()

# Emscripten-specific configuration for C library
if(BUILD_FOR_WEB)
    target_compile_options(gfx PRIVATE 
        -sDISABLE_EXCEPTION_CATCHING=0
        # -sUSE_GLFW=3
        # -sASYNCIFY=1
    )
    target_link_options(gfx PUBLIC
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        -sASYNCIFY=1
        -sALLOW_MEMORY_GROWTH=1
        -sDISABLE_EXCEPTION_CATCHING=0
    )
endif()

# Examples
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()  # BUILD_EXAMPLES

# Add tests
if(BUILD_TESTS AND NOT BUILD_FOR_WEB)
    add_subdirectory(test/gfx)
    if(BUILD_CPP_WRAPPER)
        add_subdirectory(test/gfx_cpp)
    endif()
endif()

# Install targets
if(BUILD_CPP_WRAPPER)
    install(TARGETS gfx gfx_cpp
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    install(FILES gfx/include/gfx/gfx.h DESTINATION include/gfx)
    install(FILES gfx_cpp/include/gfx_cpp/gfx.hpp DESTINATION include/gfx_cpp)
else()
    install(TARGETS gfx
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
    install(FILES gfx/include/gfx/gfx.h DESTINATION include/gfx)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "Gfx Configuration Summary:")
message(STATUS "  Library type: ${BUILD_SHARED_LIBS}")
if(BUILD_SHARED_LIBS)
    message(STATUS "  Building SHARED libraries")
else()
    message(STATUS "  Building STATIC libraries")
endif()
message(STATUS "  Build examples: ${BUILD_EXAMPLES}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "  Build C++ wrapper: ${BUILD_CPP_WRAPPER}")
message(STATUS "  Headless build: ${BUILD_HEADLESS}")
message(STATUS "  Address Sanitizer: ${ENABLE_ASAN}")
message(STATUS "  Vulkan backend: ${BUILD_VULKAN_BACKEND}")
message(STATUS "  WebGPU backend: ${BUILD_WEBGPU_BACKEND}")
if(BUILD_VULKAN_BACKEND)
    message(STATUS "  Vulkan library: ${Vulkan_LIBRARY}")
endif()
if(BUILD_WEBGPU_BACKEND AND WEBGPU_FOUND)
    message(STATUS "  WebGPU library: webgpu_dawn (target)")
endif()
message(STATUS "")