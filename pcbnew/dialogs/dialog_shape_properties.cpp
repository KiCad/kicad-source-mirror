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

#include <dialog_shape_properties_base.h>
#include <tools/drawing_tool.h>


class DIALOG_SHAPE_PROPERTIES : public DIALOG_SHAPE_PROPERTIES_BASE
{
public:
    DIALOG_SHAPE_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_SHAPE* aShape );
    ~DIALOG_SHAPE_PROPERTIES() {};

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void onFilledCheckbox( wxCommandEvent& event ) override;

    void onLayerSelection( wxCommandEvent& event ) override;

    bool Validate() override;

    // Show/hide the widgets used in net selection (shown only for copper layers)
    void showHideNetInfo()
    {
        bool isCopper = IsCopperLayer( m_LayerSelectionCtrl->GetLayerSelection() );

        m_netSelector->Show( isCopper );
        m_netLabel->Show( isCopper );
    }


private:
    PCB_BASE_EDIT_FRAME*  m_parent;
    PCB_SHAPE*            m_item;

    UNIT_BINDER           m_startX, m_startY;
    UNIT_BINDER           m_endX, m_endY;
    UNIT_BINDER           m_thickness;
    UNIT_BINDER           m_segmentLength;
    UNIT_BINDER           m_segmentAngle;
    UNIT_BINDER           m_angle;
    UNIT_BINDER           m_rectangleHeight;
    UNIT_BINDER           m_rectangleWidth;
    UNIT_BINDER           m_bezierCtrl1X, m_bezierCtrl1Y;
    UNIT_BINDER           m_bezierCtrl2X, m_bezierCtrl2Y;

    bool                  m_flipStartEnd;
};

DIALOG_SHAPE_PROPERTIES::DIALOG_SHAPE_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent, PCB_SHAPE* aShape ):
    DIALOG_SHAPE_PROPERTIES_BASE( aParent ),
    m_parent( aParent ),
    m_item( aShape ),
    m_startX( aParent, m_startXLabel, m_startXCtrl, m_startXUnits ),
    m_startY( aParent, m_startYLabel, m_startYCtrl, m_startYUnits ),
    m_endX( aParent, m_endXLabel, m_endXCtrl, m_endXUnits ),
    m_endY( aParent, m_endYLabel, m_endYCtrl, m_endYUnits ),
    m_thickness( aParent, m_thicknessLabel, m_thicknessCtrl, m_thicknessUnits ),
    m_segmentLength( aParent, m_segmentLengthLabel, m_segmentLengthCtrl, m_segmentLengthUnits ),
    m_segmentAngle( aParent, m_segmentAngleLabel, m_segmentAngleCtrl, m_segmentAngleUnits ),
    m_angle( aParent, m_angleLabel, m_angleCtrl, m_angleUnits ),
    m_rectangleHeight( aParent, m_rectangleHeightLabel, m_rectangleHeightCtrl, m_rectangleHeightUnits ),
    m_rectangleWidth( aParent, m_rectangleWidthLabel, m_rectangleWidthCtrl, m_rectangleWidthUnits ),
    m_bezierCtrl1X( aParent, m_BezierPointC1XLabel, m_BezierC1X_Ctrl, m_BezierPointC1XUnit ),
    m_bezierCtrl1Y( aParent, m_BezierPointC1YLabel, m_BezierC1Y_Ctrl, m_BezierPointC1YUnit ),
    m_bezierCtrl2X( aParent, m_BezierPointC2XLabel, m_BezierC2X_Ctrl, m_BezierPointC2XUnit ),
    m_bezierCtrl2Y( aParent, m_BezierPointC2YLabel, m_BezierC2Y_Ctrl, m_BezierPointC2YUnit ),
    m_flipStartEnd( false )
{
    SetTitle( wxString::Format( GetTitle(), m_item->GetFriendlyName() ) );
    m_hash_key = TO_UTF8( GetTitle() );

    // Configure display origin transforms
    m_startX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_startY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_endX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_endY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_bezierCtrl1X.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_bezierCtrl1Y.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_bezierCtrl2X.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_bezierCtrl2Y.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_segmentAngle.SetUnits( EDA_UNITS::DEGREES );
    m_segmentAngle.SetPrecision( 4 );

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

    for( const auto& [ lineStyle, lineStyleDesc ] : lineTypeNames )
        m_lineStyleCombo->Append( lineStyleDesc.name, KiBitmapBundle( lineStyleDesc.bitmap ) );

    m_lineStyleCombo->Append( DEFAULT_STYLE );

    m_LayerSelectionCtrl->SetLayersHotkeys( false );
    m_LayerSelectionCtrl->SetBoardFrame( m_parent );
    m_LayerSelectionCtrl->Resync();

    m_netSelector->SetBoard( aParent->GetBoard() );
    m_netSelector->SetNetInfo( &aParent->GetBoard()->GetNetInfo() );

    if( m_parent->GetFrameType() == FRAME_FOOTPRINT_EDITOR )
    {
        m_netLabel->Hide();
        m_netSelector->Hide();
    }
    else
    {
        int net = aShape->GetNetCode();

        if( net >= 0 )
        {
            m_netSelector->SetSelectedNetcode( net );
        }
        else
        {
            m_netSelector->SetIndeterminateString( INDETERMINATE_STATE );
            m_netSelector->SetIndeterminate();
        }
    }

    if( m_item->GetShape() == SHAPE_T::POLY )
        m_sizerStartEnd->Show( false );

    // Only a Bezeier curve has control points. So do not show these parameters for other shapes
    if( m_item->GetShape() != SHAPE_T::BEZIER )
        m_sizerBezier->Show( false );

    // Only a segment has this format
    if( m_item->GetShape() != SHAPE_T::SEGMENT )
    {
        m_segmentLength.Show( false );
        m_segmentAngle.Show( false );
    }

    if( m_item->GetShape() != SHAPE_T::RECTANGLE )
    {
        m_rectangleHeight.Show( false );
        m_rectangleWidth.Show( false );
    }

    // Only an arc has a angle parameter. So do not show this parameter for other shapes
    if( m_item->GetShape() != SHAPE_T::ARC )
        m_angle.Show( false );

    if( m_item->GetShape() == SHAPE_T::ARC || m_item->GetShape() == SHAPE_T::SEGMENT )
        m_filledCtrl->Show( false );

    // Change texts for circles:
    if( m_item->GetShape() == SHAPE_T::CIRCLE )
    {
        m_startPointLabel->SetLabel( _( "Center Point" ) );
        m_endPointLabel->SetLabel( _( "Radius" ) );

        m_endXLabel->Show( false );
        m_endX.SetCoordType( ORIGIN_TRANSFORMS::NOT_A_COORD );
        m_endY.Show( false );
    }

    SetInitialFocus( m_startXCtrl );
    SetupStandardButtons();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


void PCB_BASE_EDIT_FRAME::ShowGraphicItemPropertiesDialog( PCB_SHAPE* aShape )
{
    wxCHECK_RET( aShape, wxT( "ShowGraphicItemPropertiesDialog() error: NULL item" ) );

    DIALOG_SHAPE_PROPERTIES dlg( this, aShape );

    if( dlg.ShowQuasiModal() == wxID_OK )
    {
        if( aShape->IsOnLayer( GetActiveLayer() ) )
        {
            DRAWING_TOOL* drawingTool = m_toolManager->GetTool<DRAWING_TOOL>();
            drawingTool->SetStroke( aShape->GetStroke(), GetActiveLayer() );
        }
    }
}


void DIALOG_SHAPE_PROPERTIES::onLayerSelection( wxCommandEvent& event )
{
    if( m_LayerSelectionCtrl->GetLayerSelection() >= 0 )
    {
        showHideNetInfo();
    }
}


void DIALOG_SHAPE_PROPERTIES::onFilledCheckbox( wxCommandEvent& event )
{
    if( m_filledCtrl->GetValue() )
    {
        m_lineStyleCombo->SetSelection( 0 );
        m_lineStyleLabel->Enable( false );
        m_lineStyleCombo->Enable( false );
    }
    else
    {
        LINE_STYLE style = m_item->GetStroke().GetLineStyle();

        if( style == LINE_STYLE::DEFAULT )
            style = LINE_STYLE::SOLID;

        if( (int) style < (int) lineTypeNames.size() )
            m_lineStyleCombo->SetSelection( (int) style );

        m_lineStyleLabel->Enable( true );
        m_lineStyleCombo->Enable( true );
    }
}

bool DIALOG_SHAPE_PROPERTIES::TransferDataToWindow()
{
    if( !m_item )
        return false;

    if( m_item->GetShape() == SHAPE_T::ARC )
        m_angle.SetAngleValue( m_item->GetArcAngle() );

    if( m_item->GetShape() == SHAPE_T::RECTANGLE )
    {
        m_rectangleHeight.SetValue( m_item->GetRectangleHeight() );
        m_rectangleWidth.SetValue( m_item->GetRectangleWidth() );
    }

    if( m_item->GetShape() == SHAPE_T::SEGMENT )
    {
        if( m_item->GetStart().x == m_item->GetEnd().x )
            m_flipStartEnd = m_item->GetStart().y > m_item->GetEnd().y;
        else
            m_flipStartEnd = m_item->GetStart().x > m_item->GetEnd().x;

        m_segmentLength.SetValue( KiROUND( m_item->GetLength() ) );
        m_segmentAngle.SetAngleValue( m_item->GetSegmentAngle() );
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

    if( m_item->GetShape() == SHAPE_T::BEZIER )
    {
        m_bezierCtrl1X.SetValue( m_item->GetBezierC1().x );
        m_bezierCtrl1Y.SetValue( m_item->GetBezierC1().y );
        m_bezierCtrl2X.SetValue( m_item->GetBezierC2().x );
        m_bezierCtrl2Y.SetValue( m_item->GetBezierC2().y );
    }

    m_filledCtrl->SetValue( m_item->IsFilled() );
    m_locked->SetValue( m_item->IsLocked() );

    m_thickness.SetValue( m_item->GetStroke().GetWidth() );

    int style = static_cast<int>( m_item->GetStroke().GetLineStyle() );

    if( style == -1 )
        m_lineStyleCombo->SetStringSelection( DEFAULT_STYLE );
    else if( style < (int) lineTypeNames.size() )
        m_lineStyleCombo->SetSelection( style );
    else
        wxFAIL_MSG( "Line type not found in the type lookup map" );

    m_LayerSelectionCtrl->SetLayerSelection( m_item->GetLayer() );
    showHideNetInfo();

    return DIALOG_SHAPE_PROPERTIES_BASE::TransferDataToWindow();
}


bool DIALOG_SHAPE_PROPERTIES::TransferDataFromWindow()
{
    if( !DIALOG_SHAPE_PROPERTIES_BASE::TransferDataFromWindow() )
        return false;

    if( !m_item )
        return true;

    int       layer = m_LayerSelectionCtrl->GetLayerSelection();
    VECTOR2I  begin_point = m_item->GetStart();
    VECTOR2I  end_point = m_item->GetEnd();
    int       segment_length = 0;
    EDA_ANGLE segment_angle = EDA_ANGLE( 0, RADIANS_T );
    int       rectangle_height = 0;
    int       rectangle_width = 0;

    BOARD_COMMIT commit( m_parent );

    commit.Modify( m_item );

    if( m_item->GetShape() == SHAPE_T::SEGMENT )
    {
        segment_length = KiROUND( m_item->GetLength() );
        segment_angle = m_item->GetSegmentAngle().Round( 3 );
    }

    if( m_item->GetShape() == SHAPE_T::RECTANGLE )
    {
        rectangle_height = m_item->GetRectangleHeight();
        rectangle_width = m_item->GetRectangleWidth();
    }

    if( m_flipStartEnd && m_item->GetShape() != SHAPE_T::ARC )
    {
        m_item->SetEndX( m_startX.GetIntValue() );
        m_item->SetEndY( m_startY.GetIntValue() );
    }
    else
    {
        m_item->SetStartX( m_startX.GetIntValue() );
        m_item->SetStartY( m_startY.GetIntValue() );
    }

    if( m_item->GetShape() == SHAPE_T::CIRCLE )
    {
        m_item->SetRadius( m_endX.GetIntValue() );
    }
    else if( m_flipStartEnd && m_item->GetShape() != SHAPE_T::ARC )
    {
        m_item->SetStartX( m_endX.GetIntValue() );
        m_item->SetStartY( m_endY.GetIntValue() );
    }
    else
    {
        m_item->SetEndX( m_endX.GetIntValue() );
        m_item->SetEndY( m_endY.GetIntValue() );
    }

    if( m_item->GetShape() == SHAPE_T::SEGMENT )
    {
        bool      change_begin = ( begin_point != m_item->GetStart() );
        bool      change_end = ( end_point != m_item->GetEnd() );
        bool      change_length = ( segment_length != m_segmentLength.GetValue() );
        EDA_ANGLE difference = std::abs( segment_angle - m_segmentAngle.GetAngleValue() );
        bool      change_angle = ( difference >= EDA_ANGLE( 0.00049, DEGREES_T ) );

        if( !( change_begin && change_end ) )
        {
            segment_length = m_segmentLength.GetIntValue();
            segment_angle = m_segmentAngle.GetAngleValue().Round( 3 );

            if( change_length || change_angle )
            {
                if( change_end )
                {
                    m_item->SetStartX( m_item->GetEndX()
                                       - KiROUND( segment_length * segment_angle.Cos() ) );
                    m_item->SetStartY( m_item->GetEndY()
                                       + KiROUND( segment_length * segment_angle.Sin() ) );
                }
                else
                {
                    m_item->SetEndX( m_item->GetStartX()
                                     + KiROUND( segment_length * segment_angle.Cos() ) );
                    m_item->SetEndY( m_item->GetStartY()
                                     - KiROUND( segment_length * segment_angle.Sin() ) );
                }
            }
        }

        if( change_length )
            m_item->SetLength( m_segmentLength.GetIntValue() );
        else
            m_item->SetLength( m_item->GetLength() );

        if( change_angle )
            m_item->SetSegmentAngle( m_segmentAngle.GetAngleValue().Round( 3 ) );
        else
            m_item->SetSegmentAngle( m_item->GetSegmentAngle().Round( 3 ) );
    }

    if( m_item->GetShape() == SHAPE_T::RECTANGLE )
    {
        bool change_begin = ( begin_point != m_item->GetStart() );
        bool change_end = ( end_point != m_item->GetEnd() );
        bool change_height = ( rectangle_height != m_rectangleHeight.GetValue() );
        bool change_width = ( rectangle_width != m_rectangleWidth.GetValue() );

        if( !( change_begin && change_end ) )
        {
           rectangle_height = m_rectangleHeight.GetIntValue();
           rectangle_width = m_rectangleWidth.GetIntValue();

           if( change_height || change_width )
           {
               if( change_end )
               {
                   m_item->SetStartX( m_item->GetEndX() - rectangle_width );
                   m_item->SetStartY( m_item->GetEndY() - rectangle_height );
               }
               else
               {
                   m_item->SetEndX( m_item->GetStartX() + rectangle_width );
                   m_item->SetEndY( m_item->GetStartY() + rectangle_height );
               }
           }
        }

        m_item->SetRectangle( m_rectangleHeight.GetValue(), m_rectangleWidth.GetValue() );
    }


     // For Bezier curve: Set the two control points
    if( m_item->GetShape() == SHAPE_T::BEZIER )
    {
        m_item->SetBezierC1( VECTOR2I( m_bezierCtrl1X.GetIntValue(), m_bezierCtrl1Y.GetIntValue() ) );
        m_item->SetBezierC2( VECTOR2I( m_bezierCtrl2X.GetIntValue(), m_bezierCtrl2Y.GetIntValue() ) );
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

    stroke.SetWidth( m_thickness.GetIntValue() );

    auto it = lineTypeNames.begin();
    std::advance( it, m_lineStyleCombo->GetSelection() );

    if( it == lineTypeNames.end() )
        stroke.SetLineStyle( LINE_STYLE::DEFAULT );
    else
        stroke.SetLineStyle( it->first );

    m_item->SetStroke( stroke );

    m_item->SetLayer( ToLAYER_ID( layer ) );

    m_item->RebuildBezierToSegmentsPointsList( ARC_HIGH_DEF );

    if( m_item->IsOnCopperLayer() )
        m_item->SetNetCode( m_netSelector->GetSelectedNetcode() );
    else
        m_item->SetNetCode( -1 );

    commit.Push( _( "Edit Shape Properties" ) );

    // Notify clients which treat locked and unlocked items differently (ie: POINT_EDITOR)
    if( wasLocked != m_item->IsLocked() )
        m_parent->GetToolManager()->PostEvent( EVENTS::SelectedEvent );

    return true;
}


bool DIALOG_SHAPE_PROPERTIES::Validate()
{
    wxArrayString errors;

    if( !DIALOG_SHAPE_PROPERTIES_BASE::Validate() )
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
            VECTOR2D start( m_startX.GetIntValue(), m_startY.GetIntValue() );
            VECTOR2D end( m_endX.GetIntValue(), m_endY.GetIntValue() );
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

    case SHAPE_T::RECTANGLE:
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
