// Shared by the geometry vertex shaders. Include after frame_ubo.glsl.

// An element whose alpha rounds to nothing in an 8 bit target leaves the picture as it was, so it is
// not worth a quad. The overlay form leaves c*a and a*a, and a hard edge element still claims depth.
bool elementLeavesNoMark( vec4 aColor, uint aFlags )
{
    const float quantum = 0.5 / 255.0;

    if( ( aFlags & 0x80u ) != 0u )
        return false;

    if( ( aFlags & 0x400u ) != 0u )
        return aColor.a * max( aColor.r, max( aColor.g, aColor.b ) ) < quantum && aColor.a * aColor.a < quantum;

    return aColor.a < quantum;
}
