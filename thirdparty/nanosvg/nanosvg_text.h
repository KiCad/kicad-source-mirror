// KiCad additions, licensed under the same zlib license as NanoSVG.

struct NSVGtextPosition
{
    std::vector<float> values[4];
    size_t             index = 0;
};

struct NSVGtext
{
    FT_Library                    library = nullptr;
    std::vector<NSVGtextPosition> positions;
    std::vector<NSVGshape*>       chunk;
    float                         x = 0;
    float                         y = 0;
    float                         chunkStart = 0;
    int                           anchor = 0;
    int                           ignoredDepth = 0;
    bool                          chunkActive = false;
    bool                          pendingSpace = false;
    bool                          hasContent = false;

    ~NSVGtext()
    {
        if( library )
            FT_Done_FreeType( library );
    }
};

static void nsvg__deleteText( NSVGtext* text )
{
    delete text;
}

static void nsvg__finishTextChunk( NSVGtext& text )
{
    float offset = -( text.x - text.chunkStart ) * text.anchor * 0.5f;

    for( NSVGshape* shape : text.chunk )
    {
        float dx = offset * shape->xform[0];
        float dy = offset * shape->xform[1];
        shape->xform[4] += dx;
        shape->xform[5] += dy;

        for( int i = 0; i < 4; i += 2 )
        {
            shape->bounds[i] += dx;
            shape->bounds[i + 1] += dy;
        }

        for( NSVGpath* path = shape->paths; path; path = path->next )
        {
            for( int i = 0; i < 4; i += 2 )
            {
                path->bounds[i] += dx;
                path->bounds[i + 1] += dy;
            }

            for( int i = 0; i < path->npts; ++i )
            {
                path->pts[i * 2] += dx;
                path->pts[i * 2 + 1] += dy;
            }
        }
    }

    text.chunk.clear();
    text.chunkActive = false;
    text.chunkStart = text.x;
}

static void nsvg__startText( NSVGparser* p, const char* el, const char** attributes )
{
    if( !p->text )
        p->text = new NSVGtext;

    NSVGtext& text = *p->text;

    if( text.ignoredDepth || p->attrHead == NSVG_MAX_ATTR - 1 )
    {
        ++text.ignoredDepth;
        return;
    }

    if( text.positions.empty() )
    {
        if( strcmp( el, "text" ) != 0 )
            return;

        text.x = text.y = 0;
        text.chunkStart = 0;
        text.hasContent = text.pendingSpace = false;
    }

    nsvg__pushAttr( p );
    nsvg__parseAttribs( p, attributes );
    NSVGtextPosition position;
    const char*      names[] = { "x", "y", "dx", "dy" };

    for( int i = 0; attributes[i]; i += 2 )
    {
        for( int axis = 0; axis < 4; ++axis )
        {
            if( strcmp( attributes[i], names[axis] ) != 0 )
                continue;

            const char* cursor = attributes[i + 1];
            char        item[64];

            while( *cursor )
            {
                const char* next = nsvg__getNextDashItem( cursor, item );

                if( !*item || next == cursor )
                    break;

                float value = nsvg__parseCoordinate( p, item, 0,
                                                     axis % 2 ? nsvg__actualHeight( p ) : nsvg__actualWidth( p ) );

                if( std::isfinite( value ) )
                    position.values[axis].push_back( value );

                cursor = next;
            }
        }
    }

    text.positions.push_back( std::move( position ) );
}

static void nsvg__endText( NSVGparser* p )
{
    if( !p->text )
        return;

    NSVGtext& text = *p->text;

    if( text.ignoredDepth )
    {
        --text.ignoredDepth;
        return;
    }

    if( text.positions.empty() )
        return;

    text.positions.pop_back();
    nsvg__popAttr( p );

    if( text.positions.empty() )
        nsvg__finishTextChunk( text );
}

static std::vector<uint32_t> nsvg__textCodepoints( const char* s, bool cdata )
{
    std::vector<uint32_t> result;
    const char*           end = s + strlen( s );

    while( s < end )
    {
        FcChar32 codepoint = 0;
        int      length = FcUtf8ToUcs4( reinterpret_cast<const FcChar8*>( s ), &codepoint,
                                        static_cast<int>( std::min<ptrdiff_t>( end - s, 4 ) ) );

        if( length <= 0 )
        {
            length = 1;
            codepoint = 0xfffd;
        }

        if( !cdata && *s == '&' )
        {
            const char* semicolon = static_cast<const char*>( memchr( s, ';', std::min<ptrdiff_t>( end - s, 13 ) ) );

            if( semicolon && semicolon - s <= 12 )
            {
                std::string entity( s + 1, semicolon );
                const char* names[] = { "amp", "lt", "gt", "quot", "apos" };
                const char  values[] = { '&', '<', '>', '"', '\'' };
                bool        decoded = false;

                for( size_t i = 0; i < 5; ++i )
                {
                    if( entity == names[i] )
                    {
                        codepoint = values[i];
                        decoded = true;
                    }
                }

                if( !entity.empty() && entity[0] == '#' )
                {
                    bool          hex = entity.size() > 1 && entity[1] == 'x';
                    const char*   first = entity.c_str() + ( hex ? 2 : 1 );
                    char*         last = nullptr;
                    unsigned long value = strtoul( first, &last, hex ? 16 : 10 );
                    decoded = last != first && *last == 0;

                    if( decoded )
                    {
                        codepoint = value && value <= 0x10ffff && ( value < 0xd800 || value > 0xdfff )
                                            ? static_cast<uint32_t>( value )
                                            : 0xfffd;
                    }
                }

                if( decoded )
                    length = static_cast<int>( semicolon - s + 1 );
            }
        }

        result.push_back( codepoint );
        s += length;
    }

    return result;
}

struct NSVGtextOutline
{
    NSVGparser* parser;
    float       x;
    float       y;
};

static void nsvg__textContent( NSVGparser* p, const char* content, bool cdata )
{
    if( !p->text || p->text->positions.empty() || p->text->ignoredDepth || p->defsFlag )
        return;

    NSVGtext&             text = *p->text;
    NSVGattrib*           attr = nsvg__getAttr( p );
    std::vector<uint32_t> codepoints;

    for( uint32_t codepoint : nsvg__textCodepoints( content, cdata ) )
    {
        bool space = codepoint == ' ' || codepoint == '\t' || codepoint == '\r' || codepoint == '\n';

        if( !attr->preserveSpace && space )
        {
            text.pendingSpace = text.hasContent || !codepoints.empty();
            continue;
        }

        if( text.pendingSpace )
            codepoints.push_back( ' ' );

        text.pendingSpace = false;
        codepoints.push_back( space ? ' ' : codepoint );
    }

    if( codepoints.empty() || !std::isfinite( attr->fontSize ) || attr->fontSize <= 0 || attr->fontSize > 1e6f )
        return;

    if( !text.library && FT_Init_FreeType( &text.library ) )
        return;

    std::unique_ptr<FcPattern, decltype( &FcPatternDestroy )> pattern( FcPatternCreate(), FcPatternDestroy );

    if( !pattern )
        return;

    std::string families = attr->fontFamily;
    size_t      start = 0;

    while( start < families.size() )
    {
        size_t      end = families.find( ',', start );
        std::string family = families.substr( start, end - start );
        size_t      first = family.find_first_not_of( " \t\"'" );
        size_t      last = family.find_last_not_of( " \t\"'" );

        if( first != std::string::npos )
            FcPatternAddString( pattern.get(), FC_FAMILY,
                                reinterpret_cast<const FcChar8*>( family.substr( first, last - first + 1 ).c_str() ) );

        if( end == std::string::npos )
            break;

        start = end + 1;
    }

    FcPatternAddInteger( pattern.get(), FC_WEIGHT, attr->fontWeight );
    FcPatternAddInteger( pattern.get(), FC_SLANT, attr->fontItalic ? FC_SLANT_ITALIC : FC_SLANT_ROMAN );
    FcConfigSubstitute( p->fontConfig, pattern.get(), FcMatchPattern );
    FcDefaultSubstitute( pattern.get() );
    FcResult                                                  matchResult;
    std::unique_ptr<FcPattern, decltype( &FcPatternDestroy )> match(
            FcFontMatch( p->fontConfig, pattern.get(), &matchResult ), FcPatternDestroy );
    FcChar8* file = nullptr;
    int      index = 0;

    if( !match || FcPatternGetString( match.get(), FC_FILE, 0, &file ) != FcResultMatch )
        return;

    FcPatternGetInteger( match.get(), FC_INDEX, 0, &index );
    FT_Face rawFace = nullptr;

    if( FT_New_Face( text.library, reinterpret_cast<const char*>( file ), index, &rawFace ) )
        return;

    std::unique_ptr<FT_FaceRec, decltype( &FT_Done_Face )> face( rawFace, FT_Done_Face );

    if( FT_Set_Char_Size( face.get(), 0, static_cast<FT_F26Dot6>( attr->fontSize * 64 ), 72, 72 ) )
        return;

    std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )>     font( hb_ft_font_create_referenced( face.get() ),
                                                                       hb_font_destroy );
    std::unique_ptr<hb_buffer_t, decltype( &hb_buffer_destroy )> buffer( hb_buffer_create(), hb_buffer_destroy );
    hb_ft_font_set_load_flags( font.get(), FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP );
    hb_buffer_add_utf32( buffer.get(), codepoints.data(), static_cast<int>( codepoints.size() ), 0, -1 );
    hb_buffer_guess_segment_properties( buffer.get() );
    bool positioned = false;

    for( const NSVGtextPosition& position : text.positions )
    {
        for( const auto& values : position.values )
            positioned |= values.size() > 1;
    }

    // Character positions must remain independent when the source supplies advances.
    hb_feature_t features[] = { { HB_TAG( 'l', 'i', 'g', 'a' ), 0, 0, static_cast<unsigned>( -1 ) },
                                { HB_TAG( 'c', 'l', 'i', 'g' ), 0, 0, static_cast<unsigned>( -1 ) } };
    hb_shape( font.get(), buffer.get(), features, positioned ? 2 : 0 );
    unsigned             count = 0;
    hb_glyph_info_t*     glyphs = hb_buffer_get_glyph_infos( buffer.get(), &count );
    hb_glyph_position_t* advances = hb_buffer_get_glyph_positions( buffer.get(), &count );
    unsigned             previousCluster = static_cast<unsigned>( -1 );

    for( unsigned i = 0; i < count; ++i )
    {
        float values[4] = {};
        bool  present[4] = {};

        if( glyphs[i].cluster != previousCluster )
        {
            for( const NSVGtextPosition& position : text.positions )
            {
                size_t charIndex = position.index + glyphs[i].cluster;

                for( int axis = 0; axis < 4; ++axis )
                {
                    if( charIndex < position.values[axis].size() )
                    {
                        values[axis] = position.values[axis][charIndex];
                        present[axis] = true;
                    }
                }
            }
        }

        previousCluster = glyphs[i].cluster;

        if( present[0] || present[1] )
            nsvg__finishTextChunk( text );

        if( present[0] )
            text.x = values[0];

        if( present[1] )
            text.y = values[1];

        if( !text.chunkActive )
        {
            text.chunkStart = text.x;
            text.anchor = attr->textAnchor;
            text.chunkActive = true;
        }

        text.x += values[2];
        text.y += values[3];
        NSVGtextOutline outline{ p, text.x + advances[i].x_offset / 64.0f, text.y - advances[i].y_offset / 64.0f };

        if( FT_Load_Glyph( face.get(), glyphs[i].codepoint, FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP ) == 0
            && face->glyph->format == FT_GLYPH_FORMAT_OUTLINE )
        {
            FT_Outline_Funcs callbacks{};
            callbacks.move_to = []( const FT_Vector* point, void* user )
            {
                auto& ctx = *static_cast<NSVGtextOutline*>( user );

                if( ctx.parser->npts )
                    nsvg__addPath( ctx.parser, 1 );

                nsvg__resetPath( ctx.parser );
                nsvg__moveTo( ctx.parser, ctx.x + point->x / 64.0f, ctx.y - point->y / 64.0f );
                return 0;
            };
            callbacks.line_to = []( const FT_Vector* point, void* user )
            {
                auto& ctx = *static_cast<NSVGtextOutline*>( user );
                nsvg__lineTo( ctx.parser, ctx.x + point->x / 64.0f, ctx.y - point->y / 64.0f );
                return 0;
            };
            callbacks.conic_to = []( const FT_Vector* control, const FT_Vector* point, void* user )
            {
                auto&       ctx = *static_cast<NSVGtextOutline*>( user );
                NSVGparser* parser = ctx.parser;
                float       x0 = parser->pts[( parser->npts - 1 ) * 2];
                float       y0 = parser->pts[( parser->npts - 1 ) * 2 + 1];
                float       cx = ctx.x + control->x / 64.0f;
                float       cy = ctx.y - control->y / 64.0f;
                float       x = ctx.x + point->x / 64.0f;
                float       y = ctx.y - point->y / 64.0f;
                nsvg__cubicBezTo( parser, x0 + ( cx - x0 ) * 2 / 3, y0 + ( cy - y0 ) * 2 / 3, x + ( cx - x ) * 2 / 3,
                                  y + ( cy - y ) * 2 / 3, x, y );
                return 0;
            };
            callbacks.cubic_to = []( const FT_Vector* a, const FT_Vector* b, const FT_Vector* point, void* user )
            {
                auto& ctx = *static_cast<NSVGtextOutline*>( user );
                nsvg__cubicBezTo( ctx.parser, ctx.x + a->x / 64.0f, ctx.y - a->y / 64.0f, ctx.x + b->x / 64.0f,
                                  ctx.y - b->y / 64.0f, ctx.x + point->x / 64.0f, ctx.y - point->y / 64.0f );
                return 0;
            };
            nsvg__resetPath( p );
            FT_Outline_Decompose( &face->glyph->outline, &callbacks, &outline );

            if( p->npts )
                nsvg__addPath( p, 1 );

            NSVGshape* previous = p->shapesTail;
            nsvg__addShape( p );

            if( p->shapesTail != previous )
                text.chunk.push_back( p->shapesTail );
        }

        text.x += advances[i].x_advance / 64.0f;
        text.y -= advances[i].y_advance / 64.0f;
    }

    for( NSVGtextPosition& position : text.positions )
        position.index += codepoints.size();

    text.hasContent = true;
}
