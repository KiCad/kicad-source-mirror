/*******************************************************************/
/* dialog_pad_properties.cpp: Pad editing functions and dialog box */
/* see also dialog_pad_properties_base.xxx (built with wxFormBuilder)   */
/*******************************************************************/

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/dcbuffer.h>
#include <protos.h>

#include <class_board.h>
#include <class_module.h>

#include <dialog_pad_properties_base.h>
#include <html_messagebox.h>


// list of pad shapes.
static PAD_SHAPE_T CodeShape[] = {
    PAD_CIRCLE, PAD_OVAL, PAD_RECT, PAD_TRAPEZOID
};


static PAD_ATTR_T CodeType[] = {
    PAD_STANDARD, PAD_SMD, PAD_CONN, PAD_HOLE_NOT_PLATED
};

#define NBTYPES     DIM(CodeType)


// Default mask layers setup for pads according to the pad type
static long Std_Pad_Layers[] = {

    // PAD_STANDARD:
    PAD_STANDARD_DEFAULT_LAYERS,

    // PAD_CONN:
    PAD_CONN_DEFAULT_LAYERS,

    // PAD_SMD:
    PAD_SMD_DEFAULT_LAYERS,

    //PAD_HOLE_NOT_PLATED:
    PAD_HOLE_NOT_PLATED_DEFAULT_LAYERS
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
    PCB_BASE_FRAME* m_Parent;
    D_PAD*  m_CurrentPad;            // pad currently being edited
    D_PAD*  m_dummyPad;              // a working copy used to show changes
    BOARD*  m_Board;
    D_PAD&  m_Pad_Master;
    bool    m_isFlipped;            // true if the parent footprint (therefore pads) is flipped (mirrored)
                                    // in this case, some Y coordinates values must be negated
    bool    m_canUpdate;

    void initValues();

    bool padValuesOK();       ///< test if all values are acceptable for the pad

    /**
     * Function setPadLayersList
     * updates the CheckBox states in pad layers list,
     * @param layer_mask = pad layer mask (ORed layers bit mask)
     */
    void setPadLayersList( long layer_mask );

    /// Copy values from dialog field to aPad's members
    bool transferDataToPad( D_PAD* aPad );

    // event handlers:

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


DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* aParent, D_PAD* aPad ) :
    DIALOG_PAD_PROPERTIES_BASE( aParent ),
    // use aParent's parent, which is the original BOARD, not the dummy module editor BOARD,
    // since FOOTPRINT_EDIT_FRAME::GetDesignSettings() is tricked out to use the PCB_EDIT_FRAME's
    // BOARD, not its own BOARD.
    m_Pad_Master( aParent->GetDesignSettings().m_Pad_Master )
{
    m_canUpdate  = false;
    m_Parent     = aParent;
    m_CurrentPad = aPad;
    m_Board      = m_Parent->GetBoard();
    m_dummyPad   = new D_PAD( (MODULE*) NULL );

    if( aPad )
        m_dummyPad->Copy( aPad );
    else
        m_dummyPad->Copy( &m_Pad_Master );

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

    int          color = 0;

    if( m_dummyPad->GetLayerMask() & LAYER_FRONT )
    {
        color = m_Board->GetVisibleElementColor( PAD_FR_VISIBLE );
    }

    if( m_dummyPad->GetLayerMask() & LAYER_BACK )
    {
        color |= m_Board->GetVisibleElementColor( PAD_BK_VISIBLE );
    }

    if( color == 0 )
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
    int dim = m_dummyPad->GetSize().x + ABS( m_dummyPad->GetDelta().y);

    if( m_dummyPad->GetLocalClearance() > 0 )
        dim += m_dummyPad->GetLocalClearance() * 2;

    double scale = (double) dc_size.x / dim;

    dim = m_dummyPad->GetSize().y + ABS( m_dummyPad->GetDelta().x);
    if( m_dummyPad->GetLocalClearance() > 0 )
        dim += m_dummyPad->GetLocalClearance() * 2;

    double altscale = (double) dc_size.y / dim;
    scale = MIN( scale, altscale );

    // Give a margin
    scale *= 0.7;
    dc.SetUserScale( scale, scale );

    GRResetPenAndBrush( &dc );
    m_dummyPad->DrawShape( NULL, &dc, drawInfo );

    // Draw X and Y axis.
    // this is particularly useful to show the reference position of pads
    // with offset and no hole
    int width = 0;
    GRLine( NULL, &dc, -dim, 0, dim, 0, width, BLUE );   // X axis
    GRLine( NULL, &dc, 0, -dim, 0, dim, width, BLUE );   // Y axis

    event.Skip();
}


void PCB_BASE_FRAME::InstallPadOptionsFrame( D_PAD* aPad )
{
    DIALOG_PAD_PROPERTIES dlg( this, aPad );

    dlg.ShowModal();
}


void DIALOG_PAD_PROPERTIES::initValues()
{
    int         internalUnits = m_Parent->GetInternalUnits();
    wxString    msg;
    double      angle;

    // Setup layers names from board
    // Should be made first, before calling m_rbCopperLayersSel->SetSelection()
    m_rbCopperLayersSel->SetString( 0, m_Board->GetLayerName( LAYER_N_FRONT ) );
    m_rbCopperLayersSel->SetString( 1, m_Board->GetLayerName( LAYER_N_BACK ) );

    m_PadLayerAdhCmp->SetLabel( m_Board->GetLayerName( ADHESIVE_N_FRONT ) );
    m_PadLayerAdhCu->SetLabel( m_Board->GetLayerName( ADHESIVE_N_BACK ) );
    m_PadLayerPateCmp->SetLabel( m_Board->GetLayerName( SOLDERPASTE_N_FRONT ) );
    m_PadLayerPateCu->SetLabel( m_Board->GetLayerName( SOLDERPASTE_N_BACK ) );
    m_PadLayerSilkCmp->SetLabel( m_Board->GetLayerName( SILKSCREEN_N_FRONT ) );
    m_PadLayerSilkCu->SetLabel( m_Board->GetLayerName( SILKSCREEN_N_BACK ) );
    m_PadLayerMaskCmp->SetLabel( m_Board->GetLayerName( SOLDERMASK_N_FRONT ) );
    m_PadLayerMaskCu->SetLabel( m_Board->GetLayerName( SOLDERMASK_N_BACK ) );
    m_PadLayerECO1->SetLabel( m_Board->GetLayerName( ECO1_N ) );
    m_PadLayerECO2->SetLabel( m_Board->GetLayerName( ECO2_N ) );
    m_PadLayerDraft->SetLabel( m_Board->GetLayerName( DRAW_N ) );

    m_isFlipped = false;

    if( m_CurrentPad )
    {
        MODULE* module = m_CurrentPad->GetParent();

        if( module->GetLayer() == LAYER_N_BACK )
        {
            m_isFlipped = true;
            m_staticModuleSideValue->SetLabel( _( "Back side (footprint is mirrored)" ) );
        }

        msg.Printf( wxT( "%.1f" ), (double) module->GetOrientation() / 10 );
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
        m_dummyPad->SetLayerMask( ChangeSideMaskLayer( m_dummyPad->GetLayerMask() ) );
    }

    m_staticTextWarningPadFlipped->Show(m_isFlipped);

    m_PadNumCtrl->SetValue( m_dummyPad->GetPadName() );
    m_PadNetNameCtrl->SetValue( m_dummyPad->GetNetname() );

    // Display current unit name in dialog:
    m_PadPosX_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadPosY_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadDrill_X_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadDrill_Y_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadShapeSizeX_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadShapeSizeY_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadShapeOffsetX_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadShapeOffsetY_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadShapeDelta_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_PadLengthDie_Unit->SetLabel( GetUnitsLabel( g_UserUnit ) );

    // Display current pad masks clearances units
    m_NetClearanceUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_ThermalWidthUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_ThermalGapUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    // Display current pad parameters units:
    PutValueInLocalUnits( *m_PadPosition_X_Ctrl, m_dummyPad->GetPosition().x, internalUnits );
    PutValueInLocalUnits( *m_PadPosition_Y_Ctrl, m_dummyPad->GetPosition().y, internalUnits );

    PutValueInLocalUnits( *m_PadDrill_X_Ctrl, m_dummyPad->GetDrillSize().x, internalUnits );
    PutValueInLocalUnits( *m_PadDrill_Y_Ctrl, m_dummyPad->GetDrillSize().y, internalUnits );

    PutValueInLocalUnits( *m_ShapeSize_X_Ctrl, m_dummyPad->GetSize().x, internalUnits );
    PutValueInLocalUnits( *m_ShapeSize_Y_Ctrl, m_dummyPad->GetSize().y, internalUnits );

    PutValueInLocalUnits( *m_ShapeOffset_X_Ctrl, m_dummyPad->GetOffset().x, internalUnits );
    PutValueInLocalUnits( *m_ShapeOffset_Y_Ctrl, m_dummyPad->GetOffset().y, internalUnits );

    if( m_dummyPad->GetDelta().x )
    {
        PutValueInLocalUnits( *m_ShapeDelta_Ctrl, m_dummyPad->GetDelta().x, internalUnits );
        m_trapDeltaDirChoice->SetSelection( 0 );
    }
    else
    {
        PutValueInLocalUnits( *m_ShapeDelta_Ctrl, m_dummyPad->GetDelta().y, internalUnits );
        m_trapDeltaDirChoice->SetSelection( 1 );
    }

    PutValueInLocalUnits( *m_LengthDieCtrl, m_dummyPad->GetDieLength(), internalUnits );

    PutValueInLocalUnits( *m_NetClearanceValueCtrl, m_dummyPad->GetLocalClearance(), internalUnits );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl,
                          m_dummyPad->GetLocalSolderMaskMargin(),
                          internalUnits );
    PutValueInLocalUnits( *m_ThermalWidthCtrl, m_dummyPad->GetThermalWidth(), internalUnits );
    PutValueInLocalUnits( *m_ThermalGapCtrl, m_dummyPad->GetThermalGap(), internalUnits );

    // These 2 parameters are usually < 0, so prepare entering a negative value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl,
                          m_dummyPad->GetLocalSolderPasteMargin(),
                          internalUnits );

    if( m_dummyPad->GetLocalSolderPasteMargin() == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) + m_SolderPasteMarginCtrl->GetValue() );

    msg.Printf( wxT( "%.1f" ), m_dummyPad->GetLocalSolderPasteMarginRatio() * 100.0 );

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

    if( m_CurrentPad )
    {
        MODULE* module = m_CurrentPad->GetParent();

        angle = m_CurrentPad->GetOrientation() - module->GetOrientation();

        if( m_isFlipped )
            NEGATE( angle );

        m_dummyPad->SetOrientation( angle );
    }

    angle = m_dummyPad->GetOrientation();

    NORMALIZE_ANGLE_180( angle );    // ? normalizing is in D_PAD::SetOrientation()

    // Set layers used by this pad: :
    setPadLayersList( m_dummyPad->GetLayerMask() );

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

    for( unsigned ii = 0; ii < NBTYPES; ii++ )
    {
        if( CodeType[ii] == m_dummyPad->GetAttribute() )
        {
            m_PadType->SetSelection( ii );
            break;
        }
    }

    // Enable/disable Pad name,and pad length die
    // (disable for NPTH pads (mechanical pads)
    bool enable = m_dummyPad->GetAttribute() != PAD_HOLE_NOT_PLATED;

    m_PadNumCtrl->Enable( enable );
    m_PadNetNameCtrl->Enable( enable );
    m_LengthDieCtrl->Enable( enable );

    if( m_dummyPad->GetDrillShape() != PAD_OVAL )
        m_DrillShapeCtrl->SetSelection( 0 );
    else
        m_DrillShapeCtrl->SetSelection( 1 );

    // Update some dialog widgets state (Enable/disable options):
    wxCommandEvent cmd_event;
    setPadLayersList( m_dummyPad->GetLayerMask() );
    OnDrillShapeSelected( cmd_event );
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
    m_panelShowPad->Refresh();
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
    m_panelShowPad->Refresh();
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
    m_panelShowPad->Refresh();
}


void DIALOG_PAD_PROPERTIES::PadTypeSelected( wxCommandEvent& event )
{
    long        layer_mask;
    unsigned    ii = m_PadType->GetSelection();

    if( ii >= NBTYPES ) // catches < 0 also
        ii = 0;

    layer_mask = Std_Pad_Layers[ii];
    setPadLayersList( layer_mask );

    // Enable/disable drill dialog items:
    event.SetId( m_DrillShapeCtrl->GetSelection() );
    OnDrillShapeSelected( event );

    if( ii == 0 || ii == NBTYPES-1 )
        m_DrillShapeCtrl->Enable( true );
    else
        m_DrillShapeCtrl->Enable( false );

    // Enable/disable Pad name,and pad length die
    // (disable for NPTH pads (mechanical pads)
    bool enable = ii != 3;
    m_PadNumCtrl->Enable( enable );
    m_PadNetNameCtrl->Enable( enable );
    m_LengthDieCtrl->Enable( enable );
}


void DIALOG_PAD_PROPERTIES::setPadLayersList( long layer_mask )
{
    if( ( layer_mask & ALL_CU_LAYERS ) == LAYER_FRONT )
        m_rbCopperLayersSel->SetSelection(0);
    else if( ( layer_mask & ALL_CU_LAYERS ) == LAYER_BACK)
        m_rbCopperLayersSel->SetSelection(1);
    else if( ( layer_mask & ALL_CU_LAYERS ) != 0 )
        m_rbCopperLayersSel->SetSelection(2);
    else
        m_rbCopperLayersSel->SetSelection(3);

    m_PadLayerAdhCmp->SetValue( bool( layer_mask & ADHESIVE_LAYER_FRONT ) );
    m_PadLayerAdhCu->SetValue( bool( layer_mask & ADHESIVE_LAYER_BACK ) );

    m_PadLayerPateCmp->SetValue( bool( layer_mask & SOLDERPASTE_LAYER_FRONT ) );
    m_PadLayerPateCu->SetValue( bool( layer_mask & SOLDERPASTE_LAYER_BACK ) );

    m_PadLayerSilkCmp->SetValue( bool( layer_mask & SILKSCREEN_LAYER_FRONT ) );
    m_PadLayerSilkCu->SetValue( bool( layer_mask & SILKSCREEN_LAYER_BACK ) );

    m_PadLayerMaskCmp->SetValue( bool( layer_mask & SOLDERMASK_LAYER_FRONT ) );
    m_PadLayerMaskCu->SetValue( bool( layer_mask & SOLDERMASK_LAYER_BACK ) );

    m_PadLayerECO1->SetValue( bool( layer_mask & ECO1_LAYER ) );
    m_PadLayerECO2->SetValue( bool( layer_mask & ECO2_LAYER ) );

    m_PadLayerDraft->SetValue( bool( layer_mask & DRAW_LAYER ) );
}


// Called when select/deselect a layer.
void DIALOG_PAD_PROPERTIES::OnSetLayers( wxCommandEvent& event )
{
    transferDataToPad( m_dummyPad );
    m_panelShowPad->Refresh();
}


// test if all values are acceptable for the pad
bool DIALOG_PAD_PROPERTIES::padValuesOK()
{
    bool error = transferDataToPad( m_dummyPad );

    wxArrayString error_msgs;
    wxString msg;

    // Test for incorrect values
    if( (m_dummyPad->GetSize().x < m_dummyPad->GetDrillSize().x) ||
        (m_dummyPad->GetSize().y < m_dummyPad->GetDrillSize().y) )
    {
        error_msgs.Add(  _( "Incorrect value for pad drill: pad drill bigger than pad size" ) );
    }

    int padlayers_mask = m_dummyPad->GetLayerMask();
    if( ( padlayers_mask == 0 ) && ( m_dummyPad->GetAttribute() != PAD_HOLE_NOT_PLATED ) )
        error_msgs.Add( _( "Error: pad has no layer and is not a mechanical pad" ) );

    padlayers_mask &= (LAYER_BACK | LAYER_FRONT);
    if( padlayers_mask == 0 )
    {
        if( m_dummyPad->GetDrillSize().x || m_dummyPad->GetDrillSize().y )
        {
            msg = _( "Error: pad is not on a copper layer and has a hole" );
            if( m_dummyPad->GetAttribute() == PAD_HOLE_NOT_PLATED )
            {
                msg += wxT("\n");
                msg += _( "For NPTH pad, set pad drill value to pad size value,\n\
if you do not want this pad plotted in gerber files");
            }
            error_msgs.Add( msg );
        }
    }

    wxPoint max_size;
    max_size.x = ABS( m_dummyPad->GetOffset().x );
    max_size.y = ABS( m_dummyPad->GetOffset().y );
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
        case PAD_STANDARD :     // Pad through hole, a hole is expected
            if( m_dummyPad->GetDrillSize().x <= 0 )
                error_msgs.Add( _( "Incorrect value for pad drill (too small value)" ) );
            break;

        case PAD_SMD:     // SMD and Connector pads (One external copper layer only)
        case PAD_CONN:
            if( (padlayers_mask & LAYER_BACK) && (padlayers_mask & LAYER_FRONT) )
                error_msgs.Add( _( "Error: only one copper layer allowed for this pad" ) );
            break;

        case PAD_HOLE_NOT_PLATED:     // Not plated
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


void DIALOG_PAD_PROPERTIES::PadPropertiesAccept( wxCommandEvent& event )
{
    if( !padValuesOK() )
        return;

    bool rastnestIsChanged = false;
    int  isign = m_isFlipped ? -1 : 1;

    transferDataToPad( &m_Pad_Master );

    if( m_CurrentPad )   // Set current Pad parameters
    {
        wxSize  size;
        MODULE* module = m_CurrentPad->GetParent();

        m_Parent->SaveCopyInUndoList( module, UR_CHANGED );
        module->m_LastEdit_Time = time( NULL );

        // redraw the area where the pad was, without pad (delete pad on screen)
        m_CurrentPad->SetFlags( DO_NOT_DRAW );
        m_Parent->GetCanvas()->RefreshDrawingRect( m_CurrentPad->GetBoundingBox() );
        m_CurrentPad->ClearFlags( DO_NOT_DRAW );

        // Update values
        m_CurrentPad->SetShape( m_Pad_Master.GetShape() );
        m_CurrentPad->SetAttribute( m_Pad_Master.GetAttribute() );

        if( m_CurrentPad->GetPosition() != m_Pad_Master.GetPosition() )
        {
            m_CurrentPad->SetPosition( m_Pad_Master.GetPosition() );
            rastnestIsChanged = true;
        }

        // compute the pos 0 value, i.e. pad position for module with orientation = 0
        // i.e. relative to module origin (module position)
        wxPoint pt = m_CurrentPad->GetPosition() - module->GetPosition();

        RotatePoint( &pt, -module->GetOrientation() );

        m_CurrentPad->SetPos0( pt );

        m_CurrentPad->SetOrientation( m_Pad_Master.GetOrientation() * isign + module->GetOrientation() );

        m_CurrentPad->SetSize( m_Pad_Master.GetSize() );

        size = m_Pad_Master.GetDelta();
        size.y *= isign;
        m_CurrentPad->SetDelta( size );

        m_CurrentPad->SetDrillSize( m_Pad_Master.GetDrillSize() );
        m_CurrentPad->SetDrillShape( m_Pad_Master.GetDrillShape() );

        wxPoint offset = m_Pad_Master.GetOffset();
        offset.y *= isign;
        m_CurrentPad->SetOffset( offset );

        m_CurrentPad->SetDieLength( m_Pad_Master.GetDieLength() );

        if( m_CurrentPad->GetLayerMask() != m_Pad_Master.GetLayerMask() )
        {
            rastnestIsChanged = true;
            m_CurrentPad->SetLayerMask( m_Pad_Master.GetLayerMask() );
        }

        if( m_isFlipped )
            m_CurrentPad->SetLayerMask( ChangeSideMaskLayer( m_CurrentPad->GetLayerMask() ) );

        m_CurrentPad->SetPadName( m_Pad_Master.GetPadName() );

        if( m_CurrentPad->GetNetname() != m_Pad_Master.GetNetname() )
        {
            if( m_Pad_Master.GetNetname().IsEmpty() )
            {
                rastnestIsChanged = true;
                m_CurrentPad->SetNet( 0 );
                m_CurrentPad->SetNetname( wxEmptyString );
            }
            else
            {
                const NETINFO_ITEM* net = m_Parent->GetBoard()->FindNet( m_Pad_Master.GetNetname() );
                if( net )
                {
                    rastnestIsChanged = true;
                    m_CurrentPad->SetNetname( m_Pad_Master.GetNetname() );
                    m_CurrentPad->SetNet( net->GetNet() );
                }
                else
                    DisplayError( NULL, _( "Unknown netname, netname not changed" ) );
            }
        }

        m_CurrentPad->SetLocalClearance( m_Pad_Master.GetLocalClearance() );
        m_CurrentPad->SetLocalSolderMaskMargin( m_Pad_Master.GetLocalSolderMaskMargin() );
        m_CurrentPad->SetLocalSolderPasteMargin( m_Pad_Master.GetLocalSolderPasteMargin() );
        m_CurrentPad->SetLocalSolderPasteMarginRatio( m_Pad_Master.GetLocalSolderPasteMarginRatio() );
        m_CurrentPad->SetZoneConnection( m_Pad_Master.GetZoneConnection() );
        m_CurrentPad->SetThermalWidth( m_Pad_Master.GetThermalWidth() );
        m_CurrentPad->SetThermalGap( m_Pad_Master.GetThermalGap() );

        module->CalculateBoundingBox();
        m_CurrentPad->DisplayInfo( m_Parent );

        // redraw the area where the pad was
        m_Parent->GetCanvas()->RefreshDrawingRect( m_CurrentPad->GetBoundingBox() );
        m_Parent->OnModify();
    }

    EndModal( wxID_OK );

    if( rastnestIsChanged )  // The net ratsnest must be recalculated
        m_Parent->GetBoard()->m_Status_Pcb = 0;
}


bool DIALOG_PAD_PROPERTIES::transferDataToPad( D_PAD* aPad )
{
    long        padLayerMask;
    int         internalUnits = m_Parent->GetInternalUnits();
    wxString    msg;
    int         x, y;

    aPad->SetAttribute( CodeType[m_PadType->GetSelection()] );
    aPad->SetShape( CodeShape[m_PadShape->GetSelection()] );

    // Read pad clearances values:
    aPad->SetLocalClearance( ReturnValueFromTextCtrl( *m_NetClearanceValueCtrl,
                                                      internalUnits ) );
    aPad->SetLocalSolderMaskMargin( ReturnValueFromTextCtrl( *m_SolderMaskMarginCtrl,
                                                             internalUnits ) );
    aPad->SetLocalSolderPasteMargin( ReturnValueFromTextCtrl( *m_SolderPasteMarginCtrl,
                                                              internalUnits ) );
    aPad->SetThermalWidth( ReturnValueFromTextCtrl( *m_ThermalWidthCtrl,
                                                    internalUnits ) );
    aPad->SetThermalGap( ReturnValueFromTextCtrl( *m_ThermalGapCtrl,
                                                  internalUnits ) );
    double dtmp = 0.0;
    msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A margin ratio of -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;
    if( dtmp > +100 )
        dtmp = +100;

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
    x = ReturnValueFromTextCtrl( *m_PadPosition_X_Ctrl, internalUnits );
    y = ReturnValueFromTextCtrl( *m_PadPosition_Y_Ctrl, internalUnits );

    aPad->SetPosition( wxPoint( x, y ) );
    aPad->SetPos0( wxPoint( x, y ) );

    // Read pad drill:
    x = ReturnValueFromTextCtrl( *m_PadDrill_X_Ctrl, internalUnits );
    y = ReturnValueFromTextCtrl( *m_PadDrill_Y_Ctrl, internalUnits );

    if( m_DrillShapeCtrl->GetSelection() == 0 )
    {
        aPad->SetDrillShape( PAD_CIRCLE );
        y = x;
    }
    else
        aPad->SetDrillShape( PAD_OVAL );

    aPad->SetDrillSize( wxSize( x, y ) );

    // Read pad shape size:
    x = ReturnValueFromTextCtrl( *m_ShapeSize_X_Ctrl, internalUnits );
    y = ReturnValueFromTextCtrl( *m_ShapeSize_Y_Ctrl, internalUnits );
    if( aPad->GetShape() == PAD_CIRCLE )
        y = x;

    aPad->SetSize( wxSize( x, y ) );

    // Read pad length die
    aPad->SetDieLength( ReturnValueFromTextCtrl( *m_LengthDieCtrl, internalUnits ) );

    // Read pad shape delta size:
    // m_DeltaSize.x or m_DeltaSize.y must be NULL. for a trapezoid.
    wxSize delta;

    if( m_trapDeltaDirChoice->GetSelection() == 0 )
        delta.x = ReturnValueFromTextCtrl( *m_ShapeDelta_Ctrl, internalUnits );
    else
        delta.y = ReturnValueFromTextCtrl( *m_ShapeDelta_Ctrl, internalUnits );

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
    x = ReturnValueFromTextCtrl( *m_ShapeOffset_X_Ctrl, internalUnits );
    y = ReturnValueFromTextCtrl( *m_ShapeOffset_Y_Ctrl, internalUnits );
    aPad->SetOffset( wxPoint( x, y ) );

    double orient_value = 0;
    msg = m_PadOrientCtrl->GetValue();
    msg.ToDouble( &orient_value );

    aPad->SetOrientation( orient_value );

    msg = m_PadNumCtrl->GetValue().Left( 4 );
    aPad->SetPadName( msg );
    aPad->SetNetname( m_PadNetNameCtrl->GetValue() );

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
        // PAD_CONN has just a default non technical layers that differs frm SMD
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
        aPad->SetNetname( wxEmptyString );
        break;

    default:
        DisplayError( NULL, wxT( "Error: unknown pad type" ) );
        break;
    }

    padLayerMask = 0;

    switch( m_rbCopperLayersSel->GetSelection() )
    {
    case 0:
        padLayerMask |= LAYER_FRONT;
        break;

    case 1:
        padLayerMask |= LAYER_BACK;
        break;

    case 2:
        padLayerMask |= ALL_CU_LAYERS;
        break;

    case 3:     // No copper layers
        break;
    }

    if( m_PadLayerAdhCmp->GetValue() )
        padLayerMask |= ADHESIVE_LAYER_FRONT;
    if( m_PadLayerAdhCu->GetValue() )
        padLayerMask |= ADHESIVE_LAYER_BACK;
    if( m_PadLayerPateCmp->GetValue() )
        padLayerMask |= SOLDERPASTE_LAYER_FRONT;
    if( m_PadLayerPateCu->GetValue() )
        padLayerMask |= SOLDERPASTE_LAYER_BACK;
    if( m_PadLayerSilkCmp->GetValue() )
        padLayerMask |= SILKSCREEN_LAYER_FRONT;
    if( m_PadLayerSilkCu->GetValue() )
        padLayerMask |= SILKSCREEN_LAYER_BACK;
    if( m_PadLayerMaskCmp->GetValue() )
        padLayerMask |= SOLDERMASK_LAYER_FRONT;
    if( m_PadLayerMaskCu->GetValue() )
        padLayerMask |= SOLDERMASK_LAYER_BACK;
    if( m_PadLayerECO1->GetValue() )
        padLayerMask |= ECO1_LAYER;
    if( m_PadLayerECO2->GetValue() )
        padLayerMask |= ECO2_LAYER;
    if( m_PadLayerDraft->GetValue() )
        padLayerMask |= DRAW_LAYER;

    aPad->SetLayerMask( padLayerMask );

    return error;
}


void DIALOG_PAD_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( m_canUpdate )
    {
        transferDataToPad( m_dummyPad );
        m_panelShowPad->Refresh();
    }
}


void DIALOG_PAD_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}
