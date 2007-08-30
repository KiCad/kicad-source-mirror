	/******************************************************/
	/* editpads.cpp: Pad editing functions and dialog box */
	/******************************************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "pcbnew.h"
#include "autorout.h"
#include "trigo.h"

#include "drag.h"

#include "protos.h"

/* Routines Locales */

/* Variables locales */
static wxString Current_PadNetName;


#define NBSHAPES 4
int CodeShape[NBSHAPES] = /* forme des pads  */
{
	CIRCLE, OVALE, RECT, TRAPEZE
};


#define NBTYPES 5
int CodeType[NBTYPES] =
{ STANDARD, SMD, CONN, P_HOLE, MECA
};


static long Std_Pad_Layers[NBTYPES] =
{
	ALL_CU_LAYERS|SILKSCREEN_LAYER_CMP|SOLDERMASK_LAYER_CU|SOLDERMASK_LAYER_CMP,
	CMP_LAYER|SOLDERPASTE_LAYER_CMP|SOLDERMASK_LAYER_CMP,
	CMP_LAYER|SOLDERMASK_LAYER_CMP,
	ALL_CU_LAYERS|SILKSCREEN_LAYER_CU|SILKSCREEN_LAYER_CMP|
		SOLDERMASK_LAYER_CU|SOLDERMASK_LAYER_CMP,
	ALL_CU_LAYERS|SILKSCREEN_LAYER_CU|SILKSCREEN_LAYER_CMP|
		SOLDERMASK_LAYER_CU|SOLDERMASK_LAYER_CMP
};



	/************************************/
	/* class WinEDA_PadPropertiesFrame */
	/************************************/

#include "dialog_pad_edit.cpp"


/*************************************************************/
void WinEDA_BasePcbFrame::InstallPadOptionsFrame(D_PAD * Pad,
					wxDC * DC, const wxPoint & pos)
/*************************************************************/
{
	WinEDA_PadPropertiesFrame * frame = new WinEDA_PadPropertiesFrame(this,
					 Pad, DC);
	frame->ShowModal(); frame->Destroy();
}

/********************************************************/
void WinEDA_PadPropertiesFrame::SetOthersControls(void)
/********************************************************/
{
int tmp;
	
	m_PadNumCtrl->SetValue(g_Current_PadName);
	m_PadNetNameCtrl->SetValue(Current_PadNetName);

	m_PadPositionCtrl = new WinEDA_PositionCtrl(this, _("Pad Position"),
			CurrentPad ? CurrentPad->m_Pos : g_Pad_Master.m_Pos,
			g_UnitMetric, m_PadPositionBoxSizer, m_Parent->m_InternalUnits);

	m_PadSizeCtrl = new WinEDA_SizeCtrl(this, _("Pad Size"),
			CurrentPad ? CurrentPad->m_Size : g_Pad_Master.m_Size,
			g_UnitMetric, m_PadPositionBoxSizer, m_Parent->m_InternalUnits);

	m_PadDeltaSizeCtrl = new WinEDA_SizeCtrl(this, _("Delta"),
			CurrentPad ? CurrentPad->m_DeltaSize : g_Pad_Master.m_DeltaSize,
			g_UnitMetric, m_PadPositionBoxSizer, m_Parent->m_InternalUnits);

	m_PadOffsetCtrl = new WinEDA_SizeCtrl(this, _("Offset"),
			CurrentPad ? CurrentPad->m_Offset : g_Pad_Master.m_Offset,
			g_UnitMetric, m_PadPositionBoxSizer, m_Parent->m_InternalUnits);

	/* In second column */
	
	m_PadDrillCtrl = new WinEDA_SizeCtrl(this, _("Pad Drill"),
			CurrentPad ? CurrentPad->m_Drill : g_Pad_Master.m_Drill,
			g_UnitMetric, m_DrillShapeBoxSizer, m_Parent->m_InternalUnits );

	if ( CurrentPad )
	{
		tmp = CurrentPad->m_Orient - m_Module->m_Orient;
	}
	else tmp = g_Pad_Master.m_Orient;
    m_DrillShapeBoxSizer->Add(5, 5, 0, wxALIGN_CENTER_HORIZONTAL|wxALL, 5);
	m_PadOrientCtrl = new WinEDA_ValueCtrl(this, _("Pad Orient (0.1 deg)"),
			tmp, 2, m_DrillShapeBoxSizer, 1);


	// Pad Orient
	switch ( tmp )
	{
		case 0:
			m_PadOrient->SetSelection(0);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		case -2700:
		case 900:
			m_PadOrient->SetSelection(1);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		case -900:
		case 2700:
			m_PadOrient->SetSelection(2);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		case 1800:
		case -1800:
			m_PadOrient->SetSelection(3);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		default:
			m_PadOrient->SetSelection(4);
			break;
	}

	tmp = CurrentPad ? CurrentPad->m_PadShape : g_Pad_Master.m_PadShape;
	switch ( tmp )
		{
		case CIRCLE:
			m_PadDeltaSizeCtrl->Enable(FALSE, FALSE);
			m_PadSizeCtrl->Enable(TRUE, FALSE);
			m_PadShape->SetSelection(0);
			break;

		case OVALE:
			m_PadDeltaSizeCtrl->Enable(FALSE, FALSE);
			m_PadSizeCtrl->Enable(TRUE, TRUE);
			m_PadShape->SetSelection(1);
			break;

		case RECT:
			m_PadDeltaSizeCtrl->Enable(FALSE, FALSE);
			m_PadSizeCtrl->Enable(TRUE, TRUE);
			m_PadShape->SetSelection(2);
			break;

		case TRAPEZE:
			m_PadDeltaSizeCtrl->Enable(TRUE, TRUE);
			m_PadSizeCtrl->Enable(TRUE, TRUE);
			m_PadShape->SetSelection(3);
			break;
		}

	// Selection du type
	tmp = CurrentPad ? CurrentPad->m_Attribut : g_Pad_Master.m_Attribut;
	m_PadType->SetSelection( 0 );
	for ( int ii = 0; ii < NBTYPES; ii++ )
	{
		if ( CodeType[ii] == tmp )
		{
			m_PadType->SetSelection( ii ); break ;
		}
	}

	tmp = CurrentPad ? CurrentPad->m_DrillShape : g_Pad_Master.m_DrillShape;
	switch ( tmp )
	{
		case CIRCLE:
			m_DrillShapeCtrl->SetSelection(0);
			m_PadDrillCtrl->Enable(TRUE,FALSE);
			break;

		case OVALE:
			m_DrillShapeCtrl->SetSelection(1);
			m_PadDrillCtrl->Enable(TRUE, TRUE);
			break;
	}
	// Selection des couches cuivre :
	if ( CurrentPad ) SetPadLayersList(CurrentPad->m_Masque_Layer);
	else PadTypeSelected();
}


/*******************************************************************/
void WinEDA_PadPropertiesFrame::PadOrientEvent(wxCommandEvent& event)
/********************************************************************/
{
	switch ( m_PadOrient->GetSelection() )
		{
		case 0:
			m_PadOrientCtrl->SetValue(0);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		case 1:
			m_PadOrientCtrl->SetValue(900);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		case 2:
			m_PadOrientCtrl->SetValue(2700);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		case 3:
			m_PadOrientCtrl->SetValue(1800);
			m_PadOrientCtrl->Enable(FALSE);
			break;

		default:
			m_PadOrientCtrl->Enable(TRUE);
			break;
		}

}

/**************************************************************************/
void WinEDA_PadPropertiesFrame::PadTypeSelectedEvent(wxCommandEvent& event)
/**************************************************************************/
/* calcule un layer_mask type selon la selection du type du pad
*/
{
	PadTypeSelected();
}

void WinEDA_PadPropertiesFrame::PadTypeSelected(void)
{
long layer_mask;
int ii;

	ii = m_PadType->GetSelection();
	if ( (ii < 0) || ( ii >= NBTYPES) ) ii = 0;

	layer_mask = Std_Pad_Layers[ii];
	SetPadLayersList(layer_mask);
}

/****************************************************************/
void WinEDA_PadPropertiesFrame::SetPadLayersList(long layer_mask)
/****************************************************************/
/* Met a jour l'etat des CheckBoxes de la liste des layers actives,
données bit a bit dans layer_mask
*/
{
	if( layer_mask & CUIVRE_LAYER ) m_PadLayerCu->SetValue(TRUE);
	else  m_PadLayerCu->SetValue(FALSE);

	if( layer_mask & CMP_LAYER ) m_PadLayerCmp->SetValue(TRUE);
	else  m_PadLayerCmp->SetValue(FALSE);

	if( layer_mask & ADHESIVE_LAYER_CMP ) m_PadLayerAdhCmp->SetValue(TRUE);
	else  m_PadLayerAdhCmp->SetValue(FALSE);

	if( layer_mask & ADHESIVE_LAYER_CU ) m_PadLayerAdhCu->SetValue(TRUE);
	else  m_PadLayerAdhCu->SetValue(FALSE);

	if( layer_mask & SOLDERPASTE_LAYER_CMP ) m_PadLayerPateCmp->SetValue(TRUE);
	else  m_PadLayerPateCmp->SetValue(FALSE);

	if( layer_mask & SOLDERPASTE_LAYER_CU ) m_PadLayerPateCu->SetValue(TRUE);
	else  m_PadLayerPateCu->SetValue(FALSE);

	if( layer_mask & SILKSCREEN_LAYER_CMP ) m_PadLayerSilkCmp->SetValue(TRUE);
	else  m_PadLayerSilkCmp->SetValue(FALSE);

	if( layer_mask & SILKSCREEN_LAYER_CU ) m_PadLayerSilkCu->SetValue(TRUE);
	else  m_PadLayerSilkCu->SetValue(FALSE);

	if( layer_mask & SOLDERMASK_LAYER_CMP ) m_PadLayerMaskCmp->SetValue(TRUE);
	else  m_PadLayerMaskCmp->SetValue(FALSE);

	if( layer_mask & SOLDERMASK_LAYER_CU ) m_PadLayerMaskCu->SetValue(TRUE);
	else  m_PadLayerMaskCu->SetValue(FALSE);

	if( layer_mask & ECO1_LAYER ) m_PadLayerECO1->SetValue(TRUE);
	else  m_PadLayerECO1->SetValue(FALSE);

	if( layer_mask & ECO2_LAYER ) m_PadLayerECO2->SetValue(TRUE);
	else m_PadLayerECO2->SetValue(FALSE);

	if( layer_mask & DRAW_LAYER ) m_PadLayerDraft->SetValue(TRUE);
	else m_PadLayerDraft->SetValue(FALSE);
}


/*************************************************************************/
void WinEDA_PadPropertiesFrame::PadPropertiesAccept(wxCommandEvent& event)
/*************************************************************************/
/* Met a jour les differents parametres pour le composant en cours d'édition
*/
{
long PadLayerMask;
bool error = FALSE;
	
	if ( m_DC ) m_Parent->DrawPanel->CursorOff(m_DC);

	g_Pad_Master.m_Attribut = CodeType[m_PadType->GetSelection()];
	g_Pad_Master.m_PadShape = CodeShape[m_PadShape->GetSelection()];
	g_Pad_Master.m_Pos = m_PadPositionCtrl->GetValue();
	g_Pad_Master.m_Pos0 = g_Pad_Master.m_Pos;
	g_Pad_Master.m_Size = m_PadSizeCtrl->GetValue();
	if ( g_Pad_Master.m_PadShape == CIRCLE )
		g_Pad_Master.m_Size.y = g_Pad_Master.m_Size.x;
	g_Pad_Master.m_DeltaSize = m_PadDeltaSizeCtrl->GetValue();
	g_Pad_Master.m_Offset = m_PadOffsetCtrl->GetValue();
	g_Pad_Master.m_Drill = m_PadDrillCtrl->GetValue();
	if( m_DrillShapeCtrl->GetSelection() == 0 )
	{
		g_Pad_Master.m_DrillShape = CIRCLE;
		g_Pad_Master.m_Drill.y = g_Pad_Master.m_Drill.x;
	}
	else g_Pad_Master.m_DrillShape = OVALE;
	g_Pad_Master.m_Orient = m_PadOrientCtrl->GetValue();
	g_Current_PadName = m_PadNumCtrl->GetValue().Left(4);
	Current_PadNetName = m_PadNetNameCtrl->GetValue();

	/* Test for incorrect values */
	if ( (g_Pad_Master.m_Size.x < g_Pad_Master.m_Drill.x) ||
		 (g_Pad_Master.m_Size.y < g_Pad_Master.m_Drill.y) )
	{
		error = TRUE;
		DisplayError(this, _("Incorrect value for pad drill: pad drill bigger than pad size") );
	}
	if ( (g_Pad_Master.m_Size.x/2 <= ABS(g_Pad_Master.m_Offset.x)) ||
		 (g_Pad_Master.m_Size.y/2 <= ABS(g_Pad_Master.m_Offset.y)) )
	{
		error = TRUE;
		DisplayError(this, _("Incorrect value for pad offset") );
	}
	
	if ( error )
	{
		if ( m_DC ) m_Parent->DrawPanel->CursorOn(m_DC);
		return;
	}

	PadLayerMask = 0;
	if( m_PadLayerCu->GetValue() ) PadLayerMask |= CUIVRE_LAYER;
	if( m_PadLayerCmp->GetValue() ) PadLayerMask |= CMP_LAYER;
	if ( (PadLayerMask & (CUIVRE_LAYER|CMP_LAYER)) == (CUIVRE_LAYER|CMP_LAYER) )
		PadLayerMask |= ALL_CU_LAYERS;
	if( m_PadLayerAdhCmp->GetValue() ) PadLayerMask |= ADHESIVE_LAYER_CMP;
	if( m_PadLayerAdhCu->GetValue() ) PadLayerMask |= ADHESIVE_LAYER_CU;
	if( m_PadLayerPateCmp->GetValue() ) PadLayerMask |= SOLDERPASTE_LAYER_CMP;
	if( m_PadLayerPateCu->GetValue() ) PadLayerMask |= SOLDERPASTE_LAYER_CU;
	if( m_PadLayerSilkCmp->GetValue() ) PadLayerMask |= SILKSCREEN_LAYER_CMP;
	if( m_PadLayerSilkCu->GetValue() ) PadLayerMask |= SILKSCREEN_LAYER_CU;
	if( m_PadLayerMaskCmp->GetValue() ) PadLayerMask |= SOLDERMASK_LAYER_CMP;
	if( m_PadLayerMaskCu->GetValue() ) PadLayerMask |= SOLDERMASK_LAYER_CU;
	if( m_PadLayerECO1->GetValue() ) PadLayerMask |= ECO1_LAYER;
	if( m_PadLayerECO2->GetValue() ) PadLayerMask |= ECO2_LAYER;
	if( m_PadLayerDraft->GetValue() ) PadLayerMask |= DRAW_LAYER;

	g_Pad_Master.m_Masque_Layer = PadLayerMask;

	if ( CurrentPad )   // Set Pad Name & Num
	{
		m_Parent->SaveCopyInUndoList(m_Parent->m_Pcb->m_Modules);
		MODULE * Module;
		Module = (MODULE*) CurrentPad->m_Parent;
		Module->m_LastEdit_Time = time(NULL);

		if ( m_DC ) CurrentPad->Draw(m_Parent->DrawPanel, m_DC, wxPoint(0,0), GR_XOR);
		CurrentPad->m_PadShape = g_Pad_Master.m_PadShape;
		CurrentPad->m_Attribut = g_Pad_Master.m_Attribut;
		CurrentPad->m_Pos = g_Pad_Master.m_Pos;
		/* compute the pos 0 value, i.e. pad position for module orient = 0 i.e.
			refer to module origin (module position) */
		CurrentPad->m_Pos0 = CurrentPad->m_Pos;
		CurrentPad->m_Pos0.x -= Module->m_Pos.x;
		CurrentPad->m_Pos0.y -= Module->m_Pos.y;
		CurrentPad->m_Orient = g_Pad_Master.m_Orient + Module->m_Orient;
		RotatePoint( &CurrentPad->m_Pos0.x, &CurrentPad->m_Pos0.y, - Module->m_Orient );

		CurrentPad->m_Size = g_Pad_Master.m_Size;
		CurrentPad->m_DeltaSize = g_Pad_Master.m_DeltaSize;
		CurrentPad->m_Drill = g_Pad_Master.m_Drill;
		CurrentPad->m_DrillShape = g_Pad_Master.m_DrillShape;
		CurrentPad->m_Offset = g_Pad_Master.m_Offset;
		CurrentPad->m_Masque_Layer = g_Pad_Master.m_Masque_Layer;
		CurrentPad->SetPadName(g_Current_PadName);
		CurrentPad->m_Netname = Current_PadNetName;
		if ( Current_PadNetName.IsEmpty() ) CurrentPad->m_NetCode = 0;

		switch ( CurrentPad->m_PadShape )
		{
			case CIRCLE:
				CurrentPad->m_DeltaSize = wxSize(0,0);
				CurrentPad->m_Size.y = CurrentPad->m_Size.x;
				break;

			case RECT:
				CurrentPad->m_DeltaSize = wxSize(0,0);
				break;

			case OVALE:
				CurrentPad->m_DeltaSize = wxSize(0,0);
				break;

			case TRAPEZE:
				break;
		}

		switch ( CurrentPad->m_Attribut )
		{
			case STANDARD:
				break;

			case CONN:
			case SMD:
				CurrentPad->m_Offset = wxSize(0,0);
				CurrentPad->m_Drill = wxSize(0,0);
				break;

			case P_HOLE:
			case MECA:
				break;
		}

		CurrentPad->ComputeRayon();

		Module->Set_Rectangle_Encadrement();
		CurrentPad->Display_Infos(m_Parent);
		if ( m_DC ) CurrentPad->Draw(m_Parent->DrawPanel, m_DC, wxPoint(0,0), GR_OR);
		m_Parent->GetScreen()->SetModify();
	}

	Close();

	if ( m_DC ) m_Parent->DrawPanel->CursorOn(m_DC);
}

