/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <algorithm>                          // for min
#include <bitset>                             // for bitset, operator&, __bi...
#include <math.h>                             // for abs
#include <stddef.h>                           // for NULL, size_t

#include <geometry/seg.h>                     // for SEG
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>        // for SHAPE_LINE_CHAIN
#include <geometry/shape_poly_set.h>          // for SHAPE_POLY_SET, SHAPE_P...
#include <geometry/shape_segment.h>
#include <string_utils.h>
#include <macros.h>
#include <math/util.h>                        // for KiROUND, Clamp
#include <math/vector2d.h>                    // for VECTOR2I
#include <plotters/plotter_gerber.h>
#include <trigo.h>
#include <callback_gal.h>

#include <board_design_settings.h>            // for BOARD_DESIGN_SETTINGS
#include <core/typeinfo.h>                    // for dyn_cast, PCB_DIMENSION_T
#include <gbr_metadata.h>
#include <gbr_netlist_metadata.h>             // for GBR_NETLIST_METADATA
#include <layer_ids.h>                        // for LSET, IsCopperLayer
#include <pad_shapes.h>                       // for PAD_ATTRIB::NPTH
#include <pcbplot.h>
#include <pcb_plot_params.h>                  // for PCB_PLOT_PARAMS, PCB_PL...
#include <advanced_config.h>

#include <board.h>
#include <board_item.h>                       // for BOARD_ITEM, S_CIRCLE
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <zone.h>

#include <wx/debug.h>                         // for wxASSERT_MSG


COLOR4D BRDITEMS_PLOTTER::getColor( int aLayer ) const
{
    COLOR4D color = ColorSettings()->GetColor( aLayer );

    // A hack to avoid plotting a white item in white color, expecting the paper
    // is also white: use a non white color:
    if( color == COLOR4D::WHITE )
        color = COLOR4D( LIGHTGRAY );

    return color;
}


void BRDITEMS_PLOTTER::PlotPad( const PAD* aPad, const COLOR4D& aColor, OUTLINE_MODE aPlotMode )
{
    VECTOR2I     shape_pos = aPad->ShapePos();
    GBR_METADATA gbr_metadata;

    bool plotOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();
    bool plotOnExternalCopperLayer = ( m_layerMask & LSET::ExternalCuMask() ).any();

    // Pad not on the solder mask layer cannot be soldered.
    // therefore it can have a specific aperture attribute.
    // Not yet in use.
    // bool isPadOnBoardTechLayers = ( aPad->GetLayerSet() & LSET::AllBoardTechMask() ).any();

    gbr_metadata.SetCmpReference( aPad->GetParent()->GetReference() );

    if( plotOnCopperLayer )
    {
        gbr_metadata.SetNetAttribType( GBR_NETINFO_ALL );
        gbr_metadata.SetCopper( true );

        // Gives a default attribute, for instance for pads used as tracks in net ties:
        // Connector pads and SMD pads are on external layers
        // if on internal layers, they are certainly used as net tie
        // and are similar to tracks: just conductor items
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );

        const bool useUTF8 = false;
        const bool useQuoting = false;
        gbr_metadata.SetPadName( aPad->GetNumber(), useUTF8, useQuoting );

        if( !aPad->GetNumber().IsEmpty() )
            gbr_metadata.SetPadPinFunction( aPad->GetPinFunction(), useUTF8, useQuoting );

        gbr_metadata.SetNetName( aPad->GetNetname() );

        // Some pads are mechanical pads ( through hole or smd )
        // when this is the case, they have no pad name and/or are not plated.
        // In this case gerber files have slightly different attributes.
        if( aPad->GetAttribute() == PAD_ATTRIB::NPTH || aPad->GetNumber().IsEmpty() )
            gbr_metadata.m_NetlistMetadata.m_NotInNet = true;

        if( !plotOnExternalCopperLayer )
        {
            // the .P object attribute (GBR_NETLIST_METADATA::GBR_NETINFO_PAD)
            // is used on outer layers, unless the component is embedded
            // or a "etched" component (fp only drawn, not a physical component)
            // Currently, Pcbnew does not handle embedded component, so we disable the .P
            // attribute on internal layers
            // Note the Gerber doc is not really clear about through holes pads about the .P
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET |
                                           GBR_NETLIST_METADATA::GBR_NETINFO_CMP );

        }

        // Some attributes are reserved to the external copper layers:
        // GBR_APERTURE_ATTRIB_CONNECTORPAD and GBR_APERTURE_ATTRIB_SMDPAD_CUDEF
        // for instance.
        // Pad with type PAD_ATTRIB::CONN or PAD_ATTRIB::SMD that is not on outer layer
        // has its aperture attribute set to GBR_APERTURE_ATTRIB_CONDUCTOR
        switch( aPad->GetAttribute() )
        {
        case PAD_ATTRIB::NPTH:       // Mechanical pad through hole
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
            break;

        case PAD_ATTRIB::PTH :       // Pad through hole, a hole is also expected
            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_COMPONENTPAD );
            break;

        case PAD_ATTRIB::CONN:       // Connector pads, no solder paste but with solder mask.
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONNECTORPAD );
            break;

        case PAD_ATTRIB::SMD:        // SMD pads (on external copper layer only)
                                     // with solder paste and mask
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_SMDPAD_CUDEF );
            break;
        }

        // Fabrication properties can have specific GBR_APERTURE_METADATA options
        // that replace previous aperture attribute:
        switch( aPad->GetProperty() )
        {
        case PAD_PROP::BGA:          // Only applicable to outer layers
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_BGAPAD_CUDEF );
            break;

        case PAD_PROP::FIDUCIAL_GLBL:
            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_GLBL );
            break;

        case PAD_PROP::FIDUCIAL_LOCAL:
            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_LOCAL );
            break;

        case PAD_PROP::TESTPOINT:    // Only applicable to outer layers
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib(
                        GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_TESTPOINT );
            break;

        case PAD_PROP::HEATSINK:
            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_HEATSINKPAD );
            break;

        case PAD_PROP::CASTELLATED:
            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CASTELLATEDPAD );
            break;

        case PAD_PROP::NONE:
            break;
        }

        // Ensure NPTH pads have *always* the GBR_APERTURE_ATTRIB_WASHERPAD attribute
        if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
    }
    else
    {
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    }

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( aColor != WHITE ? aColor : LIGHTGRAY);

    if( aPlotMode == SKETCH )
        m_plotter->SetCurrentLineWidth( GetSketchPadLineWidth(), &gbr_metadata );

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE::CIRCLE:
        m_plotter->FlashPadCircle( shape_pos, aPad->GetSize().x, aPlotMode, &gbr_metadata );
        break;

    case PAD_SHAPE::OVAL:
        m_plotter->FlashPadOval( shape_pos, aPad->GetSize(), aPad->GetOrientation(), aPlotMode,
                                 &gbr_metadata );
        break;

    case PAD_SHAPE::RECT:
        m_plotter->FlashPadRect( shape_pos, aPad->GetSize(), aPad->GetOrientation(), aPlotMode,
                                 &gbr_metadata );
        break;

    case PAD_SHAPE::ROUNDRECT:
        m_plotter->FlashPadRoundRect( shape_pos, aPad->GetSize(), aPad->GetRoundRectCornerRadius(),
                                      aPad->GetOrientation(), aPlotMode, &gbr_metadata );
        break;

    case PAD_SHAPE::TRAPEZOID:
    {
        // Build the pad polygon in coordinates relative to the pad
        // (i.e. for a pad at pos 0,0, rot 0.0). Needed to use aperture macros,
        // to be able to create a pattern common to all trapezoid pads having the same shape
        VECTOR2I coord[4];

        // Order is lower left, lower right, upper right, upper left.
        VECTOR2I half_size = aPad->GetSize() / 2;
        VECTOR2I trap_delta = aPad->GetDelta() / 2;

        coord[0] = VECTOR2I( -half_size.x - trap_delta.y, half_size.y + trap_delta.x );
        coord[1] = VECTOR2I( half_size.x + trap_delta.y, half_size.y - trap_delta.x );
        coord[2] = VECTOR2I( half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
        coord[3] = VECTOR2I( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );

        m_plotter->FlashPadTrapez( shape_pos, coord, aPad->GetOrientation(), aPlotMode,
                                   &gbr_metadata );
    }
        break;

    case PAD_SHAPE::CHAMFERED_RECT:
        if( m_plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
        {
            GERBER_PLOTTER* gerberPlotter = static_cast<GERBER_PLOTTER*>( m_plotter );

            gerberPlotter->FlashPadChamferRoundRect( shape_pos, aPad->GetSize(),
                                                     aPad->GetRoundRectCornerRadius(),
                                                     aPad->GetChamferRectRatio(),
                                                     aPad->GetChamferPositions(),
                                                     aPad->GetOrientation(), aPlotMode,
                                                     &gbr_metadata );
            break;
        }

        KI_FALLTHROUGH;

    default:
    case PAD_SHAPE::CUSTOM:
    {
        const std::shared_ptr<SHAPE_POLY_SET>& polygons = aPad->GetEffectivePolygon();

        if( polygons->OutlineCount() )
        {
            m_plotter->FlashPadCustom( shape_pos, aPad->GetSize(), aPad->GetOrientation(),
                                       polygons.get(), aPlotMode, &gbr_metadata );
        }
    }
        break;
    }
}


void BRDITEMS_PLOTTER::PlotFootprintTextItems( const FOOTPRINT* aFootprint )
{
    const PCB_TEXT* textItem = &aFootprint->Reference();
    int             textLayer = textItem->GetLayer();

    // Reference and value are specific items, not in graphic items list
    if( GetPlotReference() && m_layerMask[textLayer]
        && ( textItem->IsVisible() || GetPlotInvisibleText() ) )
    {
        PlotFootprintTextItem( textItem, getColor( textLayer ) );
    }

    textItem  = &aFootprint->Value();
    textLayer = textItem->GetLayer();

    if( GetPlotValue() && m_layerMask[textLayer]
        && ( textItem->IsVisible() || GetPlotInvisibleText() ) )
    {
        PlotFootprintTextItem( textItem, getColor( textLayer ) );
    }

    for( const BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        textItem = dyn_cast<const PCB_TEXT*>( item );

        if( !textItem )
            continue;

        if( !textItem->IsVisible() )
            continue;

        textLayer = textItem->GetLayer();

        if( textLayer == Edge_Cuts || textLayer >= PCB_LAYER_ID_COUNT )
            continue;

        if( !m_layerMask[textLayer] || aFootprint->GetPrivateLayers().test( textLayer ) )
            continue;

        if( textItem->GetText() == wxT( "${REFERENCE}" ) && !GetPlotReference() )
            continue;

        if( textItem->GetText() == wxT( "${VALUE}" ) && !GetPlotValue() )
            continue;

        PlotFootprintTextItem( textItem, getColor( textLayer ) );
    }
}


void BRDITEMS_PLOTTER::PlotPcbGraphicItem( const BOARD_ITEM* item )
{
    switch( item->Type() )
    {
    case PCB_SHAPE_T:
        PlotPcbShape( static_cast<const PCB_SHAPE*>( item ) );
        break;

    case PCB_TEXT_T:
    {
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( item );
        PlotPcbText( text, text->GetLayer(), text->IsKnockout() );
        break;
    }

    case PCB_TEXTBOX_T:
    {
        const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( item );
        PlotPcbText( textbox, textbox->GetLayer(), textbox->IsKnockout() );
        PlotPcbShape( textbox );
        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        PlotDimension( static_cast<const PCB_DIMENSION_BASE*>( item ) );
        break;

    case PCB_TARGET_T:
        PlotPcbTarget( static_cast<const PCB_TARGET*>( item ) );
        break;

    default:
        break;
    }
}


void BRDITEMS_PLOTTER::PlotBoardGraphicItems()
{
    for( const BOARD_ITEM* item : m_board->Drawings() )
        PlotPcbGraphicItem( item );
}


void BRDITEMS_PLOTTER::PlotFootprintTextItem( const PCB_TEXT* aText, const COLOR4D& aColor )
{
    COLOR4D color = aColor;

    if( aColor == COLOR4D::WHITE )
        color = COLOR4D( LIGHTGRAY );

    m_plotter->SetColor( color );

    // calculate some text parameters :
    //VECTOR2I      size = aText->GetTextSize();
    VECTOR2I      pos = aText->GetTextPos();
    int           thickness = aText->GetEffectiveTextPenWidth();
    KIFONT::FONT* font = aText->GetFont();

    if( !font )
    {
        font = KIFONT::FONT::GetFont( m_plotter->RenderSettings()
                                            ? m_plotter->RenderSettings()->GetDefaultFont()
                                            : wxString( wxEmptyString ),
                                      aText->IsBold(), aText->IsItalic() );
    }

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    TEXT_ATTRIBUTES attrs = aText->GetAttributes();
    attrs.m_StrokeWidth = thickness;
    attrs.m_Angle = aText->GetDrawRotation();
    attrs.m_Bold = true;
    attrs.m_Multiline = false;

    GBR_METADATA gbr_metadata;

    if( IsCopperLayer( aText->GetLayer() ) )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    const FOOTPRINT* parent = aText->GetParentFootprint();
    gbr_metadata.SetCmpReference( parent->GetReference() );

    m_plotter->SetCurrentLineWidth( thickness );

    if( aText->IsKnockout() )
    {
        KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
        SHAPE_POLY_SET             knockouts;

        CALLBACK_GAL callback_gal( empty_opts,
                // Polygon callback
                [&]( const SHAPE_LINE_CHAIN& aPoly )
                {
                    knockouts.AddOutline( aPoly );
                } );

        callback_gal.SetIsFill( font->IsOutline() );
        callback_gal.SetIsStroke( font->IsStroke() );
        font->Draw( &callback_gal, aText->GetShownText( true ), aText->GetDrawPos(), attrs );

        SHAPE_POLY_SET finalPoly;
        int            margin = attrs.m_StrokeWidth * 1.5
                                    + GetKnockoutTextMargin( attrs.m_Size, attrs.m_StrokeWidth );

        aText->TransformBoundingBoxToPolygon( &finalPoly, margin );
        finalPoly.BooleanSubtract( knockouts, SHAPE_POLY_SET::PM_FAST );
        finalPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

        for( int ii = 0; ii < finalPoly.OutlineCount(); ++ii )
            m_plotter->PlotPoly( finalPoly.Outline( ii ), FILL_T::FILLED_SHAPE, 0, &gbr_metadata );
    }
    else
    {
        m_plotter->PlotText( pos, aColor, aText->GetShownText( true ), attrs, font, &gbr_metadata );
    }
}


void BRDITEMS_PLOTTER::PlotDimension( const PCB_DIMENSION_BASE* aDim )
{
    if( !m_layerMask[aDim->GetLayer()] )
        return;

    PCB_SHAPE draw;

    draw.SetStroke( STROKE_PARAMS( aDim->GetLineThickness(), PLOT_DASH_TYPE::SOLID ) );
    draw.SetLayer( aDim->GetLayer() );

    COLOR4D color = ColorSettings()->GetColor( aDim->GetLayer() );

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( color != WHITE ? color : LIGHTGRAY);

    PlotPcbText( aDim, aDim->GetLayer(), false );

    for( const std::shared_ptr<SHAPE>& shape : aDim->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();

            draw.SetShape( SHAPE_T::SEGMENT );
            draw.SetStart(  seg.A );
            draw.SetEnd( seg.B );

            PlotPcbShape( &draw );
            break;
        }

        case SH_CIRCLE:
        {
            VECTOR2I start( shape->Centre() );
            int radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();

            draw.SetShape( SHAPE_T::CIRCLE );
            draw.SetFilled( false );
            draw.SetStart( start );
            draw.SetEnd( VECTOR2I( start.x + radius, start.y ) );

            PlotPcbShape( &draw );
            break;
        }

        default:
            break;
        }
    }
}


void BRDITEMS_PLOTTER::PlotPcbTarget( const PCB_TARGET* aMire )
{
    int dx1, dx2, dy1, dy2, radius;

    if( !m_layerMask[aMire->GetLayer()] )
        return;

    m_plotter->SetColor( getColor( aMire->GetLayer() ) );

    PCB_SHAPE draw;

    draw.SetShape( SHAPE_T::CIRCLE );
    draw.SetFilled( false );
    draw.SetStroke( STROKE_PARAMS( aMire->GetWidth(), PLOT_DASH_TYPE::SOLID ) );
    draw.SetLayer( aMire->GetLayer() );
    draw.SetStart( aMire->GetPosition() );
    radius = aMire->GetSize() / 3;

    if( aMire->GetShape() )   // shape X
        radius = aMire->GetSize() / 2;

    // Draw the circle
    draw.SetEnd( VECTOR2I( draw.GetStart().x + radius, draw.GetStart().y ) );

    PlotPcbShape( &draw );

    draw.SetShape( SHAPE_T::SEGMENT );

    radius = aMire->GetSize() / 2;
    dx1    = radius;
    dy1    = 0;
    dx2    = 0;
    dy2    = radius;

    if( aMire->GetShape() )    // Shape X
    {
        dx1 = dy1 = radius;
        dx2 = dx1;
        dy2 = -dy1;
    }

    VECTOR2I mirePos( aMire->GetPosition() );

    // Draw the X or + shape:
    draw.SetStart( VECTOR2I( mirePos.x - dx1, mirePos.y - dy1 ) );
    draw.SetEnd( VECTOR2I( mirePos.x + dx1, mirePos.y + dy1 ) );
    PlotPcbShape( &draw );

    draw.SetStart( VECTOR2I( mirePos.x - dx2, mirePos.y - dy2 ) );
    draw.SetEnd( VECTOR2I( mirePos.x + dx2, mirePos.y + dy2 ) );
    PlotPcbShape( &draw );
}


void BRDITEMS_PLOTTER::PlotFootprintGraphicItems( const FOOTPRINT* aFootprint )
{
    for( const BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( aFootprint->GetPrivateLayers().test( item->GetLayer() ) )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
        {
            const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( item );

            if( m_layerMask[ shape->GetLayer() ] )
                PlotPcbShape( shape );

            break;
        }

        case PCB_TEXTBOX_T:
        {
            const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( item );

            if( m_layerMask[ textbox->GetLayer() ] )
            {
                PlotPcbText( textbox, textbox->GetLayer(), textbox->IsKnockout() );
                PlotPcbShape( textbox );
            }

            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
        {
            const PCB_DIMENSION_BASE* dimension = static_cast<const PCB_DIMENSION_BASE*>( item );

            if( m_layerMask[ dimension->GetLayer() ] )
                PlotDimension( dimension );

            break;
        }

        case PCB_TEXT_T:
            // Plotted in PlotFootprintTextItem()
            break;

        default:
            UNIMPLEMENTED_FOR( item->GetClass() );
        }
    }
}


#include <font/stroke_font.h>
void BRDITEMS_PLOTTER::PlotPcbText( const EDA_TEXT* aText, PCB_LAYER_ID aLayer, bool aIsKnockout )
{
    KIFONT::FONT* font = aText->GetFont();

    if( !font )
    {
        font = KIFONT::FONT::GetFont( m_plotter->RenderSettings()
                                            ? m_plotter->RenderSettings()->GetDefaultFont()
                                            : wxString( wxEmptyString ),
                                      aText->IsBold(), aText->IsItalic() );
    }

    wxString shownText( aText->GetShownText( true ) );

    if( shownText.IsEmpty() )
        return;

    if( !m_layerMask[aLayer] )
        return;

    GBR_METADATA gbr_metadata;

    if( IsCopperLayer( aLayer ) )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    COLOR4D color = getColor( aLayer );
    m_plotter->SetColor( color );

    VECTOR2I pos = aText->GetTextPos();

    TEXT_ATTRIBUTES attrs = aText->GetAttributes();
    attrs.m_StrokeWidth = aText->GetEffectiveTextPenWidth();
    attrs.m_Angle = aText->GetDrawRotation();
    attrs.m_Multiline = false;

    m_plotter->SetCurrentLineWidth( attrs.m_StrokeWidth );

    if( aIsKnockout )
    {
        KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
        SHAPE_POLY_SET             knockouts;

        CALLBACK_GAL callback_gal( empty_opts,
                // Polygon callback
                [&]( const SHAPE_LINE_CHAIN& aPoly )
                {
                    knockouts.AddOutline( aPoly );
                } );

        callback_gal.SetIsFill( font->IsOutline() );
        callback_gal.SetIsStroke( font->IsStroke() );
        font->Draw( &callback_gal, shownText, aText->GetDrawPos(), attrs );

        SHAPE_POLY_SET finalPoly;
        int            margin = attrs.m_StrokeWidth * 1.5
                                    + GetKnockoutTextMargin( attrs.m_Size, attrs.m_StrokeWidth );

        aText->TransformBoundingBoxToPolygon( &finalPoly, margin );
        finalPoly.BooleanSubtract( knockouts, SHAPE_POLY_SET::PM_FAST );
        finalPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

        for( int ii = 0; ii < finalPoly.OutlineCount(); ++ii )
            m_plotter->PlotPoly( finalPoly.Outline( ii ), FILL_T::FILLED_SHAPE, 0, &gbr_metadata );
    }
    else if( aText->IsMultilineAllowed() )
    {
        std::vector<VECTOR2I> positions;
        wxArrayString strings_list;
        wxStringSplit( shownText, strings_list, '\n' );
        positions.reserve(  strings_list.Count() );

        aText->GetLinePositions( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString& txt =  strings_list.Item( ii );
            m_plotter->PlotText( positions[ii], color, txt, attrs, font, &gbr_metadata );
        }
    }
    else
    {
        m_plotter->PlotText( pos, color, shownText, attrs, font, &gbr_metadata );
    }
}


void BRDITEMS_PLOTTER::PlotFilledAreas( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                        const SHAPE_POLY_SET& polysList )
{
    if( polysList.IsEmpty() )
        return;

    GBR_METADATA gbr_metadata;

    if( aZone->IsOnCopperLayer() )
    {
        gbr_metadata.SetNetName( aZone->GetNetname() );
        gbr_metadata.SetCopper( true );

        // Zones with no net name can exist.
        // they are not used to connect items, so the aperture attribute cannot
        // be set as conductor
        if( aZone->GetNetname().IsEmpty() )
        {
            gbr_metadata.SetApertureAttrib(
                    GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );
        }
        else
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
        }
    }

    m_plotter->SetColor( getColor( aLayer ) );

    m_plotter->StartBlock( nullptr );    // Clean current object attributes

    /*
     * In non filled mode the outline is plotted, but not the filling items
     */

    for( int idx = 0; idx < polysList.OutlineCount(); ++idx )
    {
        const SHAPE_LINE_CHAIN& outline = polysList.Outline( idx );

        // Plot the current filled area (as region for Gerber plotter to manage attributes)
        if( GetPlotMode() == FILLED )
        {
            if( m_plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
            {
                static_cast<GERBER_PLOTTER*>( m_plotter )->PlotGerberRegion( outline,
                                                                             &gbr_metadata );
            }
            else
            {
                m_plotter->PlotPoly( outline, FILL_T::FILLED_SHAPE, 0, &gbr_metadata );
            }
        }
        else
        {
            m_plotter->SetCurrentLineWidth( -1 );
        }
    }

    m_plotter->EndBlock( nullptr );    // Clear object attributes
}


void BRDITEMS_PLOTTER::PlotPcbShape( const PCB_SHAPE* aShape )
{
    if( !m_layerMask[aShape->GetLayer()] )
        return;

    bool           sketch = GetPlotMode() == SKETCH;
    int            thickness = aShape->GetWidth();
    PLOT_DASH_TYPE lineStyle = aShape->GetStroke().GetPlotStyle();

    m_plotter->SetColor( getColor( aShape->GetLayer() ) );

    const FOOTPRINT* parentFP = aShape->GetParentFootprint();
    GBR_METADATA     gbr_metadata;

    if( parentFP )
    {
        gbr_metadata.SetCmpReference( parentFP->GetReference() );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    }

    if( aShape->GetLayer() == Edge_Cuts )
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_EDGECUT );
    }
    else if( IsCopperLayer( aShape->GetLayer() ) )
    {
        if( parentFP )
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_ETCHEDCMP );
            gbr_metadata.SetCopper( true );
        }
        else
        {
            // Graphic items (PCB_SHAPE, TEXT) having no net have the NonConductor attribute
            // Graphic items having a net have the Conductor attribute, but are not (yet?)
            // supported in Pcbnew
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );
        }
    }

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        switch( aShape->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            m_plotter->ThickSegment( aShape->GetStart(), aShape->GetEnd(), thickness, GetPlotMode(),
                                     &gbr_metadata );
            break;

        case SHAPE_T::CIRCLE:
            if( aShape->IsFilled() )
            {
                m_plotter->FilledCircle( aShape->GetStart(), aShape->GetRadius() * 2 + thickness,
                                         GetPlotMode(), &gbr_metadata );
            }
            else
            {
                m_plotter->ThickCircle( aShape->GetStart(), aShape->GetRadius() * 2, thickness,
                                        GetPlotMode(), &gbr_metadata );
            }

            break;

        case SHAPE_T::ARC:
        {
            // when startAngle == endAngle ThickArc() doesn't know whether it's 0 deg and 360 deg
            // but it is a circle
            if( std::abs( aShape->GetArcAngle().AsDegrees() ) == 360.0 )
            {
                m_plotter->ThickCircle( aShape->GetCenter(), aShape->GetRadius() * 2, thickness,
                                        GetPlotMode(), &gbr_metadata );
            }
            else
            {
                m_plotter->ThickArc( *aShape, GetPlotMode(), &gbr_metadata );
            }

            break;
        }

        case SHAPE_T::BEZIER:
            m_plotter->BezierCurve( aShape->GetStart(), aShape->GetBezierC1(),
                                    aShape->GetBezierC2(), aShape->GetEnd(), 0, thickness );
            break;

        case SHAPE_T::POLY:
            if( aShape->IsPolyShapeValid() )
            {
                if( sketch )
                {
                    for( auto it = aShape->GetPolyShape().CIterateSegments( 0 ); it; it++ )
                    {
                        auto seg = it.Get();
                        m_plotter->ThickSegment( seg.A, seg.B, thickness, GetPlotMode(),
                                                 &gbr_metadata );
                    }
                }
                else
                {
                    m_plotter->SetCurrentLineWidth( thickness, &gbr_metadata );

                    // Draw the polygon: only one polygon is expected
                    // However we provide a multi polygon shape drawing
                    // ( for the future or to show a non expected shape )
                    // This must be simplified and fractured to prevent overlapping polygons
                    // from generating invalid Gerber files
                    SHAPE_POLY_SET tmpPoly = aShape->GetPolyShape().CloneDropTriangulation();
                    tmpPoly.Fracture( SHAPE_POLY_SET::PM_FAST );
                    FILL_T fill = aShape->IsFilled() ? FILL_T::FILLED_SHAPE : FILL_T::NO_FILL;

                    for( int jj = 0; jj < tmpPoly.OutlineCount(); ++jj )
                    {
                        SHAPE_LINE_CHAIN& poly = tmpPoly.Outline( jj );

                        // Ensure the polygon is closed:
                        poly.SetClosed( true );

                        // Plot the current filled area
                        // (as region for Gerber plotter to manage attributes)
                        if( m_plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
                        {
                            static_cast<GERBER_PLOTTER*>( m_plotter )->
                                        PlotPolyAsRegion( poly, fill, thickness, &gbr_metadata );
                        }
                        else
                        {
                            m_plotter->PlotPoly( poly, fill, thickness, &gbr_metadata );
                        }
                    }
                }
            }

            break;

        case SHAPE_T::RECT:
        {
            std::vector<VECTOR2I> pts = aShape->GetRectCorners();

            if( sketch )
            {
                m_plotter->ThickSegment( pts[0], pts[1], thickness, GetPlotMode(), &gbr_metadata );
                m_plotter->ThickSegment( pts[1], pts[2], thickness, GetPlotMode(), &gbr_metadata );
                m_plotter->ThickSegment( pts[2], pts[3], thickness, GetPlotMode(), &gbr_metadata );
                m_plotter->ThickSegment( pts[3], pts[0], thickness, GetPlotMode(), &gbr_metadata );
            }

            if( !sketch )
            {
                SHAPE_LINE_CHAIN poly;

                for( const VECTOR2I& pt : pts )
                    poly.Append( pt );

                poly.Append( pts[0] );  // Close polygon.

                FILL_T fill_mode = aShape->IsFilled() ? FILL_T::FILLED_SHAPE
                                                      : FILL_T::NO_FILL;

                if( m_plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
                {
                    static_cast<GERBER_PLOTTER*>( m_plotter )->
                                PlotPolyAsRegion( poly, fill_mode, thickness, &gbr_metadata );
                }
                else
                {
                    m_plotter->PlotPoly( poly, fill_mode, thickness, &gbr_metadata );
                }
            }

            break;
        }

        default:
            UNIMPLEMENTED_FOR( aShape->SHAPE_T_asString() );
        }
    }
    else
    {
        std::vector<SHAPE*> shapes = aShape->MakeEffectiveShapes( true );

        for( SHAPE* shape : shapes )
        {
            STROKE_PARAMS::Stroke( shape, lineStyle, thickness, m_plotter->RenderSettings(),
                                   [&]( const VECTOR2I& a, const VECTOR2I& b )
                                   {
                                       m_plotter->ThickSegment( a, b, thickness, GetPlotMode(),
                                                                &gbr_metadata );
                                   } );
        }

        for( SHAPE* shape : shapes )
            delete shape;
    }
}


void BRDITEMS_PLOTTER::plotOneDrillMark( PAD_DRILL_SHAPE_T aDrillShape, const VECTOR2I& aDrillPos,
                                         const VECTOR2I& aDrillSize, const VECTOR2I& aPadSize,
                                         const EDA_ANGLE& aOrientation, int aSmallDrill )
{
    VECTOR2I drillSize = aDrillSize;

    // Small drill marks have no significance when applied to slots
    if( aSmallDrill && aDrillShape == PAD_DRILL_SHAPE_CIRCLE )
        drillSize.x = std::min( aSmallDrill, drillSize.x );

    // Round holes only have x diameter, slots have both
    drillSize.x -= getFineWidthAdj();
    drillSize.x = Clamp( 1, drillSize.x, aPadSize.x - 1 );

    if( aDrillShape == PAD_DRILL_SHAPE_OBLONG )
    {
        drillSize.y -= getFineWidthAdj();
        drillSize.y = Clamp( 1, drillSize.y, aPadSize.y - 1 );

        m_plotter->FlashPadOval( aDrillPos, drillSize, aOrientation, GetPlotMode(), nullptr );
    }
    else
    {
        m_plotter->FlashPadCircle( aDrillPos, drillSize.x, GetPlotMode(), nullptr );
    }
}


void BRDITEMS_PLOTTER::PlotDrillMarks()
{
    /* If small drills marks were requested prepare a clamp value to pass
       to the helper function */
    int smallDrill = GetDrillMarksType() == DRILL_MARKS::SMALL_DRILL_SHAPE
                    ? pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_SmallDrillMarkSize ) : 0;

    /* In the filled trace mode drill marks are drawn white-on-black to scrape
       the underlying pad. This works only for drivers supporting color change,
       obviously... it means that:
       - PS, SVG and PDF output is correct (i.e. you have a 'donut' pad)
       - In HPGL you can't see them
       - In gerbers you can't see them, too. This is arguably the right thing to
         do since having drill marks and high speed drill stations is a sure
         recipe for broken tools and angry manufacturers. If you *really* want them
         you could start a layer with negative polarity to scrape the film.
       - In DXF they go into the 'WHITE' layer. This could be useful.
     */
    if( GetPlotMode() == FILLED )
         m_plotter->SetColor( WHITE );

    for( PCB_TRACK* tracks : m_board->Tracks() )
    {
        const PCB_VIA* via = dyn_cast<const PCB_VIA*>( tracks );

        if( via )
        {
            plotOneDrillMark( PAD_DRILL_SHAPE_CIRCLE, via->GetStart(),
                              VECTOR2I( via->GetDrillValue(), 0 ), VECTOR2I( via->GetWidth(), 0 ),
                              ANGLE_0, smallDrill );
        }
    }

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( pad->GetDrillSize().x == 0 )
                continue;

            plotOneDrillMark( pad->GetDrillShape(), pad->GetPosition(), pad->GetDrillSize(),
                              pad->GetSize(), pad->GetOrientation(), smallDrill );
        }
    }

    if( GetPlotMode() == FILLED )
        m_plotter->SetColor( BLACK );
}
