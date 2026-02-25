# C API Tests (using C++ for GoogleTest)

# =============================================================================
# Public API Tests - Always built, backend-agnostic
# =============================================================================
add_executable(gfx_api_test
    api/CommonTest.h
    api/ConstantTest.cpp
    api/BackendTest.cpp
    api/InstanceTest.cpp
    api/AdapterTest.cpp
    api/DeviceTest.cpp
    api/BufferTest.cpp
    api/TextureTest.cpp
    api/TextureViewTest.cpp
    api/SamplerTest.cpp
    api/ShaderTest.cpp
    api/BindGroupLayoutTest.cpp
    api/BindGroupTest.cpp
    api/RenderPassTest.cpp
    api/FramebufferTest.cpp
    api/RenderPipelineTest.cpp
    api/ComputePipelineTest.cpp
    api/QueueTest.cpp
    api/CommandEncoderTest.cpp
    api/RenderPassEncoderTest.cpp
    api/ComputePassEncoderTest.cpp
    api/FenceTest.cpp
    api/SemaphoreTest.cpp
    api/QuerySetTest.cpp
    api/UtilTest.cpp
    api/SurfaceTest.cpp
    api/SwapchainTest.cpp
)

target_link_libraries(gfx_api_test
    gfx
    GTest::gtest_main
    GTest::gmock
)

# Copy DLL to test directory on Windows for easy execution
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET gfx_api_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gfx>
        $<TARGET_FILE_DIR:gfx_api_test>
    )
endif()

if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_api_test
        DISCOVER_MODE_PRE_TEST
    )
endif()

# =============================================================================
# Internal Common Tests - Always built
# =============================================================================
add_executable(gfx_internal_test
    internal/GfxImplTest.cpp
    internal/ManagerTest.cpp
    internal/FactoryTest.cpp
    internal/common/LoggerTest.cpp
)

# Link to object library for internal tests (if shared library) or directly to gfx (if static)
if(BUILD_SHARED_LIBS)
    target_link_libraries(gfx_internal_test
        gfx_objects
        GTest::gtest_main
        GTest::gmock
    )
else()
    target_link_libraries(gfx_internal_test
        gfx
        GTest::gtest_main
        GTest::gmock
    )
endif()

target_include_directories(gfx_internal_test PRIVATE
    ${CMAKE_SOURCE_DIR}/gfx/src
)

if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_internal_test
        DISCOVER_MODE_PRE_TEST
    )
endif()

# =============================================================================
# Vulkan Backend Tests - Only when Vulkan backend is enabled
# =============================================================================
if(BUILD_VULKAN_BACKEND AND NOT BUILD_FOR_WEB)
    add_executable(gfx_internal_vulkan_test
        internal/backend/vulkan/converter/ConversionsTest.cpp
        internal/backend/vulkan/core/resource/BindGroupTest.cpp
        internal/backend/vulkan/core/resource/BindGroupLayoutTest.cpp
        internal/backend/vulkan/core/resource/BufferTest.cpp
        internal/backend/vulkan/core/resource/SamplerTest.cpp
        internal/backend/vulkan/core/resource/ShaderTest.cpp
        internal/backend/vulkan/core/resource/TextureTest.cpp
        internal/backend/vulkan/core/resource/TextureViewTest.cpp
        internal/backend/vulkan/core/sync/FenceTest.cpp
        internal/backend/vulkan/core/sync/SemaphoreTest.cpp
        internal/backend/vulkan/core/system/AdapterTest.cpp
        internal/backend/vulkan/core/system/DeviceTest.cpp
        internal/backend/vulkan/core/system/InstanceTest.cpp
        internal/backend/vulkan/core/system/QueueTest.cpp
        internal/backend/vulkan/core/util/CommandExecutorTest.cpp
        internal/backend/vulkan/core/util/UtilsTest.cpp
        internal/backend/vulkan/core/render/FramebufferTest.cpp
        internal/backend/vulkan/core/render/RenderPassTest.cpp
        internal/backend/vulkan/core/render/RenderPipelineTest.cpp
        internal/backend/vulkan/core/query/QuerySetTest.cpp
        internal/backend/vulkan/core/presentation/SurfaceTest.cpp
        internal/backend/vulkan/core/presentation/SwapchainTest.cpp
        internal/backend/vulkan/core/compute/ComputePipelineTest.cpp
        internal/backend/vulkan/core/command/CommandEncoderTest.cpp
        internal/backend/vulkan/core/command/ComputePassEncoderTest.cpp
        internal/backend/vulkan/core/command/RenderPassEncoderTest.cpp
    )

    # Link to object library for internal tests (if shared library) or directly to gfx (if static)
    if(BUILD_SHARED_LIBS)
        target_link_libraries(gfx_internal_vulkan_test
            gfx_objects
            GTest::gtest_main
            GTest::gmock
        )
    else()
        target_link_libraries(gfx_internal_vulkan_test
            gfx
            GTest::gtest_main
            GTest::gmock
        )
    endif()

    target_include_directories(gfx_internal_vulkan_test PRIVATE
        ${CMAKE_SOURCE_DIR}/gfx/src
    )

    if(NOT ENABLE_ASAN)
        gtest_discover_tests(gfx_internal_vulkan_test
            DISCOVER_MODE_PRE_TEST
        )
    endif()
    
    message(STATUS "Building Vulkan backend tests: gfx_internal_vulkan_test")
endif()

# =============================================================================
# WebGPU Backend Tests - Only when WebGPU backend is enabled
# =============================================================================
if(BUILD_WEBGPU_BACKEND)
    add_executable(gfx_internal_webgpu_test
        internal/backend/webgpu/core/system/InstanceTest.cpp
        internal/backend/webgpu/core/system/AdapterTest.cpp
        internal/backend/webgpu/core/system/DeviceTest.cpp
        internal/backend/webgpu/core/system/QueueTest.cpp
        internal/backend/webgpu/core/resource/BufferTest.cpp
        internal/backend/webgpu/core/resource/TextureTest.cpp
        internal/backend/webgpu/core/resource/TextureViewTest.cpp
        internal/backend/webgpu/core/resource/SamplerTest.cpp
        internal/backend/webgpu/core/resource/ShaderTest.cpp
        internal/backend/webgpu/core/resource/BindGroupLayoutTest.cpp
        internal/backend/webgpu/core/resource/BindGroupTest.cpp
        internal/backend/webgpu/core/sync/FenceTest.cpp
        internal/backend/webgpu/core/sync/SemaphoreTest.cpp
        internal/backend/webgpu/core/render/RenderPassTest.cpp
        internal/backend/webgpu/core/render/FramebufferTest.cpp
        internal/backend/webgpu/core/render/RenderPipelineTest.cpp
        internal/backend/webgpu/core/compute/ComputePipelineTest.cpp
        internal/backend/webgpu/core/query/QuerySetTest.cpp
        internal/backend/webgpu/core/presentation/SurfaceTest.cpp
        internal/backend/webgpu/core/presentation/SwapchainTest.cpp
        internal/backend/webgpu/core/command/CommandEncoderTest.cpp
        internal/backend/webgpu/core/command/ComputePassEncoderTest.cpp
        internal/backend/webgpu/core/command/RenderPassEncoderTest.cpp
        internal/backend/webgpu/core/util/BlitTest.cpp
        internal/backend/webgpu/core/util/UtilsTest.cpp
        internal/backend/webgpu/converter/ConversionsTest.cpp
    )

    # Link to object library for internal tests (if shared library) or directly to gfx (if static)
    if(BUILD_SHARED_LIBS)
        target_link_libraries(gfx_internal_webgpu_test
            gfx_objects
            GTest::gtest_main
            GTest::gmock
        )
    else()
        target_link_libraries(gfx_internal_webgpu_test
            gfx
            GTest::gtest_main
            GTest::gmock
        )
    endif()

    target_include_directories(gfx_internal_webgpu_test PRIVATE
        ${CMAKE_SOURCE_DIR}/gfx/src
    )

    if(WIN32 AND BUILD_SHARED_LIBS)
       # Copy d3dcompiler_47.dll for WebGPU shader compilation (usually in Windows SDK)
        find_file(D3DCOMPILER_DLL d3dcompiler_47.dll
            PATHS
                "$ENV{WindowsSdkDir}/Redist/D3D/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
                "$ENV{WindowsSdkDir}/Redist/D3D/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x86"
                "C:/Windows/System32"
                "C:/Windows/SysWOW64"
        )
        if(D3DCOMPILER_DLL)
            add_custom_command(TARGET gfx_internal_webgpu_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${D3DCOMPILER_DLL}
                $<TARGET_FILE_DIR:gfx_internal_webgpu_test>
            )
        endif()
    endif()

    if(NOT ENABLE_ASAN)
        gtest_discover_tests(gfx_internal_webgpu_test
            DISCOVER_MODE_PRE_TEST
        )
    endif()
    
    message(STATUS "Building WebGPU backend tests: gfx_internal_webgpu_test")
endif()
