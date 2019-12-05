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
#include <widgets/wx_grid.h>
#include <class_board.h>
#include <class_module.h>

#include <dialog_pad_properties.h>
#include <bitmaps.h>

DIALOG_PAD_PRIMITIVES_PROPERTIES::DIALOG_PAD_PRIMITIVES_PROPERTIES( wxWindow* aParent,
                                                                    PCB_BASE_FRAME* aFrame,
                                                                    PAD_CS_PRIMITIVE * aShape ) :
        DIALOG_PAD_PRIMITIVES_PROPERTIES_BASE( aParent ),
        m_shape( aShape ),
        m_startX( aFrame, m_startXLabel, m_startXCtrl, m_startXUnits, true ),
        m_startY( aFrame, m_startYLabel, m_startYCtrl, m_startYUnits, true ),
        m_ctrl1X( aFrame, m_ctrl1XLabel, m_ctrl1XCtrl, m_ctrl1XUnits, true ),
        m_ctrl1Y( aFrame, m_ctrl1YLabel, m_ctrl1YCtrl, m_ctrl1YUnits, true ),
        m_ctrl2X( aFrame, m_ctrl2XLabel, m_ctrl2XCtrl, m_ctrl2XUnits, true ),
        m_ctrl2Y( aFrame, m_ctrl2YLabel, m_ctrl2YCtrl, m_ctrl2YUnits, true ),
        m_endX( aFrame, m_endXLabel, m_endXCtrl, m_endXUnits, true ),
        m_endY( aFrame, m_endYLabel, m_endYCtrl, m_endYUnits, true ),
        m_radius( aFrame, m_radiusLabel, m_radiusCtrl, m_radiusUnits, true ),
        m_thickness( aFrame, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits, true )
{
    SetInitialFocus( m_startXCtrl );

    TransferDataToWindow();

    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();
}

bool DIALOG_PAD_PRIMITIVES_PROPERTIES::TransferDataToWindow()
{
    if( m_shape == NULL )
        return false;

    // Shows the text info about circle or ring only for S_CIRCLE shape
    if( m_shape->m_Shape != S_CIRCLE )
        m_staticTextInfo->Show( false );

    m_thickness.SetValue( m_shape->m_Thickness );

    switch( m_shape->m_Shape )
    {
    case S_SEGMENT:         // Segment with rounded ends
        SetTitle( _( "Segment" ) );
        m_startX.SetValue( m_shape->m_Start.x );
        m_startY.SetValue( m_shape->m_Start.y );
        m_endX.SetValue( m_shape->m_End.x );
        m_endY.SetValue( m_shape->m_End.y );
        m_ctrl1X.Show( false, true );
        m_ctrl1Y.Show( false, true );
        m_ctrl2X.Show( false, true );
        m_ctrl2Y.Show( false, true );
        m_staticTextPosCtrl1->Show( false );
        m_staticTextPosCtrl1->SetSize( 0, 0 );
        m_staticTextPosCtrl2->Show( false );
        m_staticTextPosCtrl2->SetSize( 0, 0 );
        m_radius.Show( false );
        break;

    case S_CURVE:         // Bezier line
        SetTitle( _( "Bezier" ) );
        m_startX.SetValue( m_shape->m_Start.x );
        m_startY.SetValue( m_shape->m_Start.y );
        m_endX.SetValue( m_shape->m_End.x );
        m_endY.SetValue( m_shape->m_End.y );
        m_ctrl1X.SetValue( m_shape->m_Ctrl1.x );
        m_ctrl1Y.SetValue( m_shape->m_Ctrl1.y );
        m_ctrl2X.SetValue( m_shape->m_Ctrl2.x );
        m_ctrl2Y.SetValue( m_shape->m_Ctrl2.y );
        m_radius.Show( false );
        break;

    case S_ARC:             // Arc with rounded ends
        SetTitle( _( "Arc" ) );
        m_startX.SetValue( m_shape->m_End.x );     // confusingly, the start point of the arc
        m_startY.SetValue( m_shape->m_End.y );
        m_staticTextPosEnd->SetLabel( _( "Center" ) );
        m_endX.SetValue( m_shape->m_Start.x );     // arc center
        m_endY.SetValue( m_shape->m_Start.y );
        m_radiusLabel->SetLabel( _( "Angle:" ) );
        m_radius.SetUnits( DEGREES );
        m_radius.SetValue( m_shape->m_ArcAngle );
        m_ctrl1X.Show( false, true );
        m_ctrl1Y.Show( false, true );
        m_ctrl2X.Show( false, true );
        m_ctrl2Y.Show( false, true );
        m_staticTextPosCtrl1->Show( false );
        m_staticTextPosCtrl1->SetSize( 0, 0 );
        m_staticTextPosCtrl2->Show( false );
        m_staticTextPosCtrl2->SetSize( 0, 0 );
        break;

    case S_CIRCLE:          //  ring or circle
        if( m_shape->m_Thickness )
            SetTitle( _( "Ring" ) );
        else
            SetTitle( _( "Circle" ) );

        // End point does not exist for a circle or ring:
        m_staticTextPosEnd->Show( false );
        m_endX.Show( false );
        m_endY.Show( false );

        // Circle center uses position controls:
        m_staticTextPosStart->SetLabel( _( "Center:" ) );
        m_startX.SetValue( m_shape->m_Start.x );
        m_startY.SetValue( m_shape->m_Start.y );
        m_radius.SetValue( m_shape->m_Radius );
        m_ctrl1X.Show( false, true );
        m_ctrl1Y.Show( false, true );
        m_ctrl2X.Show( false, true );
        m_ctrl2Y.Show( false, true );
        m_staticTextPosCtrl1->Show( false );
        m_staticTextPosCtrl1->SetSize( 0, 0 );
        m_staticTextPosCtrl2->Show( false );
        m_staticTextPosCtrl2->SetSize( 0, 0 );
        break;

    case S_POLYGON:         // polygon
        // polygon has a specific dialog editor. So nothing here
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
    m_shape->m_Thickness = m_thickness.GetValue();

    switch( m_shape->m_Shape )
    {
    case S_SEGMENT:         // Segment with rounded ends
        m_shape->m_Start.x = m_startX.GetValue();
        m_shape->m_Start.y = m_startY.GetValue();
        m_shape->m_End.x = m_endX.GetValue();
        m_shape->m_End.y = m_endY.GetValue();
        break;

    case S_CURVE:         // Segment with rounded ends
        m_shape->m_Start.x = m_startX.GetValue();
        m_shape->m_Start.y = m_startY.GetValue();
        m_shape->m_End.x = m_endX.GetValue();
        m_shape->m_End.y = m_endY.GetValue();
        m_shape->m_Ctrl1.x = m_ctrl1X.GetValue();
        m_shape->m_Ctrl1.y = m_ctrl1Y.GetValue();
        m_shape->m_Ctrl2.x = m_ctrl2X.GetValue();
        m_shape->m_Ctrl2.y = m_ctrl2Y.GetValue();
        break;

    case S_ARC:             // Arc with rounded ends
        // NB: we store the center of the arc in m_Start, and, confusingly,
        // the start point in m_End
        m_shape->m_Start.x = m_endX.GetValue();
        m_shape->m_Start.y = m_endY.GetValue();
        m_shape->m_End.x = m_startX.GetValue();
        m_shape->m_End.y = m_startY.GetValue();
        // arc angle
        m_shape->m_ArcAngle = m_radius.GetValue();
        break;

    case S_CIRCLE:          //  ring or circle
        m_shape->m_Start.x = m_startX.GetValue();
        m_shape->m_Start.y = m_startY.GetValue();
        m_shape->m_Radius = m_radius.GetValue();
        break;

    case S_POLYGON:         // polygon
        // polygon has a specific dialog editor. So nothing here
        break;

    default:
        SetTitle( "Unknown basic shape" );
        break;
    }

    return true;
}


DIALOG_PAD_PRIMITIVE_POLY_PROPS::DIALOG_PAD_PRIMITIVE_POLY_PROPS( wxWindow* aParent,
                                                                  PCB_BASE_FRAME* aFrame,
                                                                  PAD_CS_PRIMITIVE * aShape ) :
        DIALOG_PAD_PRIMITIVE_POLY_PROPS_BASE( aParent ),
        m_shape( aShape ),
        m_currshape( *m_shape ),
        m_thickness( aFrame, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits, true )
{
    m_addButton->SetBitmap( KiBitmap( small_plus_xpm ) );
    m_deleteButton->SetBitmap( KiBitmap( trash_xpm ) );
    m_warningIcon->SetBitmap( KiBitmap( dialog_warning_xpm ) );

    // Test for acceptable polygon (more than 2 corners, and not self-intersecting) and
    // remove any redundant corners.  A warning message is displayed if not OK.
    doValidate( true );

    TransferDataToWindow();

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );

    // TODO: move wxEVT_GRID_CELL_CHANGING in wxFormbuilder, when it support it
	m_gridCornersList->Connect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS::onCellChanging ), NULL, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


DIALOG_PAD_PRIMITIVE_POLY_PROPS::~DIALOG_PAD_PRIMITIVE_POLY_PROPS()
{
	m_gridCornersList->Disconnect( wxEVT_GRID_CELL_CHANGING, wxGridEventHandler( DIALOG_PAD_PRIMITIVE_POLY_PROPS::onCellChanging ), NULL, this );
}


bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::TransferDataToWindow()
{
    if( m_shape == NULL )
        return false;

    m_thickness.SetValue( m_currshape.m_Thickness );

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

        msg = StringFromValue( GetUserUnits(), m_currshape.m_Poly[row].x, true, true );
        m_gridCornersList->SetCellValue( row, 0, msg );

        msg = StringFromValue( GetUserUnits(), m_currshape.m_Poly[row].y, true, true );
        m_gridCornersList->SetCellValue( row, 1, msg );
    }

    return true;
}

bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    m_currshape.m_Thickness = m_thickness.GetValue();

    *m_shape = m_currshape;

    return true;
}


bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::Validate()
{
    // Don't remove redundant corners while user is editing corner list
    return doValidate( false );
}


// test for a valid polygon (a not self intersectiong polygon)
bool DIALOG_PAD_PRIMITIVE_POLY_PROPS::doValidate( bool aRemoveRedundantCorners )
{
    if( !m_gridCornersList->CommitPendingChanges() )
        return false;

    if( m_currshape.m_Poly.size() < 3 )
    {
        m_warningText->SetLabel( _("Polygon must have at least 3 corners" ) );
        m_warningText->Show( true );
        m_warningIcon->Show( true );
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
        m_warningText->SetLabel( _("Polygon must have at least 3 corners after simplification" ) );
        valid = false;
    }

    if( valid && polyline.SelfIntersecting() )
    {
        m_warningText->SetLabel( _("Polygon may not be self-intersecting" ) );
        valid = false;
    }

    m_warningIcon->Show( !valid );
    m_warningText->Show( !valid );

    if( aRemoveRedundantCorners )
    {
        if( polyline.PointCount() != (int)m_currshape.m_Poly.size() )
        {   // Happens after simplification
            m_currshape.m_Poly.clear();

            for( int ii = 0; ii < polyline.PointCount(); ++ii )
                m_currshape.m_Poly.emplace_back( polyline.CPoint( ii ).x, polyline.CPoint( ii ).y );

            m_warningIcon->Show( true );
            m_warningText->Show( true );
            m_warningText->SetLabel( _("Note: redundant corners removed" ) );
        }
    }

    return valid;
}


void DIALOG_PAD_PRIMITIVE_POLY_PROPS::OnButtonAdd( wxCommandEvent& event )
{
    if( !m_gridCornersList->CommitPendingChanges() )
        return;

    // Insert a new corner after the currently selected:
    wxArrayInt selections =	m_gridCornersList->GetSelectedRows();
    int row = -1;

    if( m_gridCornersList->GetNumberRows() == 0 )
        row = 0;
    else if( selections.size() > 0 )
        row = selections[ selections.size() - 1 ] + 1;
    else
        row = m_gridCornersList->GetGridCursorRow() + 1;

    if( row < 0 )
    {
        wxMessageBox( _( "Select a corner to add the new corner after." ) );
        return;
    }

    if( m_currshape.m_Poly.size() == 0 || row >= (int) m_currshape.m_Poly.size() )
        m_currshape.m_Poly.emplace_back( 0, 0 );
    else
        m_currshape.m_Poly.insert( m_currshape.m_Poly.begin() + row, wxPoint( 0, 0 ) );

    Validate();
    TransferDataToWindow();

    m_gridCornersList->ForceRefresh();
    // Select the new row
    m_gridCornersList->SelectRow( row, false );

    m_panelPoly->Refresh();
}

void DIALOG_PAD_PRIMITIVE_POLY_PROPS::OnButtonDelete( wxCommandEvent& event )
{
    if( !m_gridCornersList->CommitPendingChanges() )
        return;

    wxArrayInt selections =	m_gridCornersList->GetSelectedRows();

    if( m_gridCornersList->GetNumberRows() == 0 )
        return;

    if( selections.size() == 0 && m_gridCornersList->GetGridCursorRow() >= 0 )
        selections.push_back( m_gridCornersList->GetGridCursorRow() );

    if( selections.size() == 0 )
    {
        wxMessageBox( _( "Select a corner to delete." ) );
        return;
    }

    // remove corners:
    std::sort( selections.begin(), selections.end() );

    for( int ii = selections.size()-1; ii >= 0 ; --ii )
        m_currshape.m_Poly.erase( m_currshape.m_Poly.begin() + selections[ii] );

    Validate();
    TransferDataToWindow();

    m_gridCornersList->ForceRefresh();
    // select the row previous to the last deleted row
    m_gridCornersList->SelectRow( std::max( 0, selections[ 0 ] - 1 ) );

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
    int      row = event.GetRow();
    int      col = event.GetCol();
    wxString msg = event.GetString();

    if( msg.IsEmpty() )
        return;

    if( col == 0 )  // Set the X value
        m_currshape.m_Poly[row].x = ValueFromString( GetUserUnits(), msg, true );
    else            // Set the Y value
        m_currshape.m_Poly[row].y = ValueFromString( GetUserUnits(), msg, true );

    m_currshape.m_Thickness = m_thickness.GetValue();

    Validate();

    m_panelPoly->Refresh();
}


// A dialog to apply geometry transforms to a shape or set of shapes
// (move, rotate around origin, scaling factor, duplication).
DIALOG_PAD_PRIMITIVES_TRANSFORM::DIALOG_PAD_PRIMITIVES_TRANSFORM( wxWindow* aParent,
                                                                  PCB_BASE_FRAME* aFrame,
                                                                  std::vector<PAD_CS_PRIMITIVE*>& aList,
                                                                  bool aShowDuplicate ) :
    DIALOG_PAD_PRIMITIVES_TRANSFORM_BASE( aParent ),
    m_list( aList ),
    m_vectorX( aFrame, m_xLabel, m_xCtrl, m_xUnits, true ),
    m_vectorY( aFrame, m_yLabel, m_yCtrl, m_yUnits, true ),
    m_rotation( aFrame, m_rotationLabel, m_rotationCtrl, m_rotationUnits )
{
    m_rotation.SetUnits( DEGREES );

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
    wxPoint move_vect( m_vectorX.GetValue(), m_vectorY.GetValue() );
    double  rotation = m_rotation.GetValue();
    double scale = DoubleValueFromString( UNSCALED_UNITS, m_scaleCtrl->GetValue() );

    // Avoid too small / too large scale, which could create issues:
    if( scale < 0.01 )
        scale = 0.01;

    if( scale > 100.0 )
        scale = 100.0;

    // Transform shapes
    // shapes are scaled, then moved then rotated.
    // if aList != NULL, the initial shape will be duplicated, and transform
    // applied to the duplicated shape

    wxPoint currMoveVect = move_vect;
    double curr_rotation = rotation;

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

            case S_CURVE:           // Bezier with rounded ends
                geom_transf( shape->m_Ctrl1, currMoveVect, scale, curr_rotation );
                geom_transf( shape->m_Ctrl2, currMoveVect, scale, curr_rotation );
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

