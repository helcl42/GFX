// Uniform buffer with transformation matrices
struct Uniforms {
    model: mat4x4<f32>,
    view: mat4x4<f32>,
    projection: mat4x4<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

// Vertex input
struct VertexInput {
    @location(0) position: vec3<f32>,
    @location(1) texCoord: vec2<f32>,
}

// Vertex output / Fragment input
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) texCoord: vec2<f32>,
}

@vertex
fn main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    
    // Transform vertex position through model, view, and projection matrices
    let worldPos = uniforms.model * vec4<f32>(input.position, 1.0);
    let viewPos = uniforms.view * worldPos;
    output.position = uniforms.projection * viewPos;
    
    // Pass texture coordinates to fragment shader
    output.texCoord = input.texCoord;
    
    return output;
}
