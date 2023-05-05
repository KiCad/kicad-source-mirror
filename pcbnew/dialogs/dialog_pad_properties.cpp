/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <convert_basic_shapes_to_polygon.h> // for enum RECT_CHAMFER_POSITIONS definition
#include <geometry/shape_segment.h>
#include <dialog_pad_properties.h>
#include <dialogs/html_message_box.h>
#include <macros.h>
#include <pad.h>
#include <pcb_base_frame.h>
#include <footprint_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <view/view_controls.h>
#include <widgets/net_selector.h>
#include <tool/tool_manager.h>
#include <tools/pad_tool.h>
#include <advanced_config.h>    // for pad property feature management
#include <wx/choicdlg.h>

#include <dialog_pad_primitives_properties.h>

// list of pad shapes, ordered like the pad shape wxChoice in dialog.
static PAD_SHAPE code_shape[] =
{
    PAD_SHAPE::CIRCLE,
    PAD_SHAPE::OVAL,
    PAD_SHAPE::RECT,
    PAD_SHAPE::TRAPEZOID,
    PAD_SHAPE::ROUNDRECT,
    PAD_SHAPE::CHAMFERED_RECT,
    PAD_SHAPE::CHAMFERED_RECT,  // choice = CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT
    PAD_SHAPE::CUSTOM,          // choice = CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR
    PAD_SHAPE::CUSTOM           // choice = PAD_SHAPE::CUSTOM_RECT_ANCHOR
};


// the ordered index of the pad shape wxChoice in dialog.
// keep it consistent with code_shape[] and dialog strings
enum CODE_CHOICE
{
    CHOICE_SHAPE_CIRCLE = 0,
    CHOICE_SHAPE_OVAL,
    CHOICE_SHAPE_RECT,
    CHOICE_SHAPE_TRAPEZOID,
    CHOICE_SHAPE_ROUNDRECT,
    CHOICE_SHAPE_CHAMFERED_RECT,
    CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT,
    CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR,
    CHOICE_SHAPE_CUSTOM_RECT_ANCHOR
};


static PAD_ATTRIB code_type[] =
{
    PAD_ATTRIB::PTH,
    PAD_ATTRIB::SMD,
    PAD_ATTRIB::CONN,
    PAD_ATTRIB::NPTH,
    PAD_ATTRIB::SMD                  // Aperture pad :type SMD with no copper layers,
                                    // only on tech layers (usually only on paste layer
};


// These define have the same value as the m_PadType wxChoice GetSelected() return value
#define PTH_DLG_TYPE 0
#define SMD_DLG_TYPE 1
#define CONN_DLG_TYPE 2
#define NPTH_DLG_TYPE 3
#define APERTURE_DLG_TYPE 4


void PCB_BASE_FRAME::ShowPadPropertiesDialog( PAD* aPad )
{
    DIALOG_PAD_PROPERTIES dlg( this, aPad );

    dlg.ShowQuasiModal();
}


DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, PAD* aPad ) :
        DIALOG_PAD_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_canUpdate( false ),
        m_posX( aParent, m_posXLabel, m_posXCtrl, m_posXUnits ),
        m_posY( aParent, m_posYLabel, m_posYCtrl, m_posYUnits ),
        m_sizeX( aParent, m_sizeXLabel, m_sizeXCtrl, m_sizeXUnits ),
        m_sizeY( aParent, m_sizeYLabel, m_sizeYCtrl, m_sizeYUnits ),
        m_offsetX( aParent, m_offsetXLabel, m_offsetXCtrl, m_offsetXUnits ),
        m_offsetY( aParent, m_offsetYLabel, m_offsetYCtrl, m_offsetYUnits ),
        m_padToDie( aParent, m_padToDieLabel, m_padToDieCtrl, m_padToDieUnits ),
        m_trapDelta( aParent, m_trapDeltaLabel, m_trapDeltaCtrl, m_trapDeltaUnits ),
        m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
        m_cornerRatio( aParent, m_cornerRatioLabel, m_cornerRatioCtrl, m_cornerRatioUnits ),
        m_chamferRatio( aParent, m_chamferRatioLabel, m_chamferRatioCtrl, m_chamferRatioUnits ),
        m_mixedCornerRatio( aParent, m_mixedCornerRatioLabel, m_mixedCornerRatioCtrl,
                            m_mixedCornerRatioUnits ),
        m_mixedChamferRatio( aParent, m_mixedChamferRatioLabel, m_mixedChamferRatioCtrl,
                             m_mixedChamferRatioUnits ),
        m_holeX( aParent, m_holeXLabel, m_holeXCtrl, m_holeXUnits ),
        m_holeY( aParent, m_holeYLabel, m_holeYCtrl, m_holeYUnits ),
        m_clearance( aParent, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits ),
        m_maskMargin( aParent, m_maskMarginLabel, m_maskMarginCtrl, m_maskMarginUnits ),
        m_pasteMargin( aParent, m_pasteMarginLabel, m_pasteMarginCtrl, m_pasteMarginUnits ),
        m_pasteMarginRatio( aParent, m_pasteMarginRatioLabel, m_pasteMarginRatioCtrl,
                            m_pasteMarginRatioUnits ),
        m_thermalGap( aParent, m_thermalGapLabel, m_thermalGapCtrl, m_thermalGapUnits ),
        m_spokeWidth( aParent, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits ),
        m_spokeAngle( aParent, m_spokeAngleLabel, m_spokeAngleCtrl, m_spokeAngleUnits ),
        m_pad_orientation( aParent, m_PadOrientText, m_cb_padrotation, m_orientationUnits )
{
    SetName( PAD_PROPERTIES_DLG_NAME );
    m_isFpEditor = dynamic_cast<FOOTPRINT_EDIT_FRAME*>( aParent ) != nullptr;

    m_currentPad = aPad;        // aPad can be NULL, if the dialog is called
                                // from the footprint editor to set default pad setup

    m_board      = m_parent->GetBoard();

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_padNetSelector->SetBoard( m_board );
    m_padNetSelector->SetNetInfo( &m_board->GetNetInfo() );

    m_cbShowPadOutline->SetValue( m_sketchPreview );

    m_FlippedWarningIcon->SetBitmap( KiBitmap( BITMAPS::dialog_warning ) );
    m_nonCopperWarningIcon->SetBitmap( KiBitmap( BITMAPS::dialog_warning ) );

    m_masterPad = m_parent->GetDesignSettings().m_Pad_Master.get();
    m_previewPad = new PAD( (FOOTPRINT*) nullptr );

    if( aPad )
    {
        SetTitle( _( "Pad Properties" ) );

        *m_previewPad = *aPad;
        m_previewPad->ClearFlags( SELECTED|BRIGHTENED );
    }
    else
    {
        SetTitle( _( "Default Pad Properties for Add Pad Tool" ) );

        *m_previewPad = *m_masterPad;
    }

    if( m_isFpEditor )
    {
        m_padNetLabel->Show( false );
        m_padNetSelector->Show( false );
    }

    m_FlippedWarningSizer->Show( false );

    // Pad needs to have a parent for painting; use the parent board for its design settings
    if( !m_previewPad->GetParent() )
        m_previewPad->SetParent( m_board );

    m_cornerRatio.SetUnits( EDA_UNITS::PERCENT );
    m_chamferRatio.SetUnits( EDA_UNITS::PERCENT );
    m_mixedCornerRatio.SetUnits( EDA_UNITS::PERCENT );
    m_mixedChamferRatio.SetUnits( EDA_UNITS::PERCENT );
    m_pad_orientation.SetUnits( EDA_UNITS::DEGREES );
    m_pad_orientation.SetPrecision( 3 );

    m_spokeAngle.SetUnits( EDA_UNITS::DEGREES );
    m_spokeAngle.SetPrecision( 3 );

    m_pasteMargin.SetNegativeZero();

    m_pasteMarginRatio.SetUnits( EDA_UNITS::PERCENT );
    m_pasteMarginRatio.SetNegativeZero();

    initValues();

    wxFont infoFont = KIUI::GetInfoFont( this );
    m_copperLayersLabel->SetFont( infoFont );
    m_techLayersLabel->SetFont( infoFont );
    m_parentInfo->SetFont( infoFont );

    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
    m_nonCopperNote->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );
    m_staticTextPrimitiveListWarning->SetFont( infoFont );

    updateHoleControls();
    updatePadSizeControls();

    // Usually, TransferDataToWindow is called by OnInitDialog
    // calling it here fixes all widget sizes so FinishDialogSettings can safely fix minsizes
    TransferDataToWindow();

    // Initialize canvas to be able to display the dummy pad:
    prepareCanvas();

    SetInitialFocus( m_padNumCtrl );
    SetupStandardButtons();
    m_canUpdate = true;

    m_padNetSelector->Connect( NET_SELECTED,
                               wxCommandEventHandler( DIALOG_PAD_PROPERTIES::OnValuesChanged ),
                               nullptr, this );

    if( m_padType->GetSelection() != PTH_DLG_TYPE && m_padType->GetSelection() != NPTH_DLG_TYPE )
    {
        m_gbSizerHole->Show( false );
        m_staticline6->Show( false );
    }

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();

    // Update widgets
    wxUpdateUIEvent dummyUI;
    OnUpdateUI( dummyUI );

    // Post a dummy size event to force the pad preview panel to update the
    // view: actual size, best zoom ... after the frame is shown
    PostSizeEvent();
}


DIALOG_PAD_PROPERTIES::~DIALOG_PAD_PROPERTIES()
{
    m_padNetSelector->Disconnect( NET_SELECTED,
                                  wxCommandEventHandler( DIALOG_PAD_PROPERTIES::OnValuesChanged ),
                                  nullptr, this );

    delete m_previewPad;
    delete m_axisOrigin;
}


// Store the pad draw option during a session.
bool DIALOG_PAD_PROPERTIES::m_sketchPreview = false;


void DIALOG_PAD_PROPERTIES::OnInitDialog( wxInitDialogEvent& event )
{
    m_selectedColor = COLOR4D( 1.0, 1.0, 1.0, 0.7 );

    // Needed on some WM to be sure the pad is redrawn according to the final size
    // of the canvas, with the right zoom factor
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnCancel( wxCommandEvent& event )
{
    // Mandatory to avoid m_panelShowPadGal trying to draw something
    // in a non valid context during closing process:
    m_padPreviewGAL->StopDrawing();

    // Now call default handler for wxID_CANCEL command event
    event.Skip();
}


void DIALOG_PAD_PROPERTIES::enablePrimitivePage( bool aEnable )
{
    // Enable or disable the widgets in page managing custom shape primitives
	m_listCtrlPrimitives->Enable( aEnable );
	m_buttonDel->Enable( aEnable );
	m_buttonEditShape->Enable( aEnable );
	m_buttonAddShape->Enable( aEnable );
	m_buttonDup->Enable( aEnable );
	m_buttonGeometry->Enable( aEnable );
}


void DIALOG_PAD_PROPERTIES::prepareCanvas()
{
    // Initialize the canvas to display the pad
    m_padPreviewGAL = new PCB_DRAW_PANEL_GAL( m_boardViewPanel, -1, wxDefaultPosition,
                                              wxDefaultSize,
                                              m_parent->GetGalDisplayOptions(),
                                              m_parent->GetCanvas()->GetBackend() );

    m_padPreviewSizer->Add( m_padPreviewGAL, 12, wxEXPAND | wxALL, 5 );

    // Show the X and Y axis. It is useful because pad shape can have an offset
    // or be a complex shape.
    KIGFX::COLOR4D axis_color = LIGHTBLUE;

    m_axisOrigin = new KIGFX::ORIGIN_VIEWITEM( axis_color, KIGFX::ORIGIN_VIEWITEM::CROSS,
                                               pcbIUScale.mmToIU( 0.2 ),
                                               VECTOR2D( m_previewPad->GetPosition() ) );
    m_axisOrigin->SetDrawAtZero( true );

    m_padPreviewGAL->UpdateColors();
    m_padPreviewGAL->SetStealsFocus( false );
    m_padPreviewGAL->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );

    KIGFX::VIEW_CONTROLS* parentViewControls = m_parent->GetCanvas()->GetViewControls();
    m_padPreviewGAL->GetViewControls()->ApplySettings( parentViewControls->GetSettings() );

    m_padPreviewGAL->Show();

    KIGFX::VIEW* view = m_padPreviewGAL->GetView();

    // fix the pad render mode (filled/not filled)
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    settings->m_ForcePadSketchModeOn = m_cbShowPadOutline->IsChecked();
    settings->SetHighContrast( false );
    settings->m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;

    // gives a non null grid size (0.001mm) because GAL layer does not like a 0 size grid:
    double gridsize = 0.001 * pcbIUScale.IU_PER_MM;
    view->GetGAL()->SetGridSize( VECTOR2D( gridsize, gridsize ) );

    // And do not show the grid:
    view->GetGAL()->SetGridVisibility( false );
    view->Add( m_previewPad );
    view->Add( m_axisOrigin );

    m_padPreviewGAL->StartDrawing();
    Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PROPERTIES::OnResize ) );
}


void DIALOG_PAD_PROPERTIES::updateRoundRectCornerValues()
{
    // Note: use ChangeValue() to avoid generating a wxEVT_TEXT event
    m_cornerRadius.ChangeValue( m_previewPad->GetRoundRectCornerRadius() );

    m_cornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio() * 100.0 );
    m_mixedCornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio() * 100.0 );

    m_chamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio() * 100.0 );
    m_mixedChamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio() * 100.0 );
}


void DIALOG_PAD_PROPERTIES::onCornerRadiusChange( wxCommandEvent& event )
{
    if( m_previewPad->GetShape() != PAD_SHAPE::ROUNDRECT
            && m_previewPad->GetShape() != PAD_SHAPE::CHAMFERED_RECT )
    {
        return;
    }

    if( m_cornerRadius.GetValue() < 0 )
        m_cornerRadiusCtrl->ChangeValue( "0" );

    if( transferDataToPad( m_previewPad ) )
    {
        m_previewPad->SetRoundRectCornerRadius( m_cornerRadius.GetValue() );

        m_cornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio() * 100.0 );
        m_mixedCornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio() * 100.0 );

        redraw();
    }
}


void DIALOG_PAD_PROPERTIES::onCornerSizePercentChange( wxCommandEvent& event )
{
    if( m_previewPad->GetShape() != PAD_SHAPE::ROUNDRECT
            && m_previewPad->GetShape() != PAD_SHAPE::CHAMFERED_RECT )
    {
        return;
    }

    wxObject* ctrl = event.GetEventObject();
    wxString  value = event.GetString();
    bool      changed = false;

    if( ctrl == m_cornerRatioCtrl || ctrl == m_mixedCornerRatioCtrl )
    {
        double ratioPercent;

        if( value.ToDouble( &ratioPercent ) )
        {
            // Clamp ratioPercent to acceptable value (0.0 to 50.0)
            if( ratioPercent < 0.0 )
            {
                m_cornerRatio.SetDoubleValue( 0.0 );
                m_mixedCornerRatio.SetDoubleValue( 0.0 );
            }
            else if( ratioPercent > 50.0 )
            {
                m_cornerRatio.SetDoubleValue( 50.0 );
                m_mixedCornerRatio.SetDoubleValue( 50.0 );
            }

            if( ctrl == m_cornerRatioCtrl )
                m_mixedCornerRatioCtrl->ChangeValue( value );
            else
                m_cornerRatioCtrl->ChangeValue( value );

            changed = true;
        }
    }
    else if( ctrl == m_chamferRatioCtrl || ctrl == m_mixedChamferRatioCtrl )
    {
        double ratioPercent;

        if( value.ToDouble( &ratioPercent ) )
        {
            // Clamp ratioPercent to acceptable value (0.0 to 50.0)
            if( ratioPercent < 0.0 )
            {
                m_chamferRatio.SetDoubleValue( 0.0 );
                m_mixedChamferRatio.SetDoubleValue( 0.0 );
            }
            else if( ratioPercent > 50.0 )
            {
                m_chamferRatio.SetDoubleValue( 50.0 );
                m_mixedChamferRatio.SetDoubleValue( 50.0 );
            }

            if( ctrl == m_chamferRatioCtrl )
                m_mixedChamferRatioCtrl->ChangeValue( value );
            else
                m_chamferRatioCtrl->ChangeValue( value );

            changed = true;
        }
    }

    if( changed && transferDataToPad( m_previewPad ) )
        m_cornerRadius.ChangeValue( m_previewPad->GetRoundRectCornerRadius() );

    redraw();
}


void DIALOG_PAD_PROPERTIES::initValues()
{
    wxString    msg;

    // Disable pad net name wxTextCtrl if the caller is the footprint editor
    // because nets are living only in the board managed by the board editor
    m_canEditNetName = m_parent->IsType( FRAME_PCB_EDITOR );

    m_layerFrontAdhesive->SetLabel( m_board->GetLayerName( F_Adhes ) );
    m_layerBackAdhesive->SetLabel( m_board->GetLayerName( B_Adhes ) );
    m_layerFrontPaste->SetLabel( m_board->GetLayerName( F_Paste ) );
    m_layerBackPaste->SetLabel( m_board->GetLayerName( B_Paste ) );
    m_layerFrontSilk->SetLabel( m_board->GetLayerName( F_SilkS ) );
    m_layerBackSilk->SetLabel( m_board->GetLayerName( B_SilkS ) );
    m_layerFrontMask->SetLabel( m_board->GetLayerName( F_Mask ) );
    m_layerBackMask->SetLabel( m_board->GetLayerName( B_Mask ) );
    m_layerECO1->SetLabel( m_board->GetLayerName( Eco1_User ) );
    m_layerECO2->SetLabel( m_board->GetLayerName( Eco2_User ) );
    m_layerUserDwgs->SetLabel( m_board->GetLayerName( Dwgs_User ) );

    if( m_currentPad )
    {
        if( FOOTPRINT* footprint = m_currentPad->GetParent() )
        {
            VECTOR2I relPos = m_currentPad->GetFPRelativePosition();

            if( footprint->IsFlipped() )
            {
                // flip pad (up/down) around its position
                m_previewPad->Flip( m_previewPad->GetPosition(), false );
                relPos.y = - relPos.y;
            }

            m_previewPad->SetPosition( relPos );
            m_previewPad->SetOrientation( m_currentPad->GetFPRelativeOrientation() );

            // Display parent footprint info
            msg.Printf( _("Footprint %s (%s), %s, rotated %g deg"),
                         footprint->Reference().GetShownText( false ),
                         footprint->Value().GetShownText( false ),
                         footprint->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" ),
                         footprint->GetOrientation().AsDegrees() );

            m_FlippedWarningSizer->Show( footprint->IsFlipped() );
            m_parentInfo->SetLabel( msg );
        }
    }

    m_primitives = m_previewPad->GetPrimitives();

    if( m_currentPad )
    {
        m_padNumCtrl->SetValue( m_previewPad->GetNumber() );
    }
    else
    {
        PAD_TOOL* padTool = m_parent->GetToolManager()->GetTool<PAD_TOOL>();
        m_padNumCtrl->SetValue( padTool->GetLastPadNumber() );
    }

    m_padNetSelector->SetSelectedNetcode( m_previewPad->GetNetCode() );

    // Display current pad parameters units:
    m_posX.ChangeValue( m_previewPad->GetPosition().x );
    m_posY.ChangeValue( m_previewPad->GetPosition().y );

    m_holeX.ChangeValue( m_previewPad->GetDrillSize().x );
    m_holeY.ChangeValue( m_previewPad->GetDrillSize().y );

    m_sizeX.ChangeValue( m_previewPad->GetSize().x );
    m_sizeY.ChangeValue( m_previewPad->GetSize().y );

    m_offsetShapeOpt->SetValue( m_previewPad->GetOffset() != VECTOR2I() );
    m_offsetX.ChangeValue( m_previewPad->GetOffset().x );
    m_offsetY.ChangeValue( m_previewPad->GetOffset().y );

    if( m_previewPad->GetDelta().x )
    {
        m_trapDelta.ChangeValue( m_previewPad->GetDelta().x );
        m_trapAxisCtrl->SetSelection( 0 );
    }
    else
    {
        m_trapDelta.ChangeValue( m_previewPad->GetDelta().y );
        m_trapAxisCtrl->SetSelection( 1 );
    }

    m_padToDieOpt->SetValue( m_previewPad->GetPadToDieLength() != 0 );
    m_padToDie.ChangeValue( m_previewPad->GetPadToDieLength() );

    m_clearance.ChangeValue( m_previewPad->GetLocalClearance() );
    m_maskMargin.ChangeValue( m_previewPad->GetLocalSolderMaskMargin() );
    m_spokeWidth.ChangeValue( m_previewPad->GetThermalSpokeWidth() );
    m_spokeAngle.ChangeAngleValue( m_previewPad->GetThermalSpokeAngle() );
    m_thermalGap.ChangeValue( m_previewPad->GetThermalGap() );
    m_pasteMargin.ChangeValue( m_previewPad->GetLocalSolderPasteMargin() );
    m_pasteMarginRatio.ChangeDoubleValue( m_previewPad->GetLocalSolderPasteMarginRatio() * 100.0 );
    m_pad_orientation.ChangeAngleValue( m_previewPad->GetOrientation() );

    switch( m_previewPad->GetZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    if( m_previewPad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
        m_ZoneCustomPadShape->SetSelection( 1 );
    else
        m_ZoneCustomPadShape->SetSelection( 0 );

    switch( m_previewPad->GetShape() )
    {
    default:
    case PAD_SHAPE::CIRCLE:    m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CIRCLE );    break;
    case PAD_SHAPE::OVAL:      m_PadShapeSelector->SetSelection( CHOICE_SHAPE_OVAL );      break;
    case PAD_SHAPE::RECT:      m_PadShapeSelector->SetSelection( CHOICE_SHAPE_RECT );      break;
    case PAD_SHAPE::TRAPEZOID: m_PadShapeSelector->SetSelection( CHOICE_SHAPE_TRAPEZOID ); break;
    case PAD_SHAPE::ROUNDRECT: m_PadShapeSelector->SetSelection( CHOICE_SHAPE_ROUNDRECT ); break;

    case PAD_SHAPE::CHAMFERED_RECT:
        if( m_previewPad->GetRoundRectRadiusRatio() > 0.0 )
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT );
        else
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CHAMFERED_RECT );
        break;

    case PAD_SHAPE::CUSTOM:
        if( m_previewPad->GetAnchorPadShape() == PAD_SHAPE::RECT )
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CUSTOM_RECT_ANCHOR );
        else
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );
        break;
    }

    m_cbTopLeft->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_TOP_LEFT) );
    m_cbTopLeft1->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_TOP_LEFT) );
    m_cbTopRight->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_TOP_RIGHT) );
    m_cbTopRight1->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_TOP_RIGHT) );
    m_cbBottomLeft->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_LEFT) );
    m_cbBottomLeft1->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_LEFT) );
    m_cbBottomRight->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_RIGHT) );
    m_cbBottomRight1->SetValue( ( m_previewPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_RIGHT) );

    updateRoundRectCornerValues();

    enablePrimitivePage( PAD_SHAPE::CUSTOM == m_previewPad->GetShape() );

    // Type of pad selection
    bool aperture =
            m_previewPad->GetAttribute() == PAD_ATTRIB::SMD && m_previewPad->IsAperturePad();

    if( aperture )
    {
        m_padType->SetSelection( APERTURE_DLG_TYPE );
    }
    else
    {
        switch( m_previewPad->GetAttribute() )
        {
        case PAD_ATTRIB::PTH:    m_padType->SetSelection( PTH_DLG_TYPE ); break;
        case PAD_ATTRIB::SMD:    m_padType->SetSelection( SMD_DLG_TYPE ); break;
        case PAD_ATTRIB::CONN:   m_padType->SetSelection( CONN_DLG_TYPE ); break;
        case PAD_ATTRIB::NPTH:   m_padType->SetSelection( NPTH_DLG_TYPE ); break;
        }
    }

    switch( m_previewPad->GetProperty() )
    {
    case PAD_PROP::NONE:             m_choiceFabProperty->SetSelection( 0 ); break;
    case PAD_PROP::BGA:              m_choiceFabProperty->SetSelection( 1 ); break;
    case PAD_PROP::FIDUCIAL_LOCAL:   m_choiceFabProperty->SetSelection( 2 ); break;
    case PAD_PROP::FIDUCIAL_GLBL:    m_choiceFabProperty->SetSelection( 3 ); break;
    case PAD_PROP::TESTPOINT:        m_choiceFabProperty->SetSelection( 4 ); break;
    case PAD_PROP::HEATSINK:         m_choiceFabProperty->SetSelection( 5 ); break;
    case PAD_PROP::CASTELLATED:      m_choiceFabProperty->SetSelection( 6 ); break;
    }

    // Ensure the pad property is compatible with the pad type
    if( m_previewPad->GetAttribute() == PAD_ATTRIB::NPTH )
    {
        m_choiceFabProperty->SetSelection( 0 );
        m_choiceFabProperty->Enable( false );
    }

    if( m_previewPad->GetDrillShape() != PAD_DRILL_SHAPE_OBLONG )
        m_holeShapeCtrl->SetSelection( 0 );
    else
        m_holeShapeCtrl->SetSelection( 1 );

    updatePadLayersList( m_previewPad->GetLayerSet(), m_previewPad->GetRemoveUnconnected(),
                         m_previewPad->GetKeepTopBottom() );

    // Update some dialog widgets state (Enable/disable options):
    wxCommandEvent cmd_event;
    OnPadShapeSelection( cmd_event );
    OnOffsetCheckbox( cmd_event );

    // Update basic shapes list
    displayPrimitivesList();
}

// A small helper function, to display coordinates:
static wxString formatCoord( EDA_UNITS aUnits, VECTOR2I aCoord )
{
    return wxString::Format( wxT( "(X:%s Y:%s)" ),
                             EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, aUnits, aCoord.x ),
                             EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, aUnits, aCoord.y ) );
}

void DIALOG_PAD_PROPERTIES::displayPrimitivesList()
{
    m_listCtrlPrimitives->ClearAll();

    wxListItem itemCol;
    itemCol.SetImage(-1);

    for( int ii = 0; ii < 5; ++ii )
        m_listCtrlPrimitives->InsertColumn(ii, itemCol);

    wxString bs_info[5];

    for( unsigned ii = 0; ii < m_primitives.size(); ++ii )
    {
        const std::shared_ptr<PCB_SHAPE>& primitive = m_primitives[ii];

        for( wxString& s : bs_info )
            s.Empty();

        bs_info[4] = _( "width" ) + wxS( " " )+ EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, m_units,
                                                                      primitive->GetWidth() );

        switch( primitive->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            bs_info[0] = _( "Segment" );
            bs_info[1] = _( "from" ) + wxS( " " ) + formatCoord( m_units, primitive->GetStart() );
            bs_info[2] = _( "to" ) + wxS( " " ) +  formatCoord( m_units, primitive->GetEnd() );
            break;

        case SHAPE_T::BEZIER:
            bs_info[0] = _( "Bezier" );
            bs_info[1] = _( "from" ) + wxS( " " ) + formatCoord( m_units, primitive->GetStart() );
            bs_info[2] = _( "to" ) + wxS( " " ) +  formatCoord( m_units, primitive->GetEnd() );
            break;

        case SHAPE_T::ARC:
            bs_info[0] = _( "Arc" );
            bs_info[1] = _( "center" ) + wxS( " " ) + formatCoord( m_units, primitive->GetCenter() );
            bs_info[2] = _( "start" ) + wxS( " " ) + formatCoord( m_units, primitive->GetStart() );
            bs_info[3] = _( "angle" ) + wxS( " " ) + EDA_UNIT_UTILS::FormatAngle( primitive->GetArcAngle() );
            break;

        case SHAPE_T::CIRCLE:
            if( primitive->GetWidth() )
                bs_info[0] = _( "Ring" );
            else
                bs_info[0] = _( "Circle" );

            bs_info[1] = _( "at" ) + wxS( " " ) + formatCoord( m_units, primitive->GetStart() );
            bs_info[2] = _( "radius" ) + wxS( " " ) + EDA_UNIT_UTILS::UI::MessageTextFromValue( pcbIUScale, m_units,
                                                                            primitive->GetRadius() );
            break;

        case SHAPE_T::POLY:
            bs_info[0] = _( "Polygon" );
            bs_info[1] = wxString::Format( _( "corners count %d" ),
                                           primitive->GetPolyShape().Outline( 0 ).PointCount() );
            break;

        case SHAPE_T::RECT:
            if( primitive->IsAnnotationProxy() )
                bs_info[0] = _( "Number box" );
            else
                bs_info[0] = _( "Rectangle" );

            bs_info[1] = _( "from" ) + wxS( " " ) + formatCoord( m_units, primitive->GetStart() );
            bs_info[2] = _( "to" ) + wxS( " " ) +  formatCoord( m_units, primitive->GetEnd() );
            break;

        default:
            bs_info[0] = _( "Unknown primitive" );
            break;
        }

        long tmp = m_listCtrlPrimitives->InsertItem( ii, bs_info[0] );
        m_listCtrlPrimitives->SetItemData( tmp, ii );

        for( int jj = 0, col = 0; jj < 5; ++jj )
            m_listCtrlPrimitives->SetItem( tmp, col++, bs_info[jj] );
    }

    // Now columns are filled, ensure correct width of columns
    for( unsigned ii = 0; ii < 5; ++ii )
        m_listCtrlPrimitives->SetColumnWidth( ii, wxLIST_AUTOSIZE );
}


void DIALOG_PAD_PROPERTIES::OnResize( wxSizeEvent& event )
{
    redraw();
    event.Skip();
}


void DIALOG_PAD_PROPERTIES::onChangePadMode( wxCommandEvent& event )
{
    m_sketchPreview = m_cbShowPadOutline->GetValue();

    KIGFX::VIEW* view = m_padPreviewGAL->GetView();

    // fix the pad render mode (filled/not filled)
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    settings->m_ForcePadSketchModeOn = m_cbShowPadOutline->IsChecked();
    settings->SetHighContrast( false );
    settings->m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;

    redraw();
}


void DIALOG_PAD_PROPERTIES::OnPadShapeSelection( wxCommandEvent& event )
{
    switch( m_PadShapeSelector->GetSelection() )
    {
    case CHOICE_SHAPE_CIRCLE:
    case CHOICE_SHAPE_OVAL:
    case CHOICE_SHAPE_RECT:
        m_shapePropsBook->SetSelection( 0 );
        break;

    case CHOICE_SHAPE_TRAPEZOID:
        m_shapePropsBook->SetSelection( 1 );
        break;

    case CHOICE_SHAPE_ROUNDRECT:
    {
        m_shapePropsBook->SetSelection( 2 );

        // A reasonable default (from  IPC-7351C)
        if( m_previewPad->GetRoundRectRadiusRatio() == 0.0 )
            m_cornerRatio.ChangeDoubleValue( 25.0 );

        break;
    }

    case CHOICE_SHAPE_CHAMFERED_RECT:
        m_shapePropsBook->SetSelection( 3 );

        // Reasonable default
        if( m_previewPad->GetChamferRectRatio() == 0.0 )
            m_previewPad->SetChamferRectRatio( 0.2 );

        // Ensure the displayed value is up to date:
        m_chamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio() * 100.0 );

        // A reasonable default is one corner chamfered (usual for some SMD pads).
        if( !m_cbTopLeft->GetValue() && !m_cbTopRight->GetValue()
                && !m_cbBottomLeft->GetValue() && !m_cbBottomRight->GetValue() )
        {
            m_cbTopLeft->SetValue( true );
            m_cbTopRight->SetValue( false );
            m_cbBottomLeft->SetValue( false );
            m_cbBottomRight->SetValue( false );
        }

        break;

    case CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT:
        m_shapePropsBook->SetSelection( 4 );

        // Reasonable defaults (corner radius from  IPC-7351C)
        if( m_previewPad->GetRoundRectRadiusRatio() == 0.0
                && m_previewPad->GetChamferRectRatio() == 0.0 )
        {
            if( m_previewPad->GetRoundRectRadiusRatio() == 0.0 )
                m_previewPad->SetRoundRectRadiusRatio( 0.25 );

            if( m_previewPad->GetChamferRectRatio() == 0.0 )
                m_previewPad->SetChamferRectRatio( 0.2 );
        }

        // Ensure the displayed values are up to date:
        m_mixedChamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio() * 100.0 );
        m_mixedCornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio() * 100.0 );
        break;

    case CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR:     // PAD_SHAPE::CUSTOM, circular anchor
    case CHOICE_SHAPE_CUSTOM_RECT_ANCHOR:     // PAD_SHAPE::CUSTOM, rect anchor
        m_shapePropsBook->SetSelection( 0 );
        break;
    }

    // Note: must do this before enabling/disabling m_sizeY as we're using that as a flag to see
    // what the last shape was.
    if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CIRCLE )
    {
        if( m_sizeYCtrl->IsEnabled() && m_spokeAngle.GetAngleValue() == ANGLE_90 )
            m_spokeAngle.SetAngleValue( ANGLE_45 );
    }
    else
    {
        if( !m_sizeYCtrl->IsEnabled() && m_spokeAngle.GetAngleValue() == ANGLE_45 )
            m_spokeAngle.SetAngleValue( ANGLE_90 );
    }

    // Readjust props book size
    wxSize size = m_shapePropsBook->GetSize();
    size.y = m_shapePropsBook->GetPage( m_shapePropsBook->GetSelection() )->GetBestSize().y;
    m_shapePropsBook->SetMaxSize( size );

    m_sizeY.Enable( m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CIRCLE
                    && m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );

    m_offsetShapeOpt->Enable( m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CIRCLE
                              && m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR
                              && m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CUSTOM_RECT_ANCHOR );

    if( !m_offsetShapeOpt->IsEnabled() )
        m_offsetShapeOpt->SetValue( false );

    // Show/hide controls depending on m_offsetShapeOpt being enabled
    m_offsetCtrls->Show( m_offsetShapeOpt->GetValue() );
    m_offsetShapeOptLabel->Show( m_offsetShapeOpt->GetValue() );

    bool is_custom = m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR
                  || m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CUSTOM_RECT_ANCHOR;

    enablePrimitivePage( is_custom );

    if( transferDataToPad( m_previewPad ) )
        updateRoundRectCornerValues();

    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
        m_notebook->GetPage( i )->Layout();

    // Resize the dialog if its height is too small to show all widgets:
    if( m_MainSizer->GetSize().y < m_MainSizer->GetMinSize().y )
        m_MainSizer->SetSizeHints( this );

    updatePadSizeControls();
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnDrillShapeSelected( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    updateHoleControls();
    redraw();
}


void DIALOG_PAD_PROPERTIES::PadOrientEvent( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::UpdateLayersDropdown()
{
    m_rbCopperLayersSel->Clear();

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:
        m_rbCopperLayersSel->Append( _( "All copper layers" ) );
        m_rbCopperLayersSel->Append( wxString::Format( _( "%s, %s and connected layers" ),
                                                       m_board->GetLayerName( F_Cu ),
                                                       m_board->GetLayerName( B_Cu ) ) );
        m_rbCopperLayersSel->Append( _( "Connected layers only" ) );
        m_rbCopperLayersSel->Append( _( "None" ) );
        break;

    case NPTH_DLG_TYPE:
        m_rbCopperLayersSel->Append( wxString::Format( _( "%s and %s" ),
                                                       m_board->GetLayerName( F_Cu ),
                                                       m_board->GetLayerName( B_Cu ) ) );
        m_rbCopperLayersSel->Append( m_board->GetLayerName( F_Cu ) );
        m_rbCopperLayersSel->Append( m_board->GetLayerName( B_Cu ) );
        m_rbCopperLayersSel->Append( _( "None" ) );
        break;

    case SMD_DLG_TYPE:
    case CONN_DLG_TYPE:
        m_rbCopperLayersSel->Append( m_board->GetLayerName( F_Cu ) );
        m_rbCopperLayersSel->Append( m_board->GetLayerName( B_Cu ) );
        break;

    case APERTURE_DLG_TYPE:
        m_rbCopperLayersSel->Append( _( "None" ) );
        break;
    }
}


void DIALOG_PAD_PROPERTIES::PadTypeSelected( wxCommandEvent& event )
{
    bool hasHole = true;
    bool hasConnection = true;
    bool hasProperty = true;

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:      hasHole = true;  hasConnection = true;  hasProperty = true;  break;
    case SMD_DLG_TYPE:      hasHole = false; hasConnection = true;  hasProperty = true;  break;
    case CONN_DLG_TYPE:     hasHole = false; hasConnection = true;  hasProperty = true;  break;
    case NPTH_DLG_TYPE:     hasHole = true;  hasConnection = false; hasProperty = false; break;
    case APERTURE_DLG_TYPE: hasHole = false; hasConnection = false; hasProperty = true;  break;
    }

    // Update Layers dropdown list and selects the "best" layer set for the new pad type:
    updatePadLayersList( {}, m_previewPad->GetRemoveUnconnected(),
                         m_previewPad->GetKeepTopBottom() );

    m_gbSizerHole->Show( hasHole );
    m_staticline6->Show( hasHole );
    if( !hasHole )
    {
        m_holeX.ChangeValue( 0 );
        m_holeY.ChangeValue( 0 );
    }
    else if ( m_holeX.GetValue() == 0 && m_currentPad )
    {
        m_holeX.ChangeValue( m_currentPad->GetDrillSize().x );
        m_holeY.ChangeValue( m_currentPad->GetDrillSize().y );
    }

    if( !hasConnection )
    {
        m_padNumCtrl->ChangeValue( wxEmptyString );
        m_padNetSelector->SetSelectedNetcode( 0 );
        m_padToDieOpt->SetValue( false );
    }
    else if( m_padNumCtrl->GetValue().IsEmpty() && m_currentPad )
    {
        m_padNumCtrl->ChangeValue( m_currentPad->GetNumber() );
        m_padNetSelector->SetSelectedNetcode( m_currentPad->GetNetCode() );
    }

    if( !hasProperty )
        m_choiceFabProperty->SetSelection( 0 );

    m_choiceFabProperty->Enable( hasProperty );

    transferDataToPad( m_previewPad );

    // Layout adjustment is needed if the hole details got shown/hidden
    m_LeftBoxSizer->Layout();
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    bool hasHole = true;
    bool hasConnection = true;

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:      /* PTH */      hasHole = true;  hasConnection = true;  break;
    case SMD_DLG_TYPE:      /* SMD */      hasHole = false; hasConnection = true;  break;
    case CONN_DLG_TYPE:     /* CONN */     hasHole = false; hasConnection = true;  break;
    case NPTH_DLG_TYPE:     /* NPTH */     hasHole = true;  hasConnection = false; break;
    case APERTURE_DLG_TYPE: /* Aperture */ hasHole = false; hasConnection = false; break;
    }

    // Enable/disable hole controls
    m_holeShapeLabel->Enable( hasHole );
    m_holeShapeCtrl->Enable( hasHole );
    m_holeX.Enable( hasHole );
    m_holeY.Enable( hasHole && m_holeShapeCtrl->GetSelection() == CHOICE_SHAPE_OVAL );

    // Enable/disable number and net
    m_padNumLabel->Enable( hasConnection );
    m_padNumCtrl->Enable( hasConnection );

    if( m_padNetLabel->IsShown() )
    {
        m_padNetLabel->Enable( hasConnection && m_canEditNetName && m_currentPad );
        m_padNetSelector->Enable( hasConnection && m_canEditNetName && m_currentPad );
    }

    // Enable/disable pad length-to-die
    m_padToDieOpt->Enable( hasConnection );

    if( !m_padToDieOpt->IsEnabled() )
        m_padToDieOpt->SetValue( false );

    // We can show/hide this here because it doesn't require the layout to be refreshed.
    // All the others have to be done in their event handlers because doing a layout here
    // causes infinite looping on MSW.
    m_padToDie.Show( m_padToDieOpt->GetValue() );

    // Enable/disable Copper Layers control
    m_rbCopperLayersSel->Enable( m_padType->GetSelection() != APERTURE_DLG_TYPE );

    LSET cu_set = m_previewPad->GetLayerSet() & LSET::AllCuMask();

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:
        if( !cu_set.any() )
            m_stackupImagesBook->SetSelection( 3 );
        else if( !m_previewPad->GetRemoveUnconnected() )
            m_stackupImagesBook->SetSelection( 0 );
        else if( m_previewPad->GetKeepTopBottom() )
            m_stackupImagesBook->SetSelection( 1 );
        else
            m_stackupImagesBook->SetSelection( 2 );

        break;

    case NPTH_DLG_TYPE:
        if( cu_set.test( F_Cu ) && cu_set.test( B_Cu ) )
            m_stackupImagesBook->SetSelection( 4 );
        else if( cu_set.test( F_Cu ) )
            m_stackupImagesBook->SetSelection( 5 );
        else if( cu_set.test( B_Cu ) )
            m_stackupImagesBook->SetSelection( 6 );
        else
            m_stackupImagesBook->SetSelection( 7 );

        break;

    case SMD_DLG_TYPE:
    case CONN_DLG_TYPE:
    case APERTURE_DLG_TYPE:
        m_stackupImagesBook->ChangeSelection( 3 );
        break;
    }
}


void DIALOG_PAD_PROPERTIES::OnUpdateUINonCopperWarning( wxUpdateUIEvent& event )
{
    bool isOnCopperLayer = ( m_previewPad->GetLayerSet() & LSET::AllCuMask() ).any();
    m_nonCopperWarningBook->ChangeSelection( isOnCopperLayer ? 0 : 1 );
}


void DIALOG_PAD_PROPERTIES::updatePadLayersList( LSET layer_mask, bool remove_unconnected,
                                                 bool keep_top_bottom )
{
    UpdateLayersDropdown();

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:
        if( !layer_mask.any() )
            layer_mask = PAD::PTHMask();

        if( !( layer_mask & LSET::AllCuMask() ).any() )
            m_rbCopperLayersSel->SetSelection( 3 );
        else if( !remove_unconnected )
            m_rbCopperLayersSel->SetSelection( 0 );
        else if( keep_top_bottom )
            m_rbCopperLayersSel->SetSelection( 1 );
        else
            m_rbCopperLayersSel->SetSelection( 2 );

        break;

    case SMD_DLG_TYPE:
        if( !layer_mask.any() )
            layer_mask = PAD::SMDMask();

        if( layer_mask.test( F_Cu ) )
            m_rbCopperLayersSel->SetSelection( 0 );
        else
            m_rbCopperLayersSel->SetSelection( 1 );

        break;

    case CONN_DLG_TYPE:
        if( !layer_mask.any() )
            layer_mask = PAD::ConnSMDMask();

        if( layer_mask.test( F_Cu ) )
            m_rbCopperLayersSel->SetSelection( 0 );
        else
            m_rbCopperLayersSel->SetSelection( 1 );

        break;

    case NPTH_DLG_TYPE:
        if( !layer_mask.any() )
            layer_mask = PAD::UnplatedHoleMask();

        if( layer_mask.test( F_Cu ) && layer_mask.test( B_Cu ) )
            m_rbCopperLayersSel->SetSelection( 0 );
        else if( layer_mask.test( F_Cu ) )
            m_rbCopperLayersSel->SetSelection( 1 );
        else if( layer_mask.test( B_Cu ) )
            m_rbCopperLayersSel->SetSelection( 2 );
        else
            m_rbCopperLayersSel->SetSelection( 3 );

        break;

    case APERTURE_DLG_TYPE:
        if( !layer_mask.any() )
            layer_mask = PAD::ApertureMask();

        m_rbCopperLayersSel->SetSelection( 0 );
        break;
    }

    m_layerFrontAdhesive->SetValue( layer_mask[F_Adhes] );
    m_layerBackAdhesive->SetValue( layer_mask[B_Adhes] );

    m_layerFrontPaste->SetValue( layer_mask[F_Paste] );
    m_layerBackPaste->SetValue( layer_mask[B_Paste] );

    m_layerFrontSilk->SetValue( layer_mask[F_SilkS] );
    m_layerBackSilk->SetValue( layer_mask[B_SilkS] );

    m_layerFrontMask->SetValue( layer_mask[F_Mask] );
    m_layerBackMask->SetValue( layer_mask[B_Mask] );

    m_layerECO1->SetValue( layer_mask[Eco1_User] );
    m_layerECO2->SetValue( layer_mask[Eco2_User] );

    m_layerUserDwgs->SetValue( layer_mask[Dwgs_User] );
}


bool DIALOG_PAD_PROPERTIES::Show( bool aShow )
{
    bool retVal = DIALOG_SHIM::Show( aShow );

    if( aShow )
    {
        // It *should* work to set the stackup bitmap in the constructor, but it doesn't.
        // wxWidgets needs to have these set when the panel is visible for some reason.
        // https://gitlab.com/kicad/code/kicad/-/issues/5534
        m_stackupImage0->SetBitmap( KiBitmap( BITMAPS::pads_reset_unused ) );
        m_stackupImage1->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused_keep_bottom ) );
        m_stackupImage2->SetBitmap( KiBitmap( BITMAPS::pads_remove_unused ) );
        m_stackupImage4->SetBitmap( KiBitmap( BITMAPS::pads_npth_top_bottom ) );
        m_stackupImage5->SetBitmap( KiBitmap( BITMAPS::pads_npth_top ) );
        m_stackupImage6->SetBitmap( KiBitmap( BITMAPS::pads_npth_bottom ) );
        m_stackupImage7->SetBitmap( KiBitmap( BITMAPS::pads_npth ) );

        Layout();
    }

    return retVal;
}


void DIALOG_PAD_PROPERTIES::OnSetCopperLayers( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnSetLayers( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    redraw();
}


bool DIALOG_PAD_PROPERTIES::padValuesOK()
{
    bool error = !transferDataToPad( m_previewPad );

    wxArrayString error_msgs;
    wxArrayString warning_msgs;
    VECTOR2I      pad_size = m_previewPad->GetSize();
    VECTOR2I      drill_size = m_previewPad->GetDrillSize();

    if( m_previewPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        // allow 0-sized anchor pads
    }
    else if( m_previewPad->GetShape() == PAD_SHAPE::CIRCLE )
    {
        if( pad_size.x <= 0 )
            error_msgs.Add( _( "Error: Pad must have a positive size." ) );
    }
    else
    {
        if( pad_size.x <= 0 || pad_size.y <= 0 )
            error_msgs.Add( _( "Error: Pad must have a positive size." ) );
    }

    // Test hole against pad shape
    if( m_previewPad->IsOnCopperLayer() && m_previewPad->GetDrillSize().x > 0 )
    {
        int            maxError = m_board->GetDesignSettings().m_MaxError;
        SHAPE_POLY_SET padOutline;

        m_previewPad->TransformShapeToPolygon( padOutline, UNDEFINED_LAYER, 0, maxError,
                                             ERROR_INSIDE );

        if( !padOutline.Collide( m_previewPad->GetPosition() ) )
        {
            warning_msgs.Add( _( "Warning: Pad hole not inside pad shape." ) );
        }
        else if( m_previewPad->GetAttribute() == PAD_ATTRIB::PTH )
        {
            std::shared_ptr<SHAPE_SEGMENT> slot = m_previewPad->GetEffectiveHoleShape();
            SHAPE_POLY_SET                 slotOutline;

            TransformOvalToPolygon( slotOutline, slot->GetSeg().A, slot->GetSeg().B,
                                    slot->GetWidth(), maxError, ERROR_INSIDE );

            padOutline.BooleanSubtract( slotOutline, SHAPE_POLY_SET::PM_FAST );

            if( padOutline.IsEmpty() )
                warning_msgs.Add( _( "Warning: Pad hole will leave no copper." ) );
        }
    }

    if( m_previewPad->GetLocalClearance() < 0 )
        warning_msgs.Add( _( "Warning: Negative local clearance values will have no effect." ) );

    // Some pads need a negative solder mask clearance (mainly for BGA with small pads)
    // However the negative solder mask clearance must not create negative mask size
    // Therefore test for minimal acceptable negative value
    if( m_previewPad->GetLocalSolderMaskMargin() < 0 )
    {
        int absMargin = abs( m_previewPad->GetLocalSolderMaskMargin() );

        if( m_previewPad->GetShape() == PAD_SHAPE::CUSTOM )
        {
            for( const std::shared_ptr<PCB_SHAPE>& shape : m_previewPad->GetPrimitives() )
            {
                BOX2I shapeBBox = shape->GetBoundingBox();

                if( absMargin > shapeBBox.GetWidth() || absMargin > shapeBBox.GetHeight() )
                {
                    warning_msgs.Add( _( "Warning: Negative solder mask clearances larger than "
                                         "some shape primitives. Results may be surprising." ) );

                    break;
                }
            }
        }
        else if( absMargin > pad_size.x || absMargin > pad_size.y )
        {
            warning_msgs.Add( _( "Warning: Negative solder mask clearance larger than pad. No "
                                 "solder mask will be generated." ) );
        }
    }

    // Some pads need a positive solder paste clearance (mainly for BGA with small pads)
    // However, a positive value can create issues if the resulting shape is too big.
    // (like a solder paste creating a solder paste area on a neighbor pad or on the solder mask)
    // So we could ask for user to confirm the choice
    // For now we just check for disappearing paste
    wxSize paste_size;
    int    paste_margin = m_previewPad->GetLocalSolderPasteMargin();
    double paste_ratio = m_previewPad->GetLocalSolderPasteMarginRatio();

    paste_size.x = pad_size.x + paste_margin + KiROUND( pad_size.x * paste_ratio );
    paste_size.y = pad_size.y + paste_margin + KiROUND( pad_size.y * paste_ratio );

    if( paste_size.x <= 0 || paste_size.y <= 0 )
    {
        warning_msgs.Add( _( "Warning: Negative solder paste margins larger than pad. No solder "
                             "paste mask will be generated." ) );
    }

    LSET padlayers_mask = m_previewPad->GetLayerSet();

    if( padlayers_mask == 0 )
        error_msgs.Add( _( "Error: pad has no layer." ) );

    if( !padlayers_mask[F_Cu] && !padlayers_mask[B_Cu] )
    {
        if( ( drill_size.x || drill_size.y ) && m_previewPad->GetAttribute() != PAD_ATTRIB::NPTH )
        {
            warning_msgs.Add( _( "Warning: Plated through holes should normally have a copper pad "
                                 "on at least one layer." ) );
        }
    }

    if( error )
        error_msgs.Add(  _( "Error: Trapazoid delta is too large." ) );

    switch( m_previewPad->GetAttribute() )
    {
    case PAD_ATTRIB::NPTH:   // Not plated, but through hole, a hole is expected
    case PAD_ATTRIB::PTH:    // Pad through hole, a hole is also expected
        if( drill_size.x <= 0
            || ( drill_size.y <= 0 && m_previewPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ) )
        {
            error_msgs.Add( _( "Error: Through hole pad has no hole." ) );
        }
        break;

    case PAD_ATTRIB::CONN:      // Connector pads are smd pads, just they do not have solder paste.
        if( padlayers_mask[B_Paste] || padlayers_mask[F_Paste] )
        {
            warning_msgs.Add( _( "Warning: Connector pads normally have no solder paste. Use a "
                                 "SMD pad instead." ) );
        }
        KI_FALLTHROUGH;

    case PAD_ATTRIB::SMD:       // SMD and Connector pads (One external copper layer only)
    {
        if( drill_size.x > 0 || drill_size.y > 0 )
        {
            error_msgs.Add( _( "Error: SMD pad has a hole." ) );
        }

        LSET innerlayers_mask = padlayers_mask & LSET::InternalCuMask();

        if( ( padlayers_mask[F_Cu] && padlayers_mask[B_Cu] ) || innerlayers_mask.count() != 0 )
            warning_msgs.Add( _( "Warning: SMD pad has no outer layers." ) );
    }
        break;
    }

    if( ( m_previewPad->GetProperty() == PAD_PROP::FIDUCIAL_GLBL || m_previewPad->GetProperty() == PAD_PROP::FIDUCIAL_LOCAL )
            && m_previewPad->GetAttribute() == PAD_ATTRIB::NPTH )
    {
        warning_msgs.Add(  _( "Warning: Fiducial property makes no sense on NPTH pads." ) );
    }

    if( m_previewPad->GetProperty() == PAD_PROP::TESTPOINT
            && m_previewPad->GetAttribute() == PAD_ATTRIB::NPTH )
    {
        warning_msgs.Add(  _( "Warning: Testpoint property makes no sense on NPTH pads." ) );
    }

    if( m_previewPad->GetProperty() == PAD_PROP::HEATSINK
            && m_previewPad->GetAttribute() == PAD_ATTRIB::NPTH )
    {
        warning_msgs.Add(  _( "Warning: Heatsink property makes no sense of NPTH pads." ) );
    }

    if( m_previewPad->GetProperty() == PAD_PROP::CASTELLATED
            && m_previewPad->GetAttribute() != PAD_ATTRIB::PTH )
    {
        warning_msgs.Add(  _( "Warning: Castellated property is for PTH pads." ) );
    }

    if( m_previewPad->GetProperty() == PAD_PROP::BGA
            && m_previewPad->GetAttribute() != PAD_ATTRIB::SMD )
    {
        warning_msgs.Add(  _( "Warning: BGA property is for SMD pads." ) );
    }

    if( m_previewPad->GetShape() == PAD_SHAPE::ROUNDRECT
            || m_previewPad->GetShape() == PAD_SHAPE::CHAMFERED_RECT )
    {
        wxASSERT( m_cornerRatio.GetDoubleValue() == m_mixedCornerRatio.GetDoubleValue() );

        if( m_cornerRatio.GetDoubleValue() < 0.0 )
            error_msgs.Add( _( "Error: Negative corner size." ) );
        else if( m_cornerRatio.GetDoubleValue() > 50.0 )
            warning_msgs.Add( _( "Warning: Corner size will make pad circular." ) );
    }

    // PADSTACKS TODO: this will need to check each layer in the pad...
    if( m_previewPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        SHAPE_POLY_SET mergedPolygon;
        m_previewPad->MergePrimitivesAsPolygon( &mergedPolygon );

        if( mergedPolygon.OutlineCount() > 1 )
            error_msgs.Add( _( "Error: Custom pad shape must resolve to a single polygon." ) );
    }


    if( error_msgs.GetCount() || warning_msgs.GetCount() )
    {
        wxString title = error_msgs.GetCount() ? _( "Pad Properties Errors" )
                                               : _( "Pad Properties Warnings" );
        HTML_MESSAGE_BOX dlg( this, title );

        wxArrayString msgs = error_msgs;

        for( const wxString& msg : warning_msgs )
            msgs.Add( msg );

        dlg.ListSet( msgs );

        dlg.ShowModal();
    }

    return error_msgs.GetCount() == 0;
}


void DIALOG_PAD_PROPERTIES::redraw()
{
    if( !m_canUpdate )
        return;

    KIGFX::VIEW* view = m_padPreviewGAL->GetView();
    m_padPreviewGAL->StopDrawing();

    // The layer used to place primitive items selected when editing custom pad shapes
    // we use here a layer never used in a pad:
    #define SELECTED_ITEMS_LAYER Dwgs_User

    view->SetTopLayer( SELECTED_ITEMS_LAYER );
    KIGFX::PCB_RENDER_SETTINGS* settings =
        static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    settings->SetLayerColor( SELECTED_ITEMS_LAYER, m_selectedColor );

    view->Update( m_previewPad );

    // delete previous items if highlight list
    while( m_highlight.size() )
    {
        delete m_highlight.back(); // the dtor also removes item from view
        m_highlight.pop_back();
    }

    // highlight selected primitives:
    long select = m_listCtrlPrimitives->GetFirstSelected();

    while( select >= 0 )
    {
        PCB_SHAPE* dummyShape = static_cast<PCB_SHAPE*>( m_primitives[select]->Clone() );
        dummyShape->SetLayer( SELECTED_ITEMS_LAYER );
        dummyShape->Rotate( { 0, 0 }, m_previewPad->GetOrientation() );
        dummyShape->Move( m_previewPad->GetPosition() );

        view->Add( dummyShape );
        m_highlight.push_back( dummyShape );

        select = m_listCtrlPrimitives->GetNextSelected( select );
    }

    BOX2I bbox = m_previewPad->ViewBBox();

    if( bbox.GetSize().x > 0 && bbox.GetSize().y > 0 )
    {
        // The origin always goes in the middle of the canvas; we want offsetting the pad
        // shape to move the pad, not the hole
        bbox.Move( -m_previewPad->GetPosition() );
        int maxXExtent = std::max( abs( bbox.GetLeft() ), abs( bbox.GetRight() ) );
        int maxYExtent = std::max( abs( bbox.GetTop() ), abs( bbox.GetBottom() ) );

        // Don't blow up the GAL on too-large numbers
        if( maxXExtent > INT_MAX / 4 )
            maxXExtent = INT_MAX / 4;

        if( maxYExtent > INT_MAX / 4 )
            maxYExtent = INT_MAX / 4;

        BOX2D viewBox( m_previewPad->GetPosition(), {0, 0} );
        BOX2D canvasBox( m_previewPad->GetPosition(), {0, 0} );
        viewBox.Inflate( maxXExtent * 1.4, maxYExtent * 1.4 );  // add a margin
        canvasBox.Inflate( maxXExtent * 2.0, maxYExtent * 2.0 );

        view->SetBoundary( canvasBox );

        // Autozoom
        view->SetViewport( viewBox );

        m_padPreviewGAL->StartDrawing();
        m_padPreviewGAL->Refresh();
    }
}


bool DIALOG_PAD_PROPERTIES::TransferDataToWindow()
{
    if( !wxDialog::TransferDataToWindow() )
        return false;

    if( !m_panelGeneral->TransferDataToWindow() )
        return false;

    if( !m_localSettingsPanel->TransferDataToWindow() )
        return false;

    return true;
}


bool DIALOG_PAD_PROPERTIES::TransferDataFromWindow()
{
    BOARD_COMMIT commit( m_parent );

    if( !wxDialog::TransferDataFromWindow() )
        return false;

    if( !m_panelGeneral->TransferDataFromWindow() )
        return false;

    if( !m_localSettingsPanel->TransferDataFromWindow() )
        return false;

    if( !padValuesOK() )
        return false;

    if( !transferDataToPad( m_masterPad ) )
        return false;

    PAD_TOOL* padTool = m_parent->GetToolManager()->GetTool<PAD_TOOL>();
    padTool->SetLastPadNumber( m_masterPad->GetNumber() );

    // m_masterPad is a pattern: ensure there is no net for this pad:
    m_masterPad->SetNetCode( NETINFO_LIST::UNCONNECTED );

    if( !m_currentPad )   // Set current Pad parameters
        return true;

    commit.Modify( m_currentPad );

    // Update values
    m_currentPad->SetShape( m_masterPad->GetShape() );
    m_currentPad->SetAttribute( m_masterPad->GetAttribute() );
    m_currentPad->SetFPRelativeOrientation( m_masterPad->GetOrientation() );

    m_currentPad->SetSize( m_masterPad->GetSize() );

    VECTOR2I size = m_masterPad->GetDelta();
    m_currentPad->SetDelta( size );

    m_currentPad->SetDrillSize( m_masterPad->GetDrillSize() );
    m_currentPad->SetDrillShape( m_masterPad->GetDrillShape() );

    VECTOR2I offset = m_masterPad->GetOffset();
    m_currentPad->SetOffset( offset );

    m_currentPad->SetPadToDieLength( m_masterPad->GetPadToDieLength() );

    if( m_masterPad->GetShape() != PAD_SHAPE::CUSTOM )
        m_masterPad->DeletePrimitivesList();

    m_currentPad->SetAnchorPadShape( m_masterPad->GetAnchorPadShape() );
    m_currentPad->ReplacePrimitives( m_masterPad->GetPrimitives() );

    m_currentPad->SetLayerSet( m_masterPad->GetLayerSet() );
    m_currentPad->SetRemoveUnconnected( m_masterPad->GetRemoveUnconnected() );
    m_currentPad->SetKeepTopBottom( m_masterPad->GetKeepTopBottom() );

    m_currentPad->SetNumber( m_masterPad->GetNumber() );

    int padNetcode = NETINFO_LIST::UNCONNECTED;

    // For PAD_ATTRIB::NPTH, ensure there is no net name selected
    if( m_masterPad->GetAttribute() != PAD_ATTRIB::NPTH  )
        padNetcode = m_padNetSelector->GetSelectedNetcode();

    m_currentPad->SetNetCode( padNetcode );
    m_currentPad->SetLocalClearance( m_masterPad->GetLocalClearance() );
    m_currentPad->SetLocalSolderMaskMargin( m_masterPad->GetLocalSolderMaskMargin() );
    m_currentPad->SetLocalSolderPasteMargin( m_masterPad->GetLocalSolderPasteMargin() );
    m_currentPad->SetLocalSolderPasteMarginRatio( m_masterPad->GetLocalSolderPasteMarginRatio() );
    m_currentPad->SetThermalSpokeWidth( m_masterPad->GetThermalSpokeWidth() );
    m_currentPad->SetThermalSpokeAngle( m_masterPad->GetThermalSpokeAngle() );
    m_currentPad->SetThermalGap( m_masterPad->GetThermalGap() );
    m_currentPad->SetRoundRectRadiusRatio( m_masterPad->GetRoundRectRadiusRatio() );
    m_currentPad->SetChamferRectRatio( m_masterPad->GetChamferRectRatio() );
    m_currentPad->SetChamferPositions( m_masterPad->GetChamferPositions() );
    m_currentPad->SetZoneConnection( m_masterPad->GetZoneConnection() );

    // rounded rect pads with radius ratio = 0 are in fact rect pads.
    // So set the right shape (and perhaps issues with a radius = 0)
    if( m_currentPad->GetShape() == PAD_SHAPE::ROUNDRECT &&
        m_currentPad->GetRoundRectRadiusRatio() == 0.0 )
    {
        m_currentPad->SetShape( PAD_SHAPE::RECT );
    }

    // Set the fabrication property:
    m_currentPad->SetProperty( getSelectedProperty() );

    // define the way the clearance area is defined in zones
    m_currentPad->SetCustomShapeInZoneOpt( m_masterPad->GetCustomShapeInZoneOpt() );

    VECTOR2I relPos = m_masterPad->GetPosition();

    if( m_currentPad->GetParentFootprint() && m_currentPad->GetParentFootprint()->IsFlipped() )
    {
        // flip pad (up/down) around its position
         m_currentPad->Flip( m_currentPad->GetPosition(), false );
         relPos.y = -relPos.y;
    }

    // Must be done after flipping
    m_currentPad->SetFPRelativePosition( relPos );

    m_parent->SetMsgPanel( m_currentPad );

    // redraw the area where the pad was
    m_parent->GetCanvas()->Refresh();

    commit.Push( _( "Modify pad" ) );

    return true;
}


PAD_PROP DIALOG_PAD_PROPERTIES::getSelectedProperty()
{
    PAD_PROP prop = PAD_PROP::NONE;

    switch( m_choiceFabProperty->GetSelection() )
    {
    case 0:  prop = PAD_PROP::NONE;           break;
    case 1:  prop = PAD_PROP::BGA;            break;
    case 2:  prop = PAD_PROP::FIDUCIAL_LOCAL; break;
    case 3:  prop = PAD_PROP::FIDUCIAL_GLBL;  break;
    case 4:  prop = PAD_PROP::TESTPOINT;      break;
    case 5:  prop = PAD_PROP::HEATSINK;       break;
    case 6:  prop = PAD_PROP::CASTELLATED;    break;
    }

    return prop;
}

void DIALOG_PAD_PROPERTIES::updateHoleControls()
{
    m_holeXLabel->SetLabel( ( m_holeShapeCtrl->GetSelection() == CHOICE_SHAPE_CIRCLE )
                                    ? _( "Diameter:" )
                                    : _( "Hole size X:" ) );
    m_holeY.Show( m_holeShapeCtrl->GetSelection() != CHOICE_SHAPE_CIRCLE
                  && m_holeShapeCtrl->GetSelection() != CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );
    m_holeXLabel->GetParent()->Layout();
}

void DIALOG_PAD_PROPERTIES::updatePadSizeControls()
{
    m_sizeXLabel->SetLabel( ( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CIRCLE
                                || m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR )
                                    ? _( "Diameter:" )
                                    : _( "Pad size X:" ) );
    m_sizeY.Show( m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CIRCLE
                  && m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );
    m_sizeXLabel->GetParent()->Layout();
}


bool DIALOG_PAD_PROPERTIES::transferDataToPad( PAD* aPad )
{
    if( !Validate() )
        return false;

    if( !m_panelGeneral->Validate() )
        return false;

    if( !m_localSettingsPanel->Validate() )
        return false;

    if( !m_spokeWidth.Validate( 0, INT_MAX ) )
        return false;

    aPad->SetAttribute( code_type[m_padType->GetSelection()] );
    aPad->SetShape( code_shape[m_PadShapeSelector->GetSelection()] );

    if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CUSTOM_RECT_ANCHOR )
        aPad->SetAnchorPadShape( PAD_SHAPE::RECT );
    else
        aPad->SetAnchorPadShape( PAD_SHAPE::CIRCLE );

    if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
        aPad->ReplacePrimitives( m_primitives );

    // Read pad clearances values:
    aPad->SetLocalClearance( m_clearance.GetValue() );
    aPad->SetLocalSolderMaskMargin( m_maskMargin.GetValue() );
    aPad->SetLocalSolderPasteMargin( m_pasteMargin.GetValue() );
    aPad->SetLocalSolderPasteMarginRatio( m_pasteMarginRatio.GetDoubleValue() / 100.0 );
    aPad->SetThermalSpokeWidth( m_spokeWidth.GetValue() );
    aPad->SetThermalSpokeAngle( m_spokeAngle.GetAngleValue() );
    aPad->SetThermalGap( m_thermalGap.GetValue() );

    // And rotation
    aPad->SetOrientation( m_pad_orientation.GetAngleValue() );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0: aPad->SetZoneConnection( ZONE_CONNECTION::INHERITED ); break;
    case 1: aPad->SetZoneConnection( ZONE_CONNECTION::FULL );      break;
    case 2: aPad->SetZoneConnection( ZONE_CONNECTION::THERMAL );   break;
    case 3: aPad->SetZoneConnection( ZONE_CONNECTION::NONE );      break;
    }

    aPad->SetPosition( VECTOR2I( m_posX.GetValue(), m_posY.GetValue() ) );

    if( m_holeShapeCtrl->GetSelection() == CHOICE_SHAPE_CIRCLE )
    {
        aPad->SetDrillShape( PAD_DRILL_SHAPE_CIRCLE );
        aPad->SetDrillSize( VECTOR2I( m_holeX.GetValue(), m_holeX.GetValue() ) );
    }
    else
    {
        aPad->SetDrillShape( PAD_DRILL_SHAPE_OBLONG );
        aPad->SetDrillSize( VECTOR2I( m_holeX.GetValue(), m_holeY.GetValue() ) );
    }

    if( aPad->GetShape() == PAD_SHAPE::CIRCLE )
        aPad->SetSize( VECTOR2I( m_sizeX.GetValue(), m_sizeX.GetValue() ) );
    else
        aPad->SetSize( VECTOR2I( m_sizeX.GetValue(), m_sizeY.GetValue() ) );

    // For a trapezoid, test delta value (be sure delta is not too large for pad size)
    // remember DeltaSize.x is the Y size variation
    bool   error    = false;
    VECTOR2I delta( 0, 0 );

    if( aPad->GetShape() == PAD_SHAPE::TRAPEZOID )
    {
        // For a trapezoid, only one of delta.x or delta.y is not 0, depending on axis.
        if( m_trapAxisCtrl->GetSelection() == 0 )
            delta.x = m_trapDelta.GetValue();
        else
            delta.y = m_trapDelta.GetValue();

        if( delta.x < 0 && delta.x < -aPad->GetSize().y )
        {
            delta.x = -aPad->GetSize().y + 2;
            error = true;
        }

        if( delta.x > 0 && delta.x > aPad->GetSize().y )
        {
            delta.x = aPad->GetSize().y - 2;
            error = true;
        }

        if( delta.y < 0 && delta.y < -aPad->GetSize().x )
        {
            delta.y = -aPad->GetSize().x + 2;
            error = true;
        }

        if( delta.y > 0 && delta.y > aPad->GetSize().x )
        {
            delta.y = aPad->GetSize().x - 2;
            error = true;
        }
    }

    aPad->SetDelta( delta );

    if( m_offsetShapeOpt->GetValue() )
        aPad->SetOffset( VECTOR2I( m_offsetX.GetValue(), m_offsetY.GetValue() ) );
    else
        aPad->SetOffset( VECTOR2I() );

    // Read pad length die
    if( m_padToDieOpt->GetValue() )
        aPad->SetPadToDieLength( m_padToDie.GetValue() );
    else
        aPad->SetPadToDieLength( 0 );

    aPad->SetNumber( m_padNumCtrl->GetValue() );
    aPad->SetNetCode( m_padNetSelector->GetSelectedNetcode() );

    int chamfers = 0;

    if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CHAMFERED_RECT )
    {
        if( m_cbTopLeft->GetValue() )
            chamfers |= RECT_CHAMFER_TOP_LEFT;

        if( m_cbTopRight->GetValue() )
            chamfers |= RECT_CHAMFER_TOP_RIGHT;

        if( m_cbBottomLeft->GetValue() )
            chamfers |= RECT_CHAMFER_BOTTOM_LEFT;

        if( m_cbBottomRight->GetValue() )
            chamfers |= RECT_CHAMFER_BOTTOM_RIGHT;
    }
    else if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT )
    {
        if( m_cbTopLeft1->GetValue() )
            chamfers |= RECT_CHAMFER_TOP_LEFT;

        if( m_cbTopRight1->GetValue() )
            chamfers |= RECT_CHAMFER_TOP_RIGHT;

        if( m_cbBottomLeft1->GetValue() )
            chamfers |= RECT_CHAMFER_BOTTOM_LEFT;

        if( m_cbBottomRight1->GetValue() )
            chamfers |= RECT_CHAMFER_BOTTOM_RIGHT;
    }
    aPad->SetChamferPositions( chamfers );

    if( aPad->GetShape() == PAD_SHAPE::CUSTOM )
    {
        // The pad custom has a "anchor pad" (a basic shape: round or rect pad)
        // that is the minimal area of this pad, and is useful to ensure a hole
        // diameter is acceptable, and is used in Gerber files as flashed area
        // reference
        if( aPad->GetAnchorPadShape() == PAD_SHAPE::CIRCLE )
            aPad->SetSize( VECTOR2I( m_sizeX.GetValue(), m_sizeX.GetValue() ) );
    }

    // Define the way the clearance area is defined in zones.  Since all non-custom pad
    // shapes are convex to begin with, this really only makes any difference for custom
    // pad shapes.
    aPad->SetCustomShapeInZoneOpt( m_ZoneCustomPadShape->GetSelection() == 0 ?
                                   CUST_PAD_SHAPE_IN_ZONE_OUTLINE :
                                   CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL );

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB::PTH:
        break;

    case PAD_ATTRIB::CONN:
    case PAD_ATTRIB::SMD:
        // SMD and PAD_ATTRIB::CONN has no hole.
        // basically, SMD and PAD_ATTRIB::CONN are same type of pads
        // PAD_ATTRIB::CONN has just a default non technical layers that differs from SMD
        // and are intended to be used in virtual edge board connectors
        // However we can accept a non null offset,
        // mainly to allow complex pads build from a set of basic pad shapes
        aPad->SetDrillSize( VECTOR2I( 0, 0 ) );
        break;

    case PAD_ATTRIB::NPTH:
        // Mechanical purpose only:
        // no net name, no pad name allowed
        aPad->SetNumber( wxEmptyString );
        aPad->SetNetCode( NETINFO_LIST::UNCONNECTED );
        break;

    default:
        wxFAIL_MSG( wxT( "DIALOG_PAD_PROPERTIES::transferDataToPad: unknown pad type" ) );
        break;
    }

    if( aPad->GetShape() == PAD_SHAPE::ROUNDRECT )
    {
        aPad->SetRoundRectRadiusRatio( m_cornerRatio.GetDoubleValue() / 100.0 );
    }
    else if( aPad->GetShape() == PAD_SHAPE::CHAMFERED_RECT )
    {
        if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT )
        {
            aPad->SetChamferRectRatio( m_mixedChamferRatio.GetDoubleValue() / 100.0 );
            aPad->SetRoundRectRadiusRatio( m_mixedCornerRatio.GetDoubleValue() / 100.0 );
        }
        else    // Choice is CHOICE_SHAPE_CHAMFERED_RECT, no rounded corner
        {
            aPad->SetChamferRectRatio( m_chamferRatio.GetDoubleValue() / 100.0 );
            aPad->SetRoundRectRadiusRatio( 0 );
        }
    }

    aPad->SetProperty( getSelectedProperty() );

    LSET padLayerMask = LSET();
    int  copperLayersChoice = m_rbCopperLayersSel->GetSelection();

    aPad->SetRemoveUnconnected( false );
    aPad->SetKeepTopBottom( false );

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:
        switch( copperLayersChoice )
        {
        case 0:
            // All copper layers
            padLayerMask |= LSET::AllCuMask();
            break;

        case 1:
            // Front, back and connected
            padLayerMask |= LSET::AllCuMask();
            aPad->SetRemoveUnconnected( true );
            aPad->SetKeepTopBottom( true );
            break;

        case 2:
            // Connected only
            padLayerMask |= LSET::AllCuMask();
            aPad->SetRemoveUnconnected( true );
            break;

        case 3:
            // No copper layers
            break;
        }

        break;

    case NPTH_DLG_TYPE:
        switch( copperLayersChoice )
        {
        case 0: padLayerMask.set( F_Cu ).set( B_Cu ); break;
        case 1: padLayerMask.set( F_Cu );             break;
        case 2: padLayerMask.set( B_Cu );             break;
        default:                                      break;
        }

        break;

    case SMD_DLG_TYPE:
    case CONN_DLG_TYPE:
        switch( copperLayersChoice )
        {
        case 0: padLayerMask.set( F_Cu ); break;
        case 1: padLayerMask.set( B_Cu ); break;
        }

        break;

    case APERTURE_DLG_TYPE:
        // no copper layers
        break;
    }

    if( m_layerFrontAdhesive->GetValue() )
        padLayerMask.set( F_Adhes );

    if( m_layerBackAdhesive->GetValue() )
        padLayerMask.set( B_Adhes );

    if( m_layerFrontPaste->GetValue() )
        padLayerMask.set( F_Paste );

    if( m_layerBackPaste->GetValue() )
        padLayerMask.set( B_Paste );

    if( m_layerFrontSilk->GetValue() )
        padLayerMask.set( F_SilkS );

    if( m_layerBackSilk->GetValue() )
        padLayerMask.set( B_SilkS );

    if( m_layerFrontMask->GetValue() )
        padLayerMask.set( F_Mask );

    if( m_layerBackMask->GetValue() )
        padLayerMask.set( B_Mask );

    if( m_layerECO1->GetValue() )
        padLayerMask.set( Eco1_User );

    if( m_layerECO2->GetValue() )
        padLayerMask.set( Eco2_User );

    if( m_layerUserDwgs->GetValue() )
        padLayerMask.set( Dwgs_User );

    aPad->SetLayerSet( padLayerMask );

    return !error;
}


void DIALOG_PAD_PROPERTIES::OnOffsetCheckbox( wxCommandEvent& event )
{
    if( m_offsetShapeOpt->GetValue() )
    {
        m_offsetX.SetValue( m_previewPad->GetOffset().x );
        m_offsetY.SetValue( m_previewPad->GetOffset().y );
    }

    // Show/hide controls depending on m_offsetShapeOpt being enabled
    m_offsetCtrls->Show( m_offsetShapeOpt->GetValue() );
    m_offsetShapeOptLabel->Show( m_offsetShapeOpt->GetValue() );

    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
        m_notebook->GetPage( i )->Layout();

    OnValuesChanged( event );
}


void DIALOG_PAD_PROPERTIES::OnPadToDieCheckbox( wxCommandEvent& event )
{
    if( m_padToDieOpt->GetValue() && m_currentPad )
        m_padToDie.SetValue( m_currentPad->GetPadToDieLength() );

    OnValuesChanged( event );
}


void DIALOG_PAD_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( m_canUpdate )
    {
        if( !transferDataToPad( m_previewPad ) )
            return;

        // If the pad size has changed, update the displayed values for rounded rect pads.
        updateRoundRectCornerValues();

        redraw();
    }
}

void DIALOG_PAD_PROPERTIES::editPrimitive()
{
    long select = m_listCtrlPrimitives->GetFirstSelected();

    if( select < 0 )
    {
        wxMessageBox( _( "No shape selected" ) );
        return;
    }

    std::shared_ptr<PCB_SHAPE>& shape = m_primitives[select];

    if( shape->GetShape() == SHAPE_T::POLY )
    {
        DIALOG_PAD_PRIMITIVE_POLY_PROPS dlg( this, m_parent, shape.get() );

        if( dlg.ShowModal() != wxID_OK )
            return;

        dlg.TransferDataFromWindow();
    }

    else
    {
        DIALOG_PAD_PRIMITIVES_PROPERTIES dlg( this, m_parent, shape.get() );

        if( dlg.ShowModal() != wxID_OK )
            return;

        dlg.TransferDataFromWindow();
    }

    displayPrimitivesList();

    if( m_canUpdate && transferDataToPad( m_previewPad ) )
        redraw();
}


void DIALOG_PAD_PROPERTIES::OnPrimitiveSelection( wxListEvent& event )
{
    // Called on a double click on the basic shapes list
    // To Do: highlight the primitive(s) currently selected.
    redraw();
}


void DIALOG_PAD_PROPERTIES::onPrimitiveDClick( wxMouseEvent& event )
{
    editPrimitive();
}


void DIALOG_PAD_PROPERTIES::onEditPrimitive( wxCommandEvent& event )
{
    editPrimitive();
}


void DIALOG_PAD_PROPERTIES::onDeletePrimitive( wxCommandEvent& event )
{
    long select = m_listCtrlPrimitives->GetFirstSelected();

    if( select < 0 )
        return;

    // Multiple selections are allowed. get them and remove corresponding shapes
    std::vector<long> indexes;
    indexes.push_back( select );

    while( ( select = m_listCtrlPrimitives->GetNextSelected( select ) ) >= 0 )
        indexes.push_back( select );

    // Erase all select shapes
    for( unsigned ii = indexes.size(); ii > 0; --ii )
        m_primitives.erase( m_primitives.begin() + indexes[ii-1] );

    displayPrimitivesList();

    if( m_canUpdate && transferDataToPad( m_previewPad ) )
        redraw();
}


void DIALOG_PAD_PROPERTIES::onAddPrimitive( wxCommandEvent& event )
{
    // Ask user for shape type
    wxString shapelist[] = {
            _( "Segment" ),
            _( "Arc" ),
            _( "Bezier" ),
            _( "Ring/Circle" ),
            _( "Polygon" ),
            _( "Number box" ),
    };

    int type = wxGetSingleChoiceIndex( _( "Shape type:" ), _( "Add Primitive" ),
                                       arrayDim( shapelist ), shapelist, 0, this );

    // User pressed cancel
    if( type == -1 )
        return;

    SHAPE_T listtype[] = { SHAPE_T::SEGMENT, SHAPE_T::ARC, SHAPE_T::BEZIER, SHAPE_T::CIRCLE,
                           SHAPE_T::POLY, SHAPE_T::RECT };

    PCB_SHAPE* primitive = new PCB_SHAPE();
    primitive->SetShape( listtype[type] );

    if( type == static_cast<int>( arrayDim( shapelist ) ) - 1 )
        primitive->SetIsAnnotationProxy();

    primitive->SetStroke( STROKE_PARAMS( m_board->GetDesignSettings().GetLineThickness( F_Cu ),
                                         PLOT_DASH_TYPE::SOLID ) );
    primitive->SetFilled( true );

    if( listtype[type] == SHAPE_T::POLY )
    {
        DIALOG_PAD_PRIMITIVE_POLY_PROPS dlg( this, m_parent, primitive );

        if( dlg.ShowModal() != wxID_OK )
            return;
    }
    else
    {
        DIALOG_PAD_PRIMITIVES_PROPERTIES dlg( this, m_parent, primitive );

        if( dlg.ShowModal() != wxID_OK )
            return;
    }

    m_primitives.emplace_back( primitive );

    displayPrimitivesList();

    if( m_canUpdate && transferDataToPad( m_previewPad ) )
        redraw();
}


void DIALOG_PAD_PROPERTIES::onGeometryTransform( wxCommandEvent& event )
{
    long select = m_listCtrlPrimitives->GetFirstSelected();

    if( select < 0 )
    {
        wxMessageBox( _( "No shape selected" ) );
        return;
    }

    // Multiple selections are allowed. Build selected shapes list
    std::vector<std::shared_ptr<PCB_SHAPE>> shapeList;
    shapeList.emplace_back( m_primitives[select] );

    while( ( select = m_listCtrlPrimitives->GetNextSelected( select ) ) >= 0 )
        shapeList.emplace_back( m_primitives[select] );

    DIALOG_PAD_PRIMITIVES_TRANSFORM dlg( this, m_parent, shapeList, false );

    if( dlg.ShowModal() != wxID_OK )
        return;

    dlg.Transform();

    displayPrimitivesList();

    if( m_canUpdate && transferDataToPad( m_previewPad ) )
        redraw();
}


void DIALOG_PAD_PROPERTIES::onDuplicatePrimitive( wxCommandEvent& event )
{
    long select = m_listCtrlPrimitives->GetFirstSelected();

    if( select < 0 )
    {
        wxMessageBox( _( "No shape selected" ) );
        return;
    }

    // Multiple selections are allowed. Build selected shapes list
    std::vector<std::shared_ptr<PCB_SHAPE>> shapeList;
    shapeList.emplace_back( m_primitives[select] );

    while( ( select = m_listCtrlPrimitives->GetNextSelected( select ) ) >= 0 )
        shapeList.emplace_back( m_primitives[select] );

    DIALOG_PAD_PRIMITIVES_TRANSFORM dlg( this, m_parent, shapeList, true );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Transfer new settings
    // save duplicates to a separate vector to avoid m_primitives reallocation,
    // as shapeList contains pointers to its elements
    std::vector<std::shared_ptr<PCB_SHAPE>> duplicates;
    dlg.Transform( &duplicates, dlg.GetDuplicateCount() );
    std::move( duplicates.begin(), duplicates.end(), std::back_inserter( m_primitives ) );

    displayPrimitivesList();

    if( m_canUpdate && transferDataToPad( m_previewPad ) )
        redraw();
}
