@group(1) @binding(0) var cubeTexture: texture_2d<f32>;
@group(1) @binding(1) var cubeSampler: sampler;

// Fragment input
struct FragmentInput {
    @location(0) fragColor: vec3<f32>,
    @location(1) texCoord: vec2<f32>,
}

// Fragment output
struct FragmentOutput {
    @location(0) outColor: vec4<f32>,
}

@fragment
fn main(input: FragmentInput) -> FragmentOutput {
    var output: FragmentOutput;
    
    // Sample texture and multiply by vertex color
    let texColor = textureSample(cubeTexture, cubeSampler, input.texCoord);
    output.outColor = texColor * vec4<f32>(input.fragColor, 1.0);
    
    return output;
}
