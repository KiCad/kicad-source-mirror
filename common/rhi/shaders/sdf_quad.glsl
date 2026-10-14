// A one pixel coverage ramp centred on the edge, discarded under 0.004, ends 0.5 - 0.0367 pixels outside it.
// The quad reaches that far plus a little for rounding of the interpolated distance.
const float QUAD_REACH_PIXELS = 0.465;

// The pixel size a fragment shader ramps its edge over. A negative one tells it to cut the edge
// hard, for RHI_INSTANCE_HARD_EDGE and for a supersampled target, whose average is the ramp.
float edgePixelSize( float aPixelSize, uint aFlags )
{
    return ( supersampled() || ( aFlags & 0x80u ) != 0u ) ? -aPixelSize : aPixelSize;
}
