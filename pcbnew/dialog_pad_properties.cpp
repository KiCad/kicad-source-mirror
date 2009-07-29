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
int CodeShape[NBSHAPES] = /* forme des pads  */
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
    ALL_CU_LAYERS | SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CU | SOLDERMASK_LAYER_CMP,

    // PAD_CONN:
    CMP_LAYER | SOLDERPASTE_LAYER_CMP | SOLDERMASK_LAYER_CMP,

    // PAD_SMD:
    CMP_LAYER | SOLDERMASK_LAYER_CMP,

    //PAD_HOLE_NOT_PLATED:
    CUIVRE_LAYER | SILKSCREEN_LAYER_CMP | SOLDERMASK_LAYER_CU | SOLDERMASK_LAYER_CMP
};


#include "dialog_pad_properties_base.h"


/********************************************************************/
/* class DialogPadProperties, derived from DialogPadPropertiesBase, */
/*                            created by wxFormBuilder              */
/********************************************************************/
class DialogPadProperties : public DialogPadPropertiesBase
{
public:
    WinEDA_BasePcbFrame* m_Parent;
    wxDC*                m_DC;
    D_PAD*               m_CurrentPad;

    WinEDA_SizeCtrl*     m_PadSizeCtrl;
    WinEDA_PositionCtrl* m_PadPositionCtrl;
    WinEDA_SizeCtrl*     m_PadDeltaSizeCtrl;
    WinEDA_SizeCtrl*     m_PadOffsetCtrl;
    WinEDA_SizeCtrl*     m_PadDrillCtrl;

public:
    DialogPadProperties( WinEDA_BasePcbFrame* parent, D_PAD* Pad, wxDC* DC );
    void InitDialog( wxInitDialogEvent& event );
    void OnPadShapeSelection( wxCommandEvent& event );
    void OnDrillShapeSelected( wxCommandEvent& event );
    void PadOrientEvent( wxCommandEvent& event );
    void PadTypeSelected( wxCommandEvent& event );
    void PadPropertiesAccept( wxCommandEvent& event );
    void SetPadLayersList( long layer_mask );
};


/*******************************************************************************************/
DialogPadProperties::DialogPadProperties( WinEDA_BasePcbFrame* parent, D_PAD* Pad, wxDC* DC ) :
    DialogPadPropertiesBase( parent )
/*******************************************************************************************/
{
    m_Parent = parent;
    m_DC = DC;
    m_CurrentPad = Pad;

    if( m_CurrentPad )
    {
        Current_PadNetName = m_CurrentPad->GetNetname();
        g_Current_PadName  = m_CurrentPad->ReturnStringPadName();
    }
}


/*************************************************************/
void WinEDA_BasePcbFrame::InstallPadOptionsFrame( D_PAD* Pad, wxDC* DC, const wxPoint& pos )
/*************************************************************/
{
    DialogPadProperties* frame = new DialogPadProperties( this, Pad, DC );

    frame->ShowModal(); frame->Destroy();
}


/**************************************************************/
void DialogPadProperties::InitDialog( wxInitDialogEvent& event )
/**************************************************************/
{
    int            tmp;
    wxCommandEvent cmd_event;

    SetFocus();     // Required under wxGTK if we want to demiss the dialog with the ESC key

    m_PadNumCtrl->SetValue( g_Current_PadName );
    m_PadNetNameCtrl->SetValue( Current_PadNetName );
    D_PAD* pad = m_CurrentPad;
    if( pad == NULL )
        pad = &g_Pad_Master;

    m_PadPositionCtrl = new     WinEDA_PositionCtrl( this, _( "Pad Position" ),
        pad->m_Pos,
        g_UnitMetric, m_PadPositionBoxSizer,
        m_Parent->m_InternalUnits );

    m_PadSizeCtrl = new         WinEDA_SizeCtrl( this, _( "Pad Size" ),
        pad->m_Size,
        g_UnitMetric, m_PadPositionBoxSizer,
        m_Parent->m_InternalUnits );

    m_PadDeltaSizeCtrl = new    WinEDA_SizeCtrl( this, _( "Delta" ),
        pad->m_DeltaSize,
        g_UnitMetric, m_PadPositionBoxSizer,
        m_Parent->m_InternalUnits );

    m_PadOffsetCtrl = new       WinEDA_SizeCtrl( this, _( "Offset" ),
        pad->m_Offset,
        g_UnitMetric, m_PadPositionBoxSizer,
        m_Parent->m_InternalUnits );

    /* In second column */
    m_PadDrillCtrl = new WinEDA_SizeCtrl( this, _( "Pad drill" ),
        pad->m_Drill,
        g_UnitMetric, m_DrillShapeBoxSizer,
        m_Parent->m_InternalUnits );

    if( m_CurrentPad )
    {
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        tmp = m_CurrentPad->m_Orient - Module->m_Orient;
    }
    else
        tmp = g_Pad_Master.m_Orient;
    wxString msg; msg << tmp;
    m_PadOrientCtrl->SetValue( msg );


    // Pad Orient
    switch( tmp )
    {
    case 0:
        m_PadOrient->SetSelection( 0 );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    case - 2700:
    case 900:
        m_PadOrient->SetSelection( 1 );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    case - 900:
    case 2700:
        m_PadOrient->SetSelection( 2 );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    case 1800:
    case - 1800:
        m_PadOrient->SetSelection( 3 );
        m_PadOrientCtrl->Enable( FALSE );
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

    if( GetSizer() )
    {
        GetSizer()->SetSizeHints( this );
    }
}


/*********************************************************************/
void DialogPadProperties::OnPadShapeSelection( wxCommandEvent& event )
/*********************************************************************/
{
    switch( m_PadShape->GetSelection() )
    {
    case 0:     //CIRCLE:
        m_PadDeltaSizeCtrl->Enable( FALSE, FALSE );
        m_PadSizeCtrl->Enable( TRUE, FALSE );
        break;

    case 1:     //OVALE:
        m_PadDeltaSizeCtrl->Enable( FALSE, FALSE );
        m_PadSizeCtrl->Enable( TRUE, TRUE );
        break;

    case 2:     // PAD_RECT:
        m_PadDeltaSizeCtrl->Enable( FALSE, FALSE );
        m_PadSizeCtrl->Enable( TRUE, TRUE );
        break;

    case 3:     //TRAPEZE:
        m_PadDeltaSizeCtrl->Enable( TRUE, TRUE );
        m_PadSizeCtrl->Enable( TRUE, TRUE );
        break;
    }
}


/**********************************************************************/
void DialogPadProperties::OnDrillShapeSelected( wxCommandEvent& event )
/**********************************************************************/
{
    if ( (m_PadType->GetSelection() == 1) || (m_PadType->GetSelection() == 2) )
    {   // pad type = SMD or CONN: no hole allowed
        m_PadDrillCtrl->Enable( FALSE, FALSE );
        return;
    }
    switch( m_DrillShapeCtrl->GetSelection() )
    {
    case 0:     //CIRCLE:
        m_PadDrillCtrl->Enable( TRUE, FALSE );
        break;

    case 1:     //OVALE:
        m_PadDrillCtrl->Enable( TRUE, TRUE );
        break;
    }
}


/*******************************************************************/
void DialogPadProperties::PadOrientEvent( wxCommandEvent& event )
/********************************************************************/
{
    switch( m_PadOrient->GetSelection() )
    {
    case 0:
        m_PadOrientCtrl->SetValue( wxT( "0" ) );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    case 1:
        m_PadOrientCtrl->SetValue( wxT( "900" ) );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    case 2:
        m_PadOrientCtrl->SetValue( wxT( "2700" ) );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    case 3:
        m_PadOrientCtrl->SetValue( wxT( "1800" ) );
        m_PadOrientCtrl->Enable( FALSE );
        break;

    default:
        m_PadOrientCtrl->Enable( TRUE );
        break;
    }
}


/*****************************************************************/
void DialogPadProperties::PadTypeSelected( wxCommandEvent& event )
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
void DialogPadProperties::SetPadLayersList( long layer_mask )
/****************************************************************/

/** SetPadLayersList
 * Update the CheckBoxes state in pad layers list,
 * @param layer_mask = pad layer mask (ORed layers bit mask)
 */
{
    m_PadLayerCu->SetValue( ( layer_mask & CUIVRE_LAYER ) );
    m_PadLayerCmp->SetValue( ( layer_mask & CMP_LAYER ) );

    m_PadLayerAdhCmp->SetValue( ( layer_mask & ADHESIVE_LAYER_CMP ) );
    m_PadLayerAdhCu->SetValue( ( layer_mask & ADHESIVE_LAYER_CU ) );

    m_PadLayerPateCmp->SetValue( ( layer_mask & SOLDERPASTE_LAYER_CMP ) );
    m_PadLayerPateCu->SetValue( ( layer_mask & SOLDERPASTE_LAYER_CU ) );

    m_PadLayerSilkCmp->SetValue( ( layer_mask & SILKSCREEN_LAYER_CMP ) );
    m_PadLayerSilkCu->SetValue( ( layer_mask & SILKSCREEN_LAYER_CU ) );

    m_PadLayerMaskCmp->SetValue( ( layer_mask & SOLDERMASK_LAYER_CMP ) );
    m_PadLayerMaskCu->SetValue( ( layer_mask & SOLDERMASK_LAYER_CU ) );

    m_PadLayerECO1->SetValue( ( layer_mask & ECO1_LAYER ) );
    m_PadLayerECO2->SetValue( ( layer_mask & ECO2_LAYER ) );

    m_PadLayerDraft->SetValue( ( layer_mask & DRAW_LAYER ) );
}


/*************************************************************************/
void DialogPadProperties::PadPropertiesAccept( wxCommandEvent& event )
/*************************************************************************/

/* Met a jour les differents parametres pour le composant en cours d'édition
 */
{
    long PadLayerMask;
    bool error = FALSE;
    bool RastnestIsChanged = false;

    if( m_DC )
        m_Parent->DrawPanel->CursorOff( m_DC );

    g_Pad_Master.m_Attribut = CodeType[m_PadType->GetSelection()];
    g_Pad_Master.m_PadShape = CodeShape[m_PadShape->GetSelection()];
    g_Pad_Master.m_Pos  = m_PadPositionCtrl->GetValue();
    g_Pad_Master.m_Pos0 = g_Pad_Master.m_Pos;
    g_Pad_Master.m_Size = m_PadSizeCtrl->GetValue();
    if( g_Pad_Master.m_PadShape == PAD_CIRCLE )
        g_Pad_Master.m_Size.y = g_Pad_Master.m_Size.x;
    g_Pad_Master.m_DeltaSize = m_PadDeltaSizeCtrl->GetValue();
    g_Pad_Master.m_Offset    = m_PadOffsetCtrl->GetValue();
    g_Pad_Master.m_Drill = m_PadDrillCtrl->GetValue();
    if( m_DrillShapeCtrl->GetSelection() == 0 )
    {
        g_Pad_Master.m_DrillShape = PAD_CIRCLE;
        g_Pad_Master.m_Drill.y    = g_Pad_Master.m_Drill.x;
    }
    else
        g_Pad_Master.m_DrillShape = PAD_OVAL;
    long     ovalue = 0;
    wxString msg    = m_PadOrientCtrl->GetValue();
    msg.ToLong( &ovalue );
    g_Pad_Master.m_Orient = ovalue;

    g_Current_PadName  = m_PadNumCtrl->GetValue().Left( 4 );
    Current_PadNetName = m_PadNetNameCtrl->GetValue();

    /* Test for incorrect values */
    if( (g_Pad_Master.m_Size.x < g_Pad_Master.m_Drill.x)
       || (g_Pad_Master.m_Size.y < g_Pad_Master.m_Drill.y) )
    {
        error = TRUE;
        DisplayError( this, _( "Incorrect value for pad drill: pad drill bigger than pad size" ) );
    }
    if( ( g_Pad_Master.m_Size.x / 2 <= ABS( g_Pad_Master.m_Offset.x ) )
       || ( g_Pad_Master.m_Size.y / 2 <= ABS( g_Pad_Master.m_Offset.y ) ) )
    {
        error = TRUE;
        DisplayError( this, _( "Incorrect value for pad offset" ) );
    }

    if( error )
    {
        if( m_DC )
            m_Parent->DrawPanel->CursorOn( m_DC );
        return;
    }

    PadLayerMask = 0;
    if( m_PadLayerCu->GetValue() )
        PadLayerMask |= CUIVRE_LAYER;
    if( m_PadLayerCmp->GetValue() )
        PadLayerMask |= CMP_LAYER;
    if( ( PadLayerMask & (CUIVRE_LAYER | CMP_LAYER) ) == (CUIVRE_LAYER | CMP_LAYER) )
        PadLayerMask |= ALL_CU_LAYERS;
    if( m_PadLayerAdhCmp->GetValue() )
        PadLayerMask |= ADHESIVE_LAYER_CMP;
    if( m_PadLayerAdhCu->GetValue() )
        PadLayerMask |= ADHESIVE_LAYER_CU;
    if( m_PadLayerPateCmp->GetValue() )
        PadLayerMask |= SOLDERPASTE_LAYER_CMP;
    if( m_PadLayerPateCu->GetValue() )
        PadLayerMask |= SOLDERPASTE_LAYER_CU;
    if( m_PadLayerSilkCmp->GetValue() )
        PadLayerMask |= SILKSCREEN_LAYER_CMP;
    if( m_PadLayerSilkCu->GetValue() )
        PadLayerMask |= SILKSCREEN_LAYER_CU;
    if( m_PadLayerMaskCmp->GetValue() )
        PadLayerMask |= SOLDERMASK_LAYER_CMP;
    if( m_PadLayerMaskCu->GetValue() )
        PadLayerMask |= SOLDERMASK_LAYER_CU;
    if( m_PadLayerECO1->GetValue() )
        PadLayerMask |= ECO1_LAYER;
    if( m_PadLayerECO2->GetValue() )
        PadLayerMask |= ECO2_LAYER;
    if( m_PadLayerDraft->GetValue() )
        PadLayerMask |= DRAW_LAYER;

    g_Pad_Master.m_Masque_Layer = PadLayerMask;

    if( m_CurrentPad )   // Set Pad Name & Num
    {
        m_Parent->SaveCopyInUndoList( m_Parent->GetBoard()->m_Modules, UR_CHANGED );
        MODULE* Module = (MODULE*) m_CurrentPad->GetParent();
        Module->m_LastEdit_Time = time( NULL );

        if( m_DC ) // redraw the area where the pad was, without pad (delete pad on screen)
        {
            m_CurrentPad->m_Flags |= DO_NOT_DRAW;
            m_Parent->DrawPanel->PostDirtyRect( m_CurrentPad->GetBoundingBox() );
            m_CurrentPad->m_Flags &= ~DO_NOT_DRAW;
        }
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

        m_CurrentPad->ComputeRayon();

        Module->Set_Rectangle_Encadrement();
        m_CurrentPad->DisplayInfo( m_Parent );
        if( m_DC )  // redraw the area where the pad was
            m_Parent->DrawPanel->PostDirtyRect( m_CurrentPad->GetBoundingBox() );
        m_Parent->GetScreen()->SetModify();
    }

    Close();

    if( m_DC )
        m_Parent->DrawPanel->CursorOn( m_DC );
    if( RastnestIsChanged )  // The net ratsnest must be recalculated
        m_Parent->GetBoard()->m_Status_Pcb = 0;
}
