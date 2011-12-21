/*******************************************************************/
/* dialog_pad_properties.cpp: Pad editing functions and dialog box */
/* see also dialog_pad_properties.xxx (built with wxFormBuilder)   */
/*******************************************************************/

#include "fctsys.h"
#include "common.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "trigo.h"
#include "macros.h"
#include "wxBasePcbFrame.h"
#include "pcbcommon.h"

#include <wx/dcbuffer.h>

#include "class_board.h"
#include "class_module.h"

#include "dialog_pad_properties_base.h"


#define NBSHAPES 4
int CodeShape[NBSHAPES] = // list of pad shapes.
{
    PAD_CIRCLE, PAD_OVAL, PAD_RECT, PAD_TRAPEZOID
};


#define NBTYPES 4
int CodeType[NBTYPES] =
{
    PAD_STANDARD, PAD_SMD, PAD_CONN, PAD_HOLE_NOT_PLATED
};

// Default mask layers setup for pads according to the pad type
static long Std_Pad_Layers[NBTYPES] =
{
    // PAD_STANDARD:
    PAD_STANDARD_DEFAULT_LAYERS,

    // PAD_CONN:
    PAD_CONN_DEFAULT_LAYERS,

    // PAD_SMD:
    PAD_SMD_DEFAULT_LAYERS,

    //PAD_HOLE_NOT_PLATED:
    PAD_HOLE_NOT_PLATED_DEFAULT_LAYERS
};


extern int ChangeSideMaskLayer( int masque );


/**
 * class DIALOG_PAD_PROPERTIES, derived from DIALOG_PAD_PROPERTIES_BASE,
 * created by wxFormBuilder
 */
class DIALOG_PAD_PROPERTIES : public DIALOG_PAD_PROPERTIES_BASE
{
private:
    PCB_BASE_FRAME* m_Parent;
    D_PAD* m_CurrentPad;            // Pad currently edited
    D_PAD* m_dummyPad;              // a working copy used to show changes
    BOARD* m_Board;
    bool   m_isFlipped;             /* true if the parent footprint (therefore pads) is flipped (mirrored)
                                     *  in this case, some Y coordinates values must be negated
                                     */
    bool   m_canUpdate;

public:
    DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* parent, D_PAD* Pad );
    ~DIALOG_PAD_PROPERTIES()
    {
        delete m_dummyPad;
    }


private:
    void initValues();
    void OnPadShapeSelection( wxCommandEvent& event );
    void OnDrillShapeSelected( wxCommandEvent& event );
    void PadOrientEvent( wxCommandEvent& event );
    void PadTypeSelected( wxCommandEvent& event );
    void PadPropertiesAccept( wxCommandEvent& event );
    void SetPadLayersList( long layer_mask );
    void OnSetLayers( wxCommandEvent& event );
    void OnCancelButtonClick( wxCommandEvent& event );
    void OnPaintShowPanel( wxPaintEvent& event );
    bool TransfertDataToPad( D_PAD* aPad, bool aPromptOnError = false );
    void OnValuesChanged( wxCommandEvent& event );
};


void DIALOG_PAD_PROPERTIES::OnPaintShowPanel( wxPaintEvent& event )
{
    wxPaintDC    dc( m_panelShowPad );
    PAD_DRAWINFO drawInfo;

    int          color = 0;

    if( m_dummyPad->m_layerMask & LAYER_FRONT )
    {
        color = m_Board->GetVisibleElementColor( PAD_FR_VISIBLE );
    }

    if( m_dummyPad->m_layerMask & LAYER_BACK )
    {
        color |= m_Board->GetVisibleElementColor( PAD_BK_VISIBLE );
    }

    if( color == 0 )
        color = LIGHTGRAY;

    drawInfo.m_Color     = color;
    drawInfo.m_HoleColor = DARKGRAY;
    drawInfo.m_Offset    = m_dummyPad->m_Pos;
    drawInfo.m_Display_padnum  = true;
    drawInfo.m_Display_netname = true;
    if( m_dummyPad->m_Attribut == PAD_HOLE_NOT_PLATED )
        drawInfo.m_ShowNotPlatedHole = true;

    // Shows the local pad clearance
    drawInfo.m_PadClearance = m_dummyPad->m_LocalClearance;

    wxSize dc_size = dc.GetSize();
    dc.SetDeviceOrigin( dc_size.x / 2, dc_size.y / 2 );

    // Calculate a suitable scale to fit the available draw area
    int dim = m_dummyPad->m_Size.x + ABS( m_dummyPad->m_DeltaSize.y);
    if( m_dummyPad->m_LocalClearance > 0 )
        dim += m_dummyPad->m_LocalClearance * 2;
    double scale    = (double) dc_size.x / dim;
    dim = m_dummyPad->m_Size.y + ABS( m_dummyPad->m_DeltaSize.x);
    if( m_dummyPad->m_LocalClearance > 0 )
        dim += m_dummyPad->m_LocalClearance * 2;
    double altscale = (double) dc_size.y / dim;
    scale = MIN( scale, altscale );

    // Give a margin
    scale *= 0.7;
    dc.SetUserScale( scale, scale );

    GRResetPenAndBrush( &dc );
    m_dummyPad->DrawShape( NULL, &dc, drawInfo );

    event.Skip();
}


/*******************************************************************************************/
DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( PCB_BASE_FRAME* parent, D_PAD* Pad ) :
    DIALOG_PAD_PROPERTIES_BASE( parent )
/*******************************************************************************************/
{
    m_canUpdate  = false;
    m_Parent     = parent;
    m_CurrentPad = Pad;
    m_Board    = m_Parent->GetBoard();
    m_dummyPad = new D_PAD( (MODULE*) NULL );

    if( m_CurrentPad )
        m_dummyPad->Copy( m_CurrentPad );
    else
        m_dummyPad->Copy( &g_Pad_Master );

    initValues();

    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Center();
    m_canUpdate = true;
}


void PCB_BASE_FRAME::InstallPadOptionsFrame( D_PAD* Pad )
{
    DIALOG_PAD_PROPERTIES dlg( this, Pad );

    dlg.ShowModal();
}


/***************************************/
void DIALOG_PAD_PROPERTIES::initValues()
/***************************************/
{
    SetFocus();     // Required under wxGTK if we want to demiss the dialog with the ESC key

    int internalUnits = m_Parent->GetInternalUnits();
    wxString msg;
    m_isFlipped = false;
    if( m_CurrentPad )
    {
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        if( Module->GetLayer() ==  LAYER_N_BACK )
        {
            m_isFlipped = true;
            m_staticModuleSideValue->SetLabel( _( "Back side (footprint is mirrored)" ) );
        }
        msg.Printf( wxT( "%.1f" ), (double) Module->m_Orient / 10 );
        m_staticModuleRotValue->SetLabel( msg );
    }

    if( m_isFlipped )
    {
        NEGATE( m_dummyPad->m_Offset.y );
        NEGATE( m_dummyPad->m_DeltaSize.y );
        /* flip pads layers*/
        m_dummyPad->m_layerMask = ChangeSideMaskLayer( m_dummyPad->m_layerMask );
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
    m_NetClearanceUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    // Display current pad masks clearances units
    m_NetClearanceUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UserUnit ) );

    // Display current pad parameters units:
    PutValueInLocalUnits( *m_PadPosition_X_Ctrl, m_dummyPad->m_Pos.x, internalUnits );
    PutValueInLocalUnits( *m_PadPosition_Y_Ctrl, m_dummyPad->m_Pos.y, internalUnits );

    PutValueInLocalUnits( *m_PadDrill_X_Ctrl, m_dummyPad->m_Drill.x, internalUnits );
    PutValueInLocalUnits( *m_PadDrill_Y_Ctrl, m_dummyPad->m_Drill.y, internalUnits );

    PutValueInLocalUnits( *m_ShapeSize_X_Ctrl, m_dummyPad->m_Size.x, internalUnits );
    PutValueInLocalUnits( *m_ShapeSize_Y_Ctrl, m_dummyPad->m_Size.y, internalUnits );

    PutValueInLocalUnits( *m_ShapeOffset_X_Ctrl, m_dummyPad->m_Offset.x, internalUnits );
    PutValueInLocalUnits( *m_ShapeOffset_Y_Ctrl, m_dummyPad->m_Offset.y, internalUnits );

    if( m_dummyPad->m_DeltaSize.x )
    {
        PutValueInLocalUnits( *m_ShapeDelta_Ctrl, m_dummyPad->m_DeltaSize.x, internalUnits );
        m_radioBtnDeltaXdir->SetValue(true);
    }
    else
    {
        PutValueInLocalUnits( *m_ShapeDelta_Ctrl, m_dummyPad->m_DeltaSize.y, internalUnits );
        m_radioBtnDeltaYdir->SetValue(true);
    }

    PutValueInLocalUnits( *m_LengthDieCtrl, m_dummyPad->m_LengthDie, internalUnits );

    PutValueInLocalUnits( *m_NetClearanceValueCtrl, m_dummyPad->m_LocalClearance, internalUnits );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl,
                          m_dummyPad->m_LocalSolderMaskMargin,
                          internalUnits );

    // These 2 parameters are usually < 0, so prepare entering a negative value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl,
                          m_dummyPad->m_LocalSolderPasteMargin,
                          internalUnits );
    if( m_dummyPad->m_LocalSolderPasteMargin == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT( "-" ) + m_SolderPasteMarginCtrl->GetValue() );
    msg.Printf( wxT( "%.1f" ), m_dummyPad->m_LocalSolderPasteMarginRatio * 100.0 );

    if( m_dummyPad->m_LocalSolderPasteMarginRatio == 0.0
        && msg[0] == '0' ) // Sometimes Printf add a sign if the value is small
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    if( m_CurrentPad )
    {
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        m_dummyPad->m_Orient = m_CurrentPad->m_Orient - Module->m_Orient;
        if( m_isFlipped )
            NEGATE( m_dummyPad->m_Orient );
    }

    // adjust rotation agngle to -1800 to 1800 in internal units (0.1 deg)
    NORMALIZE_ANGLE_180( m_dummyPad->m_Orient );

    // Set layers used by this pad: :
    SetPadLayersList( m_dummyPad->m_layerMask );

    msg.Clear();
    msg << m_dummyPad->m_Orient;
    m_PadOrientCtrl->SetValue( msg );

    // Pad Orient
    switch( m_dummyPad->m_Orient )
    {
    case 0:
        m_PadOrient->SetSelection( 0 );
        break;

    case 900:
        m_PadOrient->SetSelection( 1 );
        break;

    case - 900:
        m_PadOrient->SetSelection( 2 );
        break;

    case 1800:
    case - 1800:
        m_PadOrient->SetSelection( 3 );
        break;

    default:
        m_PadOrient->SetSelection( 4 );
        break;
    }

    switch( m_dummyPad->m_PadShape )
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

    msg.Printf( wxT( "%d" ), m_dummyPad->m_Orient );
    m_PadOrientCtrl->SetValue( msg );

    // Type of pad selection
    m_PadType->SetSelection( 0 );
    for( int ii = 0; ii < NBTYPES; ii++ )
    {
        if( CodeType[ii] == m_dummyPad->m_Attribut )
        {
            m_PadType->SetSelection( ii );
            break;
        }
    }

    // Enable/disable Pad name,and pad length die
    // (disable for NPTH pads (mechanical pads)
    bool enable = m_dummyPad->m_Attribut != PAD_HOLE_NOT_PLATED;
    m_PadNumCtrl->Enable( enable );
    m_PadNetNameCtrl->Enable( enable );
    m_LengthDieCtrl->Enable( enable );

    if( m_dummyPad->m_DrillShape != PAD_OVAL )
        m_DrillShapeCtrl->SetSelection( 0 );
    else
        m_DrillShapeCtrl->SetSelection( 1 );

    // Setup layers names from board

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

    /* All init are done,
     * Update some dialog widgets state (Enable/disable options):
     */
    wxCommandEvent cmd_event;
    OnPadShapeSelection( cmd_event );
    OnDrillShapeSelected( cmd_event );
}


/*********************************************************************/
void DIALOG_PAD_PROPERTIES::OnPadShapeSelection( wxCommandEvent& event )
/*********************************************************************/
{
    switch( m_PadShape->GetSelection() )
    {
    case 0:     //CIRCLE:
        m_ShapeDelta_Ctrl->Enable( false );
        m_radioBtnDeltaXdir->Enable( false );
        m_radioBtnDeltaYdir->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( false );
        break;

    case 1:     //OVALE:
        m_ShapeDelta_Ctrl->Enable( false );
        m_radioBtnDeltaXdir->Enable( false );
        m_radioBtnDeltaYdir->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( true );
        break;

    case 2:     // PAD_RECT:
        m_ShapeDelta_Ctrl->Enable( false );
        m_radioBtnDeltaXdir->Enable( false );
        m_radioBtnDeltaYdir->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( true );
        break;

    case 3:     //TRAPEZE:
        m_ShapeDelta_Ctrl->Enable( true );
        m_radioBtnDeltaXdir->Enable( true );
        m_radioBtnDeltaYdir->Enable( true );
        m_ShapeSize_Y_Ctrl->Enable( true );
        break;
    }

    TransfertDataToPad( m_dummyPad );
    m_panelShowPad->Refresh();
}


/**********************************************************************/
void DIALOG_PAD_PROPERTIES::OnDrillShapeSelected( wxCommandEvent& event )
/**********************************************************************/
{
    if( (m_PadType->GetSelection() == 1) || (m_PadType->GetSelection() == 2) )
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

    TransfertDataToPad( m_dummyPad );
    m_panelShowPad->Refresh();
}


/*******************************************************************/
void DIALOG_PAD_PROPERTIES::PadOrientEvent( wxCommandEvent& event )
/********************************************************************/
{
    switch( m_PadOrient->GetSelection() )
    {
    case 0:
        m_dummyPad->m_Orient = 0;
        break;

    case 1:
        m_dummyPad->m_Orient = 900;
        break;

    case 2:
        m_dummyPad->m_Orient = -900;
        break;

    case 3:
        m_dummyPad->m_Orient = 1800;
        break;

    default:
        break;
    }

    wxString msg;
    msg.Printf( wxT( "%d" ), m_dummyPad->m_Orient );
    m_PadOrientCtrl->SetValue( msg );

    TransfertDataToPad( m_dummyPad );
    m_panelShowPad->Refresh();
}


/*****************************************************************/
void DIALOG_PAD_PROPERTIES::PadTypeSelected( wxCommandEvent& event )
/*****************************************************************/

/* Adjust the better mask layer according to the selected pad type
 */
{
    long layer_mask;
    int  ii;

    ii = m_PadType->GetSelection();
    if( (ii < 0) || ( ii >= NBTYPES) )
        ii = 0;

    layer_mask = Std_Pad_Layers[ii];
    SetPadLayersList( layer_mask );

    // Enable/disable drill dialog items:
    event.SetId( m_DrillShapeCtrl->GetSelection() );
    OnDrillShapeSelected( event );

    // Enable/disable Pad name,and pad length die
    // (disable for NPTH pads (mechanical pads)
    bool enable = ii != 3;
    m_PadNumCtrl->Enable( enable );
    m_PadNetNameCtrl->Enable( enable );
    m_LengthDieCtrl->Enable( enable );
}


/****************************************************************/
void DIALOG_PAD_PROPERTIES::SetPadLayersList( long layer_mask )
/****************************************************************/

/** SetPadLayersList
 * Update the CheckBoxes state in pad layers list,
 * @param layer_mask = pad layer mask (ORed layers bit mask)
 */
{
    if( ( layer_mask & ALL_CU_LAYERS )  == LAYER_FRONT )
        m_rbCopperLayersSel->SetSelection(0);
    else if( ( layer_mask & ALL_CU_LAYERS ) == LAYER_BACK)
        m_rbCopperLayersSel->SetSelection(1);
    else if( ( layer_mask & ALL_CU_LAYERS )  != 0 )
        m_rbCopperLayersSel->SetSelection(2);
    else
        m_rbCopperLayersSel->SetSelection(3);

    m_PadLayerAdhCmp->SetValue( ( layer_mask & ADHESIVE_LAYER_FRONT ) );
    m_PadLayerAdhCu->SetValue( ( layer_mask & ADHESIVE_LAYER_BACK ) );

    m_PadLayerPateCmp->SetValue( ( layer_mask & SOLDERPASTE_LAYER_FRONT ) );
    m_PadLayerPateCu->SetValue( ( layer_mask & SOLDERPASTE_LAYER_BACK ) );

    m_PadLayerSilkCmp->SetValue( ( layer_mask & SILKSCREEN_LAYER_FRONT ) );
    m_PadLayerSilkCu->SetValue( ( layer_mask & SILKSCREEN_LAYER_BACK ) );

    m_PadLayerMaskCmp->SetValue( ( layer_mask & SOLDERMASK_LAYER_FRONT ) );
    m_PadLayerMaskCu->SetValue( ( layer_mask & SOLDERMASK_LAYER_BACK ) );

    m_PadLayerECO1->SetValue( ( layer_mask & ECO1_LAYER ) );
    m_PadLayerECO2->SetValue( ( layer_mask & ECO2_LAYER ) );

    m_PadLayerDraft->SetValue( ( layer_mask & DRAW_LAYER ) );
}


// Called when select/deselect a layer.
void DIALOG_PAD_PROPERTIES::OnSetLayers( wxCommandEvent& event )
{
    TransfertDataToPad( m_dummyPad );
    m_panelShowPad->Refresh();
}


/*************************************************************************/
void DIALOG_PAD_PROPERTIES::PadPropertiesAccept( wxCommandEvent& event )
/*************************************************************************/

/* Updates the different parameters for the component being edited.
 */
{
    bool rastnestIsChanged = false;
    int  isign = m_isFlipped ? -1 : 1;

    bool success = TransfertDataToPad( m_dummyPad, true );
    if( !success )  // An error on parameters has occured
        return;

    TransfertDataToPad( &g_Pad_Master, false );

    if( m_CurrentPad )   // Set current Pad parameters
    {
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        m_Parent->SaveCopyInUndoList( Module, UR_CHANGED );
        Module->m_LastEdit_Time = time( NULL );

        // redraw the area where the pad was, without pad (delete pad on screen)
        m_CurrentPad->SetFlags( DO_NOT_DRAW );
        m_Parent->DrawPanel->RefreshDrawingRect( m_CurrentPad->GetBoundingBox() );
        m_CurrentPad->ClearFlags( DO_NOT_DRAW );

        // Update values
        m_CurrentPad->m_PadShape = g_Pad_Master.m_PadShape;
        m_CurrentPad->m_Attribut = g_Pad_Master.m_Attribut;

        if( m_CurrentPad->m_Pos != g_Pad_Master.m_Pos )
        {
            m_CurrentPad->m_Pos = g_Pad_Master.m_Pos;
            rastnestIsChanged   = true;
        }

        /* compute the pos 0 value, i.e. pad position for module orient = 0 i.e.
         *  refer to module origin (module position) */
        m_CurrentPad->m_Pos0   = m_CurrentPad->m_Pos;
        m_CurrentPad->m_Pos0  -= Module->m_Pos;
        m_CurrentPad->m_Orient = (g_Pad_Master.m_Orient * isign) + Module->m_Orient;
        RotatePoint( &m_CurrentPad->m_Pos0.x, &m_CurrentPad->m_Pos0.y, -Module->m_Orient );

        m_CurrentPad->m_Size = g_Pad_Master.m_Size;
        m_CurrentPad->m_DeltaSize    = g_Pad_Master.m_DeltaSize;
        m_CurrentPad->m_DeltaSize.y *= isign;
        m_CurrentPad->m_Drill = g_Pad_Master.m_Drill;
        m_CurrentPad->m_DrillShape = g_Pad_Master.m_DrillShape;
        m_CurrentPad->m_Offset     = g_Pad_Master.m_Offset;
        m_CurrentPad->m_Offset.y  *= isign;

        m_CurrentPad->m_LengthDie = g_Pad_Master.m_LengthDie;

        if( m_CurrentPad->m_layerMask != g_Pad_Master.m_layerMask )
        {
            rastnestIsChanged = true;
            m_CurrentPad->m_layerMask = g_Pad_Master.m_layerMask;
        }
        if( m_isFlipped )
            m_CurrentPad->m_layerMask = ChangeSideMaskLayer( m_CurrentPad->m_layerMask );

        m_CurrentPad->SetPadName( g_Pad_Master.GetPadName() );

        if( m_CurrentPad->GetNetname() != g_Pad_Master.GetNetname() )
        {
            if( g_Pad_Master.GetNetname().IsEmpty() )
            {
                rastnestIsChanged = true;
                m_CurrentPad->SetNet( 0 );
                m_CurrentPad->SetNetname( wxEmptyString );
            }
            else
            {
                const NETINFO_ITEM* net = m_Parent->GetBoard()->FindNet( g_Pad_Master.GetNetname() );
                if( net )
                {
                    rastnestIsChanged = true;
                    m_CurrentPad->SetNetname( g_Pad_Master.GetNetname() );
                    m_CurrentPad->SetNet( net->GetNet() );
                }
                else
                    DisplayError( NULL, _( "Unknown netname, netname not changed" ) );
            }
        }

        m_CurrentPad->m_LocalClearance = g_Pad_Master.m_LocalClearance;
        m_CurrentPad->m_LocalSolderMaskMargin  = g_Pad_Master.m_LocalSolderMaskMargin;
        m_CurrentPad->m_LocalSolderPasteMargin = g_Pad_Master.m_LocalSolderPasteMargin;
        m_CurrentPad->m_LocalSolderPasteMarginRatio = g_Pad_Master.m_LocalSolderPasteMarginRatio;

        m_CurrentPad->ComputeShapeMaxRadius();

        Module->CalculateBoundingBox();
        m_CurrentPad->DisplayInfo( m_Parent );

        // redraw the area where the pad was
        m_Parent->DrawPanel->RefreshDrawingRect( m_CurrentPad->GetBoundingBox() );
        m_Parent->OnModify();
    }

    EndModal( wxID_OK );

    if( rastnestIsChanged )  // The net ratsnest must be recalculated
        m_Parent->GetBoard()->m_Status_Pcb = 0;
}

// Copy values from dialog to aPad parameters.
// If aPromptOnError is true, and an incorrect value is detected,
// user will be prompted for an error
bool DIALOG_PAD_PROPERTIES::TransfertDataToPad( D_PAD* aPad, bool aPromptOnError )
{
    long     PadLayerMask;
    int      internalUnits = m_Parent->GetInternalUnits();
    wxString msg;

    aPad->m_Attribut = CodeType[m_PadType->GetSelection()];
    aPad->m_PadShape = CodeShape[m_PadShape->GetSelection()];

    // Read pad clearances values:
    aPad->m_LocalClearance = ReturnValueFromTextCtrl( *m_NetClearanceValueCtrl,
                                                      internalUnits );
    aPad->m_LocalSolderMaskMargin = ReturnValueFromTextCtrl( *m_SolderMaskMarginCtrl,
                                                             internalUnits );
    aPad->m_LocalSolderPasteMargin = ReturnValueFromTextCtrl( *m_SolderPasteMarginCtrl,
                                                              internalUnits );
    double dtmp = 0.0;
    msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;
    if( dtmp > +100 )
        dtmp = +100;
    aPad->m_LocalSolderPasteMarginRatio = dtmp / 100;

    // Read pad position:
    aPad->m_Pos.x = ReturnValueFromTextCtrl( *m_PadPosition_X_Ctrl, internalUnits );
    aPad->m_Pos.y = ReturnValueFromTextCtrl( *m_PadPosition_Y_Ctrl, internalUnits );
    aPad->m_Pos0  = aPad->m_Pos;

    // Read pad drill:
    aPad->m_Drill.x = ReturnValueFromTextCtrl( *m_PadDrill_X_Ctrl, internalUnits );
    aPad->m_Drill.y = ReturnValueFromTextCtrl( *m_PadDrill_Y_Ctrl, internalUnits );
    if( m_DrillShapeCtrl->GetSelection() == 0 )
    {
        aPad->m_DrillShape = PAD_CIRCLE;
        aPad->m_Drill.y    = aPad->m_Drill.x;
    }
    else
        aPad->m_DrillShape = PAD_OVAL;

    // Read pad shape size:
    aPad->m_Size.x = ReturnValueFromTextCtrl( *m_ShapeSize_X_Ctrl, internalUnits );
    aPad->m_Size.y = ReturnValueFromTextCtrl( *m_ShapeSize_Y_Ctrl, internalUnits );
    if( aPad->m_PadShape == PAD_CIRCLE )
        aPad->m_Size.y = aPad->m_Size.x;

    // Read pad shape delta size:
    // m_DeltaSize.x or m_DeltaSize.y must be NULL. for a trapezoid.
    wxSize delta;
    if( m_radioBtnDeltaXdir->GetValue() )
        delta.x = ReturnValueFromTextCtrl( *m_ShapeDelta_Ctrl, internalUnits );
    else
        delta.y = ReturnValueFromTextCtrl( *m_ShapeDelta_Ctrl, internalUnits );
    aPad->m_DeltaSize = delta;

    // Read pad lenght die
    aPad->m_LengthDie = ReturnValueFromTextCtrl( *m_LengthDieCtrl, internalUnits );

    // Test bad values (be sure delta values are not to large)
    // remember DeltaSize.x is the Y size variation
    bool   error    = false;
    if( (aPad->m_DeltaSize.x < 0) && (aPad->m_DeltaSize.x <= -aPad->m_Size.y) )
    {
        aPad->m_DeltaSize.x = -aPad->m_Size.y + 2;
        error = true;
    }
    if( (aPad->m_DeltaSize.x > 0) && (aPad->m_DeltaSize.x >= aPad->m_Size.y) )
    {
        aPad->m_DeltaSize.x = aPad->m_Size.y - 2;
        error = true;
    }
    if( (aPad->m_DeltaSize.y < 0) && (aPad->m_DeltaSize.y <= -aPad->m_Size.x) )
    {
        aPad->m_DeltaSize.y = -aPad->m_Size.x + 2;
        error = true;
    }
    if( (aPad->m_DeltaSize.y > 0) && (aPad->m_DeltaSize.y >= aPad->m_Size.x) )
    {
        aPad->m_DeltaSize.y = aPad->m_Size.x - 2;
        error = true;
    }

    // Read pad shape offset:
    aPad->m_Offset.x = ReturnValueFromTextCtrl( *m_ShapeOffset_X_Ctrl, internalUnits );
    aPad->m_Offset.y = ReturnValueFromTextCtrl( *m_ShapeOffset_Y_Ctrl, internalUnits );

    long orient_value = 0;
    msg = m_PadOrientCtrl->GetValue();
    msg.ToLong( &orient_value );
    aPad->m_Orient = orient_value;

    msg = m_PadNumCtrl->GetValue().Left( 4 );
    aPad->SetPadName( msg );
    aPad->SetNetname( m_PadNetNameCtrl->GetValue() );

    // Clear some values, according to the pad type and shape
    switch( aPad->m_PadShape )
    {
    case PAD_CIRCLE:
        aPad->m_Offset    = wxSize( 0, 0 );
        aPad->m_DeltaSize = wxSize( 0, 0 );
        aPad->m_Size.y    = aPad->m_Size.x;
        break;

    case PAD_RECT:
        aPad->m_DeltaSize = wxSize( 0, 0 );
        break;

    case PAD_OVAL:
        aPad->m_DeltaSize = wxSize( 0, 0 );
        break;

    case PAD_TRAPEZOID:
        break;
    }

    switch( aPad->m_Attribut )
    {
    case PAD_STANDARD:
        break;

    case PAD_CONN:
    case PAD_SMD:
        aPad->m_Offset = wxSize( 0, 0 );
        aPad->m_Drill  = wxSize( 0, 0 );
        break;

    case PAD_HOLE_NOT_PLATED:
        // Mechanical purpose only:
        // no offset, no net name, no pad name allowed
        aPad->m_Offset = wxSize( 0, 0 );
        aPad->SetPadName( wxEmptyString );
        aPad->SetNetname( wxEmptyString );
        break;

    default:
        DisplayError( NULL, wxT( "Error: unknown pad type" ) );
        break;
    }

    PadLayerMask = 0;
    switch( m_rbCopperLayersSel->GetSelection() )
    {
        case 0:
            PadLayerMask |= LAYER_FRONT;
            break;

        case 1:
            PadLayerMask |= LAYER_BACK;
            break;

        case 2:
            PadLayerMask |= ALL_CU_LAYERS;
            break;

        case 3:     // No copper layers
            break;
    }

    if( m_PadLayerAdhCmp->GetValue() )
        PadLayerMask |= ADHESIVE_LAYER_FRONT;
    if( m_PadLayerAdhCu->GetValue() )
        PadLayerMask |= ADHESIVE_LAYER_BACK;
    if( m_PadLayerPateCmp->GetValue() )
        PadLayerMask |= SOLDERPASTE_LAYER_FRONT;
    if( m_PadLayerPateCu->GetValue() )
        PadLayerMask |= SOLDERPASTE_LAYER_BACK;
    if( m_PadLayerSilkCmp->GetValue() )
        PadLayerMask |= SILKSCREEN_LAYER_FRONT;
    if( m_PadLayerSilkCu->GetValue() )
        PadLayerMask |= SILKSCREEN_LAYER_BACK;
    if( m_PadLayerMaskCmp->GetValue() )
        PadLayerMask |= SOLDERMASK_LAYER_FRONT;
    if( m_PadLayerMaskCu->GetValue() )
        PadLayerMask |= SOLDERMASK_LAYER_BACK;
    if( m_PadLayerECO1->GetValue() )
        PadLayerMask |= ECO1_LAYER;
    if( m_PadLayerECO2->GetValue() )
        PadLayerMask |= ECO2_LAYER;
    if( m_PadLayerDraft->GetValue() )
        PadLayerMask |= DRAW_LAYER;

    aPad->m_layerMask = PadLayerMask;

    /* Test for incorrect values */
    if( aPromptOnError )
    {
        if( (aPad->m_Size.x < aPad->m_Drill.x)
           || (aPad->m_Size.y < aPad->m_Drill.y) )
        {
            DisplayError( NULL, _( "Incorrect value for pad drill: pad drill bigger than pad size" ) );
            return false;
        }

        int padlayers_mask = PadLayerMask & (LAYER_BACK | LAYER_FRONT);
        if( padlayers_mask == 0 )
        {
            if( aPad->m_Drill.x || aPad->m_Drill.y )
            {
                msg = _( "Error: pad is not on a copper layer and has a hole" );
                if( aPad->m_Attribut == PAD_HOLE_NOT_PLATED )
                {
                    msg += wxT("\n");
                    msg += _( "For NPTH pad, set pad drill value to pad size value,\n\
if you do not want this pad plotted in gerber files");
                }
                DisplayError( NULL, msg );
                return false;
            }
        }

        if( ( aPad->m_Size.x / 2 <= ABS( aPad->m_Offset.x ) )
           || ( aPad->m_Size.y / 2 <= ABS( aPad->m_Offset.y ) ) )
        {
            DisplayError( NULL, _( "Incorrect value for pad offset" ) );
            return false;
        }

        if( error )
        {
            DisplayError( NULL, _( "Too large value for pad delta size" ) );
            return false;
        }
    }

    return true;
}


// Called when a dimension has change.
// Update the pad dimensions shown in the panel.
void DIALOG_PAD_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    if( m_canUpdate )
    {
        TransfertDataToPad( m_dummyPad );
        m_panelShowPad->Refresh();
    }
}


/*********************************************************************/
void DIALOG_PAD_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
/*********************************************************************/
{
    EndModal( wxID_CANCEL );
}
