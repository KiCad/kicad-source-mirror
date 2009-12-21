/*******************************************************************/
/* dialog_pad_properties.cpp: Pad editing functions and dialog box */
/* see also dialog_pad_properties.xxx (built with wxFormBuilder)   */
/*******************************************************************/

#include "fctsys.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "trigo.h"

/* Local variables */
static wxString Current_PadNetName;


#define NBSHAPES 4
int CodeShape[NBSHAPES] = /* Pad shapes.  */
{
    PAD_CIRCLE, PAD_OVAL, PAD_RECT, PAD_TRAPEZOID
};


#define NBTYPES 4
int CodeType[NBTYPES] =
{
    PAD_STANDARD, PAD_SMD, PAD_CONN, PAD_HOLE_NOT_PLATED
};

// Default mask layers for pads according to the pad type
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


#include "dialog_pad_properties_base.h"


/********************************************************************/
/* class DIALOG_PAD_PROPERTIES, derived from DIALOG_PAD_PROPERTIES_BASE, */
/*                            created by wxFormBuilder              */
/********************************************************************/
class DIALOG_PAD_PROPERTIES : public DIALOG_PAD_PROPERTIES_BASE
{
public:
    WinEDA_BasePcbFrame* m_Parent;
    D_PAD*               m_CurrentPad;

public:
    DIALOG_PAD_PROPERTIES( WinEDA_BasePcbFrame* parent, D_PAD* Pad );
    void InitDialog( );
    void OnPadShapeSelection( wxCommandEvent& event );
    void OnDrillShapeSelected( wxCommandEvent& event );
    void PadOrientEvent( wxCommandEvent& event );
    void PadTypeSelected( wxCommandEvent& event );
    void PadPropertiesAccept( wxCommandEvent& event );
    void SetPadLayersList( long layer_mask );
	void OnCancelButtonClick( wxCommandEvent& event );
};


/*******************************************************************************************/
DIALOG_PAD_PROPERTIES::DIALOG_PAD_PROPERTIES( WinEDA_BasePcbFrame* parent, D_PAD* Pad ) :
    DIALOG_PAD_PROPERTIES_BASE( parent )
/*******************************************************************************************/
{
    m_Parent = parent;
    m_CurrentPad = Pad;

    if( m_CurrentPad )
    {
        Current_PadNetName = m_CurrentPad->GetNetname();
        g_Current_PadName  = m_CurrentPad->ReturnStringPadName();
    }

    InitDialog( );
    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


/*************************************************************/
void WinEDA_BasePcbFrame::InstallPadOptionsFrame( D_PAD* Pad )
/*************************************************************/
{
    DIALOG_PAD_PROPERTIES dlg( this, Pad );
    dlg.ShowModal();
}


/**************************************************************/
void DIALOG_PAD_PROPERTIES::InitDialog( )
/**************************************************************/
{
    int            tmp;
    wxCommandEvent cmd_event;
    int internalUnits = m_Parent->m_InternalUnits;
    wxString msg;

    SetFocus();     // Required under wxGTK if we want to demiss the dialog with the ESC key

    m_PadNumCtrl->SetValue( g_Current_PadName );
    m_PadNetNameCtrl->SetValue( Current_PadNetName );
    D_PAD* pad = m_CurrentPad;
    if( pad == NULL )
        pad = &g_Pad_Master;

    // Display current unit name in dialog:
    m_PadPosX_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadPosY_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadDrill_X_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadDrill_Y_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadShapeSizeX_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadShapeSizeY_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadShapeOffsetX_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadShapeOffsetY_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadShapeDeltaX_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_PadShapeDeltaY_Unit->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_NetClearanceUnits->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    // Display current pad masks clearances units
    m_NetClearanceUnits->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_SolderMaskMarginUnits->SetLabel( GetUnitsLabel( g_UnitMetric ) );
    m_SolderPasteMarginUnits->SetLabel( GetUnitsLabel( g_UnitMetric ) );

    // Display current pad parameters units:
    PutValueInLocalUnits( *m_PadPosition_X_Ctrl, pad->m_Pos.x, internalUnits );
    PutValueInLocalUnits( *m_PadPosition_Y_Ctrl, pad->m_Pos.y, internalUnits );

    PutValueInLocalUnits( *m_PadDrill_X_Ctrl, pad->m_Drill.x, internalUnits );
    PutValueInLocalUnits( *m_PadDrill_Y_Ctrl, pad->m_Drill.y, internalUnits );

    PutValueInLocalUnits( *m_ShapeSize_X_Ctrl, pad->m_Size.x, internalUnits );
    PutValueInLocalUnits( *m_ShapeSize_Y_Ctrl, pad->m_Size.y, internalUnits );

    PutValueInLocalUnits( *m_ShapeOffset_X_Ctrl, pad->m_Offset.x, internalUnits );
    PutValueInLocalUnits( *m_ShapeOffset_Y_Ctrl, pad->m_Offset.y, internalUnits );

    PutValueInLocalUnits( *m_ShapeDelta_X_Ctrl, pad->m_DeltaSize.x, internalUnits );
    PutValueInLocalUnits( *m_ShapeDelta_Y_Ctrl, pad->m_DeltaSize.y, internalUnits );

    PutValueInLocalUnits( *m_NetClearanceValueCtrl, pad->m_LocalClearance, internalUnits );
    PutValueInLocalUnits( *m_SolderMaskMarginCtrl, pad->m_LocalSolderMaskMargin, internalUnits );
    // These 2 parameters are usually < 0, so prepare entering a negative value, if current is 0
    PutValueInLocalUnits( *m_SolderPasteMarginCtrl, pad->m_LocalSolderPasteMargin, internalUnits );
    if( pad->m_LocalSolderPasteMargin == 0 )
        m_SolderPasteMarginCtrl->SetValue( wxT("-") + m_SolderPasteMarginCtrl->GetValue() );
    if( pad->m_LocalSolderPasteMarginRatio == 0.0 )
        msg.Printf( wxT( "-%.1f" ), pad->m_LocalSolderPasteMarginRatio * 100.0 );
    else
        msg.Printf( wxT( "%.1f" ), pad->m_LocalSolderPasteMarginRatio * 100.0 );
    m_SolderPasteMarginRatioCtrl->SetValue( msg );

    if( m_CurrentPad )
    {
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        tmp = m_CurrentPad->m_Orient - Module->m_Orient;
    }
    else
        tmp = g_Pad_Master.m_Orient;
    msg.Clear();
    msg << tmp;
    m_PadOrientCtrl->SetValue( msg );


    // Pad Orient
    switch( tmp )
    {
    case 0:
        m_PadOrient->SetSelection( 0 );
        m_PadOrientCtrl->Enable( false );
        break;

    case - 2700:
    case 900:
        m_PadOrient->SetSelection( 1 );
        m_PadOrientCtrl->Enable( false );
        break;

    case - 900:
    case 2700:
        m_PadOrient->SetSelection( 2 );
        m_PadOrientCtrl->Enable( false );
        break;

    case 1800:
    case - 1800:
        m_PadOrient->SetSelection( 3 );
        m_PadOrientCtrl->Enable( false );
        break;

    default:
        m_PadOrient->SetSelection( 4 );
        break;
    }

    switch( pad->m_PadShape )
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

    cmd_event.SetId( m_PadShape->GetSelection() );
    OnPadShapeSelection( cmd_event );

    // Selection du type
    tmp = pad->m_Attribut;
    m_PadType->SetSelection( 0 );
    for( int ii = 0; ii < NBTYPES; ii++ )
    {
        if( CodeType[ii] == tmp )
        {
            m_PadType->SetSelection( ii ); break;
        }
    }

    if( pad->m_DrillShape != PAD_OVAL )
        m_DrillShapeCtrl->SetSelection( 0 );
    else
        m_DrillShapeCtrl->SetSelection( 1 );
    cmd_event.SetId( m_DrillShapeCtrl->GetSelection() );
    OnDrillShapeSelected( cmd_event );

    // Set layers used by this pad: :
    if( m_CurrentPad )
        SetPadLayersList( m_CurrentPad->m_Masque_Layer );
    else
    {
        cmd_event.SetId( m_PadType->GetSelection() );
        PadTypeSelected( cmd_event );
    }
}


/*********************************************************************/
void DIALOG_PAD_PROPERTIES::OnPadShapeSelection( wxCommandEvent& event )
/*********************************************************************/
{
    switch( m_PadShape->GetSelection() )
    {
    case 0:     //CIRCLE:
        m_ShapeDelta_X_Ctrl->Enable( false );
        m_ShapeDelta_Y_Ctrl->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( false );
        break;

    case 1:     //OVALE:
        m_ShapeDelta_X_Ctrl->Enable( false );
        m_ShapeDelta_Y_Ctrl->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( true );
        break;

    case 2:     // PAD_RECT:
        m_ShapeDelta_X_Ctrl->Enable( false );
        m_ShapeDelta_Y_Ctrl->Enable( false );
        m_ShapeSize_Y_Ctrl->Enable( true );
        break;

    case 3:     //TRAPEZE:
        m_ShapeDelta_X_Ctrl->Enable( true );
        m_ShapeDelta_Y_Ctrl->Enable( true );
        m_ShapeSize_Y_Ctrl->Enable( true );
        break;
    }
}


/**********************************************************************/
void DIALOG_PAD_PROPERTIES::OnDrillShapeSelected( wxCommandEvent& event )
/**********************************************************************/
{
    if ( (m_PadType->GetSelection() == 1) || (m_PadType->GetSelection() == 2) )
    {   // pad type = SMD or CONN: no hole allowed
        m_PadDrill_X_Ctrl->Enable( false );
        m_PadDrill_Y_Ctrl->Enable( false );
        return;
    }
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


/*******************************************************************/
void DIALOG_PAD_PROPERTIES::PadOrientEvent( wxCommandEvent& event )
/********************************************************************/
{
    switch( m_PadOrient->GetSelection() )
    {
    case 0:
        m_PadOrientCtrl->SetValue( wxT( "0" ) );
        m_PadOrientCtrl->Enable( false );
        break;

    case 1:
        m_PadOrientCtrl->SetValue( wxT( "900" ) );
        m_PadOrientCtrl->Enable( false );
        break;

    case 2:
        m_PadOrientCtrl->SetValue( wxT( "2700" ) );
        m_PadOrientCtrl->Enable( false );
        break;

    case 3:
        m_PadOrientCtrl->SetValue( wxT( "1800" ) );
        m_PadOrientCtrl->Enable( false );
        break;

    default:
        m_PadOrientCtrl->Enable( true );
        break;
    }
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
    event.SetId(m_DrillShapeCtrl->GetSelection());
    OnDrillShapeSelected( event );
}


/****************************************************************/
void DIALOG_PAD_PROPERTIES::SetPadLayersList( long layer_mask )
/****************************************************************/

/** SetPadLayersList
 * Update the CheckBoxes state in pad layers list,
 * @param layer_mask = pad layer mask (ORed layers bit mask)
 */
{
    m_PadLayerCu->SetValue( ( layer_mask & LAYER_BACK ) );
    m_PadLayerCmp->SetValue( ( layer_mask & LAYER_FRONT ) );

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


/*************************************************************************/
void DIALOG_PAD_PROPERTIES::PadPropertiesAccept( wxCommandEvent& event )
/*************************************************************************/

/* Updates the different parameters for the component being edited.
 */
{
    long PadLayerMask;
    bool error = false;
    bool RastnestIsChanged = false;
    int internalUnits = m_Parent->m_InternalUnits;
    wxString msg;

    g_Pad_Master.m_Attribut = CodeType[m_PadType->GetSelection()];
    g_Pad_Master.m_PadShape = CodeShape[m_PadShape->GetSelection()];

    // Read pad clearances values:
    g_Pad_Master.m_LocalClearance = ReturnValueFromTextCtrl( *m_NetClearanceValueCtrl, internalUnits );
    g_Pad_Master.m_LocalSolderMaskMargin = ReturnValueFromTextCtrl( *m_SolderMaskMarginCtrl, internalUnits );
    g_Pad_Master.m_LocalSolderPasteMargin = ReturnValueFromTextCtrl( *m_SolderPasteMarginCtrl, internalUnits );
    double   dtmp;
    msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );
    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;
    g_Pad_Master.m_LocalSolderPasteMarginRatio = dtmp / 100;

    // Read pad position:
    g_Pad_Master.m_Pos.x = ReturnValueFromTextCtrl( *m_PadPosition_X_Ctrl, internalUnits );
    g_Pad_Master.m_Pos.y = ReturnValueFromTextCtrl( *m_PadPosition_Y_Ctrl, internalUnits );
    g_Pad_Master.m_Pos0 = g_Pad_Master.m_Pos;

    // Read pad drill:
    g_Pad_Master.m_Drill.x = ReturnValueFromTextCtrl( *m_PadDrill_X_Ctrl, internalUnits );
    g_Pad_Master.m_Drill.y = ReturnValueFromTextCtrl( *m_PadDrill_Y_Ctrl, internalUnits );
    if( m_DrillShapeCtrl->GetSelection() == 0 )
    {
        g_Pad_Master.m_DrillShape = PAD_CIRCLE;
        g_Pad_Master.m_Drill.y    = g_Pad_Master.m_Drill.x;
    }
    else
        g_Pad_Master.m_DrillShape = PAD_OVAL;

    // Read pad shape size:
    g_Pad_Master.m_Size.x = ReturnValueFromTextCtrl( *m_ShapeSize_X_Ctrl, internalUnits );
    g_Pad_Master.m_Size.y = ReturnValueFromTextCtrl( *m_ShapeSize_Y_Ctrl, internalUnits );
    if( g_Pad_Master.m_PadShape == PAD_CIRCLE )
        g_Pad_Master.m_Size.y = g_Pad_Master.m_Size.x;

    // Read pad shape delta size:
    g_Pad_Master.m_DeltaSize.x = ReturnValueFromTextCtrl( *m_ShapeDelta_X_Ctrl, internalUnits );
    g_Pad_Master.m_DeltaSize.y = ReturnValueFromTextCtrl( *m_ShapeDelta_Y_Ctrl, internalUnits );

    // Read pad shape offset:
    g_Pad_Master.m_Offset.x = ReturnValueFromTextCtrl( *m_ShapeOffset_X_Ctrl, internalUnits );
    g_Pad_Master.m_Offset.y = ReturnValueFromTextCtrl( *m_ShapeOffset_Y_Ctrl, internalUnits );

    long     orient_value = 0;
    msg    = m_PadOrientCtrl->GetValue();
    msg.ToLong( &orient_value );
    g_Pad_Master.m_Orient = orient_value;

    g_Current_PadName  = m_PadNumCtrl->GetValue().Left( 4 );
    Current_PadNetName = m_PadNetNameCtrl->GetValue();

    /* Test for incorrect values */
    if( (g_Pad_Master.m_Size.x < g_Pad_Master.m_Drill.x)
       || (g_Pad_Master.m_Size.y < g_Pad_Master.m_Drill.y) )
    {
        error = true;
        DisplayError( this, _( "Incorrect value for pad drill: pad drill bigger than pad size" ) );
    }
    if( ( g_Pad_Master.m_Size.x / 2 <= ABS( g_Pad_Master.m_Offset.x ) )
       || ( g_Pad_Master.m_Size.y / 2 <= ABS( g_Pad_Master.m_Offset.y ) ) )
    {
        error = true;
        DisplayError( this, _( "Incorrect value for pad offset" ) );
    }

    if( error )
         return;

    PadLayerMask = 0;
    if( m_PadLayerCu->GetValue() )
        PadLayerMask |= LAYER_BACK;
    if( m_PadLayerCmp->GetValue() )
        PadLayerMask |= LAYER_FRONT;
    if( ( PadLayerMask & (LAYER_BACK | LAYER_FRONT) ) == (LAYER_BACK | LAYER_FRONT) )
        PadLayerMask |= ALL_CU_LAYERS;
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

    g_Pad_Master.m_Masque_Layer = PadLayerMask;

    if( m_CurrentPad )   // Set current Pad parameters
    {
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        m_Parent->SaveCopyInUndoList( Module, UR_CHANGED );
        Module->m_LastEdit_Time = time( NULL );

        // redraw the area where the pad was, without pad (delete pad on screen)
        m_CurrentPad->m_Flags |= DO_NOT_DRAW;
        m_Parent->DrawPanel->PostDirtyRect( m_CurrentPad->GetBoundingBox() );
        m_CurrentPad->m_Flags &= ~DO_NOT_DRAW;

        // Update values
        m_CurrentPad->m_PadShape = g_Pad_Master.m_PadShape;
        m_CurrentPad->m_Attribut = g_Pad_Master.m_Attribut;
        if( m_CurrentPad->m_Pos != g_Pad_Master.m_Pos )
        {
            m_CurrentPad->m_Pos = g_Pad_Master.m_Pos;
            RastnestIsChanged   = true;
        }

        /* compute the pos 0 value, i.e. pad position for module orient = 0 i.e.
         *  refer to module origin (module position) */
        m_CurrentPad->m_Pos0   = m_CurrentPad->m_Pos;
        m_CurrentPad->m_Pos0  -= Module->m_Pos;
        m_CurrentPad->m_Orient = g_Pad_Master.m_Orient + Module->m_Orient;
        RotatePoint( &m_CurrentPad->m_Pos0.x, &m_CurrentPad->m_Pos0.y, -Module->m_Orient );

        m_CurrentPad->m_Size       = g_Pad_Master.m_Size;
        m_CurrentPad->m_DeltaSize  = g_Pad_Master.m_DeltaSize;
        m_CurrentPad->m_Drill      = g_Pad_Master.m_Drill;
        m_CurrentPad->m_DrillShape = g_Pad_Master.m_DrillShape;
        m_CurrentPad->m_Offset     = g_Pad_Master.m_Offset;
        if( m_CurrentPad->m_Masque_Layer != g_Pad_Master.m_Masque_Layer )
        {
            RastnestIsChanged = true;
            m_CurrentPad->m_Masque_Layer = g_Pad_Master.m_Masque_Layer;
        }
        m_CurrentPad->SetPadName( g_Current_PadName );

        if( m_CurrentPad->GetNetname() != Current_PadNetName )
        {
            if( Current_PadNetName.IsEmpty() )
            {
                m_CurrentPad->SetNet( 0 );
                m_CurrentPad->SetNetname( Current_PadNetName );
            }
            else
            {
                const NETINFO_ITEM* net = m_Parent->GetBoard()->FindNet( Current_PadNetName );
                if( net )
                {
                    RastnestIsChanged = true;
                    m_CurrentPad->SetNetname( Current_PadNetName );
                    m_CurrentPad->SetNet( net->GetNet() );
                }
                else
                    DisplayError( this, _( "Unknown netname, no change" ) );
            }
        }

        switch( m_CurrentPad->m_PadShape )
        {
        case PAD_CIRCLE:
            m_CurrentPad->m_DeltaSize = wxSize( 0, 0 );
            m_CurrentPad->m_Size.y    = m_CurrentPad->m_Size.x;
            break;

        case PAD_RECT:
            m_CurrentPad->m_DeltaSize = wxSize( 0, 0 );
            break;

        case PAD_OVAL:
            m_CurrentPad->m_DeltaSize = wxSize( 0, 0 );
            break;

        case PAD_TRAPEZOID:
            break;
        }

        switch( m_CurrentPad->m_Attribut )
        {
        case PAD_STANDARD:
            break;

        case PAD_CONN:
        case PAD_SMD:
            m_CurrentPad->m_Offset = wxSize( 0, 0 );
            m_CurrentPad->m_Drill  = wxSize( 0, 0 );
            break;

        case PAD_HOLE_NOT_PLATED:
            break;

        default:
            DisplayError( this, wxT( "Error: unknown pad type" ) );
            break;
        }

        m_CurrentPad->m_LocalClearance = g_Pad_Master.m_LocalClearance;
        m_CurrentPad->m_LocalSolderMaskMargin = g_Pad_Master.m_LocalSolderMaskMargin;
        m_CurrentPad->m_LocalSolderPasteMargin = g_Pad_Master.m_LocalSolderPasteMargin;
        m_CurrentPad->m_LocalSolderPasteMarginRatio = g_Pad_Master.m_LocalSolderPasteMarginRatio;

        m_CurrentPad->ComputeRayon();

        Module->Set_Rectangle_Encadrement();
        m_CurrentPad->DisplayInfo( m_Parent );
        // redraw the area where the pad was
        m_Parent->DrawPanel->PostDirtyRect( m_CurrentPad->GetBoundingBox() );
        m_Parent->GetScreen()->SetModify();
    }

    EndModal(1);

    if( RastnestIsChanged )  // The net ratsnest must be recalculated
        m_Parent->GetBoard()->m_Status_Pcb = 0;
}

/*********************************************************************/
void DIALOG_PAD_PROPERTIES::OnCancelButtonClick( wxCommandEvent& event )
/*********************************************************************/
{
    EndModal(0);
}

