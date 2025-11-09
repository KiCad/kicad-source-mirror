/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gerbview_painter.h>
#include <gal/graphics_abstraction_layer.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <gerbview_settings.h>
#include <convert_basic_shapes_to_polygon.h>
#include <gerbview.h>
#include <trigo.h>

#include <dcode.h>
#include <gerber_draw_item.h>
#include <gerber_file_image.h>

using namespace KIGFX;


GERBVIEW_SETTINGS* gvconfig()
{
    return Pgm().GetSettingsManager().GetAppSettings<GERBVIEW_SETTINGS>( "gerbview" );
}


GERBVIEW_RENDER_SETTINGS::GERBVIEW_RENDER_SETTINGS()
{
    m_backgroundColor = COLOR4D::BLACK;

    m_componentHighlightString = "";
    m_netHighlightString       = "";
    m_attributeHighlightString = "";
    m_dcodeHighlightValue      = -1;

    update();
}


void GERBVIEW_RENDER_SETTINGS::LoadColors( const COLOR_SETTINGS* aSettings )
{
    // Layers to draw gerber data read from gerber files:
    for( int i = GERBVIEW_LAYER_ID_START;
         i < GERBVIEW_LAYER_ID_START + GERBER_DRAWLAYERS_COUNT; i++ )
    {
        COLOR4D baseColor = aSettings->GetColor( i );

        if( gvconfig()->m_Display.m_ForceOpacityMode )
            baseColor.a = gvconfig()->m_Display.m_OpacityModeAlphaValue;

        m_layerColors[i] = baseColor;
        m_layerColorsHi[i] = baseColor.Brightened( 0.5 );
        m_layerColorsSel[i] = baseColor.Brightened( 0.8 );
        m_layerColorsDark[i] = baseColor.Darkened( 0.25 );
    }

    // Draw layers specific to Gerbview:
    // LAYER_DCODES, LAYER_NEGATIVE_OBJECTS, LAYER_GERBVIEW_GRID, LAYER_GERBVIEW_AXES,
    // LAYER_GERBVIEW_BACKGROUND, LAYER_GERBVIEW_DRAWINGSHEET, LAYER_GERBVIEW_PAGE_LIMITS
    for( int i = LAYER_DCODES; i < GERBVIEW_LAYER_ID_END; i++ )
        m_layerColors[i] = aSettings->GetColor( i );

    for( int i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; i++ )
        m_layerColors[i] = aSettings->GetColor( i );

    // Ensure the generic LAYER_DRAWINGSHEET has the same color as the specialized
    // LAYER_GERBVIEW_DRAWINGSHEET
    m_layerColors[LAYER_DRAWINGSHEET] = m_layerColors[ LAYER_GERBVIEW_DRAWINGSHEET ];

    update();
}


void GERBVIEW_RENDER_SETTINGS::ClearHighlightSelections()
{
    // Clear all highlight selections (dcode, net, component, attribute selection)
    m_componentHighlightString.Empty();
    m_netHighlightString.Empty();
    m_attributeHighlightString.Empty();
    m_dcodeHighlightValue = -1;
}

COLOR4D GERBVIEW_RENDER_SETTINGS::GetColor( const VIEW_ITEM* aItem, int aLayer ) const
{
    const EDA_ITEM* item = dynamic_cast<const EDA_ITEM*>( aItem );
    static const COLOR4D transparent = COLOR4D( 0, 0, 0, 0 );
    const GERBER_DRAW_ITEM* gbrItem = nullptr;

    if( item && item->Type() == GERBER_DRAW_ITEM_T )
        gbrItem = static_cast<const GERBER_DRAW_ITEM*>( item );

    // All DCODE layers stored under a single color setting
    if( IsDCodeLayer( aLayer ) )
    {
        auto it = m_layerColors.find( LAYER_DCODES );
        return it == m_layerColors.end() ? COLOR4D::WHITE : it->second;
    }

    if( item && item->IsSelected() )
    {
        auto it = m_layerColorsSel.find( aLayer );
        return it == m_layerColorsSel.end() ? COLOR4D::WHITE : it->second;
    }

    if( gbrItem && gbrItem->GetLayerPolarity() )
    {
        if( gvconfig()->m_Appearance.show_negative_objects )
        {
            auto it = m_layerColors.find( LAYER_NEGATIVE_OBJECTS );
            return it == m_layerColors.end() ? COLOR4D::WHITE : it->second;
        }
        else
        {
            return transparent;
        }
    }

    if( !m_netHighlightString.IsEmpty() && gbrItem &&
        m_netHighlightString == gbrItem->GetNetAttributes().m_Netname )
    {
        auto it = m_layerColorsHi.find( aLayer );
        return it == m_layerColorsHi.end() ? COLOR4D::WHITE : it->second;
    }

    if( !m_componentHighlightString.IsEmpty() && gbrItem &&
        m_componentHighlightString == gbrItem->GetNetAttributes().m_Cmpref )
    {
        auto it = m_layerColorsHi.find( aLayer );
        return it == m_layerColorsHi.end() ? COLOR4D::WHITE : it->second;
    }

    if( !m_attributeHighlightString.IsEmpty() && gbrItem && gbrItem->GetDcodeDescr() &&
        m_attributeHighlightString == gbrItem->GetDcodeDescr()->m_AperFunction )
    {
        auto it = m_layerColorsHi.find( aLayer );
        return it == m_layerColorsHi.end() ? COLOR4D::WHITE : it->second;
    }

    if( m_dcodeHighlightValue> 0 && gbrItem && gbrItem->GetDcodeDescr() &&
        m_dcodeHighlightValue == gbrItem->GetDcodeDescr()->m_Num_Dcode )
    {
        auto it = m_layerColorsHi.find( aLayer );
        return it == m_layerColorsHi.end() ? COLOR4D::WHITE : it->second;
    }

    // Return grayish color for non-highlighted layers in the high contrast mode
    if( m_hiContrastEnabled && m_highContrastLayers.count( aLayer ) == 0)
    {
        auto it = m_hiContrastColor.find( aLayer );
        return it == m_hiContrastColor.end() ? COLOR4D::WHITE.Darkened( 0.25 ) : it->second;
    }

    // Catch the case when highlight and high-contraste modes are enabled
    // and we are drawing a not highlighted track
    if( m_highlightEnabled )
    {
        auto it = m_layerColorsDark.find( aLayer );
        return it == m_layerColorsDark.end() ? COLOR4D::WHITE.Darkened( 0.5 ) : it->second;
    }

    // No special modificators enabled
    auto it = m_layerColors.find( aLayer );
    return it == m_layerColors.end() ? COLOR4D::WHITE : it->second;
}


bool GERBVIEW_RENDER_SETTINGS::GetShowPageLimits() const
{
    return gvconfig()->m_Display.m_DisplayPageLimits;
}


GERBVIEW_PAINTER::GERBVIEW_PAINTER( GAL* aGal ) :
    PAINTER( aGal )
{
}


// TODO(JE): Pull up to PAINTER?
int GERBVIEW_PAINTER::getLineThickness( int aActualThickness ) const
{
    // if items have 0 thickness, draw them with the outline width, otherwise respect the set
    // value (which, no matter how small will produce something)
    if( aActualThickness == 0 )
        return m_gerbviewSettings.m_outlineWidth;

    return aActualThickness;
}


bool GERBVIEW_PAINTER::Draw( const VIEW_ITEM* aItem, int aLayer )
{
    GERBER_DRAW_ITEM* gbrItem = dynamic_cast<GERBER_DRAW_ITEM*>( const_cast<VIEW_ITEM*>( aItem ) );

    if( gbrItem )
    {
        draw( gbrItem, aLayer );
        return true;
    }

    return false;
}


// TODO(JE) aItem can't be const because of GetDcodeDescr()
// Probably that can be refactored in GERBER_DRAW_ITEM to allow const here.
void GERBVIEW_PAINTER::draw( /*const*/ GERBER_DRAW_ITEM* aItem, int aLayer )
{
    VECTOR2D start( aItem->GetABPosition( aItem->m_Start ) );   // TODO(JE) Getter
    VECTOR2D end( aItem->GetABPosition( aItem->m_End ) );       // TODO(JE) Getter
    int      width = aItem->m_Size.x;   // TODO(JE) Getter
    bool     isFilled = true;
    COLOR4D  color;
    // TODO(JE) This doesn't actually work properly for ImageNegative
    bool     isNegative = ( aItem->GetLayerPolarity() ^ aItem->m_GerberImageFile->m_ImageNegative );

    // Draw DCODE overlay text
    if( IsDCodeLayer( aLayer ) )
    {
        wxString  codeText;
        VECTOR2I  textPosition;
        int       textSize;
        EDA_ANGLE orient;

        if( !aItem->GetTextD_CodePrms( textSize, textPosition, orient ) )
            return;

        color = m_gerbviewSettings.GetColor( aItem, aLayer );
        codeText.Printf( wxT( "D%d" ), aItem->m_DCode );

        m_gal->SetIsStroke( true );
        m_gal->SetIsFill( false );
        m_gal->SetStrokeColor( color );
        m_gal->SetFillColor( COLOR4D( 0, 0, 0, 0 ) );
        m_gal->SetLineWidth( textSize/10 );
        m_gal->SetFontBold( false );
        m_gal->SetFontItalic( false );
        m_gal->SetFontUnderlined( false );
        m_gal->SetTextMirrored( false );
        m_gal->SetGlyphSize( VECTOR2D( textSize, textSize) );
        m_gal->SetHorizontalJustify( GR_TEXT_H_ALIGN_CENTER );
        m_gal->SetVerticalJustify( GR_TEXT_V_ALIGN_CENTER );
        m_gal->BitmapText( codeText, textPosition, orient );

        return;
    }

    color = m_gerbviewSettings.GetColor( aItem, aLayer );

    // TODO: Should brightened color be a preference?
    if( aItem->IsBrightened() )
        color = COLOR4D( 0.0, 1.0, 0.0, 0.75 );

    m_gal->SetNegativeDrawMode( isNegative && !gvconfig()->m_Appearance.show_negative_objects );
    m_gal->SetStrokeColor( color );
    m_gal->SetFillColor( color );
    m_gal->SetIsFill( isFilled );
    m_gal->SetIsStroke( !isFilled );

    switch( aItem->m_ShapeType )
    {
    case GBR_POLYGON:
    {
        isFilled = gvconfig()->m_Display.m_DisplayPolygonsFill;
        m_gal->SetIsFill( isFilled );
        m_gal->SetIsStroke( !isFilled );

        if( isNegative && !isFilled )
        {
            m_gal->SetNegativeDrawMode( false );
            m_gal->SetStrokeColor( GetSettings()->GetColor( aItem, aLayer ) );
        }

        if( !isFilled )
            m_gal->SetLineWidth( m_gerbviewSettings.m_outlineWidth );

        if( aItem->m_AbsolutePolygon.OutlineCount() == 0 )
        {
            std::vector<VECTOR2I> pts = aItem->m_ShapeAsPolygon.COutline( 0 ).CPoints();

            for( auto& pt : pts )
                pt = aItem->GetABPosition( pt );

            SHAPE_LINE_CHAIN chain( pts );
            chain.SetClosed( true );
            aItem->m_AbsolutePolygon.AddOutline( chain );
        }

        // Degenerated polygons (having < 3 points) are drawn as lines
        // to avoid issues in draw polygon functions
        if( !isFilled || aItem->m_AbsolutePolygon.COutline( 0 ).PointCount() < 3 )
            m_gal->DrawPolyline( aItem->m_AbsolutePolygon.COutline( 0 ) );
        else
        {
            // On Opengl, a not convex filled polygon is usually drawn by using triangles as
            // primitives. CacheTriangulation() can create basic triangle primitives to draw the
            // polygon solid shape on Opengl
            // We use the fastest CacheTriangulation calculation mode: no partition created because
            // the partition is useless in Gerbview, and very time consumming (optimized only
            // for pcbnew that has different internal unit)
            if( m_gal->IsOpenGlEngine() && !aItem->m_AbsolutePolygon.IsTriangulationUpToDate() )
                aItem->m_AbsolutePolygon.CacheTriangulation( false /* fastest triangulation calculation mode */ );

            m_gal->DrawPolygon( aItem->m_AbsolutePolygon );
        }

        break;
    }

    case GBR_CIRCLE:
    {
        isFilled = gvconfig()->m_Display.m_DisplayLinesFill;
        double radius = aItem->m_Start.Distance( aItem->m_End );
        m_gal->DrawCircle( start, radius );
        break;
    }

    case GBR_ARC:
    {
        isFilled = gvconfig()->m_Display.m_DisplayLinesFill;

        // These are swapped because wxDC fills arcs counterclockwise and GAL
        // fills them clockwise.
        VECTOR2I arcStart = aItem->m_End;
        VECTOR2I arcEnd = aItem->m_Start;

        // Gerber arcs are 3-point (start, center, end)
        // GAL needs center, radius, start angle, end angle
        double   radius = arcStart.Distance( aItem->m_ArcCentre );
        VECTOR2D center = aItem->GetABPosition( aItem->m_ArcCentre );
        VECTOR2D startVec = VECTOR2D( aItem->GetABPosition( arcStart ) ) - center;
        VECTOR2D endVec = VECTOR2D( aItem->GetABPosition( arcEnd ) ) - center;

        m_gal->SetIsFill( isFilled );
        m_gal->SetIsStroke( !isFilled );
        m_gal->SetLineWidth( isFilled ? width : m_gerbviewSettings.m_outlineWidth );

        EDA_ANGLE startAngle( startVec );
        EDA_ANGLE endAngle( endVec );

        // GAL fills in direction of increasing angle, so we have to convert
        // the angle from the -PI to PI domain of atan2() to ensure that
        // the arc goes in the right direction
        if( startAngle > endAngle )
            endAngle += ANGLE_360;

        // In Gerber, 360-degree arcs are stored in the file with start equal to end
        if( arcStart == arcEnd )
            endAngle = startAngle + ANGLE_360;

        // Adjust the allowed approx error to convert arcs to segments:
        int arc_to_seg_error = gerbIUScale.mmToIU( 0.005 );    // Allow 5 microns
        m_gal->DrawArcSegment( center, radius, startAngle, endAngle - startAngle, width,
                               arc_to_seg_error );

#if 0   // Arc Debugging only
        m_canvas->SetIsFill( false );
        m_canvas->SetIsStroke( true );
        m_canvas->SetLineWidth( 5 );
        m_canvas->SetStrokeColor( COLOR4D( 0.1, 0.5, 0.0, 0.5 ) );
        m_canvas->DrawLine( center, aItem->GetABPosition( arcStart ) );
        m_canvas->SetStrokeColor( COLOR4D( 0.6, 0.1, 0.0, 0.5 ) );
        m_canvas->DrawLine( center, aItem->GetABPosition( arcEnd ) );
#endif

#if 0   // Bbox arc Debugging only
        m_canvas->SetIsFill( false );
        m_canvas->SetIsStroke( true );
        BOX2I box = aItem->GetBoundingBox();
        m_canvas->SetLineWidth( 5 );
        m_canvas->SetStrokeColor( COLOR4D(0.9, 0.9, 0, 0.4) );
        // box coordinates are already in AB position.
        m_canvas->DrawRectangle( box.GetOrigin(), box.GetEnd() );
#endif
        break;
    }

    case GBR_SPOT_CIRCLE:
    case GBR_SPOT_RECT:
    case GBR_SPOT_OVAL:
    case GBR_SPOT_POLY:
    case GBR_SPOT_MACRO:
        isFilled = gvconfig()->m_Display.m_DisplayFlashedItemsFill;
        drawFlashedShape( aItem, isFilled );
        break;

    case GBR_SEGMENT:
    {
        /* Plot a line from m_Start to m_End.
         * Usually, a round pen is used, but some gerber files use a rectangular pen
         * In fact, any aperture can be used to plot a line.
         * currently: only a square pen is handled (I believe using a polygon gives a strange plot).
         */
        isFilled = gvconfig()->m_Display.m_DisplayLinesFill;
        m_gal->SetIsFill( isFilled );
        m_gal->SetIsStroke( !isFilled );

        if( isNegative && !isFilled )
            m_gal->SetStrokeColor( GetSettings()->GetColor( aItem, aLayer ) );

        // TODO(JE) Refactor this to allow const aItem
        D_CODE* code = aItem->GetDcodeDescr();
        if( code && code->m_ApertType == APT_RECT )
        {
            if( aItem->m_ShapeAsPolygon.OutlineCount() == 0 )
                aItem->ConvertSegmentToPolygon();

            drawPolygon( aItem, aItem->m_ShapeAsPolygon, isFilled );
        }
        else
        {
            if( !isFilled )
                m_gal->SetLineWidth( m_gerbviewSettings.m_outlineWidth );

            m_gal->DrawSegment( start, end, width );
        }
        break;
    }

    default:
        wxASSERT_MSG( false, wxT( "GERBER_DRAW_ITEM shape is unknown!" ) );
        break;
    }
    m_gal->SetNegativeDrawMode( false );

    // Enable for bounding box debugging
    #if 0
    const BOX2I& bb = aItem->ViewBBox();
    m_canvas->SetIsStroke( true );
    m_canvas->SetIsFill( true );
    m_canvas->SetLineWidth( 3 );
    m_canvas->SetStrokeColor( COLOR4D(0.9, 0.9, 0, 0.4) );
    m_canvas->SetFillColor( COLOR4D(0.9, 0.9, 0, 0.1) );
    m_canvas->DrawRectangle( bb.GetOrigin(), bb.GetEnd() );
    #endif
}


void GERBVIEW_PAINTER::drawPolygon( GERBER_DRAW_ITEM* aParent, const SHAPE_POLY_SET& aPolygon,
                                    bool aFilled, bool aShift )
{
    wxASSERT( aPolygon.OutlineCount() == 1 );

    if( aPolygon.OutlineCount() == 0 )
        return;

    SHAPE_POLY_SET poly;
    poly.NewOutline();
    const std::vector<VECTOR2I> pts = aPolygon.COutline( 0 ).CPoints();
    VECTOR2I offset = aShift ? VECTOR2I( aParent->m_Start ) : VECTOR2I( 0, 0 );

    for( const VECTOR2I& pt : pts )
        poly.Append( aParent->GetABPosition( pt + offset ) );

    if( !gvconfig()->m_Display.m_DisplayPolygonsFill )
        m_gal->SetLineWidth( m_gerbviewSettings.m_outlineWidth );

    if( !aFilled )
        m_gal->DrawPolyline( poly.COutline( 0 ) );
    else
        m_gal->DrawPolygon( poly );
}


void GERBVIEW_PAINTER::drawFlashedShape( GERBER_DRAW_ITEM* aItem, bool aFilled )
{
    D_CODE* code = aItem->GetDcodeDescr();

    wxASSERT_MSG( code, wxT( "drawFlashedShape: Item has no D_CODE!" ) );

    if( !code )
        return;

    m_gal->SetIsFill( aFilled );
    m_gal->SetIsStroke( !aFilled );
    m_gal->SetLineWidth( m_gerbviewSettings.m_outlineWidth );

    switch( aItem->m_ShapeType )
    {
    case GBR_SPOT_CIRCLE:
    {
        int radius = code->m_Size.x >> 1;
        VECTOR2D start( aItem->GetABPosition( aItem->m_Start ) );

        if( !aFilled || code->m_DrillShape == APT_DEF_NO_HOLE )
        {
            m_gal->DrawCircle( start, radius );
        }
        else    // rectangular hole
        {
            if( code->m_Polygon.OutlineCount() == 0 )
                code->ConvertShapeToPolygon( aItem );

            drawPolygon( aItem, code->m_Polygon, aFilled, true );
        }

        break;
    }

    case GBR_SPOT_RECT:
    {
        VECTOR2I codeStart;
        VECTOR2I aShapePos = aItem->m_Start;
        codeStart.x = aShapePos.x - code->m_Size.x / 2;
        codeStart.y = aShapePos.y - code->m_Size.y / 2;
        VECTOR2I codeEnd = codeStart + code->m_Size;
        codeStart = aItem->GetABPosition( codeStart );
        codeEnd = aItem->GetABPosition( codeEnd );

        if( !aFilled || code->m_DrillShape == APT_DEF_NO_HOLE  )
        {
            m_gal->DrawRectangle( VECTOR2D( codeStart ), VECTOR2D( codeEnd ) );
        }
        else
        {
            if( code->m_Polygon.OutlineCount() == 0 )
                code->ConvertShapeToPolygon( aItem );

            drawPolygon( aItem, code->m_Polygon, aFilled, true );
        }
        break;
    }

    case GBR_SPOT_OVAL:
    {
        int radius = 0;

        VECTOR2I codeStart = aItem->m_Start;
        VECTOR2I codeEnd = aItem->m_Start;

        if( code->m_Size.x > code->m_Size.y )   // horizontal oval
        {
            int delta = (code->m_Size.x - code->m_Size.y) / 2;
            codeStart.x -= delta;
            codeEnd.x   += delta;
            radius   = code->m_Size.y;
        }
        else   // horizontal oval
        {
            int delta = (code->m_Size.y - code->m_Size.x) / 2;
            codeStart.y -= delta;
            codeEnd.y   += delta;
            radius   = code->m_Size.x;
        }

        codeStart = aItem->GetABPosition( codeStart );
        codeEnd = aItem->GetABPosition( codeEnd );

        if( !aFilled || code->m_DrillShape == APT_DEF_NO_HOLE )
        {
            m_gal->DrawSegment( codeStart, codeEnd, radius );
        }
        else
        {
            if( code->m_Polygon.OutlineCount() == 0 )
                code->ConvertShapeToPolygon( aItem );

            drawPolygon( aItem, code->m_Polygon, aFilled, true );
        }
        break;
    }

    case GBR_SPOT_POLY:
        if( code->m_Polygon.OutlineCount() == 0 )
            code->ConvertShapeToPolygon( aItem );

        drawPolygon( aItem, code->m_Polygon, aFilled, true );
        break;

    case GBR_SPOT_MACRO:
        drawApertureMacro( aItem, aFilled );
        break;

    default:
        wxASSERT_MSG( false, wxT( "Unknown Gerber flashed shape!" ) );
        break;
    }
}


void GERBVIEW_PAINTER::drawApertureMacro( GERBER_DRAW_ITEM* aParent, bool aFilled )
{
    if( aParent->m_AbsolutePolygon.OutlineCount() == 0 )
    {
        D_CODE* code = aParent->GetDcodeDescr();
        APERTURE_MACRO* macro = code->GetMacro();
        aParent->m_AbsolutePolygon = *macro->GetApertureMacroShape( aParent, aParent->m_Start );
    }

    SHAPE_POLY_SET& polyset = aParent->m_AbsolutePolygon;

    if( !gvconfig()->m_Display.m_DisplayPolygonsFill )
        m_gal->SetLineWidth( m_gerbviewSettings.m_outlineWidth );

    if( !aFilled )
    {
        for( int i = 0; i < polyset.OutlineCount(); i++ )
            m_gal->DrawPolyline( polyset.COutline( i ) );
    }
    else
    {
        m_gal->DrawPolygon( polyset );
    }
}


const double GERBVIEW_RENDER_SETTINGS::MAX_FONT_SIZE = gerbIUScale.mmToIU( 10.0 );
