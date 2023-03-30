/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <dialogs/html_message_box.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <pcb_shape.h>
#include <macros.h>
#include <widgets/unit_binder.h>

#include <dialog_graphic_item_properties_base.h>
#include <tools/drawing_tool.h>


class DIALOG_GRAPHIC_ITEM_PROPERTIES : public DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE
{
public:
    DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_SHAPE* aShape );
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

    void onFilledCheckbox( wxCommandEvent& event ) override;

    bool Validate() override;

private:
    PCB_BASE_EDIT_FRAME*  m_parent;
    PCB_SHAPE*            m_item;

    UNIT_BINDER           m_startX, m_startY;
    UNIT_BINDER           m_endX, m_endY;
    UNIT_BINDER           m_angle;
    UNIT_BINDER           m_thickness;
    UNIT_BINDER           m_bezierCtrl1X, m_bezierCtrl1Y;
    UNIT_BINDER           m_bezierCtrl2X, m_bezierCtrl2Y;

    bool                  m_flipStartEnd;
};

DIALOG_GRAPHIC_ITEM_PROPERTIES::DIALOG_GRAPHIC_ITEM_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                                PCB_SHAPE* aShape ):
    DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE( aParent ),
    m_parent( aParent ),
    m_item( aShape ),
    m_startX( aParent, m_startXLabel, m_startXCtrl, m_startXUnits ),
    m_startY( aParent, m_startYLabel, m_startYCtrl, m_startYUnits ),
    m_endX( aParent, m_endXLabel, m_endXCtrl, m_endXUnits ),
    m_endY( aParent, m_endYLabel, m_endYCtrl, m_endYUnits ),
    m_angle( aParent, m_angleLabel, m_angleCtrl, m_angleUnits ),
    m_thickness( aParent, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits ),
    m_bezierCtrl1X( aParent, m_BezierPointC1XLabel, m_BezierC1X_Ctrl, m_BezierPointC1XUnit ),
    m_bezierCtrl1Y( aParent, m_BezierPointC1YLabel, m_BezierC1Y_Ctrl, m_BezierPointC1YUnit ),
    m_bezierCtrl2X( aParent, m_BezierPointC2XLabel, m_BezierC2X_Ctrl, m_BezierPointC2XUnit ),
    m_bezierCtrl2Y( aParent, m_BezierPointC2YLabel, m_BezierC2Y_Ctrl, m_BezierPointC2YUnit ),
    m_flipStartEnd( false )
{
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

    // Do not allow locking items in the footprint editor
    m_locked->Show( dynamic_cast<PCB_EDIT_FRAME*>( aParent ) != nullptr );

    // Configure the layers list selector
    if( m_parent->GetFrameType() == FRAME_FOOTPRINT_EDITOR )
    {
        LSET forbiddenLayers = LSET::ForbiddenFootprintLayers();

        // If someone went to the trouble of setting the layer in a text editor, then there's
        // very little sense in nagging them about it.
        forbiddenLayers.set( m_item->GetLayer(), false );

        m_LayerSelectionCtrl->SetNotAllowedLayerSet( forbiddenLayers );
    }

    for( const std::pair<const PLOT_DASH_TYPE, lineTypeStruct>& typeEntry : lineTypeNames )
        m_lineStyleCombo->Append( typeEntry.second.name, KiBitmap( typeEntry.second.bitmap ) );

    m_lineStyleCombo->Append( DEFAULT_STYLE );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    SetInitialFocus( m_startXCtrl );

    SetupStandardButtons();
}


void PCB_BASE_EDIT_FRAME::ShowGraphicItemPropertiesDialog( PCB_SHAPE* aShape )
{
    wxCHECK_RET( aShape != NULL, wxT( "ShowGraphicItemPropertiesDialog() error: NULL item" ) );

    DIALOG_GRAPHIC_ITEM_PROPERTIES dlg( this, aShape );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        if( aShape->IsOnLayer( GetActiveLayer(), true ) )
        {
            DRAWING_TOOL* drawingTool = m_toolManager->GetTool<DRAWING_TOOL>();
            drawingTool->SetStroke( aShape->GetStroke(), GetActiveLayer() );
        }
    }
}


void DIALOG_GRAPHIC_ITEM_PROPERTIES::onFilledCheckbox( wxCommandEvent& event )
{
    if( m_filledCtrl->GetValue() )
    {
        m_lineStyleCombo->SetSelection( 0 );
        m_lineStyleLabel->Enable( false );
        m_lineStyleCombo->Enable( false );
    }
    else
    {
        PLOT_DASH_TYPE style = m_item->GetStroke().GetPlotStyle();

        if( style == PLOT_DASH_TYPE::DEFAULT )
            style = PLOT_DASH_TYPE::SOLID;

        if( (int) style < (int) lineTypeNames.size() )
            m_lineStyleCombo->SetSelection( (int) style );

        m_lineStyleLabel->Enable( true );
        m_lineStyleCombo->Enable( true );
    }
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::TransferDataToWindow()
{
    if( !m_item )
        return false;

    // Only an arc has a angle parameter. So do not show this parameter for other shapes
    if( m_item->GetShape() != SHAPE_T::ARC )
        m_angle.Show( false );

    // Only a Bezeier curve has control points. So do not show these parameters for other shapes
    if( m_item->GetShape() != SHAPE_T::BEZIER )
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
    case SHAPE_T::CIRCLE:
        SetTitle( _( "Circle Properties" ) );
        m_startPointLabel->SetLabel( _( "Center Point" ) );

        m_endPointLabel->SetLabel( _( "Radius" ) );
        m_endXLabel->Show( false );
        m_endX.SetCoordType( ORIGIN_TRANSFORMS::NOT_A_COORD );

        m_endY.Show( false );

        m_filledCtrl->Show( true );
        break;

    case SHAPE_T::ARC:
        SetTitle( _( "Arc Properties" ) );
        m_angle.SetAngleValue( m_item->GetArcAngle() );
        m_filledCtrl->Show( false );
        break;

    case SHAPE_T::POLY:
        SetTitle( _( "Polygon Properties" ) );
        m_sizerLeft->Show( false );
        m_filledCtrl->Show( true );
        break;

    case SHAPE_T::RECT:
        SetTitle( _( "Rectangle Properties" ) );

        m_filledCtrl->Show( true );
        break;

    case SHAPE_T::SEGMENT:
        SetTitle( _( "Line Segment Properties" ) );

        if( m_item->GetStart().x == m_item->GetEnd().x )
            m_flipStartEnd = m_item->GetStart().y > m_item->GetEnd().y;
        else
            m_flipStartEnd = m_item->GetStart().x > m_item->GetEnd().x;

        m_filledCtrl->Show( false );
        break;

    case SHAPE_T::BEZIER:
        SetTitle( _( "Curve Properties" ) );
        m_filledCtrl->Show( true );
        break;

    default:
        break;
    }

    if( m_flipStartEnd && m_item->GetShape() != SHAPE_T::ARC )
    {
        m_startX.SetValue( m_item->GetEnd().x );
        m_startY.SetValue( m_item->GetEnd().y );
    }
    else
    {
        m_startX.SetValue( m_item->GetStart().x );
        m_startY.SetValue( m_item->GetStart().y );
    }

    if( m_item->GetShape() == SHAPE_T::CIRCLE )
    {
        m_endX.SetValue( m_item->GetRadius() );
    }
    else if( m_flipStartEnd && m_item->GetShape() != SHAPE_T::ARC )
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
    m_bezierCtrl1X.SetValue( m_item->GetBezierC1().x );
    m_bezierCtrl1Y.SetValue( m_item->GetBezierC1().y );
    m_bezierCtrl2X.SetValue( m_item->GetBezierC2().x );
    m_bezierCtrl2Y.SetValue( m_item->GetBezierC2().y );

    m_filledCtrl->SetValue( m_item->IsFilled() );
    m_locked->SetValue( m_item->IsLocked() );

    m_thickness.SetValue( m_item->GetStroke().GetWidth() );

    int style = static_cast<int>( m_item->GetStroke().GetPlotStyle() );

    if( style == -1 )
        m_lineStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_lineStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );

    return DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_item )
        return true;

    int          layer = m_LayerSelectionCtrl->GetLayerSelection();
    BOARD_COMMIT commit( m_parent );

    commit.Modify( m_item );

    if( m_flipStartEnd && m_item->GetShape() != SHAPE_T::ARC )
    {
        m_item->SetEndX( m_startX.GetValue() );
        m_item->SetEndY( m_startY.GetValue() );
    }
    else
    {
        m_item->SetStartX( m_startX.GetValue() );
        m_item->SetStartY( m_startY.GetValue() );
    }

    if( m_item->GetShape() == SHAPE_T::CIRCLE )
    {
        m_item->SetEnd( m_item->GetStart() + VECTOR2I( m_endX.GetValue(), 0 ) );
    }
    else if( m_flipStartEnd && m_item->GetShape() != SHAPE_T::ARC )
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
    if( m_item->GetShape() == SHAPE_T::BEZIER )
    {
        m_item->SetBezierC1( VECTOR2I( m_bezierCtrl1X.GetValue(), m_bezierCtrl1Y.GetValue() ) );
        m_item->SetBezierC2( VECTOR2I( m_bezierCtrl2X.GetValue(), m_bezierCtrl2Y.GetValue() ) );
    }

    if( m_item->GetShape() == SHAPE_T::ARC )
    {
        VECTOR2D c = CalcArcCenter( m_item->GetStart(), m_item->GetEnd(), m_angle.GetAngleValue() );

        m_item->SetCenter( c );
    }

    bool wasLocked = m_item->IsLocked();

    m_item->SetFilled( m_filledCtrl->GetValue() );
    m_item->SetLocked( m_locked->GetValue() );

    STROKE_PARAMS stroke = m_item->GetStroke();

    stroke.SetWidth( m_thickness.GetValue() );

    auto it = lineTypeNames.begin();
    std::advance( it, m_lineStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    else
        stroke.SetPlotStyle( it->first );

    m_item->SetStroke( stroke );

    m_item->SetLayer( ToLAYER_ID( layer ) );

    m_item->RebuildBezierToSegmentsPointsList( m_item->GetWidth() );

    commit.Push( _( "Modify drawing properties" ) );

    // Notify clients which treat locked and unlocked items differently (ie: POINT_EDITOR)
    if( wasLocked != m_item->IsLocked() )
        m_parent->GetToolManager()->PostEvent( EVENTS::SelectedEvent );

    return true;
}


bool DIALOG_GRAPHIC_ITEM_PROPERTIES::Validate()
{
    wxArrayString errors;

    if( !DIALOG_GRAPHIC_ITEM_PROPERTIES_BASE::Validate() )
        return false;

    // Type specific checks.
    switch( m_item->GetShape() )
    {
    case SHAPE_T::ARC:
        // Check angle of arc.
        if( m_angle.GetAngleValue() == ANGLE_0 )
            errors.Add( _( "Arc angle cannot be zero." ) );

        if( m_startX.GetValue() == m_endX.GetValue() && m_startY.GetValue() == m_endY.GetValue() )
        {
            errors.Add( wxString::Format( _( "Invalid Arc with radius %f and angle %f." ),
                                          0.0, m_angle.GetDoubleValue() ) );
        }
        else
        {
            VECTOR2D start( m_startX.GetValue(), m_startY.GetValue() );
            VECTOR2D end( m_endX.GetValue(), m_endY.GetValue() );
            VECTOR2D center = CalcArcCenter( start, end, m_angle.GetAngleValue() );

            double radius = ( center - start ).EuclideanNorm();
            double max_offset = std::max( std::abs( center.x ) + radius,
                                          std::abs( center.y ) + radius );

            if( max_offset >= ( std::numeric_limits<VECTOR2I::coord_type>::max() / 2.0 )
                    || center == start || center == end )
            {
                errors.Add( wxString::Format( _( "Invalid Arc with radius %f and angle %f." ),
                                              radius, m_angle.GetDoubleValue() ) );
            }
        }

        if( m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero." ) );

        break;

    case SHAPE_T::CIRCLE:
        // Check radius.
        if( m_endX.GetValue() <= 0 )
            errors.Add( _( "Radius must be greater than zero." ) );

        if( !m_filledCtrl->GetValue() && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled circle." ) );

        break;

    case SHAPE_T::RECT:
        // Check for null rect.
        if( m_startX.GetValue() == m_endX.GetValue() && m_startY.GetValue() == m_endY.GetValue() )
            errors.Add( _( "Rectangle cannot be empty." ) );

        if( !m_filledCtrl->GetValue() && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled rectangle." ) );

        break;

    case SHAPE_T::POLY:
        if( !m_filledCtrl->GetValue() && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled polygon." ) );

        break;

    case SHAPE_T::SEGMENT:
        if( m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero." ) );

        break;

    case SHAPE_T::BEZIER:
        if( !m_filledCtrl->GetValue() && m_thickness.GetValue() <= 0 )
            errors.Add( _( "Line width must be greater than zero for an unfilled curve." ) );

        break;

    default:
        UNIMPLEMENTED_FOR( m_item->SHAPE_T_asString() );
        break;
    }

    if( errors.GetCount() )
    {
        HTML_MESSAGE_BOX dlg( this, _( "Error List" ) );
        dlg.ListSet( errors );
        dlg.ShowModal();
    }

    return errors.GetCount() == 0;
}
