/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <drc/drc_item.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <confirm.h>
#include <core/arraydim.h>
#include <convert_basic_shapes_to_polygon.h> // for enum RECT_CHAMFER_POSITIONS definition
#include <gal/graphics_abstraction_layer.h>
#include <geometry/shape_segment.h>
#include <dialog_pad_properties.h>
#include <dialogs/html_message_box.h>
#include <macros.h>
#include <pad.h>
#include <pad_utils.h>
#include <pcb_base_frame.h>
#include <footprint_edit_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <view/view_controls.h>
#include <widgets/net_selector.h>
#include <pcb_layer_box_selector.h>
#include <tool/tool_manager.h>
#include <tools/pad_tool.h>
#include <advanced_config.h>    // for pad property feature management
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>


int DIALOG_PAD_PROPERTIES::m_page = 0;     // remember the last open page during session


// list of pad shapes, ordered like the pad shape wxChoice in dialog.
static PAD_SHAPE code_shape[] =
{
    PAD_SHAPE::CIRCLE,
    PAD_SHAPE::OVAL,
    PAD_SHAPE::RECTANGLE,
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

    // QuasiModal required for NET_SELECTOR
    dlg.ShowQuasiModal();
}


DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, PAD* aPad ) :
        DIALOG_PAD_PROPERTIES_BASE( aParent ),
        m_parent( aParent ),
        m_initialized( false ),
        m_editLayer( F_Cu ),
        m_posX( aParent, m_posXLabel, m_posXCtrl, m_posXUnits ),
        m_posY( aParent, m_posYLabel, m_posYCtrl, m_posYUnits ),
        m_sizeX( aParent, m_sizeXLabel, m_sizeXCtrl, m_sizeXUnits ),
        m_sizeY( aParent, m_sizeYLabel, m_sizeYCtrl, m_sizeYUnits ),
        m_offsetX( aParent, m_offsetXLabel, m_offsetXCtrl, m_offsetXUnits ),
        m_offsetY( aParent, m_offsetYLabel, m_offsetYCtrl, m_offsetYUnits ),
        m_padToDie( aParent, m_padToDieLabel, m_padToDieCtrl, m_padToDieUnits ),
        m_padToDieDelay( aParent, m_padToDieDelayLabel, m_padToDieDelayCtrl, m_padToDieDelayUnits ),
        m_trapDelta( aParent, m_trapDeltaLabel, m_trapDeltaCtrl, m_trapDeltaUnits ),
        m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
        m_cornerRatio( aParent, m_cornerRatioLabel, m_cornerRatioCtrl, m_cornerRatioUnits ),
        m_chamferRatio( aParent, m_chamferRatioLabel, m_chamferRatioCtrl, m_chamferRatioUnits ),
        m_mixedCornerRatio( aParent, m_mixedCornerRatioLabel, m_mixedCornerRatioCtrl, m_mixedCornerRatioUnits ),
        m_mixedChamferRatio( aParent, m_mixedChamferRatioLabel, m_mixedChamferRatioCtrl, m_mixedChamferRatioUnits ),
        m_holeX( aParent, m_holeXLabel, m_holeXCtrl, m_holeXUnits ),
        m_holeY( aParent, m_holeYLabel, m_holeYCtrl, m_holeYUnits ),
        m_clearance( aParent, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits ),
        m_maskMargin( aParent, m_maskMarginLabel, m_maskMarginCtrl, m_maskMarginUnits ),
        m_pasteMargin( aParent, m_pasteMarginLabel, m_pasteMarginCtrl, m_pasteMarginUnits ),
        m_thermalGap( aParent, m_thermalGapLabel, m_thermalGapCtrl, m_thermalGapUnits ),
        m_spokeWidth( aParent, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits ),
        m_spokeAngle( aParent, m_spokeAngleLabel, m_spokeAngleCtrl, m_spokeAngleUnits ),
        m_pad_orientation( aParent, m_PadOrientText, m_cb_padrotation, m_orientationUnits ),
        m_teardropMaxLenSetting( aParent, m_stMaxLen, m_tcTdMaxLen, m_stMaxLenUnits ),
        m_teardropMaxHeightSetting( aParent, m_stTdMaxSize, m_tcMaxHeight, m_stMaxHeightUnits ),
        m_topPostMachineSize1Binder( aParent, m_topPostMachineSize1Label, m_topPostmachineSize1, m_topPostMachineSize1Units ),
        m_topPostMachineSize2Binder( aParent, m_topPostMachineSize2Label, m_topPostMachineSize2, m_topPostMachineSize2Units ),
        m_bottomPostMachineSize1Binder( aParent, m_bottomPostMachineSize1Label, m_bottomPostMachineSize1, m_bottomPostMachineSize1Units ),
        m_bottomPostMachineSize2Binder( aParent, m_bottomPostMachineSize2Label, m_bottomPostMachineSize2, m_bottomPostMachineSize2Units ),
        m_backDrillTopSizeBinder( aParent, m_backDrillTopSizeLabel, m_backDrillTopSize, m_backDrillTopSizeUnits ),
        m_backDrillBottomSizeBinder( aParent, m_backDrillBottomSizeLabel, m_backDrillBottomSize, m_backDrillBottomSizeUnits )
{
    SetName( PAD_PROPERTIES_DLG_NAME );
    m_isFpEditor = aParent->GetFrameType() == FRAME_FOOTPRINT_EDITOR;

    m_currentPad = aPad;        // aPad can be NULL, if the dialog is called
                                // from the footprint editor to set default pad setup

    m_board      = m_parent->GetBoard();

    // Configure display origin transforms
    m_posX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_posY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    m_padNetSelector->SetNetInfo( &m_board->GetNetInfo() );

    m_cbShowPadOutline->SetValue( m_sketchPreview );

    m_FlippedWarningIcon->SetBitmap( KiBitmapBundle( BITMAPS::dialog_warning ) );
    m_nonCopperWarningIcon->SetBitmap( KiBitmapBundle( BITMAPS::dialog_warning ) );
    m_legacyTeardropsIcon->SetBitmap( KiBitmapBundle( BITMAPS::dialog_warning ) );

    m_masterPad = m_parent->GetDesignSettings().m_Pad_Master.get();
    m_previewPad = new PAD( (FOOTPRINT*) nullptr );

    if( aPad )
    {
        SetTitle( _( "Pad Properties" ) );

        *m_previewPad = *aPad;
        m_previewPad->GetTeardropParams() = aPad->GetTeardropParams();
        m_previewPad->ClearFlags( SELECTED|BRIGHTENED );
    }
    else
    {
        SetTitle( _( "Default Pad Properties for Add Pad Tool" ) );

        *m_previewPad = *m_masterPad;
        m_previewPad->GetTeardropParams() = m_masterPad->GetTeardropParams();
    }

    // TODO(JE) padstacks: should this be re-run when pad mode changes?
    // Pads have a hardcoded internal rounding ratio which is 0.25 by default, even if
    // they're not a rounded shape. This makes it hard to detect an intentional 0.25
    // ratio, or one that's only there because it's the PAD default.
    // Zero it out here to mark that we should recompute a better ratio if the user
    // selects a pad shape which would need a default rounding ratio computed for it
    m_previewPad->Padstack().ForEachUniqueLayer(
        [&]( PCB_LAYER_ID aLayer )
        {
            if( !PAD_UTILS::PadHasMeaningfulRoundingRadius( *m_previewPad, aLayer ) )
                m_previewPad->SetRoundRectRadiusRatio( aLayer, 0.0 );
        } );

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

    // Update label text and tooltip for combined offset + ratio field
    m_pasteMarginLabel->SetLabel( _( "Solder paste clearance:" ) );
    m_pasteMarginLabel->SetToolTip( _( "Local solder paste clearance for this pad.\n"
                                       "Enter an absolute value (e.g., -0.1mm), a percentage "
                                       "(e.g., -5%), or both (e.g., -0.1mm - 5%).\n"
                                       "If blank, the footprint or global value is used." ) );

    // Hide the old ratio controls - they're no longer needed
    m_pasteMarginRatioLabel->Show( false );
    m_pasteMarginRatioCtrl->Show( false );
    m_pasteMarginRatioUnits->Show( false );

    m_padToDieDelay.SetUnits( EDA_UNITS::PS );
    m_padToDieDelay.SetDataType( EDA_DATA_TYPE::TIME );

    initValues();

    m_techLayersLabel->SetFont( KIUI::GetStatusFont( this ) );
    m_parentInfo->SetFont( KIUI::GetSmallInfoFont( this ) );
    m_teardropShapeLabel->SetFont( KIUI::GetStatusFont( this ) );

    wxFont infoFont = KIUI::GetSmallInfoFont( this ).Italic();
    m_nonCopperNote->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );

    updateHoleControls();
    updatePadSizeControls();

    // Usually, TransferDataToWindow is called by OnInitDialog
    // calling it here fixes all widget sizes so FinishDialogSettings can safely fix minsizes
    TransferDataToWindow();

    // Initialize canvas to be able to display the dummy pad:
    prepareCanvas();

    m_notebook->SetSelection( m_page );

    switch( m_page )
    {
    default:
    case 0: SetInitialFocus( m_padNumCtrl );     break;
    case 1: SetInitialFocus( m_thermalGapCtrl ); break;
    case 2: SetInitialFocus( m_clearanceCtrl );  break;
    }

    SetupStandardButtons();
    m_initialized = true;

    m_padNetSelector->Connect( FILTERED_ITEM_SELECTED,
                               wxCommandEventHandler( DIALOG_PAD_PROPERTIES::OnValuesChanged ),
                               nullptr, this );

    if( m_padType->GetSelection() != PTH_DLG_TYPE && m_padType->GetSelection() != NPTH_DLG_TYPE )
    {
        m_gbSizerHole->Show( false );
        m_staticline71->Show( false );
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
    m_padNetSelector->Disconnect( FILTERED_ITEM_SELECTED,
                                  wxCommandEventHandler( DIALOG_PAD_PROPERTIES::OnValuesChanged ),
                                  nullptr, this );

    m_page = m_notebook->GetSelection();

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


void DIALOG_PAD_PROPERTIES::prepareCanvas()
{
    GAL_DISPLAY_OPTIONS_IMPL opts = m_parent->GetGalDisplayOptions();
    COLOR_SETTINGS*            colorSettings = m_parent->GetColorSettings();

    opts.m_forceDisplayCursor = false;

    // Initialize the canvas to display the pad
    m_padPreviewGAL = new PCB_DRAW_PANEL_GAL( m_boardViewPanel, -1, wxDefaultPosition,
                                              wxDefaultSize, opts,
                                              m_parent->GetCanvas()->GetBackend() );

    m_padPreviewSizer->Add( m_padPreviewGAL, 12, wxEXPAND | wxALL, 5 );

    // Show the X and Y axis. It is useful because pad shape can have an offset
    // or be a complex shape.
    m_axisOrigin = new KIGFX::ORIGIN_VIEWITEM( colorSettings->GetColor( LAYER_GRID ),
                                               KIGFX::ORIGIN_VIEWITEM::CROSS, 100000,
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

    // don't show the locked item shadow in pad preview
    view->SetLayerVisible( LAYER_LOCKED_ITEM_SHADOW, false );

    // gives a non null grid size (0.001mm) because GAL layer does not like a 0 size grid:
    double gridsize = 0.001 * pcbIUScale.IU_PER_MM;
    view->GetGAL()->SetGridSize( VECTOR2D( gridsize, gridsize ) );

    // And do not show the grid:
    view->GetGAL()->SetGridVisibility( false );
    view->GetGAL()->SetAxesEnabled( false );
    view->Add( m_previewPad );
    view->Add( m_axisOrigin );

    m_padPreviewGAL->StartDrawing();
    Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PROPERTIES::OnResize ) );
}


void DIALOG_PAD_PROPERTIES::OnPadstackModeChanged( wxCommandEvent& aEvent )
{
    transferDataToPad( m_previewPad );
    afterPadstackModeChanged();
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnEditLayerChanged( wxCommandEvent& aEvent )
{
    // Save data from the previous layer
    transferDataToPad( m_previewPad );

    switch( m_previewPad->Padstack().Mode() )
    {
    default:
    case PADSTACK::MODE::NORMAL:
        m_editLayer = F_Cu;
        break;

    case PADSTACK::MODE::FRONT_INNER_BACK:
        switch( m_cbEditLayer->GetSelection() )
        {
        default:
        case 0: m_editLayer = F_Cu;                   break;
        case 1: m_editLayer = PADSTACK::INNER_LAYERS; break;
        case 2: m_editLayer = B_Cu;                   break;
        }
        break;

    case PADSTACK::MODE::CUSTOM:
    {
        int layer = m_cbEditLayer->GetSelection();

        if( layer < 0 )
            layer = 0;

        if( m_editLayerCtrlMap.contains( layer ) )
            m_editLayer = m_editLayerCtrlMap.at( layer );
        else
            m_editLayer = F_Cu;
    }
    }

    // Load controls with the current layer
    initPadstackLayerValues();

    wxCommandEvent cmd_event;
    OnPadShapeSelection( cmd_event );
    OnOffsetCheckbox( cmd_event );

    redraw();
}


void DIALOG_PAD_PROPERTIES::updateRoundRectCornerValues()
{
    // Note: use ChangeValue() to avoid generating a wxEVT_TEXT event
    m_cornerRadius.ChangeValue( m_previewPad->GetRoundRectCornerRadius( m_editLayer ) );

    m_cornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) * 100.0 );
    m_mixedCornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) * 100.0 );

    m_chamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio( m_editLayer ) * 100.0 );
    m_mixedChamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio( m_editLayer ) * 100.0 );
}


void DIALOG_PAD_PROPERTIES::onCornerRadiusChange( wxCommandEvent& event )
{
    if( m_previewPad->GetShape( m_editLayer ) != PAD_SHAPE::ROUNDRECT
            && m_previewPad->GetShape( m_editLayer ) != PAD_SHAPE::CHAMFERED_RECT )
    {
        return;
    }

    if( m_cornerRadius.GetValue() < 0 )
        m_cornerRadiusCtrl->ChangeValue( "0" );

    if( transferDataToPad( m_previewPad ) )
    {
        m_previewPad->SetRoundRectCornerRadius( m_editLayer, m_cornerRadius.GetValue() );

        m_cornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) * 100.0 );
        m_mixedCornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) * 100.0 );

        redraw();
    }

    if( m_initialized )
        OnModify();
}


double DIALOG_PAD_PROPERTIES::getMaxChamferRatio() const
{
    // The maximum chamfer ratio is 50% of the smallest pad side if adjacent sides
    // are selected, or 100% of the smallest pad side if only one side is selected.
    double baseline = 1.0;

    auto considerCheckboxes = [&]( const std::vector<wxCheckBox*>& checkBoxes )
    {
        for( size_t ii : { 0, 1, 2, 3 } )
        {
            if( !checkBoxes[ii]->IsChecked() )
                continue;

            if( checkBoxes[( ii + 1 ) % 4]->IsChecked() || checkBoxes[( ii - 1 ) % 4]->IsChecked() )
            {
                // If two adjacent corners are selected, the maximum chamfer ratio is 50%
                baseline = std::max( baseline, 0.5 );
                break;
            }
        }
    };


    baseline = 1.0 - m_previewPad->GetRoundRectRadiusRatio( m_editLayer );
    considerCheckboxes( { m_cbTopLeft1, m_cbTopRight1, m_cbBottomRight1, m_cbBottomLeft1 } );
    considerCheckboxes( { m_cbTopLeft, m_cbTopRight, m_cbBottomRight, m_cbBottomLeft } );

    // If only one corner is selected, the maximum chamfer ratio is 100%
    return baseline;
}


double DIALOG_PAD_PROPERTIES::getMaxCornerRadius() const
{
    return 1.0 - m_previewPad->GetChamferRectRatio( m_editLayer );
}


void DIALOG_PAD_PROPERTIES::updateAllowedPadChamferCorners()
{
    auto updateCheckBoxes = []( const std::vector<wxCheckBox*>& aCheckBoxes )
    {
        for( size_t ii : { 0, 1, 2, 3 } )
        {
            bool disable = aCheckBoxes[( ii + 1 ) % 4]->IsChecked()
                        || aCheckBoxes[( ii + 3 ) % 4]->IsChecked();

            aCheckBoxes[ii]->Enable( !disable );

            if( disable )
                aCheckBoxes[ii]->SetValue( false );
        }
    };

    if( m_mixedChamferRatio.GetDoubleValue() > 50.0 )
    {
        updateCheckBoxes( { m_cbTopLeft1, m_cbTopRight1, m_cbBottomRight1, m_cbBottomLeft1 } );
    }

    if( m_chamferRatio.GetDoubleValue() > 50.0 )
    {
        updateCheckBoxes( { m_cbTopLeft, m_cbTopRight, m_cbBottomRight, m_cbBottomLeft } );
    }
}


void DIALOG_PAD_PROPERTIES::onCornerSizePercentChange( wxCommandEvent& event )
{
    if( m_previewPad->GetShape( m_editLayer ) != PAD_SHAPE::ROUNDRECT
            && m_previewPad->GetShape( m_editLayer ) != PAD_SHAPE::CHAMFERED_RECT )
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
            double maxRatio = getMaxCornerRadius();

            if( ratioPercent < 0.0 )
            {
                m_cornerRatio.SetDoubleValue( 0.0 );
                m_mixedCornerRatio.SetDoubleValue( 0.0 );
            }
            else if( ratioPercent > maxRatio * 100.0 )
            {
                m_cornerRatio.SetDoubleValue( maxRatio * 100.0 );
                m_mixedCornerRatio.SetDoubleValue( maxRatio * 100.0 );
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
            double maxRatio = getMaxChamferRatio();
            // Clamp ratioPercent to acceptable value (0.0 to maxRatio)
            if( ratioPercent < 0.0 )
            {
                m_chamferRatio.SetDoubleValue( 0.0 );
                m_mixedChamferRatio.SetDoubleValue( 0.0 );
            }
            else if( ratioPercent > maxRatio * 100.0 )
            {
                m_chamferRatio.SetDoubleValue( maxRatio * 100.0 );
                m_mixedChamferRatio.SetDoubleValue( maxRatio * 100.0 );
            }

            if( ctrl == m_chamferRatioCtrl )
                m_mixedChamferRatioCtrl->ChangeValue( value );
            else
                m_chamferRatioCtrl->ChangeValue( value );

            updateAllowedPadChamferCorners();

            changed = true;
        }
    }

    if( changed && transferDataToPad( m_previewPad ) )
        m_cornerRadius.ChangeValue( m_previewPad->GetRoundRectCornerRadius( m_editLayer ) );

    redraw();

    if( m_initialized )
        OnModify();
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

    VECTOR2I absPos;

    if( m_currentPad )
    {
        absPos = m_currentPad->GetPosition();

        if( FOOTPRINT* footprint = m_currentPad->GetParentFootprint() )
        {
            VECTOR2I relPos = m_currentPad->GetFPRelativePosition();

            if( footprint->IsFlipped() )
            {
                // flip pad (up/down) around its position
                m_previewPad->Flip( m_previewPad->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
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

        m_padNumCtrl->SetValue( m_previewPad->GetNumber() );
    }
    else
    {
        PAD_TOOL* padTool = m_parent->GetToolManager()->GetTool<PAD_TOOL>();
        m_padNumCtrl->SetValue( padTool->GetLastPadNumber() );
    }

    afterPadstackModeChanged();

    m_padNetSelector->SetSelectedNetcode( m_previewPad->GetNetCode() );

    // Display current pad parameters units:
    m_posX.ChangeValue( absPos.x );
    m_posY.ChangeValue( absPos.y );

    m_holeX.ChangeValue( m_previewPad->GetDrillSize().x );
    m_holeY.ChangeValue( m_previewPad->GetDrillSize().y );

    // TODO(JE) padstacks -- does this need to be saved/restored every time the layer changes?
    // Store the initial thermal spoke angle to restore it, because some initializations
    // can change this value (mainly after m_PadShapeSelector initializations)
    EDA_ANGLE spokeInitialAngle = m_previewPad->GetThermalSpokeAngle();

    initPadstackLayerValues();

    m_padToDieOpt->SetValue( m_previewPad->GetPadToDieLength() != 0 );
    m_padToDie.ChangeValue( m_previewPad->GetPadToDieLength() );

    m_padToDieDelayOpt->SetValue( m_previewPad->GetPadToDieDelay() != 0 );
    m_padToDieDelay.ChangeValue( m_previewPad->GetPadToDieDelay() );

    if( m_previewPad->GetLocalClearance().has_value() )
        m_clearance.ChangeValue( m_previewPad->GetLocalClearance().value() );
    else
        m_clearance.ChangeValue( wxEmptyString );

    if( m_previewPad->GetLocalSolderMaskMargin().has_value() )
        m_maskMargin.ChangeValue( m_previewPad->GetLocalSolderMaskMargin().value() );
    else
        m_maskMargin.ChangeValue( wxEmptyString );

    m_pasteMargin.SetOffsetValue( m_previewPad->GetLocalSolderPasteMargin() );
    m_pasteMargin.SetRatioValue( m_previewPad->GetLocalSolderPasteMarginRatio() );

    if( m_previewPad->GetLocalThermalSpokeWidthOverride().has_value() )
        m_spokeWidth.ChangeValue( m_previewPad->GetLocalThermalSpokeWidthOverride().value() );
    else
        m_spokeWidth.SetNull();

    if( m_previewPad->GetLocalThermalGapOverride().has_value() )
        m_thermalGap.ChangeValue( m_previewPad->GetLocalThermalGapOverride().value() );
    else
        m_thermalGap.SetNull();

    m_spokeAngle.ChangeAngleValue( m_previewPad->GetThermalSpokeAngle() );
    m_pad_orientation.ChangeAngleValue( m_previewPad->GetOrientation() );

    m_cbTeardrops->SetValue( m_previewPad->GetTeardropParams().m_Enabled );
    m_cbTeardropsUseNextTrack->SetValue( m_previewPad->GetTeardropParams().m_AllowUseTwoTracks );
    m_cbPreferZoneConnection->SetValue( !m_previewPad->GetTeardropParams().m_TdOnPadsInZones );
    m_teardropMaxLenSetting.SetValue( m_previewPad->GetTeardropParams().m_TdMaxLen );
    m_teardropMaxHeightSetting.SetValue( m_previewPad->GetTeardropParams().m_TdMaxWidth );
    m_spTeardropLenPercent->SetValue( m_previewPad->GetTeardropParams().m_BestLengthRatio *100 );
    m_spTeardropSizePercent->SetValue( m_previewPad->GetTeardropParams().m_BestWidthRatio *100 );
    m_spTeardropHDPercent->SetValue( m_previewPad->GetTeardropParams().m_WidthtoSizeFilterRatio*100 );
    m_curvedEdges->SetValue( m_previewPad->GetTeardropParams().m_CurvedEdges );

    switch( m_previewPad->GetLocalZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    if( m_previewPad->GetCustomShapeInZoneOpt() == CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL )
        m_ZoneCustomPadShape->SetSelection( 1 );
    else
        m_ZoneCustomPadShape->SetSelection( 0 );

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
    case PAD_PROP::MECHANICAL:       m_choiceFabProperty->SetSelection( 6 ); break;
    case PAD_PROP::CASTELLATED:      m_choiceFabProperty->SetSelection( 7 ); break;
    case PAD_PROP::PRESSFIT:         m_choiceFabProperty->SetSelection( 8 ); break;
    }

    if( m_previewPad->GetDrillShape() != PAD_DRILL_SHAPE::OBLONG )
        m_holeShapeCtrl->SetSelection( 0 );
    else
        m_holeShapeCtrl->SetSelection( 1 );

    // Backdrill properties
    const PADSTACK::DRILL_PROPS& secondaryDrill = m_previewPad->Padstack().SecondaryDrill();
    const PADSTACK::DRILL_PROPS& tertiaryDrill = m_previewPad->Padstack().TertiaryDrill();
    bool hasBackdrill = secondaryDrill.start != UNDEFINED_LAYER;
    bool hasTertiaryDrill = tertiaryDrill.start != UNDEFINED_LAYER;

    m_backDrillChoice->SetSelection( hasBackdrill ? ( hasTertiaryDrill ? 3 : 1 )
                                                  : ( hasTertiaryDrill ? 2 : 0 ) );

    if( !hasBackdrill )
    {
        m_backDrillBottomSizeBinder.SetValue( 0 );
    }
    else
    {
        m_backDrillBottomSizeBinder.SetValue( secondaryDrill.size.x );

        for( unsigned int i = 0; i < m_backDrillBottomLayer->GetCount(); ++i )
        {
            if( ToLAYER_ID( (intptr_t)m_backDrillBottomLayer->GetClientData( i ) ) == secondaryDrill.end )
            {
                m_backDrillBottomLayer->SetSelection( i );
                break;
            }
        }
    }

    if( !hasTertiaryDrill )
    {
        m_backDrillTopSizeBinder.SetValue( 0 );
    }
    else
    {
        m_backDrillTopSizeBinder.SetValue( tertiaryDrill.size.x );

        for( unsigned int i = 0; i < m_backDrillTopLayer->GetCount(); ++i )
        {
            if( ToLAYER_ID( (intptr_t)m_backDrillTopLayer->GetClientData( i ) ) == tertiaryDrill.end )
            {
                m_backDrillTopLayer->SetSelection( i );
                break;
            }
        }
    }

    // Post machining
    const PADSTACK::POST_MACHINING_PROPS& frontPostMachining = m_previewPad->Padstack().FrontPostMachining();

    if( frontPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
        m_topPostMachining->SetSelection( 2 );
    else if( frontPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        m_topPostMachining->SetSelection( 1 );
    else
        m_topPostMachining->SetSelection( 0 );

    m_topPostMachineSize1Binder.SetValue( frontPostMachining.size );

    if( frontPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        m_topPostMachineSize2Binder.SetValue( frontPostMachining.angle );
    else
        m_topPostMachineSize2Binder.SetValue( frontPostMachining.depth );

    const PADSTACK::POST_MACHINING_PROPS& backPostMachining = m_previewPad->Padstack().BackPostMachining();

    if( backPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE )
        m_bottomPostMachining->SetSelection( 2 );
    else if( backPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        m_bottomPostMachining->SetSelection( 1 );
    else
        m_bottomPostMachining->SetSelection( 0 );

    m_bottomPostMachineSize1Binder.SetValue( backPostMachining.size );

    if( backPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        m_bottomPostMachineSize2Binder.SetValue( backPostMachining.angle );
    else
        m_bottomPostMachineSize2Binder.SetValue( backPostMachining.depth );


    updatePadLayersList( m_previewPad->GetLayerSet(), m_previewPad->GetRemoveUnconnected(),
                         m_previewPad->GetKeepTopBottom() );

    // Update some dialog widgets state (Enable/disable options):
    wxCommandEvent cmd_event;
    OnPadShapeSelection( cmd_event );
    OnOffsetCheckbox( cmd_event );
    updateHoleControls();

    // Restore thermal spoke angle to its initial value, because it can be modified
    // by the call to OnPadShapeSelection()
    m_previewPad->SetThermalSpokeAngle( spokeInitialAngle );
    m_spokeAngle.SetAngleValue( m_previewPad->GetThermalSpokeAngle() );
}


void DIALOG_PAD_PROPERTIES::initPadstackLayerValues()
{
    m_primitives = m_previewPad->GetPrimitives( m_editLayer );

    m_sizeX.ChangeValue( m_previewPad->GetSize( m_editLayer ).x );
    m_sizeY.ChangeValue( m_previewPad->GetSize( m_editLayer ).y );

    m_offsetShapeOpt->SetValue( m_previewPad->GetOffset( m_editLayer ) != VECTOR2I() );
    m_offsetX.ChangeValue( m_previewPad->GetOffset( m_editLayer ).x );
    m_offsetY.ChangeValue( m_previewPad->GetOffset( m_editLayer ).y );

    if( m_previewPad->GetDelta( m_editLayer ).x )
    {
        m_trapDelta.ChangeValue( m_previewPad->GetDelta( m_editLayer ).x );
        m_trapAxisCtrl->SetSelection( 0 );
    }
    else
    {
        m_trapDelta.ChangeValue( m_previewPad->GetDelta( m_editLayer ).y );
        m_trapAxisCtrl->SetSelection( 1 );
    }

    switch( m_previewPad->GetShape( m_editLayer ) )
    {
    default:
    case PAD_SHAPE::CIRCLE:    m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CIRCLE );    break;
    case PAD_SHAPE::OVAL:      m_PadShapeSelector->SetSelection( CHOICE_SHAPE_OVAL );      break;
    case PAD_SHAPE::RECTANGLE: m_PadShapeSelector->SetSelection( CHOICE_SHAPE_RECT );      break;
    case PAD_SHAPE::TRAPEZOID: m_PadShapeSelector->SetSelection( CHOICE_SHAPE_TRAPEZOID ); break;
    case PAD_SHAPE::ROUNDRECT: m_PadShapeSelector->SetSelection( CHOICE_SHAPE_ROUNDRECT ); break;

    case PAD_SHAPE::CHAMFERED_RECT:
        if( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) > 0.0 )
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT );
        else
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CHAMFERED_RECT );
        break;

    case PAD_SHAPE::CUSTOM:
        if( m_previewPad->GetAnchorPadShape( m_editLayer ) == PAD_SHAPE::RECTANGLE )
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CUSTOM_RECT_ANCHOR );
        else
            m_PadShapeSelector->SetSelection( CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );
        break;
    }

    int chamferPositions = m_previewPad->GetChamferPositions( m_editLayer );

    m_cbTopLeft->SetValue( chamferPositions & RECT_CHAMFER_TOP_LEFT );
    m_cbTopLeft1->SetValue( chamferPositions & RECT_CHAMFER_TOP_LEFT );
    m_cbTopRight->SetValue( chamferPositions & RECT_CHAMFER_TOP_RIGHT );
    m_cbTopRight1->SetValue( chamferPositions & RECT_CHAMFER_TOP_RIGHT );
    m_cbBottomLeft->SetValue( chamferPositions & RECT_CHAMFER_BOTTOM_LEFT );
    m_cbBottomLeft1->SetValue( chamferPositions & RECT_CHAMFER_BOTTOM_LEFT );
    m_cbBottomRight->SetValue( chamferPositions & RECT_CHAMFER_BOTTOM_RIGHT );
    m_cbBottomRight1->SetValue( chamferPositions & RECT_CHAMFER_BOTTOM_RIGHT );

    updateRoundRectCornerValues();
}


void DIALOG_PAD_PROPERTIES::afterPadstackModeChanged()
{
    // NOTE: synchronize changes here with DIALOG_TRACK_VIA_PROPERTIES::afterPadstackModeChanged

    wxCHECK_MSG( m_board, /* void */, "Expected valid board in afterPadstackModeChanged" );
    m_cbEditLayer->Clear();

    switch( m_previewPad->Padstack().Mode() )
    {
    case PADSTACK::MODE::NORMAL:
        m_cbPadstackMode->SetSelection( 0 );
        m_cbEditLayer->Append( m_board->GetLayerName( F_Cu ) );
        m_cbEditLayer->Disable();
        m_editLayer = F_Cu;
        m_editLayerCtrlMap = { { 0, F_Cu } };
        break;

    case PADSTACK::MODE::FRONT_INNER_BACK:
    {
        m_cbPadstackMode->SetSelection( 1 );
        m_cbEditLayer->Enable();

        std::vector choices = {
            m_board->GetLayerName( F_Cu ),
            _( "Inner Layers" ),
            m_board->GetLayerName( B_Cu )
        };

        m_cbEditLayer->Append( choices );

        m_editLayerCtrlMap = {
            { 0, F_Cu },
            { 1, PADSTACK::INNER_LAYERS },
            { 2, B_Cu }
        };

        if( m_editLayer != F_Cu && m_editLayer != B_Cu )
            m_editLayer = PADSTACK::INNER_LAYERS;

        break;
    }

    case PADSTACK::MODE::CUSTOM:
    {
        m_cbPadstackMode->SetSelection( 2 );
        m_cbEditLayer->Enable();
        LSET layers = LSET::AllCuMask() & m_board->GetEnabledLayers();

        for( PCB_LAYER_ID layer : layers.UIOrder() )
        {
            int idx = m_cbEditLayer->Append( m_board->GetLayerName( layer ) );
            m_editLayerCtrlMap[idx] = layer;
        }

        break;
    }
    }

    for( const auto& [idx, layer] : m_editLayerCtrlMap )
    {
        if( layer == m_editLayer )
        {
            m_cbEditLayer->SetSelection( idx );
            break;
        }
    }
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

        // Reasonable defaults
        if( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) == 0.0 )
        {
            const double ipcRadiusRatio =
                    PAD_UTILS::GetDefaultIpcRoundingRatio( *m_previewPad, m_editLayer );
            m_cornerRatio.ChangeDoubleValue( ipcRadiusRatio * 100 );
        }

        break;
    }

    case CHOICE_SHAPE_CHAMFERED_RECT:
        m_shapePropsBook->SetSelection( 3 );

        // Reasonable default
        if( m_previewPad->GetChamferRectRatio( m_editLayer ) == 0.0 )
            m_previewPad->SetChamferRectRatio( m_editLayer, 0.2 );

        // Ensure the displayed value is up to date:
        m_chamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio( m_editLayer ) * 100.0 );

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

        // Reasonable defaults
        if( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) == 0.0
                && m_previewPad->GetChamferRectRatio( m_editLayer ) == 0.0 )
        {
            const double ipcRadiusRatio =
                    PAD_UTILS::GetDefaultIpcRoundingRatio( *m_previewPad, m_editLayer );
            m_previewPad->SetRoundRectRadiusRatio( m_editLayer, ipcRadiusRatio );
            m_previewPad->SetChamferRectRatio( m_editLayer, 0.2 );
        }

        // Ensure the displayed values are up to date:
        m_mixedChamferRatio.ChangeDoubleValue( m_previewPad->GetChamferRectRatio( m_editLayer ) * 100.0 );
        m_mixedCornerRatio.ChangeDoubleValue( m_previewPad->GetRoundRectRadiusRatio( m_editLayer ) * 100.0 );
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

    m_offsetShapeOpt->Enable( m_PadShapeSelector->GetSelection() != CHOICE_SHAPE_CIRCLE );

    if( !m_offsetShapeOpt->IsEnabled() )
        m_offsetShapeOpt->SetValue( false );

    // Show/hide controls depending on m_offsetShapeOpt being enabled
    m_offsetCtrls->Show( m_offsetShapeOpt->GetValue() );
    m_offsetShapeOptLabel->Show( m_offsetShapeOpt->GetValue() );

    if( transferDataToPad( m_previewPad ) )
        updateRoundRectCornerValues();

    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
        m_notebook->GetPage( i )->Layout();

    // Resize the dialog if its height is too small to show all widgets:
    if( m_MainSizer->GetSize().y < m_MainSizer->GetMinSize().y )
        m_MainSizer->SetSizeHints( this );

    updatePadSizeControls();
    redraw();

    if( m_initialized )
        OnModify();
}


void DIALOG_PAD_PROPERTIES::OnDrillShapeSelected( wxCommandEvent& event )
{
    if( m_holeShapeCtrl->GetSelection() != CHOICE_SHAPE_CIRCLE )
    {
        bool hasBackdrill = ( m_backDrillChoice->GetSelection() != 0 );
        bool hasTopPost = ( m_topPostMachining->GetSelection() != 0 );
        bool hasBottomPost = ( m_bottomPostMachining->GetSelection() != 0 );

        if( hasBackdrill || hasTopPost || hasBottomPost )
        {
            if( wxMessageBox( _( "Switching to non-circular hole will disable backdrills and post-machining. Continue?" ),
                              _( "Warning" ), wxOK | wxCANCEL | wxICON_WARNING, this ) != wxOK )
            {
                m_holeShapeCtrl->SetSelection( CHOICE_SHAPE_CIRCLE );
                return;
            }
        }
    }

    transferDataToPad( m_previewPad );
    updateHoleControls();
    redraw();

    if( m_initialized )
        OnModify();
}


void DIALOG_PAD_PROPERTIES::PadOrientEvent( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    redraw();

    if( m_initialized )
        OnModify();
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

    m_backDrillTopLayer->Clear();
    m_backDrillBottomLayer->Clear();

    for( PCB_LAYER_ID layerId : m_board->GetEnabledLayers().UIOrder() )
    {
        if( IsCopperLayer( layerId ) )
        {
            wxString layerName = m_board->GetLayerName( layerId );
            m_backDrillTopLayer->Append( layerName, wxBitmapBundle(), (void*)(intptr_t)layerId );
            m_backDrillBottomLayer->Append( layerName, wxBitmapBundle(), (void*)(intptr_t)layerId );
        }
    }
}


void DIALOG_PAD_PROPERTIES::PadTypeSelected( wxCommandEvent& event )
{
    bool hasHole = true;
    bool hasConnection = true;

    switch( m_padType->GetSelection() )
    {
    case PTH_DLG_TYPE:      hasHole = true;  hasConnection = true;  break;
    case SMD_DLG_TYPE:      hasHole = false; hasConnection = true;  break;
    case CONN_DLG_TYPE:     hasHole = false; hasConnection = true;  break;
    case NPTH_DLG_TYPE:     hasHole = true;  hasConnection = false; break;
    case APERTURE_DLG_TYPE: hasHole = false; hasConnection = false; break;
    }

    // Update Layers dropdown list and selects the "best" layer set for the new pad type:
    updatePadLayersList( {}, m_previewPad->GetRemoveUnconnected(),
                         m_previewPad->GetKeepTopBottom() );

    m_gbSizerHole->Show( hasHole );
    m_staticline71->Show( hasHole );

    if( !hasHole )
    {
        m_holeX.ChangeValue( 0 );
        m_holeY.ChangeValue( 0 );
    }
    else if( m_holeX.GetValue() == 0 )
    {
        if( m_currentPad )
        {
            m_holeX.ChangeValue( m_currentPad->GetDrillSize().x );
            m_holeY.ChangeValue( m_currentPad->GetDrillSize().y );
        }
        else
        {
            m_holeX.ChangeValue( pcbIUScale.mmToIU( DEFAULT_PAD_DRILL_DIAMETER_MM ) );
        }
    }

    if( !hasConnection )
    {
        m_padNumCtrl->ChangeValue( wxEmptyString );
        m_padNetSelector->SetSelectedNetcode( 0 );
        m_padToDieOpt->SetValue( false );
        m_padToDieDelayOpt->SetValue( false );
    }
    else if( m_padNumCtrl->GetValue().IsEmpty() && m_currentPad )
    {
        m_padNumCtrl->ChangeValue( m_currentPad->GetNumber() );
        m_padNetSelector->SetSelectedNetcode( m_currentPad->GetNetCode() );
    }

    transferDataToPad( m_previewPad );

    // For now, padstack controls only enabled for PTH pads
    bool enablePadstack = m_padType->GetSelection() == PTH_DLG_TYPE;
    m_padstackControls->Show( enablePadstack );

    if( !enablePadstack )
    {
        m_editLayer = F_Cu;
        afterPadstackModeChanged();
    }

    // Layout adjustment is needed if the hole details got shown/hidden
    m_LeftBoxSizer->Layout();
    redraw();

    if( m_initialized )
        OnModify();
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
    m_padToDieDelayOpt->Enable( hasConnection );

    if( !m_padToDieOpt->IsEnabled() )
        m_padToDieOpt->SetValue( false );

    if( !m_padToDieDelayOpt->IsEnabled() )
        m_padToDieDelayOpt->SetValue( false );

    // We can show/hide this here because it doesn't require the layout to be refreshed.
    // All the others have to be done in their event handlers because doing a layout here
    // causes infinite looping on MSW.
    m_padToDie.Show( m_padToDieOpt->GetValue() );
    m_padToDieDelay.Show( m_padToDieDelayOpt->GetValue() );

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

    m_legacyTeardropsWarning->Show( m_board->LegacyTeardrops() );
}


void DIALOG_PAD_PROPERTIES::onTeardropsUpdateUi( wxUpdateUIEvent& event )
{
    event.Enable( !m_board->LegacyTeardrops() );
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
        m_stackupImage0->SetBitmap( KiBitmapBundle( BITMAPS::pads_reset_unused ) );
        m_stackupImage1->SetBitmap( KiBitmapBundle( BITMAPS::pads_remove_unused_keep_bottom ) );
        m_stackupImage2->SetBitmap( KiBitmapBundle( BITMAPS::pads_remove_unused ) );
        m_stackupImage4->SetBitmap( KiBitmapBundle( BITMAPS::pads_npth_top_bottom ) );
        m_stackupImage5->SetBitmap( KiBitmapBundle( BITMAPS::pads_npth_top ) );
        m_stackupImage6->SetBitmap( KiBitmapBundle( BITMAPS::pads_npth_bottom ) );
        m_stackupImage7->SetBitmap( KiBitmapBundle( BITMAPS::pads_npth ) );

        Layout();
    }

    return retVal;
}


void DIALOG_PAD_PROPERTIES::OnSetCopperLayers( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    redraw();

    if( m_initialized )
        OnModify();
}


void DIALOG_PAD_PROPERTIES::OnSetLayers( wxCommandEvent& event )
{
    transferDataToPad( m_previewPad );
    redraw();

    if( m_initialized )
        OnModify();
}


bool DIALOG_PAD_PROPERTIES::padValuesOK()
{
    transferDataToPad( m_previewPad );

    wxArrayString error_msgs;
    wxArrayString warning_msgs;

    m_previewPad->CheckPad( m_parentFrame, true,
            [&]( int errorCode, const wxString& msg )
            {
                if( errorCode == DRCE_PADSTACK_INVALID )
                    error_msgs.Add( _( "Error: " ) + msg );
                else if( errorCode == DRCE_PADSTACK )
                    warning_msgs.Add( _( "Warning: " ) + msg );
                else if( errorCode == DRCE_PAD_TH_WITH_NO_HOLE )
                    error_msgs.Add( _( "Error: Through hole pad has no hole." ) );
            } );

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
    if( !m_initialized )
        return;

    KIGFX::VIEW*                view = m_padPreviewGAL->GetView();
    KIGFX::PCB_PAINTER*         painter = static_cast<KIGFX::PCB_PAINTER*>( view->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings = painter->GetSettings();

    m_padPreviewGAL->StopDrawing();

    // The layer used to place primitive items selected when editing custom pad shapes
    // we use here a layer never used in a pad:
    #define SELECTED_ITEMS_LAYER Dwgs_User

    view->ClearTopLayers();
    view->SetTopLayer( SELECTED_ITEMS_LAYER );
    view->SetTopLayer( m_editLayer );
    settings->SetLayerColor( SELECTED_ITEMS_LAYER, m_selectedColor );

    static const std::vector<int> topLayers = {
        LAYER_PAD_PLATEDHOLES,
        LAYER_PAD_HOLEWALLS,
        LAYER_NON_PLATEDHOLES,
        LAYER_PAD_NETNAMES
    };

    for( int layer : topLayers )
        view->SetTopLayer( layer );

    m_axisOrigin->SetPosition( m_previewPad->GetPosition() );

    view->Update( m_previewPad );

    // delete previous items if highlight list
    while( m_highlight.size() )
    {
        delete m_highlight.back(); // the dtor also removes item from view
        m_highlight.pop_back();
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

    m_padPreviewGAL->StopDrawing();

    PAD_TOOL* padTool = m_parent->GetToolManager()->GetTool<PAD_TOOL>();
    padTool->SetLastPadNumber( m_masterPad->GetNumber() );

    // m_masterPad is a pattern: ensure there is no net for this pad:
    m_masterPad->SetNetCode( NETINFO_LIST::UNCONNECTED );

    if( !m_currentPad )   // Set current Pad parameters
        return true;

    commit.Modify( m_currentPad );

    // Update values

    // transferDataToPad only handles the current edit layer, so m_masterPad isn't accurate
    // TODO(JE) this could be cleaner
    m_currentPad->SetPadstack( m_previewPad->Padstack() );

    m_currentPad->SetAttribute( m_masterPad->GetAttribute() );
    m_currentPad->SetFPRelativeOrientation( m_masterPad->GetOrientation() );
    m_currentPad->SetPadToDieLength( m_masterPad->GetPadToDieLength() );
    m_currentPad->SetPadToDieDelay( m_masterPad->GetPadToDieDelay() );
    m_currentPad->SetLayerSet( m_masterPad->GetLayerSet() );
    m_currentPad->SetNumber( m_masterPad->GetNumber() );

    int padNetcode = NETINFO_LIST::UNCONNECTED;

    // For PAD_ATTRIB::NPTH, ensure there is no net name selected
    if( m_masterPad->GetAttribute() != PAD_ATTRIB::NPTH  )
        padNetcode = m_padNetSelector->GetSelectedNetcode();

    m_currentPad->SetNetCode( padNetcode );

    m_currentPad->GetTeardropParams() = m_masterPad->GetTeardropParams();

    // Set the fabrication property:
    m_currentPad->SetProperty( getSelectedProperty() );

    // define the way the clearance area is defined in zones
    m_currentPad->SetCustomShapeInZoneOpt( m_masterPad->GetCustomShapeInZoneOpt() );

    if( m_currentPad->GetParentFootprint() && m_currentPad->GetParentFootprint()->IsFlipped() )
    {
        // flip pad (up/down) around its position
        m_currentPad->Flip( m_currentPad->GetPosition(), FLIP_DIRECTION::TOP_BOTTOM );
    }

    m_currentPad->SetPosition( m_masterPad->GetPosition() );

    m_parent->SetMsgPanel( m_currentPad );

    // redraw the area where the pad was
    m_parent->GetCanvas()->Refresh();

    commit.Push( _( "Edit Pad Properties" ) );

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
    case 6:  prop = PAD_PROP::MECHANICAL;     break;
    case 7:  prop = PAD_PROP::CASTELLATED;    break;
    case 8:  prop = PAD_PROP::PRESSFIT;       break;
    }

    return prop;
}


void DIALOG_PAD_PROPERTIES::updateHoleControls()
{
    bool isRound = ( m_holeShapeCtrl->GetSelection() == CHOICE_SHAPE_CIRCLE );

    if( isRound )
    {
        m_holeXLabel->SetLabel( _( "Diameter:" ) );
        m_holeY.Show( false );
    }
    else
    {
        m_holeXLabel->SetLabel( _( "Hole size X:" ) );
        m_holeY.Show( true );
    }

    m_holeXLabel->GetParent()->Layout();

    if( !isRound )
    {
        // Disable all
        m_backDrillChoice->Enable( false );
        m_backDrillTopLayer->Enable( false );
        m_backDrillTopLayerLabel->Enable( false );
        m_backDrillBottomLayer->Enable( false );
        m_backDrillBottomLayerLabel->Enable( false );

        m_topPostMachining->Enable( false );
        m_topPostMachineSize1Binder.Enable( false );
        m_topPostMachineSize2Binder.Enable( false );
        m_topPostMachineSize1Label->Enable( false );
        m_topPostMachineSize2Label->Enable( false );

        m_bottomPostMachining->Enable( false );
        m_bottomPostMachineSize1Binder.Enable( false );
        m_bottomPostMachineSize2Binder.Enable( false );
        m_bottomPostMachineSize1Label->Enable( false );
        m_bottomPostMachineSize2Label->Enable( false );
    }
    else
    {
        // Enable main choices
        m_backDrillChoice->Enable( true );
        m_topPostMachining->Enable( true );
        m_bottomPostMachining->Enable( true );

        // Update sub-controls based on selection
        wxCommandEvent dummy;
        onBackDrillChoice( dummy );
        onTopPostMachining( dummy );
        onBottomPostMachining( dummy );
    }
}


void DIALOG_PAD_PROPERTIES::updatePadSizeControls()
{
    if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CIRCLE
            || m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR )
    {
        m_sizeXLabel->SetLabel( _( "Diameter:" ) );
        m_sizeY.Show( false );
        m_bitmapTeardrop->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_sizes ) );
        m_minTrackWidthHint->SetLabel( _( "d" ) );
        m_stLenPercentHint->SetLabel( _( "d" ) );
        m_stWidthPercentHint->SetLabel( _( "d" ) );
    }
    else
    {
        m_sizeXLabel->SetLabel( _( "Pad size X:" ) );
        m_sizeY.Show( true );
        m_bitmapTeardrop->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_rect_sizes ) );
        m_minTrackWidthHint->SetLabel( _( "w" ) );
        m_stLenPercentHint->SetLabel( _( "w" ) );
        m_stWidthPercentHint->SetLabel( _( "w" ) );
    }

    m_sizeXLabel->GetParent()->Layout();
    resetSize();
    Layout();
    m_MainSizer->Fit( this );
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

    switch( m_cbPadstackMode->GetSelection() )
    {
    default:
    case 0: aPad->Padstack().SetMode( PADSTACK::MODE::NORMAL );           break;
    case 1: aPad->Padstack().SetMode( PADSTACK::MODE::FRONT_INNER_BACK ); break;
    case 2: aPad->Padstack().SetMode( PADSTACK::MODE::CUSTOM );           break;
    }

    aPad->SetAttribute( code_type[m_padType->GetSelection()] );
    aPad->SetShape( m_editLayer, code_shape[m_PadShapeSelector->GetSelection()] );

    if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CUSTOM_RECT_ANCHOR )
        aPad->SetAnchorPadShape( m_editLayer, PAD_SHAPE::RECTANGLE );
    else
        aPad->SetAnchorPadShape( m_editLayer, PAD_SHAPE::CIRCLE );

    if( aPad->GetShape( m_editLayer ) == PAD_SHAPE::CUSTOM )
        aPad->ReplacePrimitives( m_editLayer, m_primitives );

    aPad->GetTeardropParams().m_Enabled = m_cbTeardrops->GetValue();
    aPad->GetTeardropParams().m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack->GetValue();
    aPad->GetTeardropParams().m_TdOnPadsInZones = !m_cbPreferZoneConnection->GetValue();
    aPad->GetTeardropParams().m_TdMaxLen = m_teardropMaxLenSetting.GetIntValue();
    aPad->GetTeardropParams().m_TdMaxWidth = m_teardropMaxHeightSetting.GetIntValue();
    aPad->GetTeardropParams().m_BestLengthRatio = m_spTeardropLenPercent->GetValue() / 100;
    aPad->GetTeardropParams().m_BestWidthRatio = m_spTeardropSizePercent->GetValue() / 100;
    aPad->GetTeardropParams().m_CurvedEdges = m_curvedEdges->GetValue();
    aPad->GetTeardropParams().m_WidthtoSizeFilterRatio = m_spTeardropHDPercent->GetValue() / 100;

    // Read pad clearances values:
    if( m_clearance.IsNull() )
        aPad->SetLocalClearance( {} );
    else
        aPad->SetLocalClearance( m_clearance.GetIntValue() );

    if( m_maskMargin.IsNull() )
        aPad->SetLocalSolderMaskMargin( {} );
    else
        aPad->SetLocalSolderMaskMargin( m_maskMargin.GetIntValue() );

    aPad->SetLocalSolderPasteMargin( m_pasteMargin.GetOffsetValue() );
    aPad->SetLocalSolderPasteMarginRatio( m_pasteMargin.GetRatioValue() );

    if( m_spokeWidth.IsNull() )
        aPad->SetLocalThermalSpokeWidthOverride( {} );
    else
        aPad->SetLocalThermalSpokeWidthOverride( m_spokeWidth.GetIntValue() );

    if( m_thermalGap.IsNull() )
        aPad->SetLocalThermalGapOverride( {} );
    else
        aPad->SetLocalThermalGapOverride( m_thermalGap.GetIntValue() );

    aPad->SetThermalSpokeAngle( m_spokeAngle.GetAngleValue() );

    // And rotation
    aPad->SetOrientation( m_pad_orientation.GetAngleValue() );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0: aPad->SetLocalZoneConnection( ZONE_CONNECTION::INHERITED ); break;
    case 1: aPad->SetLocalZoneConnection( ZONE_CONNECTION::FULL );      break;
    case 2: aPad->SetLocalZoneConnection( ZONE_CONNECTION::THERMAL );   break;
    case 3: aPad->SetLocalZoneConnection( ZONE_CONNECTION::NONE );      break;
    }

    VECTOR2I pos( m_posX.GetIntValue(), m_posY.GetIntValue() );

    if( FOOTPRINT* fp = aPad->GetParentFootprint() )
    {
        pos -= fp->GetPosition();
        RotatePoint( pos, -fp->GetOrientation() );
    }

    aPad->SetPosition( pos );

    if( m_holeShapeCtrl->GetSelection() == CHOICE_SHAPE_CIRCLE )
    {
        aPad->SetDrillShape( PAD_DRILL_SHAPE::CIRCLE );
        aPad->SetDrillSize( VECTOR2I( m_holeX.GetIntValue(), m_holeX.GetIntValue() ) );
    }
    else
    {
        aPad->SetDrillShape( PAD_DRILL_SHAPE::OBLONG );
        aPad->SetDrillSize( VECTOR2I( m_holeX.GetIntValue(), m_holeY.GetIntValue() ) );
    }

    if( aPad->GetShape( m_editLayer ) == PAD_SHAPE::CIRCLE )
        aPad->SetSize( m_editLayer, VECTOR2I( m_sizeX.GetIntValue(), m_sizeX.GetIntValue() ) );
    else
        aPad->SetSize( m_editLayer, VECTOR2I( m_sizeX.GetIntValue(), m_sizeY.GetIntValue() ) );

    // For a trapezoid, test delta value (be sure delta is not too large for pad size)
    // remember DeltaSize.x is the Y size variation
    bool   error    = false;
    VECTOR2I delta( 0, 0 );

    if( aPad->GetShape( m_editLayer ) == PAD_SHAPE::TRAPEZOID )
    {
        // For a trapezoid, only one of delta.x or delta.y is not 0, depending on axis.
        if( m_trapAxisCtrl->GetSelection() == 0 )
            delta.x = m_trapDelta.GetIntValue();
        else
            delta.y = m_trapDelta.GetIntValue();

        if( delta.x < 0 && delta.x < -aPad->GetSize( m_editLayer ).y )
        {
            delta.x = -aPad->GetSize( m_editLayer ).y + 2;
            error = true;
        }

        if( delta.x > 0 && delta.x > aPad->GetSize( m_editLayer ).y )
        {
            delta.x = aPad->GetSize( m_editLayer ).y - 2;
            error = true;
        }

        if( delta.y < 0 && delta.y < -aPad->GetSize( m_editLayer ).x )
        {
            delta.y = -aPad->GetSize( m_editLayer ).x + 2;
            error = true;
        }

        if( delta.y > 0 && delta.y > aPad->GetSize( m_editLayer ).x )
        {
            delta.y = aPad->GetSize( m_editLayer ).x - 2;
            error = true;
        }
    }

    aPad->SetDelta( m_editLayer, delta );

    if( m_offsetShapeOpt->GetValue() )
        aPad->SetOffset( m_editLayer, VECTOR2I( m_offsetX.GetIntValue(), m_offsetY.GetIntValue() ) );
    else
        aPad->SetOffset( m_editLayer, VECTOR2I() );

    // Read pad length die
    if( m_padToDieOpt->GetValue() )
        aPad->SetPadToDieLength( m_padToDie.GetIntValue() );
    else
        aPad->SetPadToDieLength( 0 );

    if( m_padToDieDelayOpt->GetValue() )
        aPad->SetPadToDieDelay( m_padToDieDelay.GetIntValue() );
    else
        aPad->SetPadToDieDelay( 0 );

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

    aPad->SetChamferPositions( m_editLayer, chamfers );

    if( aPad->GetShape( m_editLayer ) == PAD_SHAPE::CUSTOM )
    {
        // The pad custom has a "anchor pad" (a basic shape: round or rect pad)
        // that is the minimal area of this pad, and is useful to ensure a hole
        // diameter is acceptable, and is used in Gerber files as flashed area
        // reference
        if( aPad->GetAnchorPadShape( m_editLayer ) == PAD_SHAPE::CIRCLE )
            aPad->SetSize( m_editLayer, VECTOR2I( m_sizeX.GetIntValue(), m_sizeX.GetIntValue() ) );
    }

    // Define the way the clearance area is defined in zones.  Since all non-custom pad
    // shapes are convex to begin with, this really only makes any difference for custom
    // pad shapes.
    aPad->SetCustomShapeInZoneOpt( m_ZoneCustomPadShape->GetSelection() == 0 ? CUSTOM_SHAPE_ZONE_MODE::OUTLINE
                                                                             : CUSTOM_SHAPE_ZONE_MODE::CONVEXHULL );

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

    if( aPad->GetShape( m_editLayer ) == PAD_SHAPE::ROUNDRECT )
    {
        aPad->SetRoundRectRadiusRatio( m_editLayer, m_cornerRatio.GetDoubleValue() / 100.0 );
    }
    else if( aPad->GetShape( m_editLayer ) == PAD_SHAPE::CHAMFERED_RECT )
    {
        if( m_PadShapeSelector->GetSelection() == CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT )
        {
            aPad->SetChamferRectRatio( m_editLayer, m_mixedChamferRatio.GetDoubleValue() / 100.0 );
            aPad->SetRoundRectRadiusRatio( m_editLayer, m_mixedCornerRatio.GetDoubleValue() / 100.0 );
        }
        else    // Choice is CHOICE_SHAPE_CHAMFERED_RECT, no rounded corner
        {
            aPad->SetChamferRectRatio( m_editLayer, m_chamferRatio.GetDoubleValue() / 100.0 );
            aPad->SetRoundRectRadiusRatio( m_editLayer, 0 );
        }
    }

    aPad->SetProperty( getSelectedProperty() );

    LSET padLayerMask = LSET();
    int  copperLayersChoice = m_rbCopperLayersSel->GetSelection();

    aPad->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::KEEP_ALL );

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
            aPad->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
            break;

        case 2:
            // Connected only
            padLayerMask |= LSET::AllCuMask();
            aPad->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_ALL );
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

    // Save backdrill properties
    PADSTACK::DRILL_PROPS secondaryDrill;
    secondaryDrill.size = VECTOR2I( m_backDrillBottomSizeBinder.GetIntValue(),
                                    m_backDrillBottomSizeBinder.GetIntValue() );
    secondaryDrill.shape = PAD_DRILL_SHAPE::CIRCLE;

    PADSTACK::DRILL_PROPS tertiaryDrill;
    tertiaryDrill.size = VECTOR2I( m_backDrillTopSizeBinder.GetIntValue(),
                                   m_backDrillTopSizeBinder.GetIntValue() );
    tertiaryDrill.shape = PAD_DRILL_SHAPE::CIRCLE;

    if( !m_backDrillChoice->GetSelection() )
    {
        secondaryDrill.start = UNDEFINED_LAYER;
        secondaryDrill.end = UNDEFINED_LAYER;
    }

    if( m_backDrillChoice->GetSelection() == 1 || m_backDrillChoice->GetSelection() == 3 ) // Front
    {
        tertiaryDrill.start = F_Cu;

        if( m_backDrillTopLayer->GetSelection() != wxNOT_FOUND )
            tertiaryDrill.end = ToLAYER_ID( (intptr_t)m_backDrillTopLayer->GetClientData( m_backDrillTopLayer->GetSelection() ) );
        else
            tertiaryDrill.end = UNDEFINED_LAYER;
    }

    if( m_backDrillChoice->GetSelection() == 2 || m_backDrillChoice->GetSelection() == 3 ) // Back
    {
        secondaryDrill.start = B_Cu;

        if( m_backDrillBottomLayer->GetSelection() != wxNOT_FOUND )
            secondaryDrill.end = ToLAYER_ID( (intptr_t)m_backDrillBottomLayer->GetClientData( m_backDrillBottomLayer->GetSelection() ) );
        else
            secondaryDrill.end = UNDEFINED_LAYER;
    }

    aPad->Padstack().SecondaryDrill() = secondaryDrill;
    aPad->Padstack().TertiaryDrill() = tertiaryDrill;

    // Front Post Machining
    PADSTACK::POST_MACHINING_PROPS frontPostMachining;

    switch( m_topPostMachining->GetSelection() )
    {
    case 1:  frontPostMachining.mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;       break;
    case 2:  frontPostMachining.mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE;       break;
    default: frontPostMachining.mode = PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED; break;
    }

    frontPostMachining.size = m_topPostMachineSize1Binder.GetIntValue();

    if( frontPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        frontPostMachining.angle = m_topPostMachineSize2Binder.GetIntValue();
    else
        frontPostMachining.depth = m_topPostMachineSize2Binder.GetIntValue();

    aPad->Padstack().FrontPostMachining() = frontPostMachining;

    // Back Post Machining
    PADSTACK::POST_MACHINING_PROPS backPostMachining;

    switch( m_bottomPostMachining->GetSelection() )
    {
    case 1:  backPostMachining.mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK;       break;
    case 2:  backPostMachining.mode = PAD_DRILL_POST_MACHINING_MODE::COUNTERBORE;       break;
    default: backPostMachining.mode = PAD_DRILL_POST_MACHINING_MODE::NOT_POST_MACHINED; break;
    }

    backPostMachining.size = m_bottomPostMachineSize1Binder.GetIntValue();

    if( backPostMachining.mode == PAD_DRILL_POST_MACHINING_MODE::COUNTERSINK )
        backPostMachining.angle = m_bottomPostMachineSize2Binder.GetIntValue();
    else
        backPostMachining.depth = m_bottomPostMachineSize2Binder.GetIntValue();

    aPad->Padstack().BackPostMachining() = backPostMachining;

    return !error;
}




void DIALOG_PAD_PROPERTIES::onBackDrillChoice( wxCommandEvent& event )
{
    int selection = m_backDrillChoice->GetSelection();
    // 0: None, 1: Top, 2: Bottom, 3: Both

    bool enableTop = ( selection == 1 || selection == 3 );
    bool enableBottom = ( selection == 2 || selection == 3 );

    m_backDrillTopSizeBinder.Enable( enableTop );
    m_backDrillTopLayer->Enable( enableTop );
    m_backDrillTopLayerLabel->Enable( enableTop );

    m_backDrillBottomSizeBinder.Enable( enableBottom );
    m_backDrillBottomLayer->Enable( enableBottom );
    m_backDrillBottomLayerLabel->Enable( enableBottom );

}


void DIALOG_PAD_PROPERTIES::onTopPostMachining( wxCommandEvent& event )
{
    int selection = m_topPostMachining->GetSelection();
    // 0: None, 1: Countersink, 2: Counterbore

    bool enable = ( selection != 0 );
    m_topPostMachineSize1Binder.Enable( enable );
    m_topPostMachineSize2Binder.Enable( enable );
    m_topPostMachineSize1Label->Enable( enable );
    m_topPostMachineSize2Label->Enable( enable );

    if( selection == 1 ) // Countersink
    {
        m_topPostMachineSize2Label->SetLabel( _( "Angle:" ) );
        m_topPostMachineSize2Units->SetLabel( _( "deg" ) );

        if( m_topPostMachineSize2Binder.IsIndeterminate() || m_topPostMachineSize2Binder.GetDoubleValue() == 0 )
             m_topPostMachineSize2Binder.SetValue( "82" );
    }
    else if( selection == 2 ) // Counterbore
    {
        m_topPostMachineSize2Label->SetLabel( _( "Depth:" ) );
        m_topPostMachineSize2Units->SetLabel( EDA_UNIT_UTILS::GetLabel( m_parent->GetUserUnits() ) );
    }
}


void DIALOG_PAD_PROPERTIES::onBottomPostMachining( wxCommandEvent& event )
{
    int selection = m_bottomPostMachining->GetSelection();
    // 0: None, 1: Countersink, 2: Counterbore

    bool enable = ( selection != 0 );
    m_bottomPostMachineSize1Binder.Enable( enable );
    m_bottomPostMachineSize2Binder.Enable( enable );
    m_bottomPostMachineSize1Label->Enable( enable );
    m_bottomPostMachineSize2Label->Enable( enable );

    if( selection == 1 ) // Countersink
    {
        m_bottomPostMachineSize2Label->SetLabel( _( "Angle:" ) );
        m_bottomPostMachineSize2Units->SetLabel( _( "deg" ) );

        if( m_bottomPostMachineSize2Binder.IsIndeterminate() || m_bottomPostMachineSize2Binder.GetDoubleValue() == 0 )
             m_bottomPostMachineSize2Binder.SetValue( "82" );
    }
    else if( selection == 2 ) // Counterbore
    {
        m_bottomPostMachineSize2Label->SetLabel( _( "Depth:" ) );
        m_bottomPostMachineSize2Units->SetLabel( EDA_UNIT_UTILS::GetLabel( m_parent->GetUserUnits() ) );
    }
}


void DIALOG_PAD_PROPERTIES::OnOffsetCheckbox( wxCommandEvent& event )
{
    if( m_offsetShapeOpt->GetValue() )
    {
        m_offsetX.SetValue( m_previewPad->GetOffset( m_editLayer ).x );
        m_offsetY.SetValue( m_previewPad->GetOffset( m_editLayer ).y );
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


void DIALOG_PAD_PROPERTIES::OnPadToDieDelayCheckbox( wxCommandEvent& event )
{
    if( m_padToDieDelayOpt->GetValue() && m_currentPad )
        m_padToDieDelay.SetValue( m_currentPad->GetPadToDieDelay() );

    OnValuesChanged( event );
}


void DIALOG_PAD_PROPERTIES::onModify( wxSpinDoubleEvent& aEvent )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_PAD_PROPERTIES::onModify( wxCommandEvent& aEvent )
{
    if( m_initialized )
        OnModify();
}


void DIALOG_PAD_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( m_initialized )
    {
        if( !transferDataToPad( m_previewPad ) )
            return;

        // If the pad size has changed, update the displayed values for rounded rect pads.
        updateAllowedPadChamferCorners();
        updateRoundRectCornerValues();

        redraw();
        OnModify();
    }
}
