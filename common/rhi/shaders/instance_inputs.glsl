// Per-vertex (unit quad)
layout( location = 0 ) in vec2 inPosition;
layout( location = 1 ) in vec4 inColor;
layout( location = 2 ) in vec2 inTexCoord;

// Per-instance from RHI_INSTANCE
layout( location = 3 ) in vec2  instPosition;
layout( location = 4 ) in vec4  instRotScaleDepth;
layout( location = 5 ) in vec4  instParams;
layout( location = 6 ) in vec4  instColor;
layout( location = 7 ) in uvec2 instFlags;
