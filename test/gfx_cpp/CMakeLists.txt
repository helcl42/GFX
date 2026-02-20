# C++ Wrapper Tests

# =============================================================================
# Public API Tests - Always built, backend-agnostic
# =============================================================================
add_executable(gfx_cpp_api_test
    api/AdapterTest.cpp
    api/BackendTest.cpp
    api/BindGroupLayoutTest.cpp
    api/BindGroupTest.cpp
    api/BufferTest.cpp
    api/CommandEncoderTest.cpp
    api/ComputePassEncoderTest.cpp
    api/ComputePipelineTest.cpp
    api/ConstantTest.cpp
    api/DeviceTest.cpp
    api/FenceTest.cpp
    api/FramebufferTest.cpp
    api/InstanceTest.cpp
    api/QuerySetTest.cpp
    api/QueueTest.cpp
    api/RenderPassEncoderTest.cpp
    api/RenderPassTest.cpp
    api/RenderPipelineTest.cpp
    api/SamplerTest.cpp
    api/SemaphoreTest.cpp
    api/ShaderTest.cpp
    api/SurfaceTest.cpp
    api/SwapchainTest.cpp
    api/TextureTest.cpp
    api/TextureViewTest.cpp
    api/UtilTest.cpp
)

target_link_libraries(gfx_cpp_api_test
    gfx_cpp
    GTest::gtest_main
)

# Copy DLLs to test directory on Windows for easy execution
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET gfx_cpp_api_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gfx>
        $<TARGET_FILE_DIR:gfx_cpp_api_test>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:gfx_cpp>
        $<TARGET_FILE_DIR:gfx_cpp_api_test>
    )

    # Copy d3dcompiler_47.dll for WebGPU shader compilation if WebGPU backend is enabled
    if(BUILD_WEBGPU_BACKEND)
        find_file(D3DCOMPILER_DLL_CPP d3dcompiler_47.dll
            PATHS
                "$ENV{WindowsSdkDir}/Redist/D3D/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64"
                "C:/Windows/System32"
                "C:/Windows/SysWOW64"
        )
        if(D3DCOMPILER_DLL_CPP)
            add_custom_command(TARGET gfx_cpp_api_test POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${D3DCOMPILER_DLL_CPP}
                $<TARGET_FILE_DIR:gfx_cpp_api_test>
            )
        endif()
    endif()
endif()

if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_cpp_api_test
        DISCOVER_MODE_PRE_TEST
    )
endif()

# =============================================================================
# Internal Tests - Always built
# =============================================================================
add_executable(gfx_cpp_internal_test
    internal/GfxImplTest.cpp
    internal/converter/ConversionsTest.cpp
    internal/core/util/HandleExtractorTest.cpp
    internal/core/resource/BufferTest.cpp
    internal/core/resource/TextureTest.cpp
    internal/core/resource/TextureViewTest.cpp
    internal/core/resource/SamplerTest.cpp
    internal/core/resource/ShaderTest.cpp
    internal/core/resource/BindGroupLayoutTest.cpp
    internal/core/resource/BindGroupTest.cpp
    internal/core/system/InstanceTest.cpp
    internal/core/system/AdapterTest.cpp
    internal/core/system/DeviceTest.cpp
    internal/core/system/QueueTest.cpp
    internal/core/sync/FenceTest.cpp
    internal/core/sync/SemaphoreTest.cpp
    internal/core/render/RenderPipelineTest.cpp
    internal/core/render/RenderPassTest.cpp
    internal/core/render/FramebufferTest.cpp
    internal/core/compute/ComputePipelineTest.cpp
    internal/core/query/QuerySetTest.cpp
    internal/core/command/CommandEncoderTest.cpp
    internal/core/command/RenderPassEncoderTest.cpp
    internal/core/command/ComputePassEncoderTest.cpp
    internal/core/presentation/SurfaceTest.cpp
    internal/core/presentation/SwapchainTest.cpp
)

target_include_directories(gfx_cpp_internal_test PRIVATE
    ${CMAKE_SOURCE_DIR}/gfx_cpp/src
)

target_link_libraries(gfx_cpp_internal_test
    gfx_cpp
    GTest::gtest_main
)

if(NOT ENABLE_ASAN)
    gtest_discover_tests(gfx_cpp_internal_test
        DISCOVER_MODE_PRE_TEST
    )
endif()
