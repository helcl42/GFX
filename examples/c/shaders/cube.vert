#version 450

// Uniform buffer with transformation matrices
layout(binding = 0) uniform Uniforms {
    mat4 model;
    mat4 view;
    mat4 projection;
} uniforms;

// Vertex input attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

// Output to fragment shader
layout(location = 0) out vec3 fragColor;

void main() {
    // Transform vertex position through model, view, and projection matrices
    vec4 worldPos = uniforms.model * vec4(position, 1.0);
    vec4 viewPos = uniforms.view * worldPos;
    gl_Position = uniforms.projection * viewPos;
    
    // Pass color to fragment shader
    fragColor = color;
}
