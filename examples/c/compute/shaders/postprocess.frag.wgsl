struct RenderUniforms {
    postProcessStrength: f32,
}

@group(0) @binding(0) var texSampler: sampler;
@group(0) @binding(1) var inputTexture: texture_2d<f32>;
@group(0) @binding(2) var<uniform> ubo: RenderUniforms;

struct FragmentInput {
    @location(0) texCoord: vec2<f32>,
}

@fragment
fn main(input: FragmentInput) -> @location(0) vec4<f32> {
    var color = textureSample(inputTexture, texSampler, input.texCoord);
    
    // Post-processing: vignette effect with animated strength
    let center = input.texCoord - 0.5;
    let vignette = 1.0 - length(center) * 0.8 * ubo.postProcessStrength;
    
    // Apply vignette
    color = vec4<f32>(color.rgb * vignette, color.a);
    
    return color;
}
