// Per-frame uniform block shared by every 2D vertex shader, kept in one place so its layout stays identical
layout( set = 0, binding = 1 ) uniform FrameUBO
{
    mat4  projection;
    vec2  viewport;
    float viewScale;    // the view's scale, which levels of detail are compared with
    float renderScale;  // target texels per viewport pixel, above 1 in a supersampled target
};

// A supersampled target is averaged into the picture, which is its antialiasing, and is drawn
// half a pixel off, see rhi_supersample.h
bool supersampled()
{
    return renderScale > 1.0;
}

// A position with that half pixel taken back out
vec4 withoutSupersampleOffset( vec4 aPosition )
{
    return supersampled() ? aPosition - vec4( aPosition.w / viewport, 0.0, 0.0 ) : aPosition;
}
