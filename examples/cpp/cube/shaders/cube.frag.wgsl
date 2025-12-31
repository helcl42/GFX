// Fragment input
struct FragmentInput {
    @location(0) fragColor: vec3<f32>,
}

// Fragment output
struct FragmentOutput {
    @location(0) outColor: vec4<f32>,
}

@fragment
fn main(input: FragmentInput) -> FragmentOutput {
    var output: FragmentOutput;
    
    // Output the interpolated color with full opacity
    output.outColor = vec4<f32>(input.fragColor, 1.0);
    
    return output;
}
