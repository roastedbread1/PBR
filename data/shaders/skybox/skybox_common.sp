layout(push_constant) uniform SkyboxPushConstants {
    mat4 view;
    mat4 proj;
    uint texCube;
    uint samplerIdx;
} pc;