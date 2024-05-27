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

#include <core/typeinfo.h>                    // for dyn_cast, PCB_DIMENSION_T
#include <gbr_metadata.h>
#include <gbr_netlist_metadata.h>             // for GBR_NETLIST_METADATA
#include <layer_ids.h>                        // for LSET, IsCopperLayer
#include <pcbplot.h>
#include <pcb_plot_params.h>                  // for PCB_PLOT_PARAMS, PCB_PL...
#include <advanced_config.h>

#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_tablecell.h>
#include <pcb_table.h>
#include <zone.h>

#include <wx/debug.h>                         // for wxASSERT_MSG


COLOR4D BRDITEMS_PLOTTER::getColor( int aLayer ) const
{
    COLOR4D color = ColorSettings()->GetColor( aLayer );

    // A hack to avoid plotting a white item in white color on white paper
    if( color == COLOR4D::WHITE )
        color = COLOR4D( LIGHTGRAY );

    return color;
}


void BRDITEMS_PLOTTER::PlotPadNumber( const PAD* aPad, const COLOR4D& aColor )
{
    wxString padNumber = UnescapeString( aPad->GetNumber() );

    if( padNumber.IsEmpty() )
        return;

    BOX2I    padBBox = aPad->GetBoundingBox();
    VECTOR2I position = padBBox.Centre();
    VECTOR2I padsize = padBBox.GetSize();

    if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        // See if we have a number box
        for( const std::shared_ptr<PCB_SHAPE>& primitive : aPad->GetPrimitives() )
        {
            if( primitive->IsProxyItem() && primitive->GetShape() == SHAPE_T::RECTANGLE )
            {
                position = primitive->GetCenter();
                RotatePoint( position, aPad->GetOrientation() );
                position += aPad->ShapePos();

                padsize.x = abs( primitive->GetBotRight().x - primitive->GetTopLeft().x );
                padsize.y = abs( primitive->GetBotRight().y - primitive->GetTopLeft().y );

                break;
            }
        }
    }

    if( aPad->GetShape() != PAD_SHAPE::CUSTOM )
    {
        // Don't allow a 45° rotation to bloat a pad's bounding box unnecessarily
        int limit = KiROUND( std::min( aPad->GetSize().x, aPad->GetSize().y ) * 1.1 );

        if( padsize.x > limit && padsize.y > limit )
        {
            padsize.x = limit;
            padsize.y = limit;
        }
    }

    TEXT_ATTRIBUTES textAttrs;

    if( padsize.x < ( padsize.y * 0.95 ) )
    {
        textAttrs.m_Angle = ANGLE_90;
        std::swap( padsize.x, padsize.y );
    }

    // approximate the size of the pad number text:
    // We use a size for at least 3 chars, to give a good look even for short numbers
    int tsize = KiROUND( padsize.x / std::max( PrintableCharCount( padNumber ), 3 ) );
    tsize = std::min( tsize, padsize.y );

    // enforce a max size
    tsize = std::min( tsize, pcbIUScale.mmToIU( 5.0 ) );

    textAttrs.m_Size = VECTOR2I( tsize, tsize );

    // use a somewhat spindly font to go with the outlined pads
    textAttrs.m_StrokeWidth = KiROUND( tsize / 12.0 );

    m_plotter->PlotText( position, aColor, padNumber, textAttrs );
}


void BRDITEMS_PLOTTER::PlotPad( const PAD* aPad, const COLOR4D& aColor, OUTLINE_MODE aPlotMode )
{
    VECTOR2I     shape_pos = aPad->ShapePos();
    GBR_METADATA metadata;

    bool plotOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();
    bool plotOnExternalCopperLayer = ( m_layerMask & LSET::ExternalCuMask() ).any();

    // Pad not on the solder mask layer cannot be soldered.
    // therefore it can have a specific aperture attribute.
    // Not yet in use.
    // bool isPadOnBoardTechLayers = ( aPad->GetLayerSet() & LSET::AllBoardTechMask() ).any();

    metadata.SetCmpReference( aPad->GetParentFootprint()->GetReference() );

    if( plotOnCopperLayer )
    {
        metadata.SetNetAttribType( GBR_NETINFO_ALL );
        metadata.SetCopper( true );

        // Gives a default attribute, for instance for pads used as tracks in net ties:
        // Connector pads and SMD pads are on external layers
        // if on internal layers, they are certainly used as net tie
        // and are similar to tracks: just conductor items
        metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );

        const bool useUTF8 = false;
        const bool useQuoting = false;
        metadata.SetPadName( aPad->GetNumber(), useUTF8, useQuoting );

        if( !aPad->GetNumber().IsEmpty() )
            metadata.SetPadPinFunction( aPad->GetPinFunction(), useUTF8, useQuoting );

        metadata.SetNetName( aPad->GetNetname() );

        // Some pads are mechanical pads ( through hole or smd )
        // when this is the case, they have no pad name and/or are not plated.
        // In this case gerber files have slightly different attributes.
        if( aPad->GetAttribute() == PAD_ATTRIB::NPTH || aPad->GetNumber().IsEmpty() )
            metadata.m_NetlistMetadata.m_NotInNet = true;

        if( !plotOnExternalCopperLayer )
        {
            // the .P object attribute (GBR_NETLIST_METADATA::GBR_NETINFO_PAD)
            // is used on outer layers, unless the component is embedded
            // or a "etched" component (fp only drawn, not a physical component)
            // Currently, Pcbnew does not handle embedded component, so we disable the .P
            // attribute on internal layers
            // Note the Gerber doc is not really clear about through holes pads about the .P
            metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET |
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
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
            break;

        case PAD_ATTRIB::PTH :       // Pad through hole, a hole is also expected
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_COMPONENTPAD );
            break;

        case PAD_ATTRIB::CONN:       // Connector pads, no solder paste but with solder mask.
            if( plotOnExternalCopperLayer )
                metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONNECTORPAD );
            break;

        case PAD_ATTRIB::SMD:        // SMD pads (on external copper layer only)
                                     // with solder paste and mask
            if( plotOnExternalCopperLayer )
                metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_SMDPAD_CUDEF );
            break;
        }

        // Fabrication properties can have specific GBR_APERTURE_METADATA options
        // that replace previous aperture attribute:
        switch( aPad->GetProperty() )
        {
        case PAD_PROP::BGA:          // Only applicable to outer layers
            if( plotOnExternalCopperLayer )
                metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_BGAPAD_CUDEF );
            break;

        case PAD_PROP::FIDUCIAL_GLBL:
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_GLBL );
            break;

        case PAD_PROP::FIDUCIAL_LOCAL:
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_LOCAL );
            break;

        case PAD_PROP::TESTPOINT:    // Only applicable to outer layers
            if( plotOnExternalCopperLayer )
                metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_TESTPOINT );
            break;

        case PAD_PROP::HEATSINK:
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_HEATSINKPAD );
            break;

        case PAD_PROP::CASTELLATED:
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CASTELLATEDPAD );
            break;

        case PAD_PROP::NONE:
        case PAD_PROP::MECHANICAL:
            break;
        }

        // Ensure NPTH pads have *always* the GBR_APERTURE_ATTRIB_WASHERPAD attribute
        if( aPad->GetAttribute() == PAD_ATTRIB::NPTH )
            metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
    }
    else
    {
        metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    }

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( aColor != WHITE ? aColor : LIGHTGRAY);

    if( aPlotMode == SKETCH )
        m_plotter->SetCurrentLineWidth( GetSketchPadLineWidth(), &metadata );

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE::CIRCLE:
        m_plotter->FlashPadCircle( shape_pos, aPad->GetSize().x, aPlotMode, &metadata );
        break;

    case PAD_SHAPE::OVAL:
        m_plotter->FlashPadOval( shape_pos, aPad->GetSize(), aPad->GetOrientation(), aPlotMode,
                                 &metadata );
        break;

    case PAD_SHAPE::RECTANGLE:
        m_plotter->FlashPadRect( shape_pos, aPad->GetSize(), aPad->GetOrientation(), aPlotMode,
                                 &metadata );
        break;

    case PAD_SHAPE::ROUNDRECT:
        m_plotter->FlashPadRoundRect( shape_pos, aPad->GetSize(), aPad->GetRoundRectCornerRadius(),
                                      aPad->GetOrientation(), aPlotMode, &metadata );
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

        m_plotter->FlashPadTrapez( shape_pos, coord, aPad->GetOrientation(), aPlotMode, &metadata );
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
                                                     aPad->GetOrientation(), aPlotMode, &metadata );
            break;
        }

        KI_FALLTHROUGH;

    default:
    case PAD_SHAPE::CUSTOM:
    {
        const std::shared_ptr<SHAPE_POLY_SET>& polygons = aPad->GetEffectivePolygon( ERROR_INSIDE );

        if( polygons->OutlineCount() )
        {
            m_plotter->FlashPadCustom( shape_pos, aPad->GetSize(), aPad->GetOrientation(),
                                       polygons.get(), aPlotMode, &metadata );
        }
    }
        break;
    }
}


void BRDITEMS_PLOTTER::PlotFootprintTextItems( const FOOTPRINT* aFootprint )
{
    if( !GetPlotFPText() )
        return;

    const PCB_TEXT* textItem = &aFootprint->Reference();
    PCB_LAYER_ID    textLayer = textItem->GetLayer();

    // Reference and value have special controls for forcing their plotting
    if( GetPlotReference() && m_layerMask[textLayer]
        && ( textItem->IsVisible() || GetPlotInvisibleText() ) )
    {
        PlotText( textItem, textLayer, textItem->IsKnockout(), textItem->GetFontMetrics() );
    }

    textItem  = &aFootprint->Value();
    textLayer = textItem->GetLayer();

    if( GetPlotValue() && m_layerMask[textLayer]
        && ( textItem->IsVisible() || GetPlotInvisibleText() ) )
    {
        PlotText( textItem, textLayer, textItem->IsKnockout(), textItem->GetFontMetrics() );
    }

    std::vector<PCB_TEXT*> texts;

    // Skip the reference and value texts that are handled specially
    for( PCB_FIELD* field : aFootprint->Fields() )
    {
        if( field->IsReference() || field->IsValue() )
            continue;

        texts.push_back( field );
    }

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        textItem = dynamic_cast<const PCB_TEXT*>( item );

        if( textItem )
            texts.push_back( static_cast<PCB_TEXT*>( item ) );
    }

    for( const PCB_TEXT* text : texts )
    {
        if( !text->IsVisible() )
            continue;

        textLayer = text->GetLayer();

        if( textLayer == Edge_Cuts || textLayer >= PCB_LAYER_ID_COUNT )
            continue;

        if( !m_layerMask[textLayer] || aFootprint->GetPrivateLayers().test( textLayer ) )
            continue;

        if( text->GetText() == wxT( "${REFERENCE}" ) && !GetPlotReference() )
            continue;

        if( text->GetText() == wxT( "${VALUE}" ) && !GetPlotValue() )
            continue;

        PlotText( text, textLayer, text->IsKnockout(), text->GetFontMetrics() );
    }
}


void BRDITEMS_PLOTTER::PlotBoardGraphicItem( const BOARD_ITEM* item )
{
    switch( item->Type() )
    {
    case PCB_SHAPE_T:
        PlotShape( static_cast<const PCB_SHAPE*>( item ) );
        break;

    case PCB_TEXT_T:
    {
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( item );
        PlotText( text, text->GetLayer(), text->IsKnockout(), text->GetFontMetrics() );
        break;
    }

    case PCB_TEXTBOX_T:
    {
        m_plotter->SetTextMode( PLOT_TEXT_MODE::STROKE );

        const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( item );
        PlotText( textbox, textbox->GetLayer(), textbox->IsKnockout(), textbox->GetFontMetrics() );

        if( textbox->IsBorderEnabled() )
            PlotShape( textbox );

        m_plotter->SetTextMode( GetTextMode() );
        break;
    }

    case PCB_TABLE_T:
    {
        const PCB_TABLE* table = static_cast<const PCB_TABLE*>( item );

        m_plotter->SetTextMode( PLOT_TEXT_MODE::STROKE );

        for( const PCB_TABLECELL* cell : table->GetCells() )
            PlotText( cell, cell->GetLayer(), cell->IsKnockout(), cell->GetFontMetrics() );

        PlotTableBorders( table );

        m_plotter->SetTextMode( GetTextMode() );
        break;
    }

    case PCB_DIM_ALIGNED_T:
    case PCB_DIM_CENTER_T:
    case PCB_DIM_RADIAL_T:
    case PCB_DIM_ORTHOGONAL_T:
    case PCB_DIM_LEADER_T:
        m_plotter->SetTextMode( PLOT_TEXT_MODE::STROKE );

        PlotDimension( static_cast<const PCB_DIMENSION_BASE*>( item ) );

        m_plotter->SetTextMode( GetTextMode() );
        break;

    case PCB_TARGET_T:
        PlotPcbTarget( static_cast<const PCB_TARGET*>( item ) );
        break;

    default:
        break;
    }
}


void BRDITEMS_PLOTTER::PlotDimension( const PCB_DIMENSION_BASE* aDim )
{
    if( !m_layerMask[aDim->GetLayer()] )
        return;

    COLOR4D color = ColorSettings()->GetColor( aDim->GetLayer() );

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( color != WHITE ? color : LIGHTGRAY);

    PlotText( aDim, aDim->GetLayer(), false, aDim->GetFontMetrics() );

    PCB_SHAPE temp_item;

    temp_item.SetStroke( STROKE_PARAMS( aDim->GetLineThickness(), LINE_STYLE::SOLID ) );
    temp_item.SetLayer( aDim->GetLayer() );

    for( const std::shared_ptr<SHAPE>& shape : aDim->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();

            temp_item.SetShape( SHAPE_T::SEGMENT );
            temp_item.SetStart( seg.A );
            temp_item.SetEnd( seg.B );

            PlotShape( &temp_item );
            break;
        }

        case SH_CIRCLE:
        {
            VECTOR2I start( shape->Centre() );
            int radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();

            temp_item.SetShape( SHAPE_T::CIRCLE );
            temp_item.SetFilled( false );
            temp_item.SetStart( start );
            temp_item.SetEnd( VECTOR2I( start.x + radius, start.y ) );

            PlotShape( &temp_item );
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

    PCB_SHAPE temp_item;

    temp_item.SetShape( SHAPE_T::CIRCLE );
    temp_item.SetFilled( false );
    temp_item.SetStroke( STROKE_PARAMS( aMire->GetWidth(), LINE_STYLE::SOLID ) );
    temp_item.SetLayer( aMire->GetLayer() );
    temp_item.SetStart( aMire->GetPosition() );
    radius = aMire->GetSize() / 3;

    if( aMire->GetShape() )   // temp_item X
        radius = aMire->GetSize() / 2;

    // Draw the circle
    temp_item.SetEnd( VECTOR2I( temp_item.GetStart().x + radius, temp_item.GetStart().y ) );

    PlotShape( &temp_item );

    temp_item.SetShape( SHAPE_T::SEGMENT );

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

    // Draw the X or + temp_item:
    temp_item.SetStart( VECTOR2I( mirePos.x - dx1, mirePos.y - dy1 ) );
    temp_item.SetEnd( VECTOR2I( mirePos.x + dx1, mirePos.y + dy1 ) );
    PlotShape( &temp_item );

    temp_item.SetStart( VECTOR2I( mirePos.x - dx2, mirePos.y - dy2 ) );
    temp_item.SetEnd( VECTOR2I( mirePos.x + dx2, mirePos.y + dy2 ) );
    PlotShape( &temp_item );
}


void BRDITEMS_PLOTTER::PlotFootprintGraphicItems( const FOOTPRINT* aFootprint )
{
    for( const BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( aFootprint->GetPrivateLayers().test( item->GetLayer() ) )
            continue;

        if( !m_layerMask[ item->GetLayer() ] )
            continue;

        switch( item->Type() )
        {
        case PCB_SHAPE_T:
            PlotShape( static_cast<const PCB_SHAPE*>( item ) );
            break;

        case PCB_TEXTBOX_T:
        {
            const PCB_TEXTBOX* textbox = static_cast<const PCB_TEXTBOX*>( item );

            m_plotter->SetTextMode( PLOT_TEXT_MODE::STROKE );

            PlotText( textbox, textbox->GetLayer(), textbox->IsKnockout(),
                      textbox->GetFontMetrics() );

            if( textbox->IsBorderEnabled() )
                PlotShape( textbox );

            m_plotter->SetTextMode( GetTextMode() );
            break;
        }

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_RADIAL_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
            PlotDimension( static_cast<const PCB_DIMENSION_BASE*>( item ) );
            break;

        case PCB_TEXT_T:
            // Plotted in PlotFootprintTextItems()
            break;

        default:
            UNIMPLEMENTED_FOR( item->GetClass() );
        }
    }
}


#include <font/stroke_font.h>
void BRDITEMS_PLOTTER::PlotText( const EDA_TEXT* aText, PCB_LAYER_ID aLayer, bool aIsKnockout,
                                 const KIFONT::METRICS& aFontMetrics )
{
    KIFONT::FONT* font = aText->GetFont();

    if( !font )
    {
        wxString defaultFontName;   // empty string is the KiCad stroke font

        if( m_plotter->RenderSettings() )
            defaultFontName = m_plotter->RenderSettings()->GetDefaultFont();

        font = KIFONT::FONT::GetFont( defaultFontName, aText->IsBold(), aText->IsItalic() );
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
        const PCB_TEXT* text = static_cast<const PCB_TEXT*>( aText );
        SHAPE_POLY_SET  finalPoly;

        text->TransformTextToPolySet( finalPoly, 0, m_board->GetDesignSettings().m_MaxError,
                                      ERROR_INSIDE );
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
            m_plotter->PlotText( positions[ii], color, txt, attrs, font, aFontMetrics, &gbr_metadata );
        }
    }
    else
    {
        m_plotter->PlotText( pos, color, shownText, attrs, font, aFontMetrics, &gbr_metadata );
    }
}


void BRDITEMS_PLOTTER::PlotZone( const ZONE* aZone, PCB_LAYER_ID aLayer,
                                 const SHAPE_POLY_SET& aPolysList )
{
    if( aPolysList.IsEmpty() )
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
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );
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

    for( int idx = 0; idx < aPolysList.OutlineCount(); ++idx )
    {
        const SHAPE_LINE_CHAIN& outline = aPolysList.Outline( idx );

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


void BRDITEMS_PLOTTER::PlotShape( const PCB_SHAPE* aShape )
{
    if( !m_layerMask[aShape->GetLayer()] )
        return;

    bool       sketch = GetPlotMode() == SKETCH;
    int        thickness = aShape->GetWidth();
    LINE_STYLE lineStyle = aShape->GetStroke().GetLineStyle();

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
        else if( aShape->GetNetCode() > 0 )
        {
            gbr_metadata.SetCopper( true );
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
            gbr_metadata.SetNetName( aShape->GetNetname() );
        }
        else
        {
            // Graphic items (PCB_SHAPE, TEXT) having no net have the NonConductor attribute
            // Graphic items having a net have the Conductor attribute, but are not (yet?)
            // supported in Pcbnew
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );
        }
    }

    if( lineStyle <= LINE_STYLE::FIRST_TYPE )
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

        case SHAPE_T::RECTANGLE:
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

                FILL_T fill_mode = aShape->IsFilled() ? FILL_T::FILLED_SHAPE : FILL_T::NO_FILL;

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


void BRDITEMS_PLOTTER::PlotTableBorders( const PCB_TABLE* aTable )
{
    VECTOR2I     pos = aTable->GetPosition();
    VECTOR2I     end = aTable->GetEnd();
    int          lineWidth;
    LINE_STYLE   lineStyle;
    GBR_METADATA gbr_metadata;

    if( const FOOTPRINT* parentFP = aTable->GetParentFootprint() )
    {
        gbr_metadata.SetCmpReference( parentFP->GetReference() );
        gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    }

    auto setupStroke =
            [&]( const STROKE_PARAMS& stroke )
            {
                lineWidth = stroke.GetWidth();
                lineStyle = stroke.GetLineStyle();
            };

    auto strokeShape =
            [&]( const SHAPE& shape )
            {
                STROKE_PARAMS::Stroke( &shape, lineStyle, lineWidth, m_plotter->RenderSettings(),
                        [&]( const VECTOR2I& a, const VECTOR2I& b )
                        {
                            m_plotter->ThickSegment( a, b, lineWidth, GetPlotMode(),
                                                     &gbr_metadata );
                        } );
            };

    auto strokeLine =
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB )
            {
                if( lineStyle <= LINE_STYLE::FIRST_TYPE )
                {
                    m_plotter->ThickSegment( ptA, ptB, lineWidth, GetPlotMode(), &gbr_metadata );
                }
                else
                {
                    SHAPE_SEGMENT seg( ptA, ptB );
                    strokeShape( seg );
                }
            };

    auto strokeRect =
            [&]( const VECTOR2I& ptA, const VECTOR2I& ptB )
            {
                strokeLine( VECTOR2I( ptA.x, ptA.y ), VECTOR2I( ptB.x, ptA.y ) );
                strokeLine( VECTOR2I( ptB.x, ptA.y ), VECTOR2I( ptB.x, ptB.y ) );
                strokeLine( VECTOR2I( ptB.x, ptB.y ), VECTOR2I( ptA.x, ptB.y ) );
                strokeLine( VECTOR2I( ptA.x, ptB.y ), VECTOR2I( ptA.x, ptA.y ) );
            };

    if( aTable->GetSeparatorsStroke().GetWidth() >= 0 )
    {
        setupStroke( aTable->GetSeparatorsStroke() );

        if( aTable->StrokeColumns() )
        {
            for( int col = 0; col < aTable->GetColCount() - 1; ++col )
            {
                for( int row = 0; row < aTable->GetRowCount(); ++row )
                {
                    PCB_TABLECELL* cell = aTable->GetCell( row, col );
                    VECTOR2I       topRight( cell->GetEndX(), cell->GetStartY() );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                        strokeLine( topRight, cell->GetEnd() );
                }
            }
        }

        if( aTable->StrokeRows() )
        {
            for( int row = 0; row < aTable->GetRowCount() - 1; ++row )
            {
                for( int col = 0; col < aTable->GetColCount(); ++col )
                {
                    PCB_TABLECELL* cell = aTable->GetCell( row, col );
                    VECTOR2I       botLeft( cell->GetStartX(), cell->GetEndY() );

                    if( cell->GetColSpan() > 0 && cell->GetRowSpan() > 0 )
                        strokeLine( botLeft, cell->GetEnd() );
                }
            }
        }
    }

    if( aTable->GetBorderStroke().GetWidth() >= 0 )
    {
        setupStroke( aTable->GetBorderStroke() );

        if( aTable->StrokeHeader() )
        {
            PCB_TABLECELL* cell = aTable->GetCell( 0, 0 );
            strokeLine( VECTOR2I( pos.x, cell->GetEndY() ), VECTOR2I( end.x, cell->GetEndY() ) );
        }

        if( aTable->StrokeExternal() )
            strokeRect( pos, end );
    }
}


void BRDITEMS_PLOTTER::plotOneDrillMark( PAD_DRILL_SHAPE aDrillShape, const VECTOR2I& aDrillPos,
                                         const VECTOR2I& aDrillSize, const VECTOR2I& aPadSize,
                                         const EDA_ANGLE& aOrientation, int aSmallDrill )
{
    VECTOR2I drillSize = aDrillSize;

    // Small drill marks have no significance when applied to slots
    if( aSmallDrill && aDrillShape == PAD_DRILL_SHAPE::CIRCLE )
        drillSize.x = std::min( aSmallDrill, drillSize.x );

    // Round holes only have x diameter, slots have both
    drillSize.x -= getFineWidthAdj();
    drillSize.x = Clamp( 1, drillSize.x, aPadSize.x - 1 );

    if( aDrillShape == PAD_DRILL_SHAPE::OBLONG )
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
    bool onCopperLayer = ( LSET::AllCuMask() & m_layerMask ).any();
    int  smallDrill =  0;

    if( GetDrillMarksType() == DRILL_MARKS::SMALL_DRILL_SHAPE )
        smallDrill = pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_SmallDrillMarkSize );

    /* In the filled trace mode drill marks are drawn white-on-black to knock-out the underlying
       pad.  This works only for drivers supporting color change, obviously... it means that:
       - PS, SVG and PDF output is correct (i.e. you have a 'donut' pad)
       - In HPGL you can't see them
       - In gerbers you can't see them, too. This is arguably the right thing to do since having
         drill marks and high speed drill stations is a sure recipe for broken tools and angry
         manufacturers. If you *really* want them you could start a layer with negative polarity
         to knock-out the film.
       - In DXF they go into the 'WHITE' layer. This could be useful.
     */
    if( GetPlotMode() == FILLED && onCopperLayer )
         m_plotter->SetColor( WHITE );

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            const PCB_VIA* via = static_cast<const PCB_VIA*>( track );

            // Via are not always on all layers
            if( ( via->GetLayerSet() & m_layerMask ).none() )
                continue;

            plotOneDrillMark( PAD_DRILL_SHAPE::CIRCLE, via->GetStart(),
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

    if( GetPlotMode() == FILLED && onCopperLayer )
        m_plotter->SetColor( BLACK );
}
