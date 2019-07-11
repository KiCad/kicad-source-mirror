/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
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

/*
 * Edit properties of Lines, Circles, Arcs and Polygons for PCBNew and ModEdit
 */

#include <fctsys.h>
#include <macros.h>
#include <confirm.h>
#include <pcb_base_edit_frame.h>
#include <wx/valnum.h>
#include <board_commit.h>
#include <pcb_layer_box_selector.h>
#include <html_messagebox.h>
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <widgets/unit_binder.h>

#include <dialog_graphic_item_properties_base.h>

class DIALOG_GRAPHIC_ITEM_PROPERTIES : public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    PCB_BASE_EDIT_FRAME*  m_parent;
    DRAWSEGMENT*          m_item;
    EDGE_MODULE*          m_moduleItem;

    UNIT_BINDER           m_startX, m_startY;
    UNIT_BINDER           m_endX, m_endY;
    UNIT_BINDER           m_angle;
    UNIT_BINDER           m_thickness;
    UNIT_BINDER           m_bezierCtrl1X, m_bezierCtrl1Y;
    UNIT_BINDER           m_bezierCtrl2X, m_bezierCtrl2Y;

    bool                  m_flipStartEnd;

    wxFloatingPointValidator<double>    m_AngleValidator;
    double                m_AngleValue;

public:
    DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, BOARD_ITEM* aItem );
    ~DIALOG_GRAPHIC_ITEM_PROPERTIES() {};

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnInitDlg( wxInitDialogEvent& event ) override
    {
        // Call the default wxDialog handler of a wxInitDialogEvent
        TransferDataToWindow();

        // Now all widgets have the size fixed, call FinishDialogSettings
        FinishDialogSettings();
    }

    bool Validate() override;
};

DIALOG_GRAPHIC_ITEM_PROPERTIES::DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                                BOARD_ITEM* aItem ):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent ),
    m_startX( aParent, m_startXLabel, m_startXCtrl, m_startXUnits ),
    m_startY( aParent, m_startYLabel, m_startYCtrl, m_startYUnits ),
    m_endX( aParent, m_endXLabel, m_endXCtrl, m_endXUnits ),
    m_endY( aParent, m_endYLabel, m_endYCtrl, m_endYUnits ),
    m_angle( aParent, m_angleLabel, m_angleCtrl, m_angleUnits ),
    m_thickness( aParent, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits, true ),
    m_bezierCtrl1X( aParent, m_BezierPointC1XLabel, m_BezierC1X_Ctrl, m_BezierPointC1XUnit ),
    m_bezierCtrl1Y( aParent, m_BezierPointC1YLabel, m_BezierC1Y_Ctrl, m_BezierPointC1YUnit ),
    m_bezierCtrl2X( aParent, m_BezierPointC2XLabel, m_BezierC2X_Ctrl, m_BezierPointC2XUnit ),
    m_bezierCtrl2Y( aParent, m_BezierPointC2YLabel, m_BezierC2Y_Ctrl, m_BezierPointC2YUnit ),
    m_flipStartEnd( false ),
    m_AngleValidator( 1, &m_AngleValue ),
    m_AngleValue( 0.0 )
{
    m_parent = aParent;
    m_item = dynamic_cast<DRAWSEGMENT*>( aItem );
    m_moduleItem = dynamic_cast<EDGE_MODULE*>( aItem );

    m_angle.SetUnits( DEGREES );
    m_AngleValidator.SetRange( -360.0, 360.0 );
    m_angleCtrl->SetValidator( m_AngleValidator );
    m_AngleValidator.SetWindow( m_angleCtrl );

    // Configure the layers list selector
    if( m_moduleItem )
    {
        LSET forbiddenLayers = LSET::ForbiddenFootprintLayers();

        // If someone went to the trouble of setting the layer in a text editor, then there's
        // very little sense in nagging them about it.
        forbiddenLayers.set( m_item->GetLayer(), false );

        m_LayerSelectionCtrl->SetNotAllowedLayerSet( forbiddenLayers );
    }

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    SetInitialFocus( m_startXCtrl );

    m_StandardButtonsSizerOK->SetDefault();
}


void PCB_BASE_EDIT_FRAME::InstallGraphicItemPropertiesDialog( BOARD_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "InstallGraphicItemPropertiesDialog() error: NULL item" ) );

    DIALOG_GRAPHIC_ITEM_PROPERTIES dlg( this, aItem );
    dlg.ShowModal();
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::TransferDataToWindow()
{
    if( !m_item )
        return false;

    // Only an arc has a angle parameter. So do not show this parameter for other shapes
    if( m_item->GetShape() != S_ARC )
        m_angle.Show( false );

    // Only a Bezeier curve has control points. So do not show these parameters for other shapes
    if( m_item->GetShape() != S_CURVE )
    {
        m_bezierCtrlPt1Label->Show( false );
        m_bezierCtrl1X.Show( false );
        m_bezierCtrl1Y.Show( false );
        m_bezierCtrlPt2Label->Show( false );
        m_bezierCtrl2X.Show( false );
        m_bezierCtrl2Y.Show( false );
    }

    // Change texts according to the segment shape:
    switch( m_item->GetShape() )
    {
    case S_CIRCLE:
        SetTitle( _( "Circle Properties" ) );
        m_startPointLabel->SetLabel( _( "Center" ) );
        m_endPointLabel->SetLabel( _( "Radius" ) );
        m_endY.Show( false );
        break;

    case S_ARC:
        SetTitle( _( "Arc Properties" ) );
        m_startPointLabel->SetLabel( _( "Center" ) );
        m_endPointLabel->SetLabel( _( "Start Point" ) );

        m_AngleValue = m_item->GetAngle() / 10.0;
        break;

    case S_POLYGON:
        SetTitle( _( "Polygon Properties" ) );
        m_sizerLeft->Show( false );
        break;

    case S_SEGMENT:
        if( m_item->GetStart().x == m_item->GetEnd().x )
            m_flipStartEnd = m_item->GetStart().y > m_item->GetEnd().y;
        else
            m_flipStartEnd = m_item->GetStart().x > m_item->GetEnd().x;

        SetTitle( _( "Line Segment Properties" ) );
        break;

    default:
        break;
    }

    if( m_flipStartEnd )
    {
        m_startX.SetValue( m_item->GetEnd().x );
        m_startY.SetValue( m_item->GetEnd().y );
    }
    else
    {
        m_startX.SetValue( m_item->GetStart().x );
        m_startY.SetValue( m_item->GetStart().y );
    }

    if(  m_item->GetShape() == S_CIRCLE )
    {
        m_endX.SetValue( m_item->GetRadius() );
    }
    else if( m_flipStartEnd )
    {
        m_endX.SetValue( m_item->GetStart().x );
        m_endY.SetValue( m_item->GetStart().y );
    }
    else
    {
        m_endX.SetValue( m_item->GetEnd().x );
        m_endY.SetValue( m_item->GetEnd().y );
    }

    // For Bezier curve:
    m_bezierCtrl1X.SetValue( m_item->GetBezControl1().x );
    m_bezierCtrl1Y.SetValue( m_item->GetBezControl1().y );
    m_bezierCtrl2X.SetValue( m_item->GetBezControl2().x );
    m_bezierCtrl2Y.SetValue( m_item->GetBezControl2().y );

    m_thickness.SetValue( m_item->GetWidth() );

    if( m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() ) < 0 )
    {
        wxMessageBox( _( "This item was on a non-existing or forbidden layer.\n"
                         "It has been moved to the first allowed layer. Please fix it." ) );
        m_LayerSelectionCtrl->SetSelection( 0 );
    }

    return DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    LAYER_NUM layer = m_LayerSelectionCtrl->GetLayerSelection();

    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_item );

    if( m_flipStartEnd )
    {
        m_item->SetEndX( m_startX.GetValue() );
        m_item->SetEndY( m_startY.GetValue() );
    }
    else
    {
        m_item->SetStartX( m_startX.GetValue() );
        m_item->SetStartY( m_startY.GetValue() );
    }

    if( m_item->GetShape() == S_CIRCLE )
    {
        m_item->SetEnd( m_item->GetStart() + wxPoint( m_endX.GetValue(), 0 ) );
    }
    else if( m_flipStartEnd )
    {
        m_item->SetStartX( m_endX.GetValue() );
        m_item->SetStartY( m_endY.GetValue() );
    }
    else
    {
        m_item->SetEndX( m_endX.GetValue() );
        m_item->SetEndY( m_endY.GetValue() );
    }

    // For Bezier curve: Set the two control points
    if( m_item->GetShape() == S_CURVE )
    {
        m_item->SetBezControl1( wxPoint( m_bezierCtrl1X.GetValue(), m_bezierCtrl1Y.GetValue() ) );
        m_item->SetBezControl2( wxPoint( m_bezierCtrl2X.GetValue(), m_bezierCtrl2Y.GetValue() ) );
    }

    if( m_moduleItem )
    {   // We are editing a footprint.
        // Init the item coordinates relative to the footprint anchor,
        // that are coordinate references
        m_moduleItem->SetStart0( m_moduleItem->GetStart() );
        m_moduleItem->SetEnd0( m_moduleItem->GetEnd() );

        if( m_moduleItem->GetShape() == S_CURVE )
        {
            m_moduleItem->SetBezier0_C1( wxPoint( m_bezierCtrl1X.GetValue(), m_bezierCtrl1Y.GetValue() ) );
            m_moduleItem->SetBezier0_C2( wxPoint( m_bezierCtrl2X.GetValue(), m_bezierCtrl2Y.GetValue() ) );
        }
    }

    m_item->SetWidth( m_thickness.GetValue() );
    m_item->SetLayer( ToLAYER_ID( layer ) );

    if( m_item->GetShape() == S_ARC )
        m_item->SetAngle( m_AngleValue * 10.0 );

    m_item->RebuildBezierToSegmentsPointsList( m_item->GetWidth() );

    commit.Push( _( "Modify drawing properties" ) );

    m_parent->SetMsgPanel( m_item );

    return true;
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::Validate()
{
    wxArrayString error_msgs;

    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::Validate() )
        return false;

    // Type specific checks.
    switch( m_item->GetShape() )
    {
    case S_ARC:
        // Check angle of arc.
        if( m_angle.GetValue() == 0 )
            error_msgs.Add( _( "The arc angle cannot be zero." ) );
        // Fall through.

    case S_CIRCLE:
        // Check radius.
        if( m_startX.GetValue() == m_endX.GetValue() && m_startY.GetValue() == m_endY.GetValue() )
            error_msgs.Add( _( "The radius must be greater than zero." ) );
        break;

    case S_POLYGON:
        break;

    default:
        // Check start and end are not the same.
        if( m_startX.GetValue() == m_endX.GetValue() && m_startY.GetValue() == m_endY.GetValue() )
            error_msgs.Add( _( "The start and end points cannot be the same." ) );
        break;
    }

    // Check the item thickness. Note the polygon outline thickness is allowed
    // to be set to 0, because if the shape is exactly the polygon, its outline
    // thickness must be 0
    int thickness = m_thickness.GetValue();

    if( m_item->GetShape() == S_POLYGON )
    {
        if( thickness < 0 )
            error_msgs.Add( _( "The polygon outline thickness must be >= 0." ) );
    }
    else
    {
        if( thickness <= 0 )
            error_msgs.Add( _( "The item thickness must be greater than zero." ) );
    }

    if( error_msgs.GetCount() )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error List" ) );
        dlg.ListSet( error_msgs );
        dlg.ShowModal();
    }

    return error_msgs.GetCount() == 0;
}
