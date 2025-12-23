#version 450

layout(location = 0) in vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler texSampler;
layout(binding = 1) uniform texture2D inputTexture;

void main() {
    vec4 color = texture(sampler2D(inputTexture, texSampler), inTexCoord);
    
    // Post-processing: vignette effect
    vec2 center = inTexCoord - 0.5;
    float vignette = 1.0 - length(center) * 0.8;
    
    // Apply vignette
    color.rgb *= vignette;
    
    outColor = color;
}
