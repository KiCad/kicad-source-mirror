/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Edit properties of Lines, Circles, Arcs and Polygons for PCBNew and Footprint Editor
 */

#include <pcb_base_edit_frame.h>
#include <pcb_edit_frame.h>
#include <wx/valnum.h>
#include <board_commit.h>
#include <pcb_layer_box_selector.h>
#include <dialogs/html_messagebox.h>
#include <pcb_shape.h>
#include <fp_shape.h>
#include <confirm.h>
#include <widgets/unit_binder.h>

#include <dialog_graphic_item_properties_base.h>

class DIALOG_GRAPHIC_ITEM_PROPERTIES : public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
private:
    PCB_BASE_EDIT_FRAME*  m_parent;
    PCB_SHAPE*            m_item;
    FP_SHAPE*             m_fp_item;

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
        finishDialogSettings();
    }

    bool Validate() override;

    void onLayer( wxCommandEvent& event ) override;
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
    m_item = dynamic_cast<PCB_SHAPE*>( aItem );
    m_fp_item = dynamic_cast<FP_SHAPE*>( aItem );

    // Configure display origin transforms
    m_startX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_startY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_endX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_endY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_bezierCtrl1X.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_bezierCtrl1Y.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_bezierCtrl2X.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_bezierCtrl2Y.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_angle.SetUnits( EDA_UNITS::DEGREES );
    m_AngleValidator.SetRange( -360.0, 360.0 );
    m_angleCtrl->SetValidator( m_AngleValidator );
    m_AngleValidator.SetWindow( m_angleCtrl );

    // Do not allow locking items in the footprint editor
    m_locked->Show( dynamic_cast<PCB_EDIT_FRAME*>( aParent ) != nullptr );

    // Configure the layers list selector
    if( m_fp_item )
    {
        LSET forbiddenLayers = LSET::ForbiddenFootprintLayers();

        // If someone went to the trouble of setting the layer in a text editor, then there's
        // very little sense in nagging them about it.
        forbiddenLayers.set( m_fp_item->GetLayer(), false );

        m_LayerSelectionCtrl->SetNotAllowedLayerSet( forbiddenLayers );
    }

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    SetInitialFocus( m_startXCtrl );

    m_StandardButtonsSizerOK->SetDefault();
}


void PCB_BASE_EDIT_FRAME::ShowGraphicItemPropertiesDialog( BOARD_ITEM* aItem )
{
    wxCHECK_RET( aItem != NULL, wxT( "ShowGraphicItemPropertiesDialog() error: NULL item" ) );

    DIALOG_GRAPHIC_ITEM_PROPERTIES dlg( this, aItem );
    dlg.ShowQuasiModal();
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
        m_endXLabel->Show( false );
        m_endX.SetCoordType( ORIGIN_TRANSFORMS::NOT_A_COORD );

        m_endY.Show( false );

        m_filledCtrl->Show( true );
        break;

    case S_ARC:
        SetTitle( _( "Arc Properties" ) );
        m_AngleValue = m_item->GetAngle() / 10.0;

        m_filledCtrl->Show( false );
        break;

    case S_POLYGON:
    {
        LSET graphicPolygonsLayers = LSET::AllLayersMask();
        graphicPolygonsLayers.reset( Edge_Cuts ).reset( F_CrtYd ).reset( B_CrtYd );

        SetTitle( _( "Polygon Properties" ) );
        m_sizerLeft->Show( false );

        m_filledCtrl->Show( true );
        m_filledCtrl->Enable( graphicPolygonsLayers.Contains( m_item->GetLayer() ) );

        // Prevent courtyard/edge cuts from being filled
        if( !graphicPolygonsLayers.Contains( m_item->GetLayer() ) )
            m_filledCtrl->SetValue( false );

        break;
    }
    case S_RECT:
        SetTitle( _( "Rectangle Properties" ) );

        m_filledCtrl->Show( true );
        break;

    case S_SEGMENT:
        if( m_item->GetStart().x == m_item->GetEnd().x )
            m_flipStartEnd = m_item->GetStart().y > m_item->GetEnd().y;
        else
            m_flipStartEnd = m_item->GetStart().x > m_item->GetEnd().x;

        SetTitle( _( "Line Segment Properties" ) );

        m_filledCtrl->Show( false );
        break;

    default:
        break;
    }

    if( m_item->GetShape() == S_ARC )
    {
        m_startX.SetValue( m_item->GetArcStart().x );
        m_startY.SetValue( m_item->GetArcStart().y );
    }
    else if( m_flipStartEnd )
    {
        m_startX.SetValue( m_item->GetEnd().x );
        m_startY.SetValue( m_item->GetEnd().y );
    }
    else
    {
        m_startX.SetValue( m_item->GetStart().x );
        m_startY.SetValue( m_item->GetStart().y );
    }

    if( m_item->GetShape() == S_CIRCLE )
    {
        m_endX.SetValue( m_item->GetRadius() );
    }
    else if( m_item->GetShape() == S_ARC )
    {
        m_endX.SetValue( m_item->GetArcEnd().x );
        m_endY.SetValue( m_item->GetArcEnd().y );
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

    m_filledCtrl->SetValue( m_item->IsFilled() );
    m_locked->SetValue( m_item->IsLocked() );
    m_thickness.SetValue( m_item->GetWidth() );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );

    return DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_thickness.Validate( 0, Millimeter2iu( 1000.0 ) ) )
        return false;

    if( m_thickness.GetValue() == 0 && !m_filledCtrl->GetValue() )
    {
        DisplayError( this, _( "Line width may not be 0 for unfilled shapes." ) );
        m_thicknessCtrl->SetFocus();
        return false;
    }

    LAYER_NUM layer = m_LayerSelectionCtrl->GetLayerSelection();

    BOARD_COMMIT commit( m_parent );
    commit.Modify( m_item );

    if( m_item->GetShape() == S_ARC )
    {
        m_item->SetArcStart( wxPoint( m_startX.GetValue(), m_startY.GetValue() ) );
    }
    else if( m_flipStartEnd )
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
    else if( m_item->GetShape() == S_ARC )
    {
        m_item->SetArcEnd( wxPoint( m_endX.GetValue(), m_endY.GetValue() ) );
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

    if( m_item->GetShape() == S_ARC )
    {
        m_item->SetCenter( GetArcCenter( m_item->GetArcStart(), m_item->GetArcEnd(), m_AngleValue ) );
        m_item->SetAngle( m_AngleValue * 10.0, false );
    }

    if( m_fp_item )
    {
        // We are editing a footprint; init the item coordinates relative to the footprint anchor.
        m_fp_item->SetStart0( m_fp_item->GetStart() );
        m_fp_item->SetEnd0( m_fp_item->GetEnd() );

        if( m_fp_item->GetShape() == S_CURVE )
        {
            m_fp_item->SetBezier0_C1( wxPoint( m_bezierCtrl1X.GetValue(), m_bezierCtrl1Y.GetValue() ) );
            m_fp_item->SetBezier0_C2( wxPoint( m_bezierCtrl2X.GetValue(), m_bezierCtrl2Y.GetValue() ) );
        }
    }

    m_item->SetFilled( m_filledCtrl->GetValue() );
    m_item->SetLocked( m_locked->GetValue() );
    m_item->SetWidth( m_thickness.GetValue() );
    m_item->SetLayer( ToLAYER_ID( layer ) );

    m_item->RebuildBezierToSegmentsPointsList( m_item->GetWidth() );

    commit.Push( _( "Modify drawing properties" ) );

    m_parent->UpdateMsgPanel();

    return true;
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::onLayer( wxCommandEvent& event )
{
    if( m_item->GetShape() == S_POLYGON )
    {
        LSET graphicPolygonsLayers = LSET::AllLayersMask();
        graphicPolygonsLayers.reset( Edge_Cuts ).reset( F_CrtYd ).reset( B_CrtYd );

        m_filledCtrl->Enable( graphicPolygonsLayers.Contains(
                ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) ) );

        // Prevent courtyard/edge cuts from being filled
        if( !graphicPolygonsLayers.Contains(
                    ToLAYER_ID( m_LayerSelectionCtrl->GetLayerSelection() ) ) )
            m_filledCtrl->SetValue( false );
    }
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

        KI_FALLTHROUGH;

    case S_CIRCLE:
        // Check radius.
        if( m_startX.GetValue() == m_endX.GetValue() && m_startY.GetValue() == m_endY.GetValue() )
            error_msgs.Add( _( "The radius cannot be zero." ) );
        break;

    case S_RECT:
        // Check for null rect.
        if( m_startX.GetValue() == m_endX.GetValue() && m_startY.GetValue() == m_endY.GetValue() )
            error_msgs.Add( _( "The rectangle cannot be empty." ) );
        break;

    case S_POLYGON:
    case S_SEGMENT:
    case S_CURVE:
        break;

    default:
        wxASSERT_MSG( false, "DIALOG_GRAPHIC_ITEM_PROPERTIES::Validate not implemented for shape"
                             + PCB_SHAPE::ShowShape( m_item->GetShape() ) );
        break;
    }

    if( error_msgs.GetCount() )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error List" ) );
        dlg.ListSet( error_msgs );
        dlg.ShowModal();
    }

    return error_msgs.GetCount() == 0;
}
