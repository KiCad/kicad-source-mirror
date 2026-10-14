// Push constants of the compositor shaders, see COMPOSITOR_PUSH_CONSTANTS
layout( push_constant ) uniform PushConstants
{
    float layerAlpha;
    vec2  uvScale;
    vec2  uvOffset;
};
