// Bitmap font atlas cells for the track vertex shader as corner and size in texture coordinates. The length
// is RHI_GLYPH_TABLE_SIZE, at least the font's glyph count, and the set is the one RHI_GAL_PIPELINES gives.
const uint GLYPH_TABLE_SIZE = 1280u;

layout( set = 1, binding = 2 ) uniform GlyphTable
{
    vec4 glyphRects[GLYPH_TABLE_SIZE];
    vec4 atlasSize;
};
