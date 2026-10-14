// Fades a retained layer as a whole for high contrast instead of recolouring each element. layerDim.a is the
// share of the element's own colour kept, so ( 0, 0, 0, 1 ) leaves colours as they are.
layout( push_constant ) uniform PushConstants
{
    vec4 layerDim;
};

vec4 applyLayerDim( vec4 aColor, bool aExempt )
{
    if( aExempt )
        return aColor;

    return vec4( mix( layerDim.rgb, aColor.rgb, layerDim.a ), aColor.a );
}
