/**
 * @file dialog_pad_basicshapes_properties.cpp
 * @brief basic shapes for pads crude editor.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>

#include <fctsys.h>
#include <common.h>
#include <confirm.h>
#include <pcbnew.h>
#include <trigo.h>
#include <macros.h>
#include <pcb_base_frame.h>
#include <base_units.h>
#include <unit_format.h>
#include <gr_basic.h>

#include <class_board.h>
#include <class_module.h>

#include <dialog_pad_properties.h>

DIALOG_PAD_PRIMITIVES_PROPERTIES::DIALOG_PAD_PRIMITIVES_PROPERTIES(
                            wxWindow* aParent, PAD_CS_PRIMITIVE * aShape )
    : DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( aParent )
{
    m_shape = aShape;
    TransferDataToWindow();

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
}

bool DIALOG_PAD_PRIMITIVES_PROPERTIES::TransferDataToWindow()
{
    if( m_shape == NULL )
        return false;

    // Shows the text info about circle or ring only for S_CIRCLE shape
    if( m_shape->m_Shape != S_CIRCLE )
        m_staticTextInfo->Show( false );

    PutValueInLocalUnits( *m_textCtrlThickness, m_shape->m_Thickness );

    // Update units and parameters names according to the shape to edit:
    wxString unit = GetAbbreviatedUnitsLabel();
    m_staticTextPosUnit->SetLabel( unit );
    m_staticTextEndUnit->SetLabel( unit );
    m_staticTextThicknessUnit->SetLabel( unit );

    m_staticTextAngleUnit->SetLabel( wxEmptyString );
    m_staticTextAngle->SetLabel( wxEmptyString );

    switch( m_shape->m_Shape )
    {
    case S_SEGMENT:         // Segment with rounded ends
        SetTitle( _( "Segment" ) );
        PutValueInLocalUnits( *m_textCtrPosX, m_shape->m_Start.x );
        PutValueInLocalUnits( *m_textCtrPosY, m_shape->m_Start.y );
        PutValueInLocalUnits( *m_textCtrEndX, m_shape->m_End.x );
        PutValueInLocalUnits( *m_textCtrEndY, m_shape->m_End.y );
        m_textCtrAngle->Show( false );
        m_staticTextAngleUnit->Show( false );
        m_staticTextAngle->Show( false );
        break;

    case S_ARC:             // Arc with rounded ends
        SetTitle( _( "Arc" ) );
        m_staticTextPosEnd->SetLabel( _( "Center" ) );
        PutValueInLocalUnits( *m_textCtrEndX, m_shape->m_Start.x ); // Start point of arc
        PutValueInLocalUnits( *m_textCtrEndY, m_shape->m_Start.y );
        PutValueInLocalUnits( *m_textCtrPosX, m_shape->m_End.x );   // arc center
        PutValueInLocalUnits( *m_textCtrPosY, m_shape->m_End.y );
        m_textCtrAngle->SetValue( FMT_ANGLE( m_shape->m_ArcAngle ) );
        m_staticTextAngle->SetLabel( _( "Angle" ) );
        m_staticTextAngleUnit->SetLabel( _( "degree" ) );
        break;

    case S_CIRCLE:          //  ring or circle
        if( m_shape->m_Thickness )
            SetTitle( _( "Ring" ) );
        else
            SetTitle( _( "Circle" ) );

        // End point does not exist for a circle or ring:
        m_textCtrEndX->Show( false );
        m_textCtrEndY->Show( false );
        m_staticTextPosEnd->Show( false );
        m_staticTextEndUnit->Show( false );
        m_staticTextEndX->Show( false );
        m_staticTextEndY->Show( false );

        // Circle center uses position controls:
        m_staticTextPosStart->SetLabel( _( "Center" ) );
        PutValueInLocalUnits( *m_textCtrPosX, m_shape->m_Start.x );
        PutValueInLocalUnits( *m_textCtrPosY, m_shape->m_Start.y );
        PutValueInLocalUnits( *m_textCtrAngle, m_shape->m_Radius );
        m_staticTextAngleUnit->SetLabel( unit );
        m_staticTextAngle->SetLabel( _( "Radius" ) );
        break;

    case S_POLYGON:         // polygon
        SetTitle( "Polygon" );
        m_staticTextPosStart->SetLabel( wxEmptyString );
        m_staticTextPosEnd->SetLabel( wxEmptyString );
        m_staticTextAngle->SetLabel( _( "corners count" ) );
        m_textCtrAngle->SetValue( wxString::Format( "%d", m_shape->m_Poly.size() ) );
        break;

    default:
        SetTitle( "Unknown basic shape" );
        break;
    }

    return true;
}

bool DIALOG_PAD_PRIMITIVES_PROPERTIES::TransferDataFromWindow()
{
    // Transfer data out of the GUI.
    m_shape->m_Thickness = ValueFromString( g_UserUnit, m_textCtrlThickness->GetValue() );

    switch( m_shape->m_Shape )
    {
    case S_SEGMENT:         // Segment with rounded ends
        m_shape->m_Start.x = ValueFromString( g_UserUnit, m_textCtrPosX->GetValue() );
        m_shape->m_Start.y = ValueFromString( g_UserUnit, m_textCtrPosY->GetValue() );
        m_shape->m_End.x = ValueFromString( g_UserUnit, m_textCtrEndX->GetValue() );
        m_shape->m_End.y = ValueFromString( g_UserUnit, m_textCtrEndY->GetValue() );
        break;

    case S_ARC:             // Arc with rounded ends
        // Start point of arc
        m_shape->m_Start.x = ValueFromString( g_UserUnit, m_textCtrEndX->GetValue() );
        m_shape->m_Start.y = ValueFromString( g_UserUnit, m_textCtrEndY->GetValue() );
        // arc center
        m_shape->m_End.x = ValueFromString( g_UserUnit, m_textCtrPosX->GetValue() );
        m_shape->m_End.y = ValueFromString( g_UserUnit, m_textCtrPosY->GetValue() );
        m_shape->m_ArcAngle = ValueFromString( DEGREES, m_textCtrAngle->GetValue() );
        break;

    case S_CIRCLE:          //  ring or circle
        m_shape->m_Start.x = ValueFromString( g_UserUnit, m_textCtrPosX->GetValue() );
        m_shape->m_Start.y = ValueFromString( g_UserUnit, m_textCtrPosY->GetValue() );
        //radius
        m_shape->m_Radius = ValueFromString( g_UserUnit, m_textCtrAngle->GetValue() );
        break;

    case S_POLYGON:         // polygon
        // polygon has a specific dialog editor. No nothing here
        break;

    default:
        SetTitle( "Unknown basic shape" );
        break;
    }

    return true;
}


DIALOG_PAD_PRIMITIVE_POLY_PROPS::DIALOG_PAD_PRIMITIVE_POLY_PROPS(
                            wxWindow* aParent, PAD_CS_PRIMITIVE * aShape )
    : DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( aParent ),
        m_shape( aShape ), m_currshape( *m_shape )
{
    TransferDataToWindow();

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );

    // TODO: move wxEVT_GRID_CELL_CHANGING in wxFormbuilder, when it support it
	m_gridCornersList->Connect( wxEVT_GRID_CELL_CHANGING,
                                wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS::onCellChanging ), NULL, this );
}


DIALOG_PAD_PRIMITIVE_POLY_PROPS::~DIALOG_PAD_PRIMITIVE_POLY_PROPS()
{
	m_gridCornersList->Disconnect( wxEVT_GRID_CELL_CHANGING,
                                   wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS::onCellChanging ), NULL, this );
}


bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::TransferDataToWindow()
{
    if( m_shape == NULL )
        return false;

    // Update units and parameters names according to the shape to edit:
    wxString unit = GetAbbreviatedUnitsLabel();
    m_staticTextThicknessUnit->SetLabel( unit );

    PutValueInLocalUnits( *m_textCtrlThickness, m_currshape.m_Thickness );

    // Test for acceptable polygon (more than 2 corners, and not self-intersecting)
    // A warning message is displayed if not OK
    Validate();

    // If the number of corners is < 2 (Happens for a new shape, prepare 2 dummy corners
    while( m_currshape.m_Poly.size() < 2 )
        m_currshape.m_Poly.push_back( wxPoint( 0, 0 ) );

    // Populates the list of corners
    int extra_rows = m_currshape.m_Poly.size() - m_gridCornersList->GetNumberRows();

    if( extra_rows > 0 )
    {
        m_gridCornersList->AppendRows( extra_rows );
    }
    else if( extra_rows < 0 )
    {
        extra_rows = -extra_rows;
        m_gridCornersList->DeleteRows( 0, extra_rows );
    }

    // enter others corner coordinates
    wxString msg;
    for( unsigned row = 0; row < m_currshape.m_Poly.size(); ++row )
    {
        // Row label is "Corner x"
        msg.Printf( "Corner %d", row+1 );
        m_gridCornersList->SetRowLabelValue( row, msg );

        msg = StringFromValue( g_UserUnit, m_currshape.m_Poly[row].x );
        m_gridCornersList->SetCellValue( row, 0, msg );

        msg = StringFromValue( g_UserUnit, m_currshape.m_Poly[row].y );
        m_gridCornersList->SetCellValue( row, 1, msg );
    }

    return true;
}

bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    // Transfer data out of the GUI.
    m_currshape.m_Thickness = ValueFromString( g_UserUnit, m_textCtrlThickness->GetValue() );
    *m_shape = m_currshape;

    return true;
}


// test for a valid polygon (a not self intersectiong polygon)
bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::Validate()
{
    if( m_currshape.m_Poly.size() < 3 )
    {
        m_staticTextValidate->SetLabel( _("Incorrect polygon: less than 3 corners" ) );
        m_staticTextValidate->Show( true );
        return false;
    }

    bool valid = true;

    SHAPE_LINE_CHAIN polyline;

    for( unsigned ii = 0; ii < m_currshape.m_Poly.size(); ++ii )
        polyline.Append( m_currshape.m_Poly[ii].x, m_currshape.m_Poly[ii].y );

    // The polyline describes a polygon: close it.
    polyline.SetClosed( true );

    // Remove redundant corners:
    polyline.Simplify();

    if(  polyline.PointCount() < 3 )
    {
        m_staticTextValidate->SetLabel( _("Incorrect polygon: too few corners after simplification" ) );
        valid = false;
    }

    if( valid && polyline.SelfIntersecting() )
    {
        m_staticTextValidate->SetLabel( _("Incorrect polygon: self intersecting" ) );
        valid = false;
    }

    if( valid )
        m_staticTextValidate->SetLabel( _("Polygon:" ) );

    if( polyline.PointCount() != (int)m_currshape.m_Poly.size() )
    {   // Happens after simplification
        m_currshape.m_Poly.clear();

        for( int ii = 0; ii < polyline.PointCount(); ++ii )
            m_currshape.m_Poly.push_back( wxPoint( polyline.CPoint( ii ).x, polyline.CPoint( ii ).y ) );

        m_staticTextValidate->SetLabel( _("Polygon: redundant corners removed" ) );
    }

    return valid;
}


void DIALOG_PAD_PRIMITIVE_POLY_PROPS::onButtonAdd( wxCommandEvent& event )
{
    // Insert a new corner after the currently selected:
    int row = -1;

    if( m_gridCornersList->GetNumberRows() )
    {
        wxArrayInt selections =	m_gridCornersList->GetSelectedRows();

        if( selections.size() > 0 )
        {
            std::sort( selections.begin(), selections.end() );
            row = selections[0];
        }
        else
        {
            row = m_gridCornersList->GetGridCursorRow();
        }

        if( row < 0 )
        {
            wxMessageBox( _( "Select a corner before adding a new corner" ) );
            return;
        }
    }
    else
        row = 0;

    m_gridCornersList->SelectRow( row, false );

    if( m_currshape.m_Poly.size() == 0 )
        m_currshape.m_Poly.push_back( wxPoint(0,0) );
    else
        m_currshape.m_Poly.insert( m_currshape.m_Poly.begin() + row + 1, wxPoint(0,0) );

    TransferDataToWindow();

    m_panelPoly->Refresh();
}

void DIALOG_PAD_PRIMITIVE_POLY_PROPS::OnButtonDelete( wxCommandEvent& event )
{
    wxArrayInt selections =	m_gridCornersList->GetSelectedRows();
    std::sort( selections.begin(), selections.end() );

    // remove corners:
    for( int ii = selections.size()-1; ii >= 0 ; --ii )
    {
        m_currshape.m_Poly.erase( m_currshape.m_Poly.begin() + selections[ii] );
    }

    // Unselect all raws:
    m_gridCornersList->SelectRow( -1, false );

    TransferDataToWindow();

    m_panelPoly->Refresh();
}

void DIALOG_PAD_PRIMITIVE_POLY_PROPS::onPaintPolyPanel( wxPaintEvent& event )
{
    wxPaintDC dc( m_panelPoly );
    wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Calculate a suitable scale to fit the available draw area
    wxSize minsize;

    for( unsigned ii = 0; ii < m_currshape.m_Poly.size(); ++ii )
    {
        minsize.x = std::max( minsize.x, std::abs( m_currshape.m_Poly[ii].x ) );
        minsize.y = std::max( minsize.y, std::abs( m_currshape.m_Poly[ii].y ) );
    }

    // The draw origin is the center of the window.
    // Therefore the window size is twice the minsize just calculated
    minsize.x *= 2;
    minsize.y *= 2;
    minsize.x += m_currshape.m_Thickness;
    minsize.y += m_currshape.m_Thickness;

    // Avoid null or too small size:
    int mindim = Millimeter2iu( 0.5 );

    if( minsize.x < mindim )
        minsize.x = mindim;

    if( minsize.y < mindim )
        minsize.y = mindim;

    double scale = std::min( (double) dc_size.x / minsize.x, (double) dc_size.y / minsize.y );

    // Give a margin
    scale *= 0.9;
    dc.SetUserScale( scale, scale );

    GRResetPenAndBrush( &dc );

    // Draw X and Y axis. This is particularly useful to show the
    // reference position of basic shape
    // Axis are drawn before the polygon to avoid masking segments on axis
    GRLine( NULL, &dc, -int( dc_size.x/scale ), 0, int( dc_size.x/scale ), 0, 0, LIGHTBLUE );   // X axis
    GRLine( NULL, &dc, 0, -int( dc_size.y/scale ), 0, int( dc_size.y/scale ), 0, LIGHTBLUE );   // Y axis

    // Draw polygon.
    // The selected edge(s) are shown in selectcolor, the others in normalcolor.
    EDA_COLOR_T normalcolor = WHITE;
    EDA_COLOR_T selectcolor = RED;

    for( unsigned ii = 0; ii < m_currshape.m_Poly.size(); ++ii )
    {
        EDA_COLOR_T color = normalcolor;

        if( m_gridCornersList->IsInSelection (ii, 0) ||
            m_gridCornersList->IsInSelection (ii, 1) ||
            m_gridCornersList->GetGridCursorRow() == (int)ii )
            color = selectcolor;

        unsigned jj = ii + 1;

        if( jj >= m_currshape.m_Poly.size() )
            jj = 0;

        GRLine( NULL, &dc, m_currshape.m_Poly[ii], m_currshape.m_Poly[jj], m_currshape.m_Thickness, color );
    }

    event.Skip();
}

void DIALOG_PAD_PRIMITIVE_POLY_PROPS::onPolyPanelResize( wxSizeEvent& event )
{
    m_panelPoly->Refresh();
    event.Skip();
}

void DIALOG_PAD_PRIMITIVE_POLY_PROPS::onGridSelect( wxGridRangeSelectEvent& event )
{
    m_panelPoly->Refresh();
}

void DIALOG_PAD_PRIMITIVE_POLY_PROPS::onCellChanging( wxGridEvent& event )
{
    int row = event.GetRow();
    int col = event.GetCol();

    wxString msg = event.GetString();

    if( msg.IsEmpty() )
        return;

    int value = ValueFromString( g_UserUnit, msg );

    if( col == 0 )  // Set the X value
        m_currshape.m_Poly[row].x = value;
    else            // Set the Y value
        m_currshape.m_Poly[row].y = value;

    m_currshape.m_Thickness = ValueFromString( g_UserUnit, m_textCtrlThickness->GetValue() );

    Validate();

    m_panelPoly->Refresh();
}


// A dialog to apply geometry transforms to a shape or set of shapes
// (move, rotate around origin, scaling factor, duplication).
DIALOG_PAD_PRIMITIVES_TRANSFORM::DIALOG_PAD_PRIMITIVES_TRANSFORM(
                                        wxWindow* aParent,
                                        std::vector<PAD_CS_PRIMITIVE*>& aList, bool aShowDuplicate )
    :DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( aParent ), m_list( aList )
{
    wxString unit = GetAbbreviatedUnitsLabel();
    m_staticTextMoveUnit->SetLabel( unit );

    if( !aShowDuplicate )     // means no duplicate transform
    {
		m_staticTextDupCnt->Show( false );
		m_spinCtrlDuplicateCount->Show( false );
    }

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
}

// A helper function in geometry transform
inline void geom_transf( wxPoint& aCoord, wxPoint& aMove, double aScale, double aRotation )
{
    aCoord.x = KiROUND( aCoord.x * aScale );
    aCoord.y = KiROUND( aCoord.y * aScale );
    aCoord += aMove;
    RotatePoint( &aCoord, aRotation );
}

void DIALOG_PAD_PRIMITIVES_TRANSFORM::Transform( std::vector<PAD_CS_PRIMITIVE>* aList, int aDuplicateCount )
{
    // Get parameters from dlg:
    wxPoint move_vect;
    move_vect.x = ValueFromString( g_UserUnit, m_textCtrMoveX->GetValue() );
    move_vect.y = ValueFromString( g_UserUnit, m_textCtrMoveY->GetValue() );
    wxPoint currMoveVect = move_vect;

    double rotation = DoubleValueFromString( DEGREES, m_textCtrAngle->GetValue() );
    double curr_rotation = rotation;
    double scale = DoubleValueFromString( UNSCALED_UNITS, m_textCtrlScalingFactor->GetValue() );

    // Avoid too small /too large scale, which could create issues:
    if( scale < 0.01 )
        scale = 0.01;

    if( scale > 100.0 )
        scale = 100.0;

    // Transform shapes
    // shapes are scaled, then moved then rotated.
    // if aList != NULL, the initial shape will be duplicated, and transform
    // applied to the duplicated shape
    do {
        for( unsigned idx = 0; idx < m_list.size(); ++idx )
        {
            PAD_CS_PRIMITIVE* shape;

            if( aList == NULL )
                shape = m_list[idx];
            else
            {
                PAD_CS_PRIMITIVE new_shape( *m_list[idx] );
                aList->push_back( new_shape );
                shape = &aList->back();
            }

            // Transform parameters common to all shape types (some can be unused)
            shape->m_Thickness = KiROUND( shape->m_Thickness * scale );
            geom_transf( shape->m_Start, currMoveVect, scale, curr_rotation );
            geom_transf( shape->m_End, currMoveVect, scale, curr_rotation );

            // specific parameters:
            switch( shape->m_Shape )
            {
            case S_SEGMENT:         // Segment with rounded ends
                break;

            case S_ARC:             // Arc with rounded ends
                break;

            case S_CIRCLE:          //  ring or circle
                shape->m_Radius = KiROUND( shape->m_Radius * scale );
                break;

            case S_POLYGON:         // polygon
                for( unsigned ii = 0; ii < shape->m_Poly.size(); ++ii )
                    geom_transf( shape->m_Poly[ii], currMoveVect, scale, curr_rotation );
                break;

            default:
                break;
            }
        }

        // Prepare new transform on duplication:
        // Each new item is rotated (or moved) by the transform from the last duplication
        curr_rotation += rotation;
        currMoveVect += move_vect;
    } while( aList && --aDuplicateCount > 0 );
}

