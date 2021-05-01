/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <vector>                             // for vector, __vector_base<>...

#include <geometry/seg.h>                     // for SEG
#include <geometry/shape_circle.h>
#include <geometry/shape_line_chain.h>        // for SHAPE_LINE_CHAIN
#include <geometry/shape_poly_set.h>          // for SHAPE_POLY_SET, SHAPE_P...
#include <geometry/shape_segment.h>
#include <kicad_string.h>
#include <macros.h>
#include <math/util.h>                        // for KiROUND, Clamp
#include <math/vector2d.h>                    // for VECTOR2I
#include <plotter.h>
#include <plotters_specific.h>
#include <trigo.h>

#include <board_design_settings.h>            // for BOARD_DESIGN_SETTINGS
#include <core/typeinfo.h>                    // for dyn_cast, PCB_DIMENSION_T
#include <outline_mode.h>
#include <gal/color4d.h>                      // for COLOR4D, operator!=
#include <gbr_metadata.h>
#include <gbr_netlist_metadata.h>             // for GBR_NETLIST_METADATA
#include <layers_id_colors_and_visibility.h>  // for LSET, IsCopperLayer
#include <pad_shapes.h>                       // for PAD_ATTRIB_NPTH
#include <pcbplot.h>
#include <pcb_plot_params.h>                  // for PCB_PLOT_PARAMS, PCB_PL...
#include <advanced_config.h>

#include <board.h>
#include <board_item.h>                       // for BOARD_ITEM, S_CIRCLE
#include <dimension.h>
#include <pcb_shape.h>
#include <fp_shape.h>
#include <footprint.h>
#include <fp_text.h>
#include <track.h>
#include <pad.h>
#include <pcb_target.h>
#include <pcb_text.h>
#include <zone.h>

#include <wx/debug.h>                         // for wxASSERT_MSG
#include <wx/wx.h>                            // for wxPoint, wxSize, wxArra...


/* class BRDITEMS_PLOTTER is a helper class to plot board items
 * and a group of board items
 */

COLOR4D BRDITEMS_PLOTTER::getColor( LAYER_NUM aLayer ) const
{
    COLOR4D color = ColorSettings()->GetColor( aLayer );

    // A hack to avoid plotting a white item in white color, expecting the paper
    // is also white: use a non white color:
    if( color == COLOR4D::WHITE )
        color = COLOR4D( LIGHTGRAY );

    return color;
}


void BRDITEMS_PLOTTER::PlotPad( const PAD* aPad, COLOR4D aColor, OUTLINE_MODE aPlotMode )
{
    wxPoint shape_pos = aPad->ShapePos();
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
        gbr_metadata.SetPadName( aPad->GetName(), useUTF8, useQuoting );

        if( !aPad->GetName().IsEmpty() )
            gbr_metadata.SetPadPinFunction( aPad->GetPinFunction(), useUTF8, useQuoting );

        gbr_metadata.SetNetName( aPad->GetNetname() );

        // Some pads are mechanical pads ( through hole or smd )
        // when this is the case, they have no pad name and/or are not plated.
        // In this case gerber files have slightly different attributes.
        if( aPad->GetAttribute() == PAD_ATTRIB_NPTH || aPad->GetName().IsEmpty() )
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
        // Pad with type PAD_ATTRIB_CONN or PAD_ATTRIB_SMD that is not on outer layer
        // has its aperture attribute set to GBR_APERTURE_ATTRIB_CONDUCTOR
        switch( aPad->GetAttribute() )
        {
        case PAD_ATTRIB_NPTH:       // Mechanical pad through hole
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
            break;

        case PAD_ATTRIB_PTH :       // Pad through hole, a hole is also expected
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_COMPONENTPAD );
            break;

        case PAD_ATTRIB_CONN:       // Connector pads, no solder paste but with solder mask.
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONNECTORPAD );
            break;

        case PAD_ATTRIB_SMD:        // SMD pads (on external copper layer only)
                                    // with solder paste and mask
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_SMDPAD_CUDEF );
            break;
        }

        // Fabrication properties can have specific GBR_APERTURE_METADATA options
        // that replace previous aperture attribute:
        switch( aPad->GetProperty() )
        {
        case PAD_PROP_BGA:          // Only applicable to outer layers
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_BGAPAD_CUDEF );
            break;

        case PAD_PROP_FIDUCIAL_GLBL:
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_GLBL );
            break;

        case PAD_PROP_FIDUCIAL_LOCAL:
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_LOCAL );
            break;

        case PAD_PROP_TESTPOINT:    // Only applicable to outer layers
            if( plotOnExternalCopperLayer )
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_TESTPOINT );
            break;

        case PAD_PROP_HEATSINK:
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_HEATSINKPAD );
            break;

        case PAD_PROP_CASTELLATED:
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CASTELLATEDPAD );
            break;

        case PAD_PROP_NONE:
            break;
        }

        // Ensure NPTH pads have *always* the GBR_APERTURE_ATTRIB_WASHERPAD attribute
        if( aPad->GetAttribute() == PAD_ATTRIB_NPTH )
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
    case PAD_SHAPE_CIRCLE:
        m_plotter->FlashPadCircle( shape_pos, aPad->GetSize().x, aPlotMode, &gbr_metadata );
        break;

    case PAD_SHAPE_OVAL:
        m_plotter->FlashPadOval( shape_pos, aPad->GetSize(), aPad->GetOrientation(), aPlotMode,
                                 &gbr_metadata );
        break;

    case PAD_SHAPE_RECT:
        m_plotter->FlashPadRect( shape_pos, aPad->GetSize(), aPad->GetOrientation(), aPlotMode,
                                 &gbr_metadata );
        break;

    case PAD_SHAPE_ROUNDRECT:
        m_plotter->FlashPadRoundRect( shape_pos, aPad->GetSize(), aPad->GetRoundRectCornerRadius(),
                                      aPad->GetOrientation(), aPlotMode, &gbr_metadata );
        break;

    case PAD_SHAPE_TRAPEZOID:
    {
        // Build the pad polygon in coordinates relative to the pad
        // (i.e. for a pad at pos 0,0, rot 0.0). Needed to use aperture macros,
        // to be able to create a pattern common to all trapezoid pads having the same shape
        wxPoint coord[4];
        // Order is lower left, lower right, upper right, upper left
        wxSize half_size = aPad->GetSize()/2;
        wxSize trap_delta = aPad->GetDelta()/2;

        coord[0] = wxPoint( -half_size.x - trap_delta.y,  half_size.y + trap_delta.x );
        coord[1] = wxPoint( half_size.x + trap_delta.y,  half_size.y - trap_delta.x );
        coord[2] = wxPoint( half_size.x - trap_delta.y, -half_size.y + trap_delta.x );
        coord[3] = wxPoint( -half_size.x + trap_delta.y, -half_size.y - trap_delta.x );

        m_plotter->FlashPadTrapez( shape_pos, coord, aPad->GetOrientation(), aPlotMode,
                                   &gbr_metadata );
    }
        break;

    case PAD_SHAPE_CHAMFERED_RECT:
        if( m_plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
        {
            static_cast<GERBER_PLOTTER*>( m_plotter )->FlashPadChamferRoundRect(
                                    shape_pos, aPad->GetSize(),
                                    aPad->GetRoundRectCornerRadius(),
                                    aPad->GetChamferRectRatio(),
                                    aPad->GetChamferPositions(),
                                    aPad->GetOrientation(), aPlotMode, &gbr_metadata );
            break;
        }
        KI_FALLTHROUGH;

    default:
    case PAD_SHAPE_CUSTOM:
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
    const FP_TEXT* textItem = &aFootprint->Reference();
    LAYER_NUM textLayer = textItem->GetLayer();

    // Reference and value are specfic items, not in graphic items list
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
        textItem = dyn_cast<const FP_TEXT*>( item );

        if( !textItem )
            continue;

        if( !textItem->IsVisible() )
            continue;

        textLayer = textItem->GetLayer();

        if( textLayer == Edge_Cuts || textLayer >= PCB_LAYER_ID_COUNT )
            continue;

        if( !m_layerMask[textLayer] )
            continue;

        if( textItem->GetText() == wxT( "${REFERENCE}" ) && !GetPlotReference() )
            continue;

        if( textItem->GetText() == wxT( "${VALUE}" ) && !GetPlotValue() )
            continue;

        PlotFootprintTextItem( textItem, getColor( textLayer ) );
    }
}


// plot items like text and graphics, but not tracks and footprints
void BRDITEMS_PLOTTER::PlotBoardGraphicItems()
{
    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        switch( item->Type() )
        {
        case PCB_SHAPE_T:
            PlotPcbShape( (PCB_SHAPE*) item );
            break;

        case PCB_TEXT_T:
            if( item->GetLayer() != Edge_Cuts )
                PlotPcbText( (PCB_TEXT*) item );

            break;

        case PCB_DIM_ALIGNED_T:
        case PCB_DIM_CENTER_T:
        case PCB_DIM_ORTHOGONAL_T:
        case PCB_DIM_LEADER_T:
            if( item->GetLayer() != Edge_Cuts )
                PlotDimension( (DIMENSION_BASE*) item );

            break;

        case PCB_TARGET_T:
            PlotPcbTarget( (PCB_TARGET*) item );
            break;

        default:
            break;
        }
    }
}

void BRDITEMS_PLOTTER::PlotFootprintTextItem( const FP_TEXT* aTextMod, COLOR4D aColor )
{
    if( aColor == COLOR4D::WHITE )
        aColor = COLOR4D( LIGHTGRAY );

    m_plotter->SetColor( aColor );

    // calculate some text parameters :
    wxSize  size      = aTextMod->GetTextSize();
    wxPoint pos       = aTextMod->GetTextPos();
    double  orient    = aTextMod->GetDrawRotation();
    int     thickness = aTextMod->GetEffectiveTextPenWidth();

    if( aTextMod->IsMirrored() )
        size.x = -size.x;  // Text is mirrored

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = true;

    GBR_METADATA gbr_metadata;

    if( IsCopperLayer( aTextMod->GetLayer() ) )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    const FOOTPRINT* parent = static_cast<const FOOTPRINT*> ( aTextMod->GetParent() );
    gbr_metadata.SetCmpReference( parent->GetReference() );

    m_plotter->SetCurrentLineWidth( thickness );

    m_plotter->Text( pos, aColor, aTextMod->GetShownText(), orient, size,
                     aTextMod->GetHorizJustify(), aTextMod->GetVertJustify(), thickness,
                     aTextMod->IsItalic(), allow_bold, false, &gbr_metadata );
}


void BRDITEMS_PLOTTER::PlotDimension( const DIMENSION_BASE* aDim )
{
    if( !m_layerMask[aDim->GetLayer()] )
        return;

    PCB_SHAPE draw;

    draw.SetWidth( aDim->GetLineThickness() );
    draw.SetLayer( aDim->GetLayer() );

    COLOR4D color = ColorSettings()->GetColor( aDim->GetLayer() );

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( color != WHITE ? color : LIGHTGRAY);

    PlotPcbText( &aDim->Text() );

    for( const std::shared_ptr<SHAPE>& shape : aDim->GetShapes() )
    {
        switch( shape->Type() )
        {
        case SH_SEGMENT:
        {
            const SEG& seg = static_cast<const SHAPE_SEGMENT*>( shape.get() )->GetSeg();

            draw.SetShape( PCB_SHAPE_TYPE::SEGMENT );
            draw.SetStart( wxPoint( seg.A ) );
            draw.SetEnd( wxPoint( seg.B ) );

            PlotPcbShape( &draw );
            break;
        }

        case SH_CIRCLE:
        {
            wxPoint start( shape->Centre() );
            int radius = static_cast<const SHAPE_CIRCLE*>( shape.get() )->GetRadius();

            draw.SetShape( PCB_SHAPE_TYPE::CIRCLE );
            draw.SetFilled( false );
            draw.SetStart( start );
            draw.SetEnd( wxPoint( start.x + radius, start.y ) );

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

    draw.SetShape( PCB_SHAPE_TYPE::CIRCLE );
    draw.SetFilled( false );
    draw.SetWidth( aMire->GetWidth() );
    draw.SetLayer( aMire->GetLayer() );
    draw.SetStart( aMire->GetPosition() );
    radius = aMire->GetSize() / 3;

    if( aMire->GetShape() )   // shape X
        radius = aMire->GetSize() / 2;

    // Draw the circle
    draw.SetEnd( wxPoint( draw.GetStart().x + radius, draw.GetStart().y ) );

    PlotPcbShape( &draw );

    draw.SetShape( PCB_SHAPE_TYPE::SEGMENT );

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

    wxPoint mirePos( aMire->GetPosition() );

    // Draw the X or + shape:
    draw.SetStart( wxPoint( mirePos.x - dx1, mirePos.y - dy1 ) );
    draw.SetEnd(   wxPoint( mirePos.x + dx1, mirePos.y + dy1 ) );
    PlotPcbShape( &draw );

    draw.SetStart( wxPoint( mirePos.x - dx2, mirePos.y - dy2 ) );
    draw.SetEnd(   wxPoint( mirePos.x + dx2, mirePos.y + dy2 ) );
    PlotPcbShape( &draw );
}


// Plot footprints graphic items (outlines)
void BRDITEMS_PLOTTER::PlotFootprintGraphicItems( const FOOTPRINT* aFootprint )
{
    for( const BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        const FP_SHAPE* shape = dynamic_cast<const FP_SHAPE*>( item );

        if( shape && m_layerMask[ shape->GetLayer() ] )
            PlotFootprintGraphicItem( shape );
    }
}


//* Plot a graphic item (outline) relative to a footprint
void BRDITEMS_PLOTTER::PlotFootprintGraphicItem( const FP_SHAPE* aShape )
{
    if( aShape->Type() != PCB_FP_SHAPE_T )
        return;

    m_plotter->SetColor( getColor( aShape->GetLayer() ) );

    bool    sketch = GetPlotMode() == SKETCH;
    int     thickness = aShape->GetWidth();
    wxPoint pos( aShape->GetStart() );
    wxPoint end( aShape->GetEnd() );

    GBR_METADATA gbr_metadata;
    gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    const FOOTPRINT* parent = static_cast<const FOOTPRINT*> ( aShape->GetParent() );
    gbr_metadata.SetCmpReference( parent->GetReference() );

    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();

    if( aShape->GetLayer() == Edge_Cuts )   // happens also when plotting copper layers
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_EDGECUT );
    }
    else if( isOnCopperLayer )  // only for items not on Edge_Cuts.
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_ETCHEDCMP );
        gbr_metadata.SetCopper( true );
    }

    int     radius;             // Circle/arc radius.

    switch( aShape->GetShape() )
    {
    case PCB_SHAPE_TYPE::SEGMENT:
        m_plotter->ThickSegment( pos, end, thickness, GetPlotMode(), &gbr_metadata );
        break;

    case PCB_SHAPE_TYPE::RECT:
    {
        std::vector<wxPoint> pts = aShape->GetRectCorners();

        if( sketch || thickness > 0 )
        {
            m_plotter->ThickSegment( pts[0], pts[1], thickness, GetPlotMode(), &gbr_metadata );
            m_plotter->ThickSegment( pts[1], pts[2], thickness, GetPlotMode(), &gbr_metadata );
            m_plotter->ThickSegment( pts[2], pts[3], thickness, GetPlotMode(), &gbr_metadata );
            m_plotter->ThickSegment( pts[3], pts[0], thickness, GetPlotMode(), &gbr_metadata );
        }

        if( !sketch && aShape->IsFilled() )
        {
            SHAPE_LINE_CHAIN poly;

            for( const wxPoint& pt : pts )
                poly.Append( pt );

            m_plotter->PlotPoly( poly, FILL_TYPE::FILLED_SHAPE, -1, &gbr_metadata );
        }
    }
        break;

    case PCB_SHAPE_TYPE::CIRCLE:
        radius = KiROUND( GetLineLength( end, pos ) );

        if( aShape->IsFilled() )
            m_plotter->FilledCircle( pos, radius * 2 + thickness, GetPlotMode(), &gbr_metadata );
        else
            m_plotter->ThickCircle( pos, radius * 2, thickness, GetPlotMode(), &gbr_metadata );

        break;

    case PCB_SHAPE_TYPE::ARC:
    {
        radius = KiROUND( GetLineLength( end, pos ) );
        double startAngle  = ArcTangente( end.y - pos.y, end.x - pos.x );
        double endAngle = startAngle + aShape->GetAngle();

        // when startAngle == endAngle ThickArc() doesn't know whether it's 0 deg and 360 deg
        if( std::abs( aShape->GetAngle() ) == 3600.0 )
        {
            m_plotter->ThickCircle( pos, radius * 2, thickness, GetPlotMode(), &gbr_metadata );
        }
        else
        {
            m_plotter->ThickArc( pos, -endAngle, -startAngle, radius, thickness, GetPlotMode(),
                                 &gbr_metadata );
        }
    }
        break;

    case PCB_SHAPE_TYPE::POLYGON:
        if( aShape->IsPolyShapeValid() )
        {
            const std::vector<wxPoint> &polyPoints = aShape->BuildPolyPointsList();

            // We must compute board coordinates from m_PolyList which are relative to the parent
            // position at orientation 0
            const FOOTPRINT *parentFootprint = aShape->GetParentFootprint();

            std::vector<wxPoint> cornerList;

            cornerList.reserve( polyPoints.size() );

            for( wxPoint corner : polyPoints )
            {
                if( parentFootprint )
                {
                    RotatePoint( &corner, parentFootprint->GetOrientation() );
                    corner += parentFootprint->GetPosition();
                }

                cornerList.push_back( corner );
            }

            if( sketch || thickness > 0 )
            {
                for( size_t i = 1; i < cornerList.size(); i++ )
                {
                    m_plotter->ThickSegment( cornerList[i - 1], cornerList[i], thickness,
                                             GetPlotMode(), &gbr_metadata );
                }

                m_plotter->ThickSegment( cornerList.back(), cornerList.front(), thickness,
                                         GetPlotMode(), &gbr_metadata );

            }

            if( !sketch && aShape->IsFilled() )
            {
                // This must be simplified and fractured to prevent overlapping polygons
                // from generating invalid Gerber files

                SHAPE_LINE_CHAIN line( cornerList );
                SHAPE_POLY_SET tmpPoly;

                line.SetClosed( true );
                tmpPoly.AddOutline( line );
                tmpPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

                for( int jj = 0; jj < tmpPoly.OutlineCount(); ++jj )
                {
                    SHAPE_LINE_CHAIN &poly = tmpPoly.Outline( jj );
                    m_plotter->PlotPoly( poly, FILL_TYPE::FILLED_SHAPE, thickness, &gbr_metadata );
                }
            }
        }
        break;

    case PCB_SHAPE_TYPE::CURVE:
        m_plotter->BezierCurve( aShape->GetStart(), aShape->GetBezControl1(),
                                aShape->GetBezControl2(), aShape->GetEnd(), 0, thickness );
        break;

    default:
        wxASSERT_MSG( false, "Unhandled FP_SHAPE shape" );
        break;
    }
}


// Plot a PCB Text, i.e. a text found on a copper or technical layer
void BRDITEMS_PLOTTER::PlotPcbText( const PCB_TEXT* aText )
{
    wxString shownText( aText->GetShownText() );

    if( shownText.IsEmpty() )
        return;

    if( !m_layerMask[aText->GetLayer()] )
        return;

    GBR_METADATA gbr_metadata;

    if( IsCopperLayer( aText->GetLayer() ) )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    COLOR4D color = getColor( aText->GetLayer() );
    m_plotter->SetColor( color );

    wxSize  size      = aText->GetTextSize();
    wxPoint pos       = aText->GetTextPos();
    double  orient    = aText->GetTextAngle();
    int     thickness = aText->GetEffectiveTextPenWidth();

    if( aText->IsMirrored() )
        size.x = -size.x;

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = true;

    m_plotter->SetCurrentLineWidth( thickness );

    if( aText->IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString strings_list;
        wxStringSplit( shownText, strings_list, '\n' );
        positions.reserve(  strings_list.Count() );

        aText->GetLinePositions( positions, strings_list.Count() );

        for( unsigned ii = 0; ii < strings_list.Count(); ii++ )
        {
            wxString& txt =  strings_list.Item( ii );
            m_plotter->Text( positions[ii], color, txt, orient, size, aText->GetHorizJustify(),
                             aText->GetVertJustify(), thickness, aText->IsItalic(),
                             allow_bold, false, &gbr_metadata );
        }
    }
    else
    {
        m_plotter->Text( pos, color, shownText, orient, size, aText->GetHorizJustify(),
                         aText->GetVertJustify(), thickness, aText->IsItalic(), allow_bold,
                         false, &gbr_metadata );
    }
}


void BRDITEMS_PLOTTER::PlotFilledAreas( const ZONE* aZone, const SHAPE_POLY_SET& polysList )
{
    if( polysList.IsEmpty() )
        return;

    GBR_METADATA gbr_metadata;

    bool isOnCopperLayer = aZone->IsOnCopperLayer();

    if( isOnCopperLayer )
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

    // We need a buffer to store corners coordinates:
    std::vector< wxPoint > cornerList;

    m_plotter->SetColor( getColor( aZone->GetLayer() ) );

    m_plotter->StartBlock( nullptr );    // Clean current object attributes

    /* Plot all filled areas: filled areas have a filled area and a thick
     * outline (depending on the fill area option we must plot the filled area itself
     * and plot the thick outline itself, if the thickness has meaning (at least is > 1)
     *
     * in non filled mode the outline is plotted, but not the filling items
     */
    int outline_thickness = aZone->GetFilledPolysUseThickness() ? aZone->GetMinThickness() : 0;

    for( int idx = 0; idx < polysList.OutlineCount(); ++idx )
    {
        const SHAPE_LINE_CHAIN& outline = polysList.Outline( idx );

        cornerList.clear();
        cornerList.reserve( outline.PointCount() );

        for( int ic = 0; ic < outline.PointCount(); ++ic )
        {
            cornerList.emplace_back( wxPoint( outline.CPoint( ic ) ) );
        }

        if( cornerList.size() )   // Plot the current filled area outline
        {
            // First, close the outline
            if( cornerList[0] != cornerList[cornerList.size() - 1] )
                cornerList.push_back( cornerList[0] );

            // Plot the current filled area (as region for Gerber plotter
            // to manage attributes) and its outline for thick outline
            if( GetPlotMode() == FILLED )
            {
                if( m_plotter->GetPlotterType() == PLOT_FORMAT::GERBER )
                {
                    if( outline_thickness > 0 )
                    {
                        m_plotter->PlotPoly( cornerList, FILL_TYPE::NO_FILL, outline_thickness,
                                             &gbr_metadata );
                    }

                    static_cast<GERBER_PLOTTER*>( m_plotter )->PlotGerberRegion( cornerList,
                                                                                 &gbr_metadata );
                }
                else
                {
                    m_plotter->PlotPoly( cornerList, FILL_TYPE::FILLED_SHAPE, outline_thickness,
                                         &gbr_metadata );
                }
            }
            else
            {
                if( outline_thickness )
                {
                    for( unsigned jj = 1; jj < cornerList.size(); jj++ )
                    {
                        m_plotter->ThickSegment( cornerList[jj -1], cornerList[jj],
                                                 outline_thickness, GetPlotMode(), &gbr_metadata );
                    }
                }

                m_plotter->SetCurrentLineWidth( -1 );
            }
        }
    }

    m_plotter->EndBlock( nullptr );    // Clear object attributes
}


/* Plot items type PCB_SHAPE on layers allowed by aLayerMask
 */
void BRDITEMS_PLOTTER::PlotPcbShape( const PCB_SHAPE* aShape )
{
    if( !m_layerMask[aShape->GetLayer()] )
        return;

    int     radius = 0;
    double  StAngle = 0, EndAngle = 0;
    bool    sketch = GetPlotMode() == SKETCH;
    int     thickness = aShape->GetWidth();

    m_plotter->SetColor( getColor( aShape->GetLayer() ) );

    wxPoint start( aShape->GetStart() );
    wxPoint end( aShape->GetEnd() );

    GBR_METADATA gbr_metadata;

    if( aShape->GetLayer() == Edge_Cuts )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_EDGECUT );

    if( IsCopperLayer( aShape->GetLayer() ) )
        // Graphic items (PCB_SHAPE, TEXT) having no net have the NonConductor attribute
        // Graphic items having a net have the Conductor attribute, but are not (yet?)
        // supported in Pcbnew
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    switch( aShape->GetShape() )
    {
    case PCB_SHAPE_TYPE::SEGMENT:
        m_plotter->ThickSegment( start, end, thickness, GetPlotMode(), &gbr_metadata );
        break;

    case PCB_SHAPE_TYPE::CIRCLE:
        radius = KiROUND( GetLineLength( end, start ) );

        if( aShape->IsFilled() )
            m_plotter->FilledCircle( start, radius * 2 + thickness, GetPlotMode(), &gbr_metadata );
        else
            m_plotter->ThickCircle( start, radius * 2, thickness, GetPlotMode(), &gbr_metadata );

        break;

    case PCB_SHAPE_TYPE::ARC:
        radius = KiROUND( GetLineLength( end, start ) );
        StAngle  = ArcTangente( end.y - start.y, end.x - start.x );
        EndAngle = StAngle + aShape->GetAngle();

        // when startAngle == endAngle ThickArc() doesn't know whether it's 0 deg and 360 deg
        if( std::abs( aShape->GetAngle() ) == 3600.0 )
        {
            m_plotter->ThickCircle( start, radius * 2, thickness, GetPlotMode(), &gbr_metadata );
        }
        else
        {
            m_plotter->ThickArc( start, -EndAngle, -StAngle, radius, thickness, GetPlotMode(),
                                 &gbr_metadata );
        }
        break;

    case PCB_SHAPE_TYPE::CURVE:
        m_plotter->BezierCurve( aShape->GetStart(), aShape->GetBezControl1(),
                                aShape->GetBezControl2(), aShape->GetEnd(), 0, thickness );
        break;

    case PCB_SHAPE_TYPE::POLYGON:
        if( aShape->IsPolyShapeValid() )
        {
            if( sketch || thickness > 0 )
            {
                for( auto it = aShape->GetPolyShape().CIterateSegments( 0 ); it; it++ )
                {
                    auto seg = it.Get();
                    m_plotter->ThickSegment( wxPoint( seg.A ), wxPoint( seg.B ),
                                             thickness, GetPlotMode(), &gbr_metadata );
                }
            }

            if( !sketch && aShape->IsFilled() )
            {
                m_plotter->SetCurrentLineWidth( thickness, &gbr_metadata );
                // Draw the polygon: only one polygon is expected
                // However we provide a multi polygon shape drawing
                // ( for the future or to show a non expected shape )
                // This must be simplified and fractured to prevent overlapping polygons
                // from generating invalid Gerber files
                auto tmpPoly = SHAPE_POLY_SET( aShape->GetPolyShape() );
                tmpPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

                for( int jj = 0; jj < tmpPoly.OutlineCount(); ++jj )
                {
                    SHAPE_LINE_CHAIN& poly = tmpPoly.Outline( jj );
                    m_plotter->PlotPoly( poly, FILL_TYPE::FILLED_SHAPE, thickness, &gbr_metadata );
                }
            }
        }
        break;

    case PCB_SHAPE_TYPE::RECT:
    {
        std::vector<wxPoint> pts = aShape->GetRectCorners();

        if( sketch || thickness > 0 )
        {
            m_plotter->ThickSegment( pts[0], pts[1], thickness, GetPlotMode(), &gbr_metadata );
            m_plotter->ThickSegment( pts[1], pts[2], thickness, GetPlotMode(), &gbr_metadata );
            m_plotter->ThickSegment( pts[2], pts[3], thickness, GetPlotMode(), &gbr_metadata );
            m_plotter->ThickSegment( pts[3], pts[0], thickness, GetPlotMode(), &gbr_metadata );
        }

        if( !sketch && aShape->IsFilled() )
        {
            SHAPE_LINE_CHAIN poly;

            for( const wxPoint& pt : pts )
                poly.Append( pt );

            m_plotter->PlotPoly( poly, FILL_TYPE::FILLED_SHAPE, -1, &gbr_metadata );
        }
    }
        break;

    default:
        wxASSERT_MSG( false, "Unhandled PCB_SHAPE shape" );
        m_plotter->ThickSegment( start, end, thickness, GetPlotMode(), &gbr_metadata );
    }
}


/** Helper function to plot a single drill mark. It compensate and clamp
 *   the drill mark size depending on the current plot options
 */
void BRDITEMS_PLOTTER::plotOneDrillMark( PAD_DRILL_SHAPE_T aDrillShape, const wxPoint &aDrillPos,
                                         wxSize aDrillSize, const wxSize &aPadSize,
                                         double aOrientation, int aSmallDrill )
{
    // Small drill marks have no significance when applied to slots
    if( aSmallDrill && aDrillShape == PAD_DRILL_SHAPE_CIRCLE )
        aDrillSize.x = std::min( aSmallDrill, aDrillSize.x );

    // Round holes only have x diameter, slots have both
    aDrillSize.x -= getFineWidthAdj();
    aDrillSize.x = Clamp( 1, aDrillSize.x, aPadSize.x - 1 );

    if( aDrillShape == PAD_DRILL_SHAPE_OBLONG )
    {
        aDrillSize.y -= getFineWidthAdj();
        aDrillSize.y = Clamp( 1, aDrillSize.y, aPadSize.y - 1 );
        m_plotter->FlashPadOval( aDrillPos, aDrillSize, aOrientation, GetPlotMode(), NULL );
    }
    else
    {
        m_plotter->FlashPadCircle( aDrillPos, aDrillSize.x, GetPlotMode(), NULL );
    }
}


void BRDITEMS_PLOTTER::PlotDrillMarks()
{
    /* If small drills marks were requested prepare a clamp value to pass
       to the helper function */
    int smallDrill = GetDrillMarksType() == PCB_PLOT_PARAMS::SMALL_DRILL_SHAPE
                    ? Millimeter2iu( ADVANCED_CFG::GetCfg().m_SmallDrillMarkSize ) : 0;

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

    for( TRACK* tracks : m_board->Tracks() )
    {
        const VIA* via = dyn_cast<const VIA*>( tracks );

        if( via )
        {
            plotOneDrillMark( PAD_DRILL_SHAPE_CIRCLE, via->GetStart(),
                              wxSize( via->GetDrillValue(), 0 ),
                              wxSize( via->GetWidth(), 0 ), 0, smallDrill );
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
