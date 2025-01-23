#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec3 tintColor; // New uniform for tint
uniform float tintStrength; // Control tint intensity
out vec4 finalColor;

const float renderWidth = 800.0;
const float renderHeight = 450.0;

const float kernel[25] = float[](
    1.0/256.0,  4.0/256.0,  6.0/256.0,  4.0/256.0, 1.0/256.0,
    4.0/256.0, 16.0/256.0, 24.0/256.0, 16.0/256.0, 4.0/256.0,
    6.0/256.0, 24.0/256.0, 36.0/256.0, 24.0/256.0, 6.0/256.0,
    4.0/256.0, 16.0/256.0, 24.0/256.0, 16.0/256.0, 4.0/256.0,
    1.0/256.0,  4.0/256.0,  6.0/256.0,  4.0/256.0, 1.0/256.0
);

void main()
{
    vec2 texelSize = 1.0 / vec2(renderWidth, renderHeight);
    vec3 blurColor = vec3(0.0);
    
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            vec2 offset = vec2(x, y) * texelSize;
            blurColor += texture(texture0, fragTexCoord + offset).rgb * 
                kernel[(x+2) + (y+2) * 5];
        }
    }
    
    // Apply tint
    blurColor = mix(blurColor, blurColor * tintColor, tintStrength);
    
    finalColor = vec4(blurColor, 1.0);
}