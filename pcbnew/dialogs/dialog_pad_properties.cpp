/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <base_units.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <class_board.h>
#include <class_module.h>
#include <confirm.h>
#include <convert_basic_shapes_to_polygon.h> // for enum RECT_CHAMFER_POSITIONS definition
#include <dialog_pad_properties.h>
#include <gal/graphics_abstraction_layer.h>
#include <html_messagebox.h>
#include <macros.h>
#include <pcb_base_frame.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <settings/color_settings.h>
#include <view/view_controls.h>
#include <widgets/net_selector.h>
#include <tool/tool_manager.h>
#include <tools/footprint_editor_tools.h>
#include <advanced_config.h>    // for pad property feature management


// list of pad shapes, ordered like the pad shape wxChoice in dialog.
static PAD_SHAPE_T code_shape[] =
{
    PAD_SHAPE_CIRCLE,
    PAD_SHAPE_OVAL,
    PAD_SHAPE_RECT,
    PAD_SHAPE_TRAPEZOID,
    PAD_SHAPE_ROUNDRECT,
    PAD_SHAPE_CHAMFERED_RECT,
    PAD_SHAPE_CHAMFERED_RECT,  // choice = CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT
    PAD_SHAPE_CUSTOM,          // choice = CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR
    PAD_SHAPE_CUSTOM           // choice = PAD_SHAPE_CUSTOM_RECT_ANCHOR
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

static PAD_ATTR_T code_type[] =
{
    PAD_ATTRIB_STANDARD,
    PAD_ATTRIB_SMD,
    PAD_ATTRIB_CONN,
    PAD_ATTRIB_HOLE_NOT_PLATED,
    PAD_ATTRIB_SMD                  // Aperture pad :type SMD with no copper layers,
                                    // only on tech layers (usually only on paste layer
};

// Default mask layers setup for pads according to the pad type
static const LSET std_pad_layers[] =
{
    D_PAD::StandardMask(),        // PAD_ATTRIB_STANDARD:
    D_PAD::SMDMask(),             // PAD_ATTRIB_SMD:
    D_PAD::ConnSMDMask(),         // PAD_ATTRIB_CONN:
    D_PAD::UnplatedHoleMask(),    // PAD_ATTRIB_HOLE_NOT_PLATED:
    D_PAD::ApertureMask()
};


void PCB_BASE_FRAME::InstallPadOptionsFrame( D_PAD* aPad )
{
    DIALOG_PAD_PROPERTIES dlg( this, aPad );

    if( dlg.ShowQuasiModal() == wxID_OK )       // QuasiModal required for NET_SELECTOR
    {
        // aPad can be NULL, if the dialog is called from the footprint editor
        // to set the default pad setup
        if( aPad )
        {
            FOOTPRINT_EDITOR_TOOLS* fpTools = m_toolManager->GetTool<FOOTPRINT_EDITOR_TOOLS>();

            if( fpTools )
                fpTools->SetLastPadName( aPad->GetName() );
        }
    }
}


DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, D_PAD* aPad ) :
    DIALOG_PAD_PROPERTIES_BASE( aParent ),
    m_parent( aParent ),
    m_canUpdate( false ),
    m_posX( aParent, m_posXLabel, m_posXCtrl, m_posXUnits ),
    m_posY( aParent, m_posYLabel, m_posYCtrl, m_posYUnits ),
    m_sizeX( aParent, m_sizeXLabel, m_sizeXCtrl, m_sizeXUnits, true ),
    m_sizeY( aParent, m_sizeYLabel, m_sizeYCtrl, m_sizeYUnits, true ),
    m_offsetX( aParent, m_offsetXLabel, m_offsetXCtrl, m_offsetXUnits, true ),
    m_offsetY( aParent, m_offsetYLabel, m_offsetYCtrl, m_offsetYUnits, true ),
    m_padToDie( aParent, m_padToDieLabel, m_padToDieCtrl, m_padToDieUnits, true ),
    m_trapDelta( aParent, m_trapDeltaLabel, m_trapDeltaCtrl, m_trapDeltaUnits, true ),
    m_cornerRadius( aParent, m_cornerRadiusLabel, m_tcCornerRadius, m_cornerRadiusUnits, true ),
    m_holeX( aParent, m_holeXLabel, m_holeXCtrl, m_holeXUnits, true ),
    m_holeY( aParent, m_holeYLabel, m_holeYCtrl, m_holeYUnits, true ),
    m_OrientValidator( 1, &m_OrientValue ),
    m_clearance( aParent, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits, true ),
    m_maskClearance( aParent, m_maskClearanceLabel, m_maskClearanceCtrl, m_maskClearanceUnits, true ),
    m_pasteClearance( aParent, m_pasteClearanceLabel, m_pasteClearanceCtrl, m_pasteClearanceUnits, true ),
    m_spokeWidth( aParent, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits, true ),
    m_thermalGap( aParent, m_thermalGapLabel, m_thermalGapCtrl, m_thermalGapUnits, true )
{
    m_currentPad = aPad;        // aPad can be NULL, if the dialog is called
                                // from the footprint editor to set default pad setup

    m_board      = m_parent->GetBoard();

    // Disable the pad property if not allowed in advanced config
    if( !ADVANCED_CFG::GetCfg().m_EnableUsePadProperty )
    {
        m_staticTextFabProperty->Show( false );
        m_choiceFabProperty->Show( false );
    }

    m_PadNetSelector->SetBoard( m_board );
    m_PadNetSelector->SetNetInfo( &m_board->GetNetInfo() );

    m_OrientValidator.SetRange( -360.0, 360.0 );
    m_orientation->SetValidator( m_OrientValidator );
    m_OrientValidator.SetWindow( m_orientation );

    m_cbShowPadOutline->SetValue( m_sketchPreview );

    m_FlippedWarningIcon->SetBitmap( KiBitmap( dialog_warning_xpm ) );
    m_nonCopperWarningIcon->SetBitmap( KiBitmap( dialog_warning_xpm ) );

    m_padMaster  = &m_parent->GetDesignSettings().m_Pad_Master;
    m_dummyPad   = new D_PAD( (MODULE*) NULL );

    if( aPad )
    {
        *m_dummyPad = *aPad;
        m_dummyPad->ClearFlags( SELECTED|HIGHLIGHTED|BRIGHTENED );
    }
    else    // We are editing a "master" pad, i.e. a template to create new pads
        *m_dummyPad = *m_padMaster;

    initValues();

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_techLayersLabel->SetFont( infoFont );
    m_parentInfoLine1->SetFont( infoFont );
    m_parentInfoLine2->SetFont( infoFont );

    infoFont.SetStyle( wxFONTSTYLE_ITALIC );
    m_nonCopperNote->SetFont( infoFont );
    m_staticTextInfoPaste->SetFont( infoFont );
    m_staticTextInfoNegVal->SetFont( infoFont );
    m_staticTextInfoPosValue->SetFont( infoFont );
    m_staticTextPrimitiveListWarning->SetFont( infoFont );

    // Usually, TransferDataToWindow is called by OnInitDialog
    // calling it here fixes all widget sizes so FinishDialogSettings can safely fix minsizes
    TransferDataToWindow();

    // Initialize canvas to be able to display the dummy pad:
    prepareCanvas();

    SetInitialFocus( m_PadNumCtrl );
    m_sdbSizerOK->SetDefault();
    m_canUpdate = true;

    m_PadNetSelector->Connect( NET_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES::OnValuesChanged ), NULL, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();

    wxUpdateUIEvent dummy;
    OnUpdateUI( dummy );
}


DIALOG_PAD_PROPERTIES::~DIALOG_PAD_PROPERTIES()
{
    m_PadNetSelector->Disconnect( NET_SELECTED, wxCommandEventHandler( DIALOG_PAD_PROPERTIES::OnValuesChanged ), NULL, this );

    delete m_dummyPad;
    delete m_axisOrigin;
}


bool DIALOG_PAD_PROPERTIES::m_sketchPreview = false;   // Stores the pad draw option during a session


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
#ifdef __WXMAC__
    // Cairo renderer doesn't handle Retina displays
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
#else
    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
#endif
    m_padPreviewGAL = new PCB_DRAW_PANEL_GAL( this, -1, wxDefaultPosition, wxDefaultSize,
                                              m_parent->GetGalDisplayOptions(), backend );

    m_padPreviewSizer->Add( m_padPreviewGAL, 12, wxEXPAND | wxALL, 5 );

    // Show the X and Y axis. It is usefull because pad shape can have an offset
    // or be a complex shape.
    KIGFX::COLOR4D axis_color = LIGHTBLUE;

    m_axisOrigin = new KIGFX::ORIGIN_VIEWITEM( axis_color, KIGFX::ORIGIN_VIEWITEM::CROSS,
                                               Millimeter2iu( 0.2 ),
                                               VECTOR2D( m_dummyPad->GetPosition() ) );
    m_axisOrigin->SetDrawAtZero( true );

    m_padPreviewGAL->UpdateColors();
    m_padPreviewGAL->SetStealsFocus( false );

    m_padPreviewGAL->GetViewControls()->ApplySettings(
            m_parent->GetCanvas()->GetViewControls()->GetSettings() );

    m_padPreviewGAL->Show();

    KIGFX::VIEW* view = m_padPreviewGAL->GetView();

    // fix the pad render mode (filled/not filled)
    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    bool sketchMode = m_cbShowPadOutline->IsChecked();
    settings->SetSketchMode( LAYER_PADS_TH, sketchMode );
    settings->SetSketchMode( LAYER_PAD_FR, sketchMode );
    settings->SetSketchMode( LAYER_PAD_BK, sketchMode );
    settings->SetSketchModeGraphicItems( sketchMode );

    // gives a non null grid size (0.001mm) because GAL layer does not like a 0 size grid:
    double gridsize = 0.001 * IU_PER_MM;
    view->GetGAL()->SetGridSize( VECTOR2D( gridsize, gridsize ) );
    // And do not show the grid:
    view->GetGAL()->SetGridVisibility( false );
    view->Add( m_dummyPad );
    view->Add( m_axisOrigin );

    m_padPreviewGAL->StartDrawing();
    Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PROPERTIES::OnResize ) );
}


void DIALOG_PAD_PROPERTIES::updateRoundRectCornerValues()
{
    // Note: use m_tcCornerSizeRatio->ChangeValue() to avoid generating a wxEVT_TEXT event

    if( m_dummyPad->GetShape() == PAD_SHAPE_ROUNDRECT ||
        m_dummyPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
    {
        wxString ratio = wxString::Format( "%.1f", m_dummyPad->GetRoundRectRadiusRatio() * 100 );
        m_tcCornerSizeRatio->ChangeValue( ratio );
        m_tcCornerSizeRatio1->ChangeValue( ratio );
        m_cornerRadius.ChangeValue( m_dummyPad->GetRoundRectCornerRadius() );

        ratio = wxString::Format( "%.1f", m_dummyPad->GetChamferRectRatio() * 100 );
        m_tcChamferRatio->ChangeValue( ratio );
        m_tcChamferRatio1->ChangeValue( ratio );
    }
}


void DIALOG_PAD_PROPERTIES::onCornerRadiusChange( wxCommandEvent& event )
{
    if( m_dummyPad->GetShape() != PAD_SHAPE_ROUNDRECT &&
        m_dummyPad->GetShape() != PAD_SHAPE_CHAMFERED_RECT )
        return;

    double rrRadius = m_cornerRadius.GetValue();

    if( rrRadius < 0.0 )
    {
        rrRadius = 0.0;
        m_tcCornerRadius->ChangeValue( wxString::Format( "%.1f", rrRadius ) );
    }

    transferDataToPad( m_dummyPad );
    m_dummyPad->SetRoundRectCornerRadius( rrRadius );

    auto ratio = wxString::Format( "%.1f", m_dummyPad->GetRoundRectRadiusRatio() * 100 );
    m_tcCornerSizeRatio->ChangeValue( ratio );
    m_tcCornerSizeRatio1->ChangeValue( ratio );
    redraw();
}


void DIALOG_PAD_PROPERTIES::onCornerSizePercentChange( wxCommandEvent& event )
{
    if( m_dummyPad->GetShape() != PAD_SHAPE_ROUNDRECT &&
        m_dummyPad->GetShape() != PAD_SHAPE_CHAMFERED_RECT )
    {
        return;
    }

    wxObject* ctrl = event.GetEventObject();
    wxString  value = event.GetString();
    bool      changed = false;

    if( ctrl == m_tcCornerSizeRatio || ctrl == m_tcCornerSizeRatio1 )
    {
        double ratioPercent;

        if( value.ToDouble( &ratioPercent ) )
        {
            // Clamp ratioPercent to acceptable value (0.0 to 50.0)
            if( ratioPercent < 0.0 )
            {
                ratioPercent = 0.0;
                value.Printf( "%.1f", ratioPercent );
                m_tcCornerSizeRatio->ChangeValue( value );
                m_tcCornerSizeRatio1->ChangeValue( value );
            }

            if( ratioPercent > 50.0 )
            {
                ratioPercent = 0.5;
                value.Printf( "%.1f", ratioPercent*100.0 );
                m_tcCornerSizeRatio->ChangeValue( value );
                m_tcCornerSizeRatio1->ChangeValue( value );
            }

            if( ctrl == m_tcCornerSizeRatio )
                m_tcCornerSizeRatio1->ChangeValue( value );
            else
                m_tcCornerSizeRatio->ChangeValue( value );

            changed = true;
        }
    }
    else if( ctrl == m_tcChamferRatio || ctrl == m_tcChamferRatio1 )
    {
        double ratioPercent;

        if( value.ToDouble( &ratioPercent ) )
        {
            // Clamp ratioPercent to acceptable value (0.0 to 50.0)
            if( ratioPercent < 0.0 )
            {
                ratioPercent = 0.0;
                value.Printf( "%.1f", ratioPercent );
                m_tcChamferRatio->ChangeValue( value );
                m_tcChamferRatio1->ChangeValue( value );
            }

            if( ratioPercent > 50.0 )
            {
                ratioPercent = 0.5;
                value.Printf( "%.1f", ratioPercent*100.0 );
                m_tcChamferRatio->ChangeValue( value );
                m_tcChamferRatio1->ChangeValue( value );
            }

            if( ctrl == m_tcChamferRatio )
                m_tcChamferRatio1->ChangeValue( value );
            else
                m_tcChamferRatio->ChangeValue( value );

            changed = true;
        }
    }

    if( changed )
    {
        transferDataToPad( m_dummyPad );
        m_cornerRadius.ChangeValue( m_dummyPad->GetRoundRectCornerRadius() );
        redraw();
    }
}


void DIALOG_PAD_PROPERTIES::initValues()
{
    wxString    msg;
    double      angle;

    // Disable pad net name wxTextCtrl if the caller is the footprint editor
    // because nets are living only in the board managed by the board editor
    m_canEditNetName = m_parent->IsType( FRAME_PCB_EDITOR );

    // Setup layers names from board
    // Should be made first, before calling m_rbCopperLayersSel->SetSelection()
    m_rbCopperLayersSel->SetString( 0, m_board->GetLayerName( F_Cu ) );
    m_rbCopperLayersSel->SetString( 1, m_board->GetLayerName( B_Cu ) );

    m_PadLayerAdhCmp->SetLabel( m_board->GetLayerName( F_Adhes ) );
    m_PadLayerAdhCu->SetLabel( m_board->GetLayerName( B_Adhes ) );
    m_PadLayerPateCmp->SetLabel( m_board->GetLayerName( F_Paste ) );
    m_PadLayerPateCu->SetLabel( m_board->GetLayerName( B_Paste ) );
    m_PadLayerSilkCmp->SetLabel( m_board->GetLayerName( F_SilkS ) );
    m_PadLayerSilkCu->SetLabel( m_board->GetLayerName( B_SilkS ) );
    m_PadLayerMaskCmp->SetLabel( m_board->GetLayerName( F_Mask ) );
    m_PadLayerMaskCu->SetLabel( m_board->GetLayerName( B_Mask ) );
    m_PadLayerECO1->SetLabel( m_board->GetLayerName( Eco1_User ) );
    m_PadLayerECO2->SetLabel( m_board->GetLayerName( Eco2_User ) );
    m_PadLayerDraft->SetLabel( m_board->GetLayerName( Dwgs_User ) );

    m_isFlipped = false;

    if( m_currentPad )
    {
        m_isFlipped = m_currentPad->IsFlipped();

        // Diplay parent footprint info
        MODULE* footprint = m_currentPad->GetParent();
        wxString msg1, msg2;

        if( footprint )
        {
            wxString side = footprint->IsFlipped() ? _( "back side (mirrored)" ) : _( "front side" );
            msg1.Printf( _("Footprint %s (%s),"), footprint->GetReference(), footprint->GetValue() );
            msg2.Printf( _("%s, rotated %.1f deg"), side, footprint->GetOrientation() / 10.0 );
        }

        m_parentInfoLine1->SetLabel( msg1 );
        m_parentInfoLine2->SetLabel( msg2 );
    }

    if( m_isFlipped )
    {
        wxPoint pt = m_dummyPad->GetOffset();
        pt.y = -pt.y;
        m_dummyPad->SetOffset( pt );

        wxSize sz = m_dummyPad->GetDelta();
        sz.y = -sz.y;
        m_dummyPad->SetDelta( sz );

        // flip pad's layers
        m_dummyPad->SetLayerSet( FlipLayerMask( m_dummyPad->GetLayerSet() ) );

        // flip custom pad shapes
        m_dummyPad->FlipPrimitives();
    }

    m_primitives = m_dummyPad->GetPrimitives();

    m_FlippedWarningSizer->Show( m_isFlipped );

    m_PadNumCtrl->SetValue( m_dummyPad->GetName() );
    m_PadNetSelector->SetSelectedNetcode( m_dummyPad->GetNetCode() );

    // Display current pad parameters units:
    m_posX.SetValue( m_dummyPad->GetPosition().x );
    m_posY.SetValue( m_dummyPad->GetPosition().y );

    m_holeX.SetValue( m_dummyPad->GetDrillSize().x );
    m_holeY.SetValue(  m_dummyPad->GetDrillSize().y );

    m_sizeX.SetValue( m_dummyPad->GetSize().x );
    m_sizeY.SetValue( m_dummyPad->GetSize().y );

    m_offsetShapeOpt->SetValue( m_dummyPad->GetOffset() != wxPoint() );
    m_offsetX.SetValue( m_dummyPad->GetOffset().x );
    m_offsetY.SetValue( m_dummyPad->GetOffset().y );

    if( m_dummyPad->GetDelta().x )
    {
        m_trapDelta.SetValue( m_dummyPad->GetDelta().x );
        m_trapAxisCtrl->SetSelection( 0 );
    }
    else
    {
        m_trapDelta.SetValue( m_dummyPad->GetDelta().y );
        m_trapAxisCtrl->SetSelection( 1 );
    }

    m_padToDieOpt->SetValue( m_dummyPad->GetPadToDieLength() != 0 );
    m_padToDie.SetValue( m_dummyPad->GetPadToDieLength() );

    m_clearance.SetValue( m_dummyPad->GetLocalClearance() );
    m_maskClearance.SetValue( m_dummyPad->GetLocalSolderMaskMargin() );
    m_spokeWidth.SetValue( m_dummyPad->GetThermalWidth() );
    m_thermalGap.SetValue( m_dummyPad->GetThermalGap() );
    m_pasteClearance.SetValue( m_dummyPad->GetLocalSolderPasteMargin() );

    // Prefer "-0" to "0" for normally negative values
    if( m_dummyPad->GetLocalSolderPasteMargin() == 0 )
        m_pasteClearanceCtrl->SetValue( wxT( "-" ) + m_pasteClearanceCtrl->GetValue() );

    msg.Printf( wxT( "%f" ), m_dummyPad->GetLocalSolderPasteMarginRatio() * 100.0 );

    if( m_dummyPad->GetLocalSolderPasteMarginRatio() == 0.0 && msg[0] == '0' )
        // Sometimes Printf adds a sign if the value is small
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    switch( m_dummyPad->GetZoneConnection() )
    {
    default:
    case ZONE_CONNECTION::INHERITED: m_ZoneConnectionChoice->SetSelection( 0 ); break;
    case ZONE_CONNECTION::FULL:      m_ZoneConnectionChoice->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THERMAL:   m_ZoneConnectionChoice->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:      m_ZoneConnectionChoice->SetSelection( 3 ); break;
    }

    if( m_dummyPad->GetCustomShapeInZoneOpt() == CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL )
        m_ZoneCustomPadShape->SetSelection( 1 );
    else
        m_ZoneCustomPadShape->SetSelection( 0 );

    if( m_currentPad )
    {
        angle = m_currentPad->GetOrientation();
        MODULE* footprint = m_currentPad->GetParent();

        if( footprint )
            angle -= footprint->GetOrientation();

        if( m_isFlipped )
            angle = -angle;

        m_dummyPad->SetOrientation( angle );
    }

    angle = m_dummyPad->GetOrientation();

    NORMALIZE_ANGLE_180( angle );    // ? normalizing is in D_PAD::SetOrientation()

    // Set layers used by this pad: :
    setPadLayersList( m_dummyPad->GetLayerSet() );

    // Pad Orient
    // Note: use ChangeValue() instead of SetValue() so that we don't generate events
    m_orientation->ChangeValue( StringFromValue( EDA_UNITS::DEGREES, angle ) );

    switch( m_dummyPad->GetShape() )
    {
    default:
    case PAD_SHAPE_CIRCLE:    m_PadShape->SetSelection( CHOICE_SHAPE_CIRCLE );    break;
    case PAD_SHAPE_OVAL:      m_PadShape->SetSelection( CHOICE_SHAPE_OVAL );      break;
    case PAD_SHAPE_RECT:      m_PadShape->SetSelection( CHOICE_SHAPE_RECT );      break;
    case PAD_SHAPE_TRAPEZOID: m_PadShape->SetSelection( CHOICE_SHAPE_TRAPEZOID ); break;
    case PAD_SHAPE_ROUNDRECT: m_PadShape->SetSelection( CHOICE_SHAPE_ROUNDRECT ); break;

    case PAD_SHAPE_CHAMFERED_RECT:
        if( m_dummyPad->GetRoundRectRadiusRatio() > 0.0 )
            m_PadShape->SetSelection( CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT );
        else
            m_PadShape->SetSelection( CHOICE_SHAPE_CHAMFERED_RECT );
        break;

    case PAD_SHAPE_CUSTOM:
        if( m_dummyPad->GetAnchorPadShape() == PAD_SHAPE_RECT )
            m_PadShape->SetSelection( CHOICE_SHAPE_CUSTOM_RECT_ANCHOR );
        else
            m_PadShape->SetSelection( CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );
        break;
    }

    m_cbTopLeft->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_TOP_LEFT) );
    m_cbTopLeft1->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_TOP_LEFT) );
    m_cbTopRight->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_TOP_RIGHT) );
    m_cbTopRight1->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_TOP_RIGHT) );
    m_cbBottomLeft->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_LEFT) );
    m_cbBottomLeft1->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_LEFT) );
    m_cbBottomRight->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_RIGHT) );
    m_cbBottomRight1->SetValue( (m_dummyPad->GetChamferPositions() & RECT_CHAMFER_BOTTOM_RIGHT) );

    updateRoundRectCornerValues();

    enablePrimitivePage( PAD_SHAPE_CUSTOM == m_dummyPad->GetShape() );

    // Type of pad selection
    bool aperture = m_dummyPad->GetAttribute() == PAD_ATTRIB_CONN && m_dummyPad->IsAperturePad();

    if( aperture )
    {
        m_PadType->SetSelection( 4 );
    }
    else
    {
        switch( m_dummyPad->GetAttribute() )
        {
        case PAD_ATTRIB_STANDARD:        m_PadType->SetSelection( 0 ); break;
        case PAD_ATTRIB_SMD:             m_PadType->SetSelection( 1 ); break;
        case PAD_ATTRIB_CONN:            m_PadType->SetSelection( 2 ); break;
        case PAD_ATTRIB_HOLE_NOT_PLATED: m_PadType->SetSelection( 3 ); break;
        }
    }

    switch( m_dummyPad->GetProperty() )
    {
    case PAD_PROP_NONE:             m_choiceFabProperty->SetSelection( 0 ); break;
    case PAD_PROP_BGA:              m_choiceFabProperty->SetSelection( 1 ); break;
    case PAD_PROP_FIDUCIAL_LOCAL:   m_choiceFabProperty->SetSelection( 2 ); break;
    case PAD_PROP_FIDUCIAL_GLBL:    m_choiceFabProperty->SetSelection( 3 ); break;
    case PAD_PROP_TESTPOINT:        m_choiceFabProperty->SetSelection( 4 ); break;
    case PAD_PROP_HEATSINK:         m_choiceFabProperty->SetSelection( 5 ); break;
    case PAD_PROP_CASTELLATED:      m_choiceFabProperty->SetSelection( 6 ); break;
    }

    // Ensure the pad property is compatible with the pad type
    if( m_dummyPad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
    {
        m_choiceFabProperty->SetSelection( 0 );
        m_choiceFabProperty->Enable( false );
    }

    if( m_dummyPad->GetDrillShape() != PAD_DRILL_SHAPE_OBLONG )
        m_holeShapeCtrl->SetSelection( 0 );
    else
        m_holeShapeCtrl->SetSelection( 1 );

    // Update some dialog widgets state (Enable/disable options):
    wxCommandEvent cmd_event;
    setPadLayersList( m_dummyPad->GetLayerSet() );
    OnPadShapeSelection( cmd_event );
    OnOffsetCheckbox( cmd_event );

    // Update basic shapes list
    displayPrimitivesList();
}

// A small helper function, to display coordinates:
static wxString formatCoord( EDA_UNITS aUnits, wxPoint aCoord )
{
    return wxString::Format( "(X:%s Y:%s)",
                             MessageTextFromValue( aUnits, aCoord.x, true ),
                             MessageTextFromValue( aUnits, aCoord.y, true ) );
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
        const PAD_CS_PRIMITIVE& primitive = m_primitives[ii];

        for( unsigned jj = 0; jj < 5; ++jj )
            bs_info[jj].Empty();

        bs_info[4] = wxString::Format( _( "width %s" ),
                                    MessageTextFromValue( m_units, primitive.m_Thickness, true ) );

        switch( primitive.m_Shape )
        {
        case S_SEGMENT:         // usual segment : line with rounded ends
            bs_info[0] = _( "Segment" );
            bs_info[1] = _( "from " ) + formatCoord( m_units, primitive.m_Start );
            bs_info[2] = _( "to " ) +  formatCoord( m_units, primitive.m_End );
            break;

        case S_CURVE:         // Bezier segment
            bs_info[0] = _( "Bezier" );
            bs_info[1] = _( "from " ) + formatCoord( m_units, primitive.m_Start );
            bs_info[2] = _( "to " ) +  formatCoord( m_units, primitive.m_End );
            break;

        case S_ARC:             // Arc with rounded ends
            bs_info[0] = _( "Arc" );
            bs_info[1] = _( "center " ) + formatCoord( m_units, primitive.m_Start );// Center
            bs_info[2] = _( "start " ) + formatCoord( m_units, primitive.m_End );   // Start point
            bs_info[3] = wxString::Format( _( "angle %s" ), FormatAngle( primitive.m_ArcAngle ) );
            break;

        case S_CIRCLE:          //  ring or circle
            if( primitive.m_Thickness )
                bs_info[0] = _( "ring" );
            else
                bs_info[0] = _( "circle" );

            bs_info[1] = formatCoord( m_units, primitive.m_Start );
            bs_info[2] = wxString::Format( _( "radius %s" ),
                                       MessageTextFromValue( m_units, primitive.m_Radius, true ) );
            break;

        case S_POLYGON:         // polygon
            bs_info[0] = "Polygon";
            bs_info[1] = wxString::Format( _( "corners count %d" ), (int) primitive.m_Poly.size() );
            break;

        default:
            bs_info[0] = "Unknown primitive";
            break;
        }

        long tmp = m_listCtrlPrimitives->InsertItem(ii, bs_info[0]);
        m_listCtrlPrimitives->SetItemData(tmp, ii);

        for( int jj = 0, col = 0; jj < 5; ++jj )
        {
            m_listCtrlPrimitives->SetItem(tmp, col++, bs_info[jj]);
        }
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
    KIGFX::PCB_RENDER_SETTINGS* settings =
        static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );

    settings->SetSketchMode( LAYER_PADS_TH, m_sketchPreview );
    settings->SetSketchMode( LAYER_PAD_FR, m_sketchPreview );
    settings->SetSketchMode( LAYER_PAD_BK, m_sketchPreview );
    settings->SetSketchModeGraphicItems( m_sketchPreview );

    redraw();
}



void DIALOG_PAD_PROPERTIES::OnPadShapeSelection( wxCommandEvent& event )
{
    switch( m_PadShape->GetSelection() )
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
        if( m_dummyPad->GetRoundRectRadiusRatio() == 0.0 )
            m_tcCornerSizeRatio->SetValue( "25" );
    }
        break;

    case CHOICE_SHAPE_CHAMFERED_RECT:
        m_shapePropsBook->SetSelection( 3 );

        // A reasonable default is all corners chamferred.
        if( !m_cbTopLeft->GetValue() && !m_cbTopRight->GetValue()
                && !m_cbBottomLeft->GetValue() && !m_cbBottomRight->GetValue() )
        {
            m_cbTopLeft->SetValue( true );
            m_cbTopRight->SetValue( true );
            m_cbBottomLeft->SetValue( true );
            m_cbBottomRight->SetValue( true );
        }
        break;

    case CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT:
    {
        m_shapePropsBook->SetSelection( 4 );

        // Reasonable defaults (corner radius from  IPC-7351C)
        if( m_dummyPad->GetRoundRectRadiusRatio() == 0.0 )
            m_tcCornerSizeRatio->SetValue( "25" );

        if( m_dummyPad->GetChamferRectRatio() == 0.0 )
            m_tcChamferRatio1->ChangeValue( "20" );
    }
        break;

    case CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR:     // PAD_SHAPE_CUSTOM, circular anchor
    case CHOICE_SHAPE_CUSTOM_RECT_ANCHOR:     // PAD_SHAPE_CUSTOM, rect anchor
        m_shapePropsBook->SetSelection( 0 );
        break;
    }

    // Readjust props book size
    wxSize size = m_shapePropsBook->GetSize();
    size.y = m_shapePropsBook->GetPage( m_shapePropsBook->GetSelection() )->GetBestSize().y;
    m_shapePropsBook->SetMaxSize( size );

    m_sizeY.Enable( m_PadShape->GetSelection() != CHOICE_SHAPE_CIRCLE
                    && m_PadShape->GetSelection() != CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR );

    m_offsetShapeOpt->Enable( m_PadShape->GetSelection() != CHOICE_SHAPE_CIRCLE
                              && m_PadShape->GetSelection() != CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR
                              && m_PadShape->GetSelection() != CHOICE_SHAPE_CUSTOM_RECT_ANCHOR );

    if( !m_offsetShapeOpt->IsEnabled() )
        m_offsetShapeOpt->SetValue( false );

    // Show/hide controls depending on m_offsetShapeOpt being enabled
    m_offsetCtrls->Show( m_offsetShapeOpt->GetValue() );
    m_offsetShapeOptLabel->Show( m_offsetShapeOpt->GetValue() );

    bool is_custom = m_PadShape->GetSelection() == CHOICE_SHAPE_CUSTOM_CIRC_ANCHOR
                  || m_PadShape->GetSelection() == CHOICE_SHAPE_CUSTOM_RECT_ANCHOR;

    enablePrimitivePage( is_custom );
    m_staticTextcps->Enable( is_custom );
    m_ZoneCustomPadShape->Enable( is_custom );

    transferDataToPad( m_dummyPad );

    updateRoundRectCornerValues();

    for( size_t i = 0; i < m_notebook->GetPageCount(); ++i )
        m_notebook->GetPage( i )->Layout();

    // Resize the dialog if its height is too small to show all widgets:
    if( m_MainSizer->GetSize().y < m_MainSizer->GetMinSize().y )
        m_MainSizer->SetSizeHints( this );

    redraw();
}


void DIALOG_PAD_PROPERTIES::OnDrillShapeSelected( wxCommandEvent& event )
{
    transferDataToPad( m_dummyPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::PadOrientEvent( wxCommandEvent& event )
{
    transferDataToPad( m_dummyPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::PadTypeSelected( wxCommandEvent& event )
{
    int ii = m_PadType->GetSelection();

    if( (unsigned)ii >= arrayDim( code_type ) ) // catches < 0 also
        ii = 0;

    bool hasHole, hasConnection, hasProperty;

    switch( ii )
    {
    default:
    case 0: /* PTH */      hasHole = true;  hasConnection = true;  hasProperty = true;  break;
    case 1: /* SMD */      hasHole = false; hasConnection = true;  hasProperty = true;  break;
    case 2: /* CONN */     hasHole = false; hasConnection = true;  hasProperty = true;  break;
    case 3: /* NPTH */     hasHole = true;  hasConnection = false; hasProperty = false; break;
    case 4: /* Aperture */ hasHole = false; hasConnection = false; hasProperty = true;  break;
    }

    LSET layer_mask = std_pad_layers[ii];
    setPadLayersList( layer_mask );

    if( !hasHole )
    {
        m_holeX.SetValue( 0 );
        m_holeY.SetValue( 0 );
    }
    else if ( m_holeX.GetValue() == 0 && m_currentPad )
    {
        m_holeX.SetValue( m_currentPad->GetDrillSize().x );
        m_holeY.SetValue( m_currentPad->GetDrillSize().y );
    }

    if( !hasConnection )
    {
        m_PadNumCtrl->SetValue( wxEmptyString );
        m_PadNetSelector->SetSelectedNetcode( 0 );
        m_padToDieOpt->SetValue( false );
    }
    else if( m_PadNumCtrl->GetValue().IsEmpty() && m_currentPad )
    {
        m_PadNumCtrl->SetValue( m_currentPad->GetName() );
        m_PadNetSelector->SetSelectedNetcode( m_currentPad->GetNetCode() );
    }

    if( !hasProperty )
        m_choiceFabProperty->SetSelection( 0 );

    m_choiceFabProperty->Enable( hasProperty );

    transferDataToPad( m_dummyPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    int ii = m_PadType->GetSelection();

    if( (unsigned)ii >= arrayDim( code_type ) ) // catches < 0 also
        ii = 0;

    bool hasHole, hasConnection;

    switch( ii )
    {
    default:
    case 0: /* PTH */      hasHole = true;  hasConnection = true;  break;
    case 1: /* SMD */      hasHole = false; hasConnection = true;  break;
    case 2: /* CONN */     hasHole = false; hasConnection = true;  break;
    case 3: /* NPTH */     hasHole = true;  hasConnection = false; break;
    case 4: /* Aperture */ hasHole = false; hasConnection = false; break;
    }

    // Enable/disable hole controls
    m_holeShapeLabel->Enable( hasHole );
    m_holeShapeCtrl->Enable( hasHole );
    m_holeX.Enable( hasHole );
    m_holeY.Enable( hasHole && m_holeShapeCtrl->GetSelection() == 1 );

    // Enable/disable Pad number, net and pad length-to-die
    m_PadNumText->Enable( hasConnection );
    m_PadNumCtrl->Enable( hasConnection );
    m_PadNameText->Enable( hasConnection );
    m_PadNetSelector->Enable( hasConnection && m_canEditNetName && m_currentPad );
    m_padToDieOpt->Enable( hasConnection );

    if( !m_padToDieOpt->IsEnabled() )
        m_padToDieOpt->SetValue( false );

    // We can show/hide this here because it doesn't require the layout to be refreshed.
    // All the others have to be done in their event handlers because doing a layout here
    // causes infinite looping on MSW.
    m_padToDie.Show( m_padToDieOpt->GetValue() );

    // Enable/disable Copper Layers control
    m_rbCopperLayersSel->Enable( ii != 4 );
}


void DIALOG_PAD_PROPERTIES::setPadLayersList( LSET layer_mask )
{
    LSET cu_set = layer_mask & LSET::AllCuMask();

    if( cu_set == LSET( F_Cu ) )
        m_rbCopperLayersSel->SetSelection( 0 );
    else if( cu_set == LSET( B_Cu ) )
        m_rbCopperLayersSel->SetSelection( 1 );
    else if( cu_set.any() )
        m_rbCopperLayersSel->SetSelection( 2 );
    else
        m_rbCopperLayersSel->SetSelection( 3 );

    m_PadLayerAdhCmp->SetValue( layer_mask[F_Adhes] );
    m_PadLayerAdhCu->SetValue( layer_mask[B_Adhes] );

    m_PadLayerPateCmp->SetValue( layer_mask[F_Paste] );
    m_PadLayerPateCu->SetValue( layer_mask[B_Paste] );

    m_PadLayerSilkCmp->SetValue( layer_mask[F_SilkS] );
    m_PadLayerSilkCu->SetValue( layer_mask[B_SilkS] );

    m_PadLayerMaskCmp->SetValue( layer_mask[F_Mask] );
    m_PadLayerMaskCu->SetValue( layer_mask[B_Mask] );

    m_PadLayerECO1->SetValue( layer_mask[Eco1_User] );
    m_PadLayerECO2->SetValue( layer_mask[Eco2_User] );

    m_PadLayerDraft->SetValue( layer_mask[Dwgs_User] );
}


// Called when select/deselect a layer.
void DIALOG_PAD_PROPERTIES::OnSetLayers( wxCommandEvent& event )
{
    transferDataToPad( m_dummyPad );
    redraw();
}


// test if all values are acceptable for the pad
bool DIALOG_PAD_PROPERTIES::padValuesOK()
{
    bool error = transferDataToPad( m_dummyPad );
    bool skip_tstoffset = false;    // the offset prm is not always tested

    wxArrayString error_msgs;
    wxString msg;

    // Test for incorrect values
    if( (m_dummyPad->GetSize().x <= 0) ||
       ((m_dummyPad->GetSize().y <= 0) && (m_dummyPad->GetShape() != PAD_SHAPE_CIRCLE)) )
    {
        error_msgs.Add( _( "Pad size must be greater than zero" ) );
    }

    if( (m_dummyPad->GetSize().x < m_dummyPad->GetDrillSize().x) ||
        (m_dummyPad->GetSize().y < m_dummyPad->GetDrillSize().y) )
    {
        error_msgs.Add(  _( "Incorrect value for pad drill: pad drill bigger than pad size" ) );
        skip_tstoffset = true;  // offset prm will be not tested because if the drill value
                                // is incorrect the offset prm is always seen as incorrect, even if it is 0
    }

    if( m_dummyPad->GetLocalClearance() < 0 )
    {
        error_msgs.Add( _( "Pad local clearance must be zero or greater than zero" ) );
    }

    // Some pads need a negative solder mask clearance (mainly for BGA with small pads)
    // However the negative solder mask clearance must not create negative mask size
    // Therefore test for minimal acceptable negative value
    // Hovewer, a negative value can give strange result with custom shapes, so it is not
    // allowed for custom pad shape
    if( m_dummyPad->GetLocalSolderMaskMargin() < 0 )
    {
        if( m_dummyPad->GetShape() == PAD_SHAPE_CUSTOM )
            error_msgs.Add( _( "Pad local solder mask clearance must be zero or greater than zero" ) );
        else
        {
            int min_smClearance = -std::min( m_dummyPad->GetSize().x, m_dummyPad->GetSize().y )/2;

            if( m_dummyPad->GetLocalSolderMaskMargin() <= min_smClearance )
            {
                error_msgs.Add( wxString::Format(
                                _( "Pad local solder mask clearance must be greater than %s" ),
                                StringFromValue( GetUserUnits(), min_smClearance, true, true ) ) );
            }
        }
    }

    // Some pads need a positive solder paste clearance (mainly for BGA with small pads)
    // Hovewer, a positive value can create issues if the resulting shape is too big.
    // (like a solder paste creating a solder paste area on a neighbour pad or on the solder mask)
    // So we could ask for user to confirm the choice
    // Currently there are no test

    LSET padlayers_mask = m_dummyPad->GetLayerSet();

    if( padlayers_mask == 0 )
        error_msgs.Add( _( "Error: pad has no layer" ) );

    if( !padlayers_mask[F_Cu] && !padlayers_mask[B_Cu] )
    {
        if( m_dummyPad->GetDrillSize().x || m_dummyPad->GetDrillSize().y )
        {
            // Note: he message is shown in an HTML window
            msg = _( "Error: the pad is not on a copper layer and has a hole" );

            if( m_dummyPad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
            {
                msg += wxT( "<br><br><i>" );
                msg += _( "For NPTH pad, set pad size value to pad drill value,"
                          " if you do not want this pad plotted in gerber files" );
            }

            error_msgs.Add( msg );
        }
    }

    if( !skip_tstoffset )
    {
        wxPoint max_size;
        max_size.x = std::abs( m_dummyPad->GetOffset().x );
        max_size.y = std::abs( m_dummyPad->GetOffset().y );
        max_size.x += m_dummyPad->GetDrillSize().x / 2;
        max_size.y += m_dummyPad->GetDrillSize().y / 2;

        if( ( m_dummyPad->GetSize().x / 2 < max_size.x ) ||
            ( m_dummyPad->GetSize().y / 2 < max_size.y ) )
        {
            error_msgs.Add( _( "Incorrect value for pad offset" ) );
        }
    }

    if( error )
        error_msgs.Add(  _( "Too large value for pad delta size" ) );

    switch( m_dummyPad->GetAttribute() )
    {
    case PAD_ATTRIB_HOLE_NOT_PLATED:   // Not plated, but through hole, a hole is expected
    case PAD_ATTRIB_STANDARD :         // Pad through hole, a hole is also expected
        if( m_dummyPad->GetDrillSize().x <= 0 ||
            ( m_dummyPad->GetDrillSize().y <= 0 && m_dummyPad->GetDrillShape() == PAD_DRILL_SHAPE_OBLONG ) )
            error_msgs.Add( _( "Error: Through hole pad: drill diameter set to 0" ) );
        break;

    case PAD_ATTRIB_CONN:      // Connector pads are smd pads, just they do not have solder paste.
        if( padlayers_mask[B_Paste] || padlayers_mask[F_Paste] )
            error_msgs.Add( _( "Error: Connector pads are not on the solder paste layer\n"
                               "Use SMD pads instead" ) );
        KI_FALLTHROUGH;

    case PAD_ATTRIB_SMD:       // SMD and Connector pads (One external copper layer only)
        {
        LSET innerlayers_mask = padlayers_mask & LSET::InternalCuMask();

        if( ( padlayers_mask[F_Cu] && padlayers_mask[B_Cu] ) ||
            innerlayers_mask.count() != 0 )
            error_msgs.Add( _( "Error: only one external copper layer allowed for SMD or Connector pads" ) );
        }
        break;
    }

    if( m_dummyPad->GetProperty() != PAD_PROP_NONE &&
        m_dummyPad->GetAttribute() == PAD_ATTRIB_HOLE_NOT_PLATED )
        error_msgs.Add(  _( "Property cannot be set for NPTH" ) );

    if( m_dummyPad->GetProperty() == PAD_PROP_CASTELLATED &&
        m_dummyPad->GetAttribute() != PAD_ATTRIB_STANDARD )
        error_msgs.Add(  _( "Castellated property can be set only for PTH" ) );

    if( m_dummyPad->GetProperty() == PAD_PROP_BGA &&
        m_dummyPad->GetAttribute() != PAD_ATTRIB_SMD )
        error_msgs.Add(  _( "BGA property can be set only for SMD pads" ) );

    if( m_dummyPad->GetShape() == PAD_SHAPE_ROUNDRECT ||
        m_dummyPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
    {
        wxASSERT( m_tcCornerSizeRatio->GetValue() == m_tcCornerSizeRatio1->GetValue() );
        wxString value = m_tcCornerSizeRatio->GetValue();
        double rrRadiusRatioPercent;

        if( !value.ToDouble( &rrRadiusRatioPercent ) )
            error_msgs.Add( _( "Incorrect corner size value" ) );
        else
        {
            if( rrRadiusRatioPercent < 0.0 )
                error_msgs.Add( _( "Incorrect (negative) corner size value" ) );
            else if( rrRadiusRatioPercent > 50.0 )
                error_msgs.Add( _( "Corner size value must be smaller than 50%" ) );
        }
    }

    if( m_dummyPad->GetShape() == PAD_SHAPE_CUSTOM )
    {
        SHAPE_POLY_SET mergedPolygon;
        m_dummyPad->MergePrimitivesAsPolygon( &mergedPolygon );

        if( mergedPolygon.OutlineCount() > 1 )
            error_msgs.Add( _( "Incorrect pad shape: the shape must be equivalent to only one polygon" ) );
    }


    if( error_msgs.GetCount() )
    {
        HTML_MESSAGE_BOX dlg( this, _("Pad setup errors list" ) );
        dlg.ListSet( error_msgs );
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

    view->Update( m_dummyPad );

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
        PAD_CS_PRIMITIVE& primitive = m_primitives[select];

        DRAWSEGMENT* dummySegment = new DRAWSEGMENT;
        dummySegment->SetLayer( SELECTED_ITEMS_LAYER );
        primitive.ExportTo( dummySegment );
        dummySegment->Rotate( wxPoint( 0, 0), m_dummyPad->GetOrientation() );
        dummySegment->Move( m_dummyPad->GetPosition() );

        // Update selected primitive (highlight selected)
        switch( primitive.m_Shape )
        {
        case S_SEGMENT:
        case S_ARC:
        case S_CURVE:
            break;

        case S_CIRCLE:          //  ring or circle
            if( primitive.m_Thickness == 0 )    // filled circle
            {   // the filled circle option does not exist in a DRAWSEGMENT
                // but it is easy to create it with a circle having the
                // right radius and outline width
                wxPoint end = dummySegment->GetCenter();
                end.x += primitive.m_Radius / 2;
                dummySegment->SetEnd( end );
                dummySegment->SetWidth( primitive.m_Radius );
            }
            break;

        case S_POLYGON:
            break;

        default:
            delete dummySegment;
            dummySegment = nullptr;
            break;
        }

        if( dummySegment )
        {
            view->Add( dummySegment );
            m_highlight.push_back( dummySegment );
        }

        select = m_listCtrlPrimitives->GetNextSelected( select );
    }

    BOX2I bbox = m_dummyPad->ViewBBox();

    if( bbox.GetSize().x > 0 && bbox.GetSize().y > 0 )
    {
        // The origin always goes in the middle of the canvas; we want offsetting the pad
        // shape to move the pad, not the hole
        bbox.Move( -m_dummyPad->GetPosition() );
        int maxXExtent = std::max( abs( bbox.GetLeft() ), abs( bbox.GetRight() ) );
        int maxYExtent = std::max( abs( bbox.GetTop() ), abs( bbox.GetBottom() ) );

        // Don't blow up the GAL on too-large numbers
        if( maxXExtent > INT_MAX / 4 )
            maxXExtent = INT_MAX / 4;

        if( maxYExtent > INT_MAX / 4 )
            maxYExtent = INT_MAX / 4;

        BOX2D viewBox( m_dummyPad->GetPosition(), {0, 0} );
        BOX2D canvasBox( m_dummyPad->GetPosition(), {0, 0} );
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

    int  isign = m_isFlipped ? -1 : 1;

    transferDataToPad( m_padMaster );
    // m_padMaster is a pattern: ensure there is no net for this pad:
    m_padMaster->SetNetCode( NETINFO_LIST::UNCONNECTED );

    if( !m_currentPad )   // Set current Pad parameters
        return true;

    commit.Modify( m_currentPad );

    // redraw the area where the pad was, without pad (delete pad on screen)
    m_currentPad->SetFlags( DO_NOT_DRAW );
    m_parent->GetCanvas()->Refresh();
    m_currentPad->ClearFlags( DO_NOT_DRAW );

    // Update values
    m_currentPad->SetShape( m_padMaster->GetShape() );
    m_currentPad->SetAttribute( m_padMaster->GetAttribute() );
    m_currentPad->SetPosition( m_padMaster->GetPosition() );

    wxSize  size;
    MODULE* footprint = m_currentPad->GetParent();

    if( footprint )
    {
        footprint->SetLastEditTime();

        // compute the pos 0 value, i.e. pad position for footprint with orientation = 0
        // i.e. relative to footprint origin (footprint position)
        wxPoint pt = m_currentPad->GetPosition() - footprint->GetPosition();
        RotatePoint( &pt, -footprint->GetOrientation() );
        m_currentPad->SetPos0( pt );
        m_currentPad->SetOrientation( m_padMaster->GetOrientation() * isign
                                        + footprint->GetOrientation() );
    }

    m_currentPad->SetSize( m_padMaster->GetSize() );

    size = m_padMaster->GetDelta();
    size.y *= isign;
    m_currentPad->SetDelta( size );

    m_currentPad->SetDrillSize( m_padMaster->GetDrillSize() );
    m_currentPad->SetDrillShape( m_padMaster->GetDrillShape() );

    wxPoint offset = m_padMaster->GetOffset();
    offset.y *= isign;
    m_currentPad->SetOffset( offset );

    m_currentPad->SetPadToDieLength( m_padMaster->GetPadToDieLength() );

    if( m_padMaster->GetShape() != PAD_SHAPE_CUSTOM )
        m_padMaster->DeletePrimitivesList();


    m_currentPad->SetAnchorPadShape( m_padMaster->GetAnchorPadShape() );
    m_currentPad->SetPrimitives( m_padMaster->GetPrimitives() );

    if( m_isFlipped )
    {
        m_currentPad->SetLayerSet( FlipLayerMask( m_currentPad->GetLayerSet() ) );
        m_currentPad->FlipPrimitives();
    }

    m_currentPad->SetLayerSet( m_padMaster->GetLayerSet() );

    if( m_isFlipped )
    {
        m_currentPad->SetLayerSet( FlipLayerMask( m_currentPad->GetLayerSet() ) );
    }

    m_currentPad->SetName( m_padMaster->GetName() );

    int padNetcode = NETINFO_LIST::UNCONNECTED;

    // For PAD_ATTRIB_HOLE_NOT_PLATED, ensure there is no net name selected
    if( m_padMaster->GetAttribute() != PAD_ATTRIB_HOLE_NOT_PLATED  )
        padNetcode = m_PadNetSelector->GetSelectedNetcode();

    m_currentPad->SetNetCode( padNetcode );
    m_currentPad->SetLocalClearance( m_padMaster->GetLocalClearance() );
    m_currentPad->SetLocalSolderMaskMargin( m_padMaster->GetLocalSolderMaskMargin() );
    m_currentPad->SetLocalSolderPasteMargin( m_padMaster->GetLocalSolderPasteMargin() );
    m_currentPad->SetLocalSolderPasteMarginRatio( m_padMaster->GetLocalSolderPasteMarginRatio() );
    m_currentPad->SetThermalWidth( m_padMaster->GetThermalWidth() );
    m_currentPad->SetThermalGap( m_padMaster->GetThermalGap() );
    m_currentPad->SetRoundRectRadiusRatio( m_padMaster->GetRoundRectRadiusRatio() );
    m_currentPad->SetChamferRectRatio( m_padMaster->GetChamferRectRatio() );
    m_currentPad->SetChamferPositions( m_padMaster->GetChamferPositions() );
    m_currentPad->SetZoneConnection( m_padMaster->GetEffectiveZoneConnection() );

    // rounded rect pads with radius ratio = 0 are in fact rect pads.
    // So set the right shape (and perhaps issues with a radius = 0)
    if( m_currentPad->GetShape() == PAD_SHAPE_ROUNDRECT &&
        m_currentPad->GetRoundRectRadiusRatio() == 0.0 )
    {
        m_currentPad->SetShape( PAD_SHAPE_RECT );
    }

    // Set the fabrication property:
    m_currentPad->SetProperty( getSelectedProperty() );

    // define the way the clearance area is defined in zones
    m_currentPad->SetCustomShapeInZoneOpt( m_padMaster->GetCustomShapeInZoneOpt() );

    if( footprint )
        footprint->CalculateBoundingBox();

    m_parent->SetMsgPanel( m_currentPad );

    // redraw the area where the pad was
    m_parent->GetCanvas()->Refresh();

    commit.Push( _( "Modify pad" ) );

    return true;
}


PAD_PROP_T DIALOG_PAD_PROPERTIES::getSelectedProperty()
{
    PAD_PROP_T prop = PAD_PROP_NONE;

    switch( m_choiceFabProperty->GetSelection() )
    {
    case 0:     prop = PAD_PROP_NONE; break;
    case 1:     prop = PAD_PROP_BGA; break;
    case 2:     prop = PAD_PROP_FIDUCIAL_LOCAL; break;
    case 3:     prop = PAD_PROP_FIDUCIAL_GLBL; break;
    case 4:     prop = PAD_PROP_TESTPOINT; break;
    case 5:     prop = PAD_PROP_HEATSINK; break;
    case 6:     prop = PAD_PROP_CASTELLATED; break;
    }

    return prop;
}


bool DIALOG_PAD_PROPERTIES::transferDataToPad( D_PAD* aPad )
{
    wxString    msg;

    if( !Validate() )
        return true;
    if( !m_panelGeneral->Validate() )
        return true;
    if( !m_localSettingsPanel->Validate() )
        return true;
    if( !m_spokeWidth.Validate( 0, INT_MAX ) )
        return false;

    m_OrientValidator.TransferFromWindow();

    aPad->SetAttribute( code_type[m_PadType->GetSelection()] );
    aPad->SetShape( code_shape[m_PadShape->GetSelection()] );
    aPad->SetAnchorPadShape( m_PadShape->GetSelection() == CHOICE_SHAPE_CUSTOM_RECT_ANCHOR ?
                                PAD_SHAPE_RECT : PAD_SHAPE_CIRCLE );

    if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
        aPad->SetPrimitives( m_primitives );

    // Read pad clearances values:
    aPad->SetLocalClearance( m_clearance.GetValue() );
    aPad->SetLocalSolderMaskMargin( m_maskClearance.GetValue() );
    aPad->SetLocalSolderPasteMargin( m_pasteClearance.GetValue() );
    aPad->SetThermalWidth( m_spokeWidth.GetValue() );
    aPad->SetThermalGap( m_thermalGap.GetValue() );
    double dtmp = 0.0;
    msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A -50% margin ratio means no paste on a pad, the ratio must be >= -50%
    if( dtmp < -50.0 )
        dtmp = -50.0;
    // A margin ratio is always <= 0
    // 0 means use full pad copper area
    if( dtmp > 0.0 )
        dtmp = 0.0;

    aPad->SetLocalSolderPasteMarginRatio( dtmp / 100 );

    switch( m_ZoneConnectionChoice->GetSelection() )
    {
    default:
    case 0: aPad->SetZoneConnection( ZONE_CONNECTION::INHERITED ); break;
    case 1: aPad->SetZoneConnection( ZONE_CONNECTION::FULL );      break;
    case 2: aPad->SetZoneConnection( ZONE_CONNECTION::THERMAL );   break;
    case 3: aPad->SetZoneConnection( ZONE_CONNECTION::NONE );      break;
    }

    aPad->SetPosition( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );
    aPad->SetPos0( wxPoint( m_posX.GetValue(), m_posY.GetValue() ) );

    if( m_holeShapeCtrl->GetSelection() == 0 )
    {
        aPad->SetDrillShape( PAD_DRILL_SHAPE_CIRCLE );
        aPad->SetDrillSize( wxSize( m_holeX.GetValue(), m_holeX.GetValue() ) );
    }
    else
    {
        aPad->SetDrillShape( PAD_DRILL_SHAPE_OBLONG );
        aPad->SetDrillSize( wxSize( m_holeX.GetValue(), m_holeY.GetValue() ) );
    }

    if( aPad->GetShape() == PAD_SHAPE_CIRCLE )
        aPad->SetSize( wxSize( m_sizeX.GetValue(), m_sizeX.GetValue() ) );
    else
        aPad->SetSize( wxSize( m_sizeX.GetValue(), m_sizeY.GetValue() ) );

    // For a trapezoid, test delta value (be sure delta is not too large for pad size)
    // remember DeltaSize.x is the Y size variation
    bool   error    = false;
    wxSize delta( 0, 0 );

    if( aPad->GetShape() == PAD_SHAPE_TRAPEZOID )
    {
        // For a trapezoid, only one of delta.x or delta.y is not 0, depending on axis.
        if( m_trapAxisCtrl->GetSelection() == 0 )
            delta.x = m_trapDelta.GetValue();
        else
            delta.y = m_trapDelta.GetValue();

        if( delta.x < 0 && delta.x <= -aPad->GetSize().y )
        {
            delta.x = -aPad->GetSize().y + 2;
            error = true;
        }

        if( delta.x > 0 && delta.x >= aPad->GetSize().y )
        {
            delta.x = aPad->GetSize().y - 2;
            error = true;
        }

        if( delta.y < 0 && delta.y <= -aPad->GetSize().x )
        {
            delta.y = -aPad->GetSize().x + 2;
            error = true;
        }

        if( delta.y > 0 && delta.y >= aPad->GetSize().x )
        {
            delta.y = aPad->GetSize().x - 2;
            error = true;
        }
    }

    aPad->SetDelta( delta );

    if( m_offsetShapeOpt->GetValue() )
        aPad->SetOffset( wxPoint( m_offsetX.GetValue(), m_offsetY.GetValue() ) );
    else
        aPad->SetOffset( wxPoint() );

    // Read pad length die
    if( m_padToDieOpt->GetValue() )
        aPad->SetPadToDieLength( m_padToDie.GetValue() );
    else
        aPad->SetPadToDieLength( 0 );

    aPad->SetOrientation( m_OrientValue * 10.0 );
    aPad->SetName( m_PadNumCtrl->GetValue() );
    aPad->SetNetCode( m_PadNetSelector->GetSelectedNetcode() );

    int chamfers = 0;

    if( m_PadShape->GetSelection() == CHOICE_SHAPE_CHAMFERED_RECT )
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
    else if( m_PadShape->GetSelection() == CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT )
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

    if( aPad->GetShape() == PAD_SHAPE_CUSTOM )
    {
        // The pad custom has a "anchor pad" (a basic shape: round or rect pad)
        // that is the minimal area of this pad, and is usefull to ensure a hole
        // diameter is acceptable, and is used in Gerber files as flashed area
        // reference
        if( aPad->GetAnchorPadShape() == PAD_SHAPE_CIRCLE )
            aPad->SetSize( wxSize( m_sizeX.GetValue(), m_sizeX.GetValue() ) );

        // define the way the clearance area is defined in zones
        aPad->SetCustomShapeInZoneOpt( m_ZoneCustomPadShape->GetSelection() == 0 ?
                                       CUST_PAD_SHAPE_IN_ZONE_OUTLINE :
                                       CUST_PAD_SHAPE_IN_ZONE_CONVEXHULL );
    }

    switch( aPad->GetAttribute() )
    {
    case PAD_ATTRIB_STANDARD:
        break;

    case PAD_ATTRIB_CONN:
    case PAD_ATTRIB_SMD:
        // SMD and PAD_ATTRIB_CONN has no hole.
        // basically, SMD and PAD_ATTRIB_CONN are same type of pads
        // PAD_ATTRIB_CONN has just a default non technical layers that differs from SMD
        // and are intended to be used in virtual edge board connectors
        // However we can accept a non null offset,
        // mainly to allow complex pads build from a set of basic pad shapes
        aPad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case PAD_ATTRIB_HOLE_NOT_PLATED:
        // Mechanical purpose only:
        // no net name, no pad name allowed
        aPad->SetName( wxEmptyString );
        aPad->SetNetCode( NETINFO_LIST::UNCONNECTED );
        break;

    default:
        DisplayError( NULL, wxT( "Error: unknown pad type" ) );
        break;
    }

    if( aPad->GetShape() == PAD_SHAPE_ROUNDRECT )
    {
        double ratioPercent;

        m_tcCornerSizeRatio->GetValue().ToDouble( &ratioPercent );
        aPad->SetRoundRectRadiusRatio( ratioPercent / 100.0 );
    }
    else if( aPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT
            && m_PadShape->GetSelection() == CHOICE_SHAPE_CHAMFERED_ROUNDED_RECT )
    {
        double ratioPercent;

        m_tcCornerSizeRatio1->GetValue().ToDouble( &ratioPercent );
        aPad->SetRoundRectRadiusRatio( ratioPercent / 100.0 );
    }

    if( aPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT )
    {
        double ratioPercent;

        m_tcChamferRatio->GetValue().ToDouble( &ratioPercent );
        aPad->SetChamferRectRatio( ratioPercent / 100.0 );
    }

    aPad->SetProperty( getSelectedProperty() );

    LSET padLayerMask;

    switch( m_rbCopperLayersSel->GetSelection() )
    {
    case 0: padLayerMask.set( F_Cu );          break;
    case 1: padLayerMask.set( B_Cu );          break;
    case 2: padLayerMask |= LSET::AllCuMask(); break;
    case 3:                                    break;     // No copper layers
    }

    if( m_PadLayerAdhCmp->GetValue() )
        padLayerMask.set( F_Adhes );

    if( m_PadLayerAdhCu->GetValue() )
        padLayerMask.set( B_Adhes );

    if( m_PadLayerPateCmp->GetValue() )
        padLayerMask.set( F_Paste );

    if( m_PadLayerPateCu->GetValue() )
        padLayerMask.set( B_Paste );

    if( m_PadLayerSilkCmp->GetValue() )
        padLayerMask.set( F_SilkS );

    if( m_PadLayerSilkCu->GetValue() )
        padLayerMask.set( B_SilkS );

    if( m_PadLayerMaskCmp->GetValue() )
        padLayerMask.set( F_Mask );

    if( m_PadLayerMaskCu->GetValue() )
        padLayerMask.set( B_Mask );

    if( m_PadLayerECO1->GetValue() )
        padLayerMask.set( Eco1_User );

    if( m_PadLayerECO2->GetValue() )
        padLayerMask.set( Eco2_User );

    if( m_PadLayerDraft->GetValue() )
        padLayerMask.set( Dwgs_User );

    aPad->SetLayerSet( padLayerMask );

    return error;
}


void DIALOG_PAD_PROPERTIES::OnOffsetCheckbox( wxCommandEvent& event )
{
    if( m_offsetShapeOpt->GetValue() && m_currentPad )
    {
        m_offsetX.SetValue( m_currentPad->GetOffset().x );
        m_offsetY.SetValue( m_currentPad->GetOffset().y );
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
        transferDataToPad( m_dummyPad );
        // If the pad size has changed, update the displayed values
        // for rounded rect pads
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

    PAD_CS_PRIMITIVE& shape = m_primitives[select];

    if( shape.m_Shape == S_POLYGON )
    {
        DIALOG_PAD_PRIMITIVE_POLY_PROPS dlg( this, m_parent, &shape );

        if( dlg.ShowModal() != wxID_OK )
            return;

        dlg.TransferDataFromWindow();
    }

    else
    {
        DIALOG_PAD_PRIMITIVES_PROPERTIES dlg( this, m_parent, &shape );

        if( dlg.ShowModal() != wxID_OK )
            return;

        dlg.TransferDataFromWindow();
    }

    displayPrimitivesList();

    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        redraw();
    }
}


void DIALOG_PAD_PROPERTIES::OnPrimitiveSelection( wxListEvent& event )
{
    // Called on a double click on the basic shapes list
    // To Do: highligth the primitive(s) currently selected.
    redraw();
}


/// Called on a double click on the basic shapes list
void DIALOG_PAD_PROPERTIES::onPrimitiveDClick( wxMouseEvent& event )
{
    editPrimitive();
}


// Called on a click on basic shapes list panel button
void DIALOG_PAD_PROPERTIES::onEditPrimitive( wxCommandEvent& event )
{
    editPrimitive();
}

// Called on a click on basic shapes list panel button
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

    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        redraw();
    }
}


void DIALOG_PAD_PROPERTIES::onAddPrimitive( wxCommandEvent& event )
{
    // Ask user for shape type
    wxString shapelist[] = { _( "Segment" ), _( "Arc" ), _( "Bezier" ),
                             _( "Ring/Circle" ), _( "Polygon" ) };

    int type = wxGetSingleChoiceIndex( _( "Shape type:" ), _( "Add Primitive" ),
                                       arrayDim( shapelist ), shapelist, 0, this );

    // User pressed cancel
    if( type == -1 )
        return;

    STROKE_T listtype[] = { S_SEGMENT, S_ARC, S_CURVE, S_CIRCLE, S_POLYGON };

    PAD_CS_PRIMITIVE primitive( listtype[type] );
    primitive.m_Thickness = m_board->GetDesignSettings().GetLineThickness( F_Cu );

    if( listtype[type] == S_POLYGON )
    {
        DIALOG_PAD_PRIMITIVE_POLY_PROPS dlg( this, m_parent, &primitive );

        if( dlg.ShowModal() != wxID_OK )
            return;
    }
    else
    {
        DIALOG_PAD_PRIMITIVES_PROPERTIES dlg( this, m_parent, &primitive );

        if( dlg.ShowModal() != wxID_OK )
            return;
    }

    m_primitives.push_back( primitive );

    displayPrimitivesList();

    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        redraw();
    }
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
    std::vector<PAD_CS_PRIMITIVE*> shapeList;
    shapeList.push_back( &m_primitives[select] );

    while( ( select = m_listCtrlPrimitives->GetNextSelected( select ) ) >= 0 )
        shapeList.push_back( &m_primitives[select] );

    DIALOG_PAD_PRIMITIVES_TRANSFORM dlg( this, m_parent, shapeList, false );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Transfert new settings:
    dlg.Transform();

    displayPrimitivesList();

    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        redraw();
    }
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
    std::vector<PAD_CS_PRIMITIVE*> shapeList;
    shapeList.push_back( &m_primitives[select] );

    while( ( select = m_listCtrlPrimitives->GetNextSelected( select ) ) >= 0 )
        shapeList.push_back( &m_primitives[select] );

    DIALOG_PAD_PRIMITIVES_TRANSFORM dlg( this, m_parent, shapeList, true );

    if( dlg.ShowModal() != wxID_OK )
        return;

    // Transfer new settings
    // save duplicates to a separate vector to avoid m_primitives reallocation,
    // as shapeList contains pointers to its elements
    std::vector<PAD_CS_PRIMITIVE> duplicates;
    dlg.Transform( &duplicates, dlg.GetDuplicateCount() );
    std::move( duplicates.begin(), duplicates.end(), std::back_inserter( m_primitives ) );

    displayPrimitivesList();

    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        redraw();
    }
}
