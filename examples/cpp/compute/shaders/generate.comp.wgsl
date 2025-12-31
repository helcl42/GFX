// Compute shader that generates a procedural pattern

struct ComputeUniforms {
    time: f32,
}

@group(0) @binding(0) var outputImage: texture_storage_2d<rgba8unorm, write>;
@group(0) @binding(1) var<uniform> ubo: ComputeUniforms;

@compute @workgroup_size(16, 16, 1)
fn main(@builtin(global_invocation_id) globalId: vec3<u32>) {
    let texelCoord = vec2<i32>(globalId.xy);
    let size = textureDimensions(outputImage);
    
    let uv = vec2<f32>(texelCoord) / vec2<f32>(size);
    
    // Create animated pattern using time
    let wave1 = sin(uv.x * 10.0 + ubo.time * 2.0) * 0.5 + 0.5;
    let wave2 = cos(uv.y * 10.0 + ubo.time * 1.5) * 0.5 + 0.5;
    let pattern = wave1 * wave2;
    
    // Color based on position, pattern, and time
    let color = vec3<f32>(
        pattern * uv.x,
        pattern * uv.y * (0.5 + 0.5 * sin(ubo.time)),
        pattern * (1.0 - uv.x) * (0.5 + 0.5 * cos(ubo.time))
    );
    
    textureStore(outputImage, texelCoord, vec4<f32>(color, 1.0));
}
