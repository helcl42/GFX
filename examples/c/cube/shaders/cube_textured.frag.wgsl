@group(1) @binding(0) var cubeTexture: texture_2d<f32>;
@group(1) @binding(1) var cubeSampler: sampler;

// Fragment input
struct FragmentInput {
    @location(0) texCoord: vec2<f32>,
}

// Fragment output
struct FragmentOutput {
    @location(0) outColor: vec4<f32>,
}

@fragment
fn main(input: FragmentInput) -> FragmentOutput {
    var output: FragmentOutput;
    
    // Sample texture
    output.outColor = textureSample(cubeTexture, cubeSampler, input.texCoord);
    
    return output;
}
