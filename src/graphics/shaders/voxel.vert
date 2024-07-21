#version 460

layout (location = 0) in uint packed;

layout (location = 0) out vec4 color;

layout (location = 1) uniform mat4 mvp;

void main() {
    float x = bitfieldExtract(packed, 0, 6);
    float y = bitfieldExtract(packed, 6, 8);
    float z = bitfieldExtract(packed, 14, 6);

    uint r = bitfieldExtract(packed, 20, 4);
    uint g = bitfieldExtract(packed, 24, 4);
    uint b = bitfieldExtract(packed, 28, 4);

    gl_Position = mvp * vec4(x, y, z, 1.0);
    color = vec4(r / 15.0, g / 15.0, b / 15.0, 1.0);
}
