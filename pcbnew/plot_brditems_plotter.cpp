/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_struct.h>
#include <common.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/seg.h>                     // for SEG
#include <geometry/shape_line_chain.h>        // for SHAPE_LINE_CHAIN
#include <geometry/shape_poly_set.h>          // for SHAPE_POLY_SET, SHAPE_P...
#include <math/util.h>                        // for KiROUND, Clamp
#include <math/vector2d.h>                    // for VECTOR2I
#include <plotter.h>
#include <trigo.h>

#include <board_design_settings.h>            // for BOARD_DESIGN_SETTINGS
#include <colors.h>                           // for LIGHTGRAY, WHITE
#include <core/typeinfo.h>                    // for dyn_cast, PCB_DIMENSION_T
#include <eda_text.h>                         // for FILLED, EDA_DRAW_MODE_T
#include <gal/color4d.h>                      // for COLOR4D, operator!=
#include <gbr_metadata.h>
#include <gbr_netlist_metadata.h>             // for GBR_NETLIST_METADATA
#include <layers_id_colors_and_visibility.h>  // for LSET, IsCopperLayer
#include <pad_shapes.h>                       // for PAD_ATTRIB_HOLE_NOT_PLATED
#include <pcbplot.h>
#include <pcb_plot_params.h>                  // for PCB_PLOT_PARAMS, PCB_PL...

#include <class_board.h>
#include <class_board_item.h>                 // for BOARD_ITEM, S_CIRCLE
#include <class_dimension.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <class_module.h>
#include <class_text_mod.h>                   // for TEXTE_MODULE
#include <class_track.h>
#include <class_pad.h>                        // for D_PAD
#include <class_pcb_target.h>
#include <class_pcb_text.h>
#include <class_zone.h>

#include <wx/debug.h>                         // for wxASSERT_MSG
#include <wx/wx.h>                            // for wxPoint, wxSize, wxArra...


/* class BRDITEMS_PLOTTER is a helper class to plot board items
 * and a group of board items
 */

COLOR4D BRDITEMS_PLOTTER::getColor( LAYER_NUM aLayer )
{
    COLOR4D color = ColorSettings()->GetColor( aLayer );

    // A hack to avoid plotting a white item in white color, expecting the paper
    // is also white: use a non white color:
    if( color == COLOR4D::WHITE )
        color = COLOR4D( LIGHTGRAY );

    return color;
}


void BRDITEMS_PLOTTER::PlotPad( D_PAD* aPad, COLOR4D aColor, EDA_DRAW_MODE_T aPlotMode )
{
    wxPoint shape_pos = aPad->ShapePos();
    GBR_METADATA gbr_metadata;

    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();
    bool isOnExternalCopperLayer = ( m_layerMask & LSET::ExternalCuMask() ).any();
    bool isPadOnBoardTechLayers = ( aPad->GetLayerSet() & LSET::AllBoardTechMask() ).any();

    gbr_metadata.SetCmpReference( aPad->GetParent()->GetReference() );

    if( isOnCopperLayer )
    {
        gbr_metadata.SetNetAttribType( GBR_NETINFO_ALL );
        gbr_metadata.SetCopper( true );

        if( isOnExternalCopperLayer )
        {
            const bool useUTF8 = false;
            const bool useQuoting = false;
            gbr_metadata.SetPadName( aPad->GetName(), useUTF8, useQuoting );

            if( !aPad->GetName().IsEmpty() )
                gbr_metadata.SetPadPinFunction( aPad->GetPinFunction(), useUTF8, useQuoting );
        }

        gbr_metadata.SetNetName( aPad->GetNetname() );

        // Some pads are mechanical pads ( through hole or smd )
        // when this is the case, they have no pad name and/or are not plated.
        // In this case gerber files have slightly different attributes.
        if( aPad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED || aPad->GetName().IsEmpty() )
            gbr_metadata.m_NetlistMetadata.m_NotInNet = true;

        if( !isOnExternalCopperLayer || !isPadOnBoardTechLayers )
        {
            // On internal layers one cannot use the GBR_NETLIST_METADATA::GBR_INFO_FLASHED_PAD
            // attribute when the component is on an external layer (most of the case)
            // Also, if a SMD pad is not on a tech layer (masks) use also net+cmp attribute, because
            // it is not really a pad (can be a "pad", actually a node in a virtual component)
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET |
                                           GBR_NETLIST_METADATA::GBR_NETINFO_CMP );

            if( !isPadOnBoardTechLayers )
                // such a pad is not soldered and is not a connecting point.
                // Just set aperture attribute as conductor
                // If it is a through hole pad, it will be adjusted later
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );

            switch( aPad->GetAttribute() )
            {
            case PAD_ATTRIB_HOLE_NOT_PLATED:    // Mechanical pad through hole
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
                break;

            case PAD_ATTRIB_STANDARD :  // Pad through hole, a hole is also expected
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_VIAPAD );
                break;

            default:
                break;
            }
        }
        else    // Some attributes are reserved to the external copper layers
        {
            switch( aPad->GetAttribute() )
            {
            case PAD_ATTRIB_HOLE_NOT_PLATED:    // Mechanical pad through hole
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_WASHERPAD );
                break;

            case PAD_ATTRIB_STANDARD :  // Pad through hole, a hole is also expected
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_COMPONENTPAD );
                break;

            case PAD_ATTRIB_CONN:      // Connector pads have no solder paste.
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONNECTORPAD );
                break;

            case PAD_ATTRIB_SMD:       // SMD pads (One external copper layer only) with solder paste
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_SMDPAD_CUDEF );
                break;
            }

            // Fabrication properties can have specific GBR_APERTURE_METADATA options:
            switch( aPad->GetProperty() )
            {
            case PAD_PROP_BGA:
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_BGAPAD_CUDEF );
                break;

            case PAD_PROP_FIDUCIAL_GLBL:
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_GLBL );
                break;

            case PAD_PROP_FIDUCIAL_LOCAL:
                gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_FIDUCIAL_LOCAL );
                break;

            case PAD_PROP_TESTPOINT:    // Only on outer layers
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
        }

        if( aPad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
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
        m_plotter->FlashPadOval( shape_pos, aPad->GetSize(),
                                 aPad->GetOrientation(), aPlotMode, &gbr_metadata );
        break;

    case PAD_SHAPE_TRAPEZOID:
        {
        wxPoint coord[4];
        aPad->BuildPadPolygon( coord, wxSize(0,0), 0 );
        m_plotter->FlashPadTrapez( shape_pos, coord,
                                   aPad->GetOrientation(), aPlotMode, &gbr_metadata );
        }
        break;

    case PAD_SHAPE_ROUNDRECT:
        m_plotter->FlashPadRoundRect( shape_pos, aPad->GetSize(), aPad->GetRoundRectCornerRadius(),
                                      aPad->GetOrientation(), aPlotMode, &gbr_metadata );
        break;

    case PAD_SHAPE_CHAMFERED_RECT:
        {
        SHAPE_POLY_SET polygons;
        const int corner_radius = aPad->GetRoundRectCornerRadius( aPad->GetSize() );
        TransformRoundChamferedRectToPolygon( polygons, shape_pos, aPad->GetSize(),
                aPad->GetOrientation(), corner_radius, aPad->GetChamferRectRatio(),
                aPad->GetChamferPositions(), m_board->GetDesignSettings().m_MaxError );

        if( polygons.OutlineCount() == 0 )
            break;

        int min_dim = std::min( aPad->GetSize().x, aPad->GetSize().y ) /2;
        m_plotter->FlashPadCustom( shape_pos,wxSize( min_dim, min_dim ), &polygons, aPlotMode, &gbr_metadata );
        }
        break;

    case PAD_SHAPE_CUSTOM:
        {
        SHAPE_POLY_SET polygons;
        aPad->MergePrimitivesAsPolygon( &polygons );

        if( polygons.OutlineCount() == 0 )
            break;

        aPad->CustomShapeAsPolygonToBoardPosition( &polygons, shape_pos, aPad->GetOrientation() );
        m_plotter->FlashPadCustom( shape_pos, aPad->GetSize(), &polygons, aPlotMode, &gbr_metadata );
        }
        break;

    case PAD_SHAPE_RECT:
    default:
        m_plotter->FlashPadRect( shape_pos, aPad->GetSize(),
                                 aPad->GetOrientation(), aPlotMode, &gbr_metadata );
        break;
    }
}


void BRDITEMS_PLOTTER::PlotFootprintTextItems( MODULE* aModule )
{
    TEXTE_MODULE* textModule = &aModule->Reference();
    LAYER_NUM     textLayer = textModule->GetLayer();

    // Reference and value are specfic items, not in graphic items list
    if( GetPlotReference() && m_layerMask[textLayer]
        && ( textModule->IsVisible() || GetPlotInvisibleText() ) )
    {
        PlotFootprintTextItem( textModule, getColor( textLayer ));
    }

    textModule = &aModule->Value();
    textLayer = textModule->GetLayer();

    if( GetPlotValue() && m_layerMask[textLayer]
        && ( textModule->IsVisible() || GetPlotInvisibleText() ) )
    {
        PlotFootprintTextItem( textModule, getColor( textLayer ));
    }

    for( auto item : aModule->GraphicalItems() )
    {
        textModule = dyn_cast<TEXTE_MODULE*>( item );

        if( !textModule )
            continue;

        if( !textModule->IsVisible() )
            continue;

        textLayer = textModule->GetLayer();

        if( textLayer >= PCB_LAYER_ID_COUNT )
            return;

        if( !m_layerMask[textLayer] )
            continue;

        if( textModule->GetText() == wxT( "${REFERENCE}" ) && !GetPlotReference() )
            continue;

        if( textModule->GetText() == wxT( "${VALUE}" ) && !GetPlotValue() )
            continue;

        PlotFootprintTextItem( textModule, getColor( textLayer ));
    }
}


// plot items like text and graphics, but not tracks and module
void BRDITEMS_PLOTTER::PlotBoardGraphicItems()
{
    for( auto item : m_board->Drawings() )
    {
        switch( item->Type() )
        {
        case PCB_LINE_T:      PlotDrawSegment( (DRAWSEGMENT*) item); break;
        case PCB_TEXT_T:      PlotTextePcb( (TEXTE_PCB*) item );     break;
        case PCB_DIMENSION_T: PlotDimension( (DIMENSION*) item );    break;
        case PCB_TARGET_T:    PlotPcbTarget( (PCB_TARGET*) item );   break;
        default:              break;
        }
    }
}

void BRDITEMS_PLOTTER::PlotFootprintTextItem( TEXTE_MODULE* aTextMod, COLOR4D aColor )
{
    if( aColor == COLOR4D::WHITE )
        aColor = COLOR4D( LIGHTGRAY );

    m_plotter->SetColor( aColor );

    // calculate some text parameters :
    wxSize  size = aTextMod->GetTextSize();
    wxPoint pos = aTextMod->GetTextPos();
    double  orient = aTextMod->GetDrawRotation();
    int     thickness = aTextMod->GetEffectiveTextPenWidth();

    if( aTextMod->IsMirrored() )
        size.x = -size.x;  // Text is mirrored

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = true;

    GBR_METADATA gbr_metadata;
    gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    MODULE* parent = static_cast<MODULE*> ( aTextMod->GetParent() );
    gbr_metadata.SetCmpReference( parent->GetReference() );

    m_plotter->SetCurrentLineWidth( thickness );

    m_plotter->Text( pos, aColor, aTextMod->GetShownText(), orient, size,
                     aTextMod->GetHorizJustify(), aTextMod->GetVertJustify(), thickness,
                     aTextMod->IsItalic(), allow_bold, false, &gbr_metadata );
}


void BRDITEMS_PLOTTER::PlotDimension( DIMENSION* aDim )
{
    if( !m_layerMask[aDim->GetLayer()] )
        return;

    DRAWSEGMENT draw;

    draw.SetWidth( aDim->GetWidth() );
    draw.SetLayer( aDim->GetLayer() );

    COLOR4D color = ColorSettings()->GetColor( aDim->GetLayer() );

    // Set plot color (change WHITE to LIGHTGRAY because
    // the white items are not seen on a white paper or screen
    m_plotter->SetColor( color != WHITE ? color : LIGHTGRAY);

    PlotTextePcb( &aDim->Text() );

    draw.SetStart( aDim->m_crossBarO );
    draw.SetEnd( aDim->m_crossBarF );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_featureLineGO);
    draw.SetEnd( aDim->m_featureLineGF );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_featureLineDO );
    draw.SetEnd( aDim->m_featureLineDF );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarF );
    draw.SetEnd( aDim->m_arrowD1F );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarF );
    draw.SetEnd( aDim->m_arrowD2F );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarO );
    draw.SetEnd( aDim->m_arrowG1F );
    PlotDrawSegment( &draw );

    draw.SetStart( aDim->m_crossBarO );
    draw.SetEnd( aDim->m_arrowG2F );
    PlotDrawSegment( &draw );
}


void BRDITEMS_PLOTTER::PlotPcbTarget( PCB_TARGET* aMire )
{
    int     dx1, dx2, dy1, dy2, radius;

    if( !m_layerMask[aMire->GetLayer()] )
        return;

    m_plotter->SetColor( getColor( aMire->GetLayer() ) );

    DRAWSEGMENT  draw;

    draw.SetShape( S_CIRCLE );
    draw.SetWidth( aMire->GetWidth() );
    draw.SetLayer( aMire->GetLayer() );
    draw.SetStart( aMire->GetPosition() );
    radius = aMire->GetSize() / 3;

    if( aMire->GetShape() )   // shape X
        radius = aMire->GetSize() / 2;

    // Draw the circle
    draw.SetEnd( wxPoint( draw.GetStart().x + radius, draw.GetStart().y ));

    PlotDrawSegment( &draw );

    draw.SetShape( S_SEGMENT );

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
    draw.SetStart( wxPoint( mirePos.x - dx1, mirePos.y - dy1 ));
    draw.SetEnd(   wxPoint( mirePos.x + dx1, mirePos.y + dy1 ));
    PlotDrawSegment( &draw );

    draw.SetStart( wxPoint( mirePos.x - dx2, mirePos.y - dy2 ));
    draw.SetEnd(   wxPoint( mirePos.x + dx2, mirePos.y + dy2 ));
    PlotDrawSegment( &draw );
}


// Plot footprints graphic items (outlines)
void BRDITEMS_PLOTTER::PlotFootprintGraphicItems( MODULE* aModule )
{
    for( BOARD_ITEM* item : aModule->GraphicalItems() )
    {
        EDGE_MODULE* edge = dynamic_cast<EDGE_MODULE*>( item );

        if( edge && m_layerMask[ edge->GetLayer() ] )
            PlotFootprintGraphicItem( edge );
    }
}


//* Plot a graphic item (outline) relative to a footprint
void BRDITEMS_PLOTTER::PlotFootprintGraphicItem( EDGE_MODULE* aEdge )
{
    if( aEdge->Type() != PCB_MODULE_EDGE_T )
        return;

    m_plotter->SetColor( getColor( aEdge->GetLayer() ) );

    int     thickness = aEdge->GetWidth();
    wxPoint pos( aEdge->GetStart() );
    wxPoint end( aEdge->GetEnd() );

    GBR_METADATA gbr_metadata;
    gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_CMP );
    MODULE* parent = static_cast<MODULE*> ( aEdge->GetParent() );
    gbr_metadata.SetCmpReference( parent->GetReference() );

    bool isOnCopperLayer = ( m_layerMask & LSET::AllCuMask() ).any();

    if( isOnCopperLayer )
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_ETCHEDCMP );
        gbr_metadata.SetCopper( true );
    }
    else if( aEdge->GetLayer() == Edge_Cuts )   // happens also when plotting copper layers
    {
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_EDGECUT );
    }

    int     radius;             // Circle/arc radius.

    switch( aEdge->GetShape() )
    {
    case S_SEGMENT:
        m_plotter->ThickSegment( pos, end, thickness, GetPlotMode(), &gbr_metadata );
        break;

    case S_CIRCLE:
        radius = KiROUND( GetLineLength( end, pos ) );
        m_plotter->ThickCircle( pos, radius * 2, thickness, GetPlotMode(), &gbr_metadata );
        break;

    case S_ARC:
    {
        radius = KiROUND( GetLineLength( end, pos ) );
        double startAngle  = ArcTangente( end.y - pos.y, end.x - pos.x );
        double endAngle = startAngle + aEdge->GetAngle();

        // when startAngle == endAngle ThickArc() doesn't know whether it's 0 deg and 360 deg
        if( std::abs( aEdge->GetAngle() ) == 3600.0 )
            m_plotter->ThickCircle( pos, radius * 2, thickness, GetPlotMode(), &gbr_metadata );
        else
            m_plotter->ThickArc( pos, -endAngle, -startAngle, radius, thickness, GetPlotMode(), &gbr_metadata );
    }
        break;

    case S_POLYGON:
        if( aEdge->IsPolyShapeValid() )
        {
            const std::vector<wxPoint> &polyPoints = aEdge->BuildPolyPointsList();

            // We must compute true coordinates from m_PolyList
            // which are relative to module position, orientation 0
            MODULE *module = aEdge->GetParentModule();

            std::vector<wxPoint> cornerList;

            cornerList.reserve( polyPoints.size() );

            for( wxPoint corner : polyPoints )
            {
                if( module )
                {
                    RotatePoint( &corner, module->GetOrientation() );
                    corner += module->GetPosition();
                }

                cornerList.push_back( corner );
            }

            if( !aEdge->IsPolygonFilled() )
            {
                for( size_t i = 1; i < cornerList.size(); i++ )
                {
                    m_plotter->ThickSegment( cornerList[i - 1], cornerList[i], thickness,
                            GetPlotMode(), &gbr_metadata );
                }

                m_plotter->ThickSegment( cornerList.back(), cornerList.front(), thickness,
                        GetPlotMode(), &gbr_metadata );

            }
            else
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
                    m_plotter->PlotPoly( poly, FILLED_SHAPE, thickness, &gbr_metadata );
                }
            }
        }
        break;

    case S_CURVE:
        m_plotter->BezierCurve( aEdge->GetStart(), aEdge->GetBezControl1(),
                                aEdge->GetBezControl2(), aEdge->GetEnd(),
                                0, thickness );
        break;

    default:
        wxASSERT_MSG( false, "Unhandled EDGE_MODULE shape" );
        break;
    }
}


// Plot a PCB Text, i.e. a text found on a copper or technical layer
void BRDITEMS_PLOTTER::PlotTextePcb( TEXTE_PCB* pt_texte )
{
    wxString shownText( pt_texte->GetShownText() );

    if( shownText.IsEmpty() )
        return;

    if( !m_layerMask[pt_texte->GetLayer()] )
        return;

    GBR_METADATA gbr_metadata;

    if( IsCopperLayer( pt_texte->GetLayer() ) )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    COLOR4D color = getColor( pt_texte->GetLayer() );
    m_plotter->SetColor( color );

    wxSize  size      = pt_texte->GetTextSize();
    wxPoint pos       = pt_texte->GetTextPos();
    double  orient    = pt_texte->GetTextAngle();
    int     thickness = pt_texte->GetEffectiveTextPenWidth();

    if( pt_texte->IsMirrored() )
        size.x = -size.x;

    // Non bold texts thickness is clamped at 1/6 char size by the low level draw function.
    // but in Pcbnew we do not manage bold texts and thickness up to 1/4 char size
    // (like bold text) and we manage the thickness.
    // So we set bold flag to true
    bool allow_bold = true;

    m_plotter->SetCurrentLineWidth( thickness );

    if( pt_texte->IsMultilineAllowed() )
    {
        std::vector<wxPoint> positions;
        wxArrayString strings_list;
        wxStringSplit( shownText, strings_list, '\n' );
        positions.reserve(  strings_list.Count() );

        pt_texte->GetLinePositions( positions, strings_list.Count());

        for( unsigned ii = 0; ii <  strings_list.Count(); ii++ )
        {
            wxString& txt =  strings_list.Item( ii );
            m_plotter->Text( positions[ii], color, txt, orient, size, pt_texte->GetHorizJustify(),
                             pt_texte->GetVertJustify(), thickness, pt_texte->IsItalic(),
                             allow_bold, false, &gbr_metadata );
        }
    }
    else
    {
        m_plotter->Text( pos, color, shownText, orient, size, pt_texte->GetHorizJustify(),
                         pt_texte->GetVertJustify(), thickness, pt_texte->IsItalic(), allow_bold,
                         false, &gbr_metadata );
    }
}


void BRDITEMS_PLOTTER::PlotFilledAreas( ZONE_CONTAINER* aZone, SHAPE_POLY_SET& polysList )
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
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );
        else
        {
            gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_CONDUCTOR );
            gbr_metadata.SetNetAttribType( GBR_NETLIST_METADATA::GBR_NETINFO_NET );
        }
    }

    // We need a buffer to store corners coordinates:
    std::vector< wxPoint > cornerList;

    m_plotter->SetColor( getColor( aZone->GetLayer() ) );

    /* Plot all filled areas: filled areas have a filled area and a thick
     * outline (depending on the fill area option we must plot the filled area itself
     * and plot the thick outline itself, if the thickness has meaning (at least is > 1)
     *
     * in non filled mode the outline is plotted, but not the filling items
     */
    int outline_thickness = aZone->GetFilledPolysUseThickness() ? aZone->GetMinThickness() : 0;

    for( int idx = 0; idx < polysList.OutlineCount(); ++idx )
    {
        SHAPE_LINE_CHAIN& outline = polysList.Outline( idx );

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

            // Plot the current filled area and its outline
            if( GetPlotMode() == FILLED )
            {
                m_plotter->PlotPoly( cornerList, FILLED_SHAPE, outline_thickness, &gbr_metadata );
            }
            else
            {
                if( outline_thickness )
                {
                    for( unsigned jj = 1; jj < cornerList.size(); jj++ )
                    {
                        m_plotter->ThickSegment( cornerList[jj -1], cornerList[jj],
                                                 outline_thickness,
                                                 GetPlotMode(), &gbr_metadata );
                    }
                }

                m_plotter->SetCurrentLineWidth( -1 );
            }
        }
    }
}


/* Plot items type DRAWSEGMENT on layers allowed by aLayerMask
 */
void BRDITEMS_PLOTTER::PlotDrawSegment( DRAWSEGMENT* aSeg )
{
    if( !m_layerMask[aSeg->GetLayer()] )
        return;

    int     radius = 0;
    double  StAngle = 0, EndAngle = 0;
    int     thickness = aSeg->GetWidth();

    m_plotter->SetColor( getColor( aSeg->GetLayer() ) );

    wxPoint start( aSeg->GetStart() );
    wxPoint end( aSeg->GetEnd() );

    GBR_METADATA gbr_metadata;

    if( aSeg->GetLayer() == Edge_Cuts )
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_EDGECUT );

    if( IsCopperLayer( aSeg->GetLayer() ) )
        // Graphic items (DRAWSEGMENT, TEXT) having no net have the NonConductor attribute
        // Graphic items having a net have the Conductor attribute, but are not (yet?)
        // supported in Pcbnew
        gbr_metadata.SetApertureAttrib( GBR_APERTURE_METADATA::GBR_APERTURE_ATTRIB_NONCONDUCTOR );

    switch( aSeg->GetShape() )
    {
    case S_CIRCLE:
        radius = KiROUND( GetLineLength( end, start ) );
        m_plotter->ThickCircle( start, radius * 2, thickness, GetPlotMode(), &gbr_metadata );
        break;

    case S_ARC:
        radius = KiROUND( GetLineLength( end, start ) );
        StAngle  = ArcTangente( end.y - start.y, end.x - start.x );
        EndAngle = StAngle + aSeg->GetAngle();

        // when startAngle == endAngle ThickArc() doesn't know whether it's 0 deg and 360 deg
        if( std::abs( aSeg->GetAngle() ) == 3600.0 )
            m_plotter->ThickCircle( start, radius * 2, thickness, GetPlotMode(), &gbr_metadata );
        else
            m_plotter->ThickArc( start, -EndAngle, -StAngle, radius, thickness, GetPlotMode(), &gbr_metadata );
        break;

    case S_CURVE:
        m_plotter->BezierCurve( aSeg->GetStart(), aSeg->GetBezControl1(), aSeg->GetBezControl2(),
                                aSeg->GetEnd(), 0, thickness );
        break;

    case S_POLYGON:
        {
            if( !aSeg->IsPolygonFilled() )
            {
                for( auto it = aSeg->GetPolyShape().CIterateSegments( 0 ); it; it++ )
                {
                    auto seg = it.Get();
                    m_plotter->ThickSegment( wxPoint( seg.A ), wxPoint( seg.B ),
                                             thickness, GetPlotMode(), &gbr_metadata );
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
                auto tmpPoly = SHAPE_POLY_SET( aSeg->GetPolyShape() );
                tmpPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

                for( int jj = 0; jj < tmpPoly.OutlineCount(); ++jj )
                {
                    SHAPE_LINE_CHAIN& poly = tmpPoly.Outline( jj );
                    m_plotter->PlotPoly( poly, FILLED_SHAPE, thickness, &gbr_metadata );
                }
            }
        }
        break;

    default:
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
        m_plotter->FlashPadCircle( aDrillPos, aDrillSize.x, GetPlotMode(), NULL );
}


void BRDITEMS_PLOTTER::PlotDrillMarks()
{
    /* If small drills marks were requested prepare a clamp value to pass
       to the helper function */
    int small_drill = (GetDrillMarksType() == PCB_PLOT_PARAMS::SMALL_DRILL_SHAPE) ?
                        SMALL_DRILL : 0;

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

    for( auto pts : m_board->Tracks() )
    {
        const VIA* via = dyn_cast<const VIA*>( pts );

        if( via )
        {
            plotOneDrillMark( PAD_DRILL_SHAPE_CIRCLE, via->GetStart(),
                    wxSize( via->GetDrillValue(), 0 ),
                    wxSize( via->GetWidth(), 0 ), 0, small_drill );
        }
    }

    for( auto Module : m_board->Modules() )
    {
        for( auto pad : Module->Pads() )
        {
            if( pad->GetDrillSize().x == 0 )
                continue;

            plotOneDrillMark( pad->GetDrillShape(),
                              pad->GetPosition(), pad->GetDrillSize(),
                              pad->GetSize(), pad->GetOrientation(),
                              small_drill );
        }
    }

    if( GetPlotMode() == FILLED )
        m_plotter->SetColor( GetColor() );
}
