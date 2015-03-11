/**
 * @file dialog_pad_properties.cpp
 * @brief Pad editing functions and dialog pad editor.
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <common.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <pcbnew.h>
#include <trigo.h>
#include <macros.h>
#include <wxBasePcbFrame.h>
#include <pcbcommon.h>
#include <base_units.h>

#include <wx/dcbuffer.h>

#include <class_board.h>
#include <class_module.h>

#include <dialog_pad_properties_base.h>
#include <html_messagebox.h>


// list of pad shapes.
static PAD_SHAPE_T code_shape[] = {
    PAD_CIRCLE,
    PAD_OVAL,
    PAD_RECT,
    PAD_TRAPEZOID
};


static PAD_ATTR_T code_type[] = {
    PAD_STANDARD,
    PAD_SMD,
    PAD_CONN,
    PAD_HOLE_NOT_PLATED
};


// Default mask layers setup for pads according to the pad type
static const LSET std_pad_layers[] = {
    // PAD_STANDARD:
    D_PAD::StandardMask(),

    // PAD_SMD:
    D_PAD::SMDMask(),

    // PAD_CONN:
    D_PAD::ConnSMDMask(),

    // PAD_HOLE_NOT_PLATED:
    D_PAD::UnplatedHoleMask()
};


/**
 * class DIALOG_PAD_PROPERTIES, derived from DIALOG_PAD_PROPERTIES_BASE,
 * created by wxFormBuilder
 */
class DIALOG_PAD_PROPERTIES : public DIALOG_PAD_PROPERTIES_BASE
{
public:
    DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, D_PAD* aPad );
    ~DIALOG_PAD_PROPERTIES()
    {
        delete m_dummyPad;
    }

private:
    PCB_BASE_FRAME* m_parent;
    D_PAD*  m_currentPad;           // pad currently being edited
    D_PAD*  m_dummyPad;             // a working copy used to show changes
    D_PAD*  m_padMaster;            // The pad used to create new pads in board or
                                    // footprint editor
    BOARD*  m_board;                // the main board: this is the board handled by
                                    // the PCB editor, if running or the dummy
                                    // board used by the footprint editor
                                    // (could happen when the Footprint editor will be run
                                    // alone, outside the board editor
    bool    m_isFlipped;            // true if the parent footprint (therefore pads) is flipped (mirrored)
                                    // in this case, some Y coordinates values must be negated
    bool    m_canUpdate;
    bool    m_canEditNetName;       // true only if the called is the board editor

private:
    void initValues();

    bool padValuesOK();       ///< test if all values are acceptable for the pad

    void redraw();

    /**
     * Function setPadLayersList
     * updates the CheckBox states in pad layers list,
     * @param layer_mask = pad layer mask (ORed layers bit mask)
     */
    void setPadLayersList( LSET layer_mask );

    /// Copy values from dialog field to aPad's members
    bool transferDataToPad( D_PAD* aPad );

    // event handlers:
    void OnResize( wxSizeEvent& event );

    void OnPadShapeSelection( wxCommandEvent& event );
    void OnDrillShapeSelected( wxCommandEvent& event );

    void PadOrientEvent( wxCommandEvent& event );
    void PadTypeSelected( wxCommandEvent& event );

    void OnSetLayers( wxCommandEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnPaintShowPanel( wxPaintEvent& event );

    /// Called when a dimension has changed.
    /// Update the graphical pad shown in the panel.
    void OnValuesChanged( wxCommandEvent& event );

    /// Updates the different parameters for the component being edited.
    /// Fired from the OK button click.
    void PadPropertiesAccept( wxCommandEvent& event );
};


void PCB_BASE_FRAME::InstallPadOptionsFrame( D_PAD* aPad )
{
    DIALOG_PAD_PROPERTIES dlg( this, aPad );

    dlg.ShowModal();
}


DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, D_PAD* aPad ) :
    DIALOG_PAD_PROPERTIES_BASE( aParent )
{
    m_canUpdate  = false;
    m_parent     = aParent;
    m_currentPad = aPad;        // aPad can be NULL, if the dialog is called
                                // from the module editor to set default pad characteristics

    m_board      = m_parent->GetBoard();

    m_padMaster  = &m_parent->GetDesignSettings().m_Pad_Master;
    m_dummyPad   = new D_PAD( (MODULE*) NULL );

    if( aPad )
        m_dummyPad->Copy( aPad );
    else    // We are editing a "master" pad, i.e. a pad used to create new pads
        m_dummyPad->Copy( m_padMaster );

    if( m_parent->IsGalCanvasActive() )
    {
        m_panelShowPadGal->UseColorScheme( m_board->GetColorsSettings() );
        m_panelShowPadGal->SwitchBackend( m_parent->GetGalCanvas()->GetBackend() );
#if !wxCHECK_VERSION( 3, 0, 0 )
        m_panelShowPadGal->SetSize( m_panelShowPad->GetSize() );
#endif
        m_panelShowPadGal->Show();
        m_panelShowPad->Hide();
        m_panelShowPadGal->GetView()->Add( m_dummyPad );
        m_panelShowPadGal->StartDrawing();

        Connect( wxEVT_SIZE, wxSizeEventHandler( DIALOG_PAD_PROPERTIES::OnResize ) );
    }
    else
    {
        m_panelShowPad->Show();
        m_panelShowPadGal->Hide();
    }

    initValues();

    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );

    m_PadNumCtrl->SetFocus();
    m_canUpdate = true;
}


void DIALOG_PAD_PROPERTIES::OnPaintShowPanel( wxPaintEvent& event )
{
    wxPaintDC    dc( m_panelShowPad );
    PAD_DRAWINFO drawInfo;

    EDA_COLOR_T color = BLACK;

    if( m_dummyPad->GetLayerSet()[F_Cu] )
    {
        color = m_board->GetVisibleElementColor( PAD_FR_VISIBLE );
    }

    if( m_dummyPad->GetLayerSet()[B_Cu] )
    {
        color = ColorMix( color, m_board->GetVisibleElementColor( PAD_BK_VISIBLE ) );
    }

    // What could happen: the pad color is *actually* black, or no
    // copper was selected
    if( color == BLACK )
        color = LIGHTGRAY;

    drawInfo.m_Color     = color;
    drawInfo.m_HoleColor = DARKGRAY;
    drawInfo.m_Offset    = m_dummyPad->GetPosition();
    drawInfo.m_Display_padnum  = true;
    drawInfo.m_Display_netname = true;

    if( m_dummyPad->GetAttribute() == PAD_HOLE_NOT_PLATED )
        drawInfo.m_ShowNotPlatedHole = true;

    // Shows the local pad clearance
    drawInfo.m_PadClearance = m_dummyPad->GetLocalClearance();

    wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Calculate a suitable scale to fit the available draw area
    int dim = m_dummyPad->GetSize().x + std::abs( m_dummyPad->GetDelta().y );

    // Invalid x size. User could enter zero, or have deleted all text prior to
    // entering a new value; this is also treated as zero. If dim is left at
    // zero, the drawing scale is zero and we get a crash.
    if( dim == 0 )
    {
        // If drill size has been set, use that. Otherwise default to 1mm.
        dim = m_dummyPad->GetDrillSize().x;
        if( dim == 0 )
            dim = 1000000;
    }

    if( m_dummyPad->GetLocalClearance() > 0 )
        dim += m_dummyPad->GetLocalClearance() * 2;

    double scale = (double) dc_size.x / dim;

    // If the pad is a circle, use the x size here instead.
    int ysize;
    if( m_dummyPad->GetShape() == PAD_CIRCLE )
        ysize = m_dummyPad->GetSize().x;
    else
        ysize = m_dummyPad->GetSize().y;

    dim = ysize + std::abs( m_dummyPad->GetDelta().x );

    // Invalid y size. See note about x size above.
    if( dim == 0 )
    {
        dim = m_dummyPad->GetDrillSize().y;
        if( dim == 0 )
            dim = 1000000;
    }

    if( m_dummyPad->GetLocalClearance() > 0 )
        dim += m_dummyPad->GetLocalClearance() * 2;

    double altscale = (double) dc_size.y / dim;
    scale = std::min( scale, altscale );

    // Give a margin
    scale *= 0.7;
    dc.SetUserScale( scale, scale );

    GRResetPenAndBrush( &dc );
    m_dummyPad->DrawShape( NULL, &dc, drawInfo );

    // Draw X and Y axis.
    // this is particularly useful to show the reference position of pads
    // with offset and no hole
    GRLine( NULL, &dc, -dim, 0, dim, 0, 0, BLUE );   // X axis
    GRLine( NULL, &dc, 0, -dim, 0, dim, 0, BLUE );   // Y axis

    event.Skip();
}


void DIALOG_PAD_PROPERTIES::initValues()
{
    wxString    msg;
    double      angle;

    // Disable pad net name wxTextCtrl if the caller is the footprint editor
    // because nets are living only in the board managed by the board editor
    m_canEditNetName = m_parent->IsType( FRAME_PCB );


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
        MODULE* module = m_currentPad->GetParent();

        if( module->GetLayer() == B_Cu )
        {
            m_isFlipped = true;
            m_staticModuleSideValue->SetLabel( _( "Back side (footprint is mirrored)" ) );
        }

        msg.Printf( wxT( "%.1f" ), module->GetOrientation() / 10.0 );
        m_staticModuleRotValue->SetLabel( msg );
    }

    if( m_isFlipped )
    {
        wxPoint pt = m_dummyPad->GetOffset();
        NEGATE( pt.y );
        m_dummyPad->SetOffset( pt );

        wxSize sz = m_dummyPad->GetDelta();
        NEGATE( sz.y );
        m_dummyPad->SetDelta( sz );

        // flip pad's layers
        m_dummyPad->SetLayerSet( FlipLayerMask( m_dummyPad->GetLayerSet() ) );
    }

    m_staticTextWarningPadFlipped->Show(m_isFlipped);

    m_PadNumCtrl->SetValue( m_dummyPad->GetPadName() );
    m_PadNetNameCtrl->SetValue( m_dummyPad->GetNetname() );

    // Display current unit name in dialog:
    m_PadPosX_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadPosY_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadDrill_X_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadDrill_Y_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadShapeSizeX_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadShapeSizeY_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadShapeOffsetX_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadShapeOffsetY_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadShapeDelta_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_PadLengthDie_Unit->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    // Display current pad masks clearances units
    m_NetClearanceUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_SolderMaskMarginUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_ThermalWidthUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );
    m_ThermalGapUnits->SetLabel( GetAbbreviatedUnitsLabel( g_UserUnit ) );

    // Display current pad parameters units:
    PutValueInLocalUnits( *m_PadPosition_X_Ctrl, m_dummyPad->GetPosition().x );
    PutValueInLocalUnits( *m_PadPosition_Y_Ctrl, m_dummyPad->GetPosition().y );

    PutValueInLocalUnits( *m_PadDrill_X_Ctrl, m_dummyPad->GetDrillSize().x );
    PutValueInLocalUnits( *m_PadDrill_Y_Ctrl, m_dummyPad->GetDrillSize().y );

    PutValueInLocalUnits( *m_ShapeSize_X_Ctrl, m_dummyPad->GetSize().x );
    PutValueInLocalUnits( *m_ShapeSize_Y_Ctrl, m_dummyPad->GetSize().y );

    PutValueInLocalUnits( *m_ShapeOffset_X_Ctrl, m_dummyPad->GetOffset().x );
    PutValueInLocalUnits( *m_ShapeOffset_Y_Ctrl, m_dummyPad->GetOffset().y );

    if( m_dummyPad->GetDelta().x )
    {
        PutValueInLocalUnits( *m_ShapeDelta_Ctrl, m_dummyPad->GetDelta().x );
        m_trapDeltaDirChoice->SetSelection( 0 );
    }
    else
    {
        PutValueInLocalUnits( *m_ShapeDelta_Ctrl, m_dummyPad->GetDelta().y );
        m_trapDeltaDirChoice->SetSelection( 1 );
    }

    PutValueInLocalUnits( *m_LengthPadToDieCtrl, m_dummyPad->GetPadToDieLength() );

    PutValueInLocalUnits( *m_NetClearanceValueCtrl, m_dummyPad->GetLocalClearance() );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl, m_dummyPad->GetLocalSolderMaskMargin() );
    PutValueInLocalUnits( *m_ThermalWidthCtrl, m_dummyPad->GetThermalWidth() );
    PutValueInLocalUnits( *m_ThermalGapCtrl, m_dummyPad->GetThermalGap() );

    // These 2 parameters are usually < 0, so prepare entering a negative value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl, m_dummyPad->GetLocalSolderPasteMargin() );

    if( m_dummyPad->GetLocalSolderPasteMargin() == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) + m_SolderPasteMarginCtrl->GetValue() );

    msg.Printf( wxT( "%f" ), m_dummyPad->GetLocalSolderPasteMarginRatio() * 100.0 );

    if( m_dummyPad->GetLocalSolderPasteMarginRatio() == 0.0 && msg[0] == '0' )
        // Sometimes Printf adds a sign if the value is small
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    switch( m_dummyPad->GetZoneConnection() )
    {
    default:
    case UNDEFINED_CONNECTION:
        m_ZoneConnectionChoice->SetSelection( 0 );
        break;

    case PAD_IN_ZONE:
        m_ZoneConnectionChoice->SetSelection( 1 );
        break;

    case THERMAL_PAD:
        m_ZoneConnectionChoice->SetSelection( 2 );
        break;

    case PAD_NOT_IN_ZONE:
        m_ZoneConnectionChoice->SetSelection( 3 );
        break;
    }

    if( m_currentPad )
    {
        MODULE* module = m_currentPad->GetParent();

        angle = m_currentPad->GetOrientation() - module->GetOrientation();

        if( m_isFlipped )
            NEGATE( angle );

        m_dummyPad->SetOrientation( angle );
    }

    angle = m_dummyPad->GetOrientation();

    NORMALIZE_ANGLE_180( angle );    // ? normalizing is in D_PAD::SetOrientation()

    // Set layers used by this pad: :
    setPadLayersList( m_dummyPad->GetLayerSet() );

    // Pad Orient
    switch( int( angle ) )
    {
    case 0:
        m_PadOrient->SetSelection( 0 );
        break;

    case 900:
        m_PadOrient->SetSelection( 1 );
        break;

    case -900:
        m_PadOrient->SetSelection( 2 );
        break;

    case 1800:
    case -1800:
        m_PadOrient->SetSelection( 3 );
        break;

    default:
        m_PadOrient->SetSelection( 4 );
        break;
    }

    switch( m_dummyPad->GetShape() )
    {
    default:
    case PAD_CIRCLE:
        m_PadShape->SetSelection( 0 );
        break;

    case PAD_OVAL:
        m_PadShape->SetSelection( 1 );
        break;

    case PAD_RECT:
        m_PadShape->SetSelection( 2 );
        break;

    case PAD_TRAPEZOID:
        m_PadShape->SetSelection( 3 );
        break;
    }

    msg.Printf( wxT( "%g" ), angle );
    m_PadOrientCtrl->SetValue( msg );

    // Type of pad selection
    m_PadType->SetSelection( 0 );

    for( unsigned ii = 0; ii < DIM( code_type ); ii++ )
    {
        if( code_type[ii] == m_dummyPad->GetAttribute() )
        {
            m_PadType->SetSelection( ii );
            break;
        }
    }

    // Enable/disable Pad name,and pad length die
    // (disable for NPTH pads (mechanical pads)
    bool enable = m_dummyPad->GetAttribute() != PAD_HOLE_NOT_PLATED;

    m_PadNumCtrl->Enable( enable );
    m_PadNetNameCtrl->Enable( m_canEditNetName && enable && m_currentPad != NULL );
    m_LengthPadToDieCtrl->Enable( enable );

    if( m_dummyPad->GetDrillShape() != PAD_DRILL_OBLONG )
        m_DrillShapeCtrl->SetSelection( 0 );
    else
        m_DrillShapeCtrl->SetSelection( 1 );

    // Update some dialog widgets state (Enable/disable options):
    wxCommandEvent cmd_event;
    setPadLayersList( m_dummyPad->GetLayerSet() );
    OnDrillShapeSelected( cmd_event );
    OnPadShapeSelection( cmd_event );
}


void DIALOG_PAD_PROPERTIES::OnResize( wxSizeEvent& event )
{
    redraw();
    event.Skip();
}


void DIALOG_PAD_PROPERTIES::OnPadShapeSelection( wxCommandEvent& event )
{
    switch( m_PadShape->GetSelection() )
    {
    case 0:     // CIRCLE:
        m_ShapeDelta_Ctrl->Enable( false );
        m_trapDeltaDirChoice->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( false );
        m_ShapeOffset_X_Ctrl->Enable( false );
        m_ShapeOffset_Y_Ctrl->Enable( false );
        break;

    case 1:     // OVAL:
        m_ShapeDelta_Ctrl->Enable( false );
        m_trapDeltaDirChoice->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( true );
        m_ShapeOffset_X_Ctrl->Enable( true );
        m_ShapeOffset_Y_Ctrl->Enable( true );
        break;

    case 2:     // PAD_RECT:
        m_ShapeDelta_Ctrl->Enable( false );
        m_trapDeltaDirChoice->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( true );
        m_ShapeOffset_X_Ctrl->Enable( true );
        m_ShapeOffset_Y_Ctrl->Enable( true );
        break;

    case 3:     // TRAPEZOID:
        m_ShapeDelta_Ctrl->Enable( true );
        m_trapDeltaDirChoice->Enable( true );
        m_ShapeSize_Y_Ctrl->Enable( true );
        m_ShapeOffset_X_Ctrl->Enable( true );
        m_ShapeOffset_Y_Ctrl->Enable( true );
        break;
    }

    transferDataToPad( m_dummyPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::OnDrillShapeSelected( wxCommandEvent& event )
{
    if( m_PadType->GetSelection() == 1 || m_PadType->GetSelection() == 2 )
    {
        // pad type = SMD or CONN: no hole allowed
        m_PadDrill_X_Ctrl->Enable( false );
        m_PadDrill_Y_Ctrl->Enable( false );
    }
    else
    {
        switch( m_DrillShapeCtrl->GetSelection() )
        {
        case 0:     //CIRCLE:
            m_PadDrill_X_Ctrl->Enable( true );
            m_PadDrill_Y_Ctrl->Enable( false );
            break;

        case 1:     //OVALE:
            m_PadDrill_X_Ctrl->Enable( true );
            m_PadDrill_Y_Ctrl->Enable( true );
            break;
        }
    }

    transferDataToPad( m_dummyPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::PadOrientEvent( wxCommandEvent& event )
{
    switch( m_PadOrient->GetSelection() )
    {
    case 0:
        m_dummyPad->SetOrientation( 0 );
        break;

    case 1:
        m_dummyPad->SetOrientation( 900 );
        break;

    case 2:
        m_dummyPad->SetOrientation( -900 );
        break;

    case 3:
        m_dummyPad->SetOrientation( 1800 );
        break;

    default:
        break;
    }

    wxString msg;
    msg.Printf( wxT( "%g" ), m_dummyPad->GetOrientation() );
    m_PadOrientCtrl->SetValue( msg );

    transferDataToPad( m_dummyPad );
    redraw();
}


void DIALOG_PAD_PROPERTIES::PadTypeSelected( wxCommandEvent& event )
{
    unsigned ii = m_PadType->GetSelection();

    if( ii >= DIM( code_type ) ) // catches < 0 also
        ii = 0;

    LSET layer_mask = std_pad_layers[ii];
    setPadLayersList( layer_mask );

    // Enable/disable drill dialog items:
    event.SetId( m_DrillShapeCtrl->GetSelection() );
    OnDrillShapeSelected( event );

    if( ii == 0 || ii == DIM( code_type )-1 )
        m_DrillShapeCtrl->Enable( true );
    else
        m_DrillShapeCtrl->Enable( false );

    // Enable/disable Pad name,and pad length die
    // (disable for NPTH pads (mechanical pads)
    bool enable = ii != 3;
    m_PadNumCtrl->Enable( enable );
    m_PadNetNameCtrl->Enable( m_canEditNetName && enable && m_currentPad != NULL );
    m_LengthPadToDieCtrl->Enable( enable );
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

    wxArrayString error_msgs;
    wxString msg;

    // Test for incorrect values
    if( (m_dummyPad->GetSize().x <= 0) ||
       ((m_dummyPad->GetSize().y <= 0) && (m_dummyPad->GetShape() != PAD_CIRCLE)) )
    {
        error_msgs.Add( _( "Pad size must be greater than zero" ) );
    }

    if( (m_dummyPad->GetSize().x < m_dummyPad->GetDrillSize().x) ||
        (m_dummyPad->GetSize().y < m_dummyPad->GetDrillSize().y) )
    {
        error_msgs.Add(  _( "Incorrect value for pad drill: pad drill bigger than pad size" ) );
    }

    LSET padlayers_mask = m_dummyPad->GetLayerSet();

    if( padlayers_mask == 0 )
        error_msgs.Add( _( "Error: pad has no layer" ) );

    if( !padlayers_mask[F_Cu] && !padlayers_mask[B_Cu] )
    {
        if( m_dummyPad->GetDrillSize().x || m_dummyPad->GetDrillSize().y )
        {
            // Note: he message is shown in an HTML window
            msg = _( "Error: the pad is not on a copper layer and has a hole" );

            if( m_dummyPad->GetAttribute() == PAD_HOLE_NOT_PLATED )
            {
                msg += wxT( "<br><br><i>" );
                msg += _( "For NPTH pad, set pad size value to pad drill value,"
                          " if you do not want this pad plotted in gerber files"
                    );
            }

            error_msgs.Add( msg );
        }
    }

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

    if( error )
    {
        error_msgs.Add(  _( "Too large value for pad delta size" ) );
    }

    switch( m_dummyPad->GetAttribute() )
    {
    case PAD_HOLE_NOT_PLATED:   // Not plated, but through hole, a hole is expected
    case PAD_STANDARD :         // Pad through hole, a hole is also expected
        if( m_dummyPad->GetDrillSize().x <= 0 )
            error_msgs.Add( _( "Error: Through hole pad: drill diameter set to 0" ) );
        break;

    case PAD_CONN:      // Connector pads are smd pads, just they do not have solder paste.
        if( padlayers_mask[B_Paste] || padlayers_mask[F_Paste] )
            error_msgs.Add( _( "Error: Connector pads are not on the solder paste layer\n"
                               "Use SMD pads instead" ) );
        // Fall trough
    case PAD_SMD:       // SMD and Connector pads (One external copper layer only)
/*
        if( padlayers_mask[B_Cu] && padlayers_mask[F_Cu] )
            error_msgs.Add( _( "Error: only one copper layer allowed for SMD or Connector pads" ) );
*/
        break;
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
    if( m_parent->IsGalCanvasActive() )
    {
        m_dummyPad->ViewUpdate();

        BOX2I bbox = m_dummyPad->ViewBBox();

        if( bbox.GetSize().x > 0 && bbox.GetSize().y > 0 )
        {
            // Autozoom
            m_panelShowPadGal->GetView()->SetViewport( BOX2D( bbox.GetOrigin(), bbox.GetSize() ) );

            // Add a margin
            m_panelShowPadGal->GetView()->SetScale( m_panelShowPadGal->GetView()->GetScale() * 0.7 );

            m_panelShowPadGal->Refresh();
        }
    }
    else
    {
        m_panelShowPad->Refresh();
    }
}


void DIALOG_PAD_PROPERTIES::PadPropertiesAccept( wxCommandEvent& event )
{
    if( !padValuesOK() )
        return;

    bool rastnestIsChanged = false;
    int  isign = m_isFlipped ? -1 : 1;

    transferDataToPad( m_padMaster );
    // m_padMaster is a pattern: ensure there is no net for this pad:
    m_padMaster->SetNetCode( NETINFO_LIST::UNCONNECTED );

    if( m_currentPad )   // Set current Pad parameters
    {
        wxSize  size;
        MODULE* module = m_currentPad->GetParent();

        m_parent->SaveCopyInUndoList( module, UR_CHANGED );
        module->SetLastEditTime();

        // redraw the area where the pad was, without pad (delete pad on screen)
        m_currentPad->SetFlags( DO_NOT_DRAW );
        m_parent->GetCanvas()->RefreshDrawingRect( m_currentPad->GetBoundingBox() );
        m_currentPad->ClearFlags( DO_NOT_DRAW );

        // Update values
        m_currentPad->SetShape( m_padMaster->GetShape() );
        m_currentPad->SetAttribute( m_padMaster->GetAttribute() );

        if( m_currentPad->GetPosition() != m_padMaster->GetPosition() )
        {
            m_currentPad->SetPosition( m_padMaster->GetPosition() );
            rastnestIsChanged = true;
        }

        // compute the pos 0 value, i.e. pad position for module with orientation = 0
        // i.e. relative to module origin (module position)
        wxPoint pt = m_currentPad->GetPosition() - module->GetPosition();

        RotatePoint( &pt, -module->GetOrientation() );

        m_currentPad->SetPos0( pt );

        m_currentPad->SetOrientation( m_padMaster->GetOrientation() * isign + module->GetOrientation() );

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

        if( m_currentPad->GetLayerSet() != m_padMaster->GetLayerSet() )
        {
            rastnestIsChanged = true;
            m_currentPad->SetLayerSet( m_padMaster->GetLayerSet() );
        }

        if( m_isFlipped )
            m_currentPad->SetLayerSet( FlipLayerMask( m_currentPad->GetLayerSet() ) );

        m_currentPad->SetPadName( m_padMaster->GetPadName() );

        wxString padNetname;

        // For PAD_HOLE_NOT_PLATED, ensure there is no net name selected
        if( m_padMaster->GetAttribute() != PAD_HOLE_NOT_PLATED  )
            padNetname = m_PadNetNameCtrl->GetValue();

        if( m_currentPad->GetNetname() != padNetname )
        {
            const NETINFO_ITEM* netinfo = m_board->FindNet( padNetname );

            if( !padNetname.IsEmpty() && netinfo == NULL )
            {
                DisplayError( NULL, _( "Unknown netname, netname not changed" ) );
            }
            else if( netinfo )
            {
                rastnestIsChanged = true;
                m_currentPad->SetNetCode( netinfo->GetNet() );
            }
        }

        m_currentPad->SetLocalClearance( m_padMaster->GetLocalClearance() );
        m_currentPad->SetLocalSolderMaskMargin( m_padMaster->GetLocalSolderMaskMargin() );
        m_currentPad->SetLocalSolderPasteMargin( m_padMaster->GetLocalSolderPasteMargin() );
        m_currentPad->SetLocalSolderPasteMarginRatio( m_padMaster->GetLocalSolderPasteMarginRatio() );
        m_currentPad->SetZoneConnection( m_padMaster->GetZoneConnection() );
        m_currentPad->SetThermalWidth( m_padMaster->GetThermalWidth() );
        m_currentPad->SetThermalGap( m_padMaster->GetThermalGap() );

        module->CalculateBoundingBox();
        m_parent->SetMsgPanel( m_currentPad );

        // redraw the area where the pad was
        m_parent->GetCanvas()->RefreshDrawingRect( m_currentPad->GetBoundingBox() );
        m_parent->OnModify();
    }

    EndModal( wxID_OK );

    if( rastnestIsChanged )  // The net ratsnest must be recalculated
        m_board->m_Status_Pcb = 0;
}


bool DIALOG_PAD_PROPERTIES::transferDataToPad( D_PAD* aPad )
{
    wxString    msg;
    int         x, y;

    aPad->SetAttribute( code_type[m_PadType->GetSelection()] );
    aPad->SetShape( code_shape[m_PadShape->GetSelection()] );

    // Read pad clearances values:
    aPad->SetLocalClearance( ValueFromTextCtrl( *m_NetClearanceValueCtrl ) );
    aPad->SetLocalSolderMaskMargin( ValueFromTextCtrl( *m_SolderMaskMarginCtrl ) );
    aPad->SetLocalSolderPasteMargin( ValueFromTextCtrl( *m_SolderPasteMarginCtrl ) );
    aPad->SetThermalWidth( ValueFromTextCtrl( *m_ThermalWidthCtrl ) );
    aPad->SetThermalGap( ValueFromTextCtrl( *m_ThermalGapCtrl ) );
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
    case 0:
        aPad->SetZoneConnection( UNDEFINED_CONNECTION );
        break;

    case 1:
        aPad->SetZoneConnection( PAD_IN_ZONE );
        break;

    case 2:
        aPad->SetZoneConnection( THERMAL_PAD );
        break;

    case 3:
        aPad->SetZoneConnection( PAD_NOT_IN_ZONE );
        break;
    }

    // Read pad position:
    x = ValueFromTextCtrl( *m_PadPosition_X_Ctrl );
    y = ValueFromTextCtrl( *m_PadPosition_Y_Ctrl );

    aPad->SetPosition( wxPoint( x, y ) );
    aPad->SetPos0( wxPoint( x, y ) );

    // Read pad drill:
    x = ValueFromTextCtrl( *m_PadDrill_X_Ctrl );
    y = ValueFromTextCtrl( *m_PadDrill_Y_Ctrl );

    if( m_DrillShapeCtrl->GetSelection() == 0 )
    {
        aPad->SetDrillShape( PAD_DRILL_CIRCLE );
        y = x;
    }
    else
        aPad->SetDrillShape( PAD_DRILL_OBLONG );

    aPad->SetDrillSize( wxSize( x, y ) );

    // Read pad shape size:
    x = ValueFromTextCtrl( *m_ShapeSize_X_Ctrl );
    y = ValueFromTextCtrl( *m_ShapeSize_Y_Ctrl );
    if( aPad->GetShape() == PAD_CIRCLE )
        y = x;

    aPad->SetSize( wxSize( x, y ) );

    // Read pad length die
    aPad->SetPadToDieLength( ValueFromTextCtrl( *m_LengthPadToDieCtrl ) );

    // Read pad shape delta size:
    // m_DeltaSize.x or m_DeltaSize.y must be NULL. for a trapezoid.
    wxSize delta;

    if( m_trapDeltaDirChoice->GetSelection() == 0 )
        delta.x = ValueFromTextCtrl( *m_ShapeDelta_Ctrl );
    else
        delta.y = ValueFromTextCtrl( *m_ShapeDelta_Ctrl );

    // Test bad values (be sure delta values are not too large)
    // remember DeltaSize.x is the Y size variation
    bool   error    = false;

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

    aPad->SetDelta( delta );

    // Read pad shape offset:
    x = ValueFromTextCtrl( *m_ShapeOffset_X_Ctrl );
    y = ValueFromTextCtrl( *m_ShapeOffset_Y_Ctrl );
    aPad->SetOffset( wxPoint( x, y ) );

    double orient_value = 0;
    msg = m_PadOrientCtrl->GetValue();
    msg.ToDouble( &orient_value );

    aPad->SetOrientation( orient_value );

    msg = m_PadNumCtrl->GetValue().Left( 4 );
    aPad->SetPadName( msg );

    // Check if user has set an existing net name
    const NETINFO_ITEM* netinfo = m_board->FindNet( m_PadNetNameCtrl->GetValue() );

    if( netinfo != NULL )
        aPad->SetNetCode( netinfo->GetNet() );
    else
        aPad->SetNetCode( NETINFO_LIST::UNCONNECTED );

    // Clear some values, according to the pad type and shape
    switch( aPad->GetShape() )
    {
    case PAD_CIRCLE:
        aPad->SetOffset( wxPoint( 0, 0 ) );
        aPad->SetDelta( wxSize( 0, 0 ) );
        x = aPad->GetSize().x;
        aPad->SetSize( wxSize( x, x ) );
        break;

    case PAD_RECT:
        aPad->SetDelta( wxSize( 0, 0 ) );
        break;

    case PAD_OVAL:
        aPad->SetDelta( wxSize( 0, 0 ) );
        break;

    case PAD_TRAPEZOID:
        break;

    default:
        ;
    }

    switch( aPad->GetAttribute() )
    {
    case PAD_STANDARD:
        break;

    case PAD_CONN:
    case PAD_SMD:
        // SMD and PAD_CONN has no hole.
        // basically, SMD and PAD_CONN are same type of pads
        // PAD_CONN has just a default non technical layers that differs from SMD
        // and are intended to be used in virtual edge board connectors
        // However we can accept a non null offset,
        // mainly to allow complex pads build from a set of from basic pad shapes
        aPad->SetDrillSize( wxSize( 0, 0 ) );
        break;

    case PAD_HOLE_NOT_PLATED:
        // Mechanical purpose only:
        // no offset, no net name, no pad name allowed
        aPad->SetOffset( wxPoint( 0, 0 ) );
        aPad->SetPadName( wxEmptyString );
        aPad->SetNetCode( NETINFO_LIST::UNCONNECTED );
        break;

    default:
        DisplayError( NULL, wxT( "Error: unknown pad type" ) );
        break;
    }

    LSET padLayerMask;

    switch( m_rbCopperLayersSel->GetSelection() )
    {
    case 0:
        padLayerMask.set( F_Cu );
        break;

    case 1:
        padLayerMask.set( B_Cu );
        break;

    case 2:
        padLayerMask |= LSET::AllCuMask();
        break;

    case 3:     // No copper layers
        break;
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


void DIALOG_PAD_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        redraw();
    }
}


void DIALOG_PAD_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}
