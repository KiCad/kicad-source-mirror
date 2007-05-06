	/************************************/
	/* Delete.cpp: routines d'effacement */
	/************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#define SELECTEDNODE 1	/* flag indiquant que la structure a deja selectionnee */
#define STRUCT_DELETED 2	/* Bit flag de Status pour structures effacee */
#define CANDIDATE 4		/* flag indiquant que la structure est connectee */
#define SKIP_STRUCT 8	/* flag indiquant que la structure ne doit pas etre traitee */

/********************************************************************************/
static int CountConnectedItems(WinEDA_SchematicFrame * frame,
		EDA_BaseStruct * ListStruct, wxPoint pos, bool TstJunction)
/********************************************************************************/
/*  Count number of items connected to point pos :
		pins, end wire or bus, and junctions if TstJunction == TRUE
	Return this count

	Used by WinEDA_SchematicFrame::DeleteConnection()
*/
{
EDA_BaseStruct * Struct;
int count = 0;

	if ( frame->LocatePinEnd(ListStruct, pos) ) count++;

	for ( Struct = ListStruct; Struct != NULL; Struct = Struct->Pnext)
	{
		if ( Struct->m_Flags & STRUCT_DELETED ) continue;
		if ( Struct->m_Flags & SKIP_STRUCT ) continue;

			
		if ( TstJunction && (Struct->m_StructType == DRAW_JUNCTION_STRUCT_TYPE) )
		{
		#define JUNCTION ((DrawJunctionStruct*)Struct)
			if ( (JUNCTION->m_Pos.x == pos.x) && (JUNCTION->m_Pos.y == pos.y) )
				count++;
		#undef JUNCTION
		}

		if ( Struct->m_StructType != DRAW_SEGMENT_STRUCT_TYPE ) continue;
		#define SEGM ((EDA_DrawLineStruct*)Struct)
		if ( SEGM->IsOneEndPointAt(pos) ) count++;
		#undef SEGM
	}

	return count;
}

/************************************************************************************/
static bool MarkConnected(WinEDA_SchematicFrame * frame, EDA_BaseStruct * ListStruct,
		EDA_DrawLineStruct* segment)
/************************************************************************************/
/* Mark to "CANDIDATE" all wires or junction connected to "segment" in list "ListStruct"
	Search wire stop at an any  pin

	Used by WinEDA_SchematicFrame::DeleteConnection()
*/
{
EDA_BaseStruct * Struct;

	for ( Struct = ListStruct; Struct != NULL; Struct = Struct->Pnext)
	{
		if ( Struct->m_Flags ) continue;
		if ( Struct->m_StructType == DRAW_JUNCTION_STRUCT_TYPE )
		{
		#define JUNCTION ((DrawJunctionStruct*)Struct)
			if ( segment->IsOneEndPointAt(JUNCTION->m_Pos) ) Struct->m_Flags |= CANDIDATE;
			continue;
		#undef JUNCTION
		}

		if ( Struct->m_StructType != DRAW_SEGMENT_STRUCT_TYPE ) continue;

		#define SEGM ((EDA_DrawLineStruct*)Struct)
		if ( segment->IsOneEndPointAt(SEGM->m_Start) )
		{
			if ( ! frame->LocatePinEnd(ListStruct,SEGM->m_Start) )
			{
				Struct->m_Flags |= CANDIDATE;
				MarkConnected(frame, ListStruct, SEGM);
			}
		}
		if ( segment->IsOneEndPointAt(SEGM->m_End) )
		{
			if ( ! frame->LocatePinEnd(ListStruct,SEGM->m_End) )
			{
				Struct->m_Flags |= CANDIDATE;
				MarkConnected(frame, ListStruct, SEGM);
			}
		}
		#undef SEGM
	}

	return TRUE;
}

/********************************************************************************/
void WinEDA_SchematicFrame::DeleteConnection(wxDC * DC, bool DeleteFullConnection)
/********************************************************************************/
/* Delete a connection, i.e wires or bus connected
	stop on a node (more than 2 wires (bus) connected)
*/
{
wxPoint refpos = GetScreen()->m_Curseur;
EDA_BaseStruct * DelStruct ;
DrawPickedStruct * PickedItem, *PickedList = NULL;

	/* Clear .m_Flags member for all items */
	for(DelStruct = GetScreen()->EEDrawList; DelStruct != NULL; DelStruct=DelStruct->Pnext)
		DelStruct->m_Flags = 0;

	BreakSegmentOnJunction( GetScreen() );
	DelStruct = GetScreen()->EEDrawList;
	
	/* Locate all the wires, bus or junction under the mouse cursor, and put them in a list
		of items to delete
	*/
	while ( DelStruct &&
			(DelStruct = PickStruct(GetScreen()->m_Curseur,
				DelStruct, JUNCTIONITEM|WIREITEM|BUSITEM)) != NULL )
	{
		DelStruct->m_Flags = SELECTEDNODE|STRUCT_DELETED;
		/* Put this structure in the picked list: */
		PickedItem = new DrawPickedStruct(DelStruct);
		PickedItem->Pnext = PickedList;
		PickedList = PickedItem;
		DelStruct=DelStruct->Pnext;
	}

	 /* Mark all wires, junctions, .. connected to one of the item to delete
	*/
	if ( DeleteFullConnection )
	{
		for ( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL; DelStruct = DelStruct->Pnext)
		{
			if ( ! (DelStruct->m_Flags & SELECTEDNODE) ) continue;
			#define SEGM ((EDA_DrawLineStruct*)DelStruct)
			if ( DelStruct->m_StructType != DRAW_SEGMENT_STRUCT_TYPE ) continue;
			MarkConnected(this, GetScreen()->EEDrawList, SEGM);
			#undef SEGM
		}

		// Search all removable wires (i.e wire with one new dangling end )
		for ( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL; DelStruct = DelStruct->Pnext)
		{
			bool noconnect = FALSE;

			if ( DelStruct->m_Flags & STRUCT_DELETED ) continue;		// Already seen
			if ( ! (DelStruct->m_Flags & CANDIDATE) ) continue;	// Already seen
			if ( DelStruct->m_StructType != DRAW_SEGMENT_STRUCT_TYPE ) continue;
			DelStruct->m_Flags |= SKIP_STRUCT;
			#define SEGM ((EDA_DrawLineStruct*)DelStruct)
			/* Test the SEGM->m_Start point: if this point was connected to an STRUCT_DELETED wire,
				and now is not connected, the wire can be deleted */
			EDA_BaseStruct * removed_struct;
			for ( removed_struct = GetScreen()->EEDrawList; removed_struct != NULL; removed_struct = removed_struct->Pnext)
			{
				if( (removed_struct->m_Flags & STRUCT_DELETED) == 0 ) continue;
				if ( removed_struct->m_StructType != DRAW_SEGMENT_STRUCT_TYPE ) continue;
				#define WIRE ((EDA_DrawLineStruct*)removed_struct)
				if ( WIRE->IsOneEndPointAt(SEGM->m_Start) ) break;
			}
			
			if ( WIRE && ! CountConnectedItems(this, GetScreen()->EEDrawList, SEGM->m_Start, TRUE) )
				noconnect = TRUE;
			/* Test the SEGM->m_End point: if this point was connected to an STRUCT_DELETED wire,
				and now is not connected, the wire can be deleted */
			for ( removed_struct = GetScreen()->EEDrawList; removed_struct != NULL; removed_struct = removed_struct->Pnext)
			{
				if( (removed_struct->m_Flags & STRUCT_DELETED) == 0 ) continue;
				if ( removed_struct->m_StructType != DRAW_SEGMENT_STRUCT_TYPE ) continue;
				if ( WIRE->IsOneEndPointAt(SEGM->m_End) ) break;
			}
			if ( removed_struct && ! CountConnectedItems(this, GetScreen()->EEDrawList, SEGM->m_End, TRUE) )
				noconnect = TRUE;
			DelStruct->m_Flags &= ~SKIP_STRUCT;

			if ( noconnect )
			{
				DelStruct->m_Flags |= STRUCT_DELETED;
				/* Put this structure in the picked list: */
				PickedItem = new DrawPickedStruct(DelStruct);
				PickedItem->Pnext = PickedList;
				PickedList = PickedItem;
				DelStruct = GetScreen()->EEDrawList;
			}
			#undef SEGM
		}

		// Delete redundant junctions (junctions which connect < 3 end wires and no pin are removed)
		for ( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL; DelStruct = DelStruct->Pnext)
		{
			int count;
			if ( DelStruct->m_Flags & STRUCT_DELETED ) continue;
			if ( ! (DelStruct->m_Flags & CANDIDATE) ) continue;
			if ( DelStruct->m_StructType == DRAW_JUNCTION_STRUCT_TYPE )
			{
				#define JUNCTION ((DrawJunctionStruct*)DelStruct)
				count = CountConnectedItems(this, GetScreen()->EEDrawList, JUNCTION->m_Pos, FALSE);
				if ( count <= 2 )
				{
					DelStruct->m_Flags |= STRUCT_DELETED;
					/* Put this structure in the picked list: */
					PickedItem = new DrawPickedStruct(DelStruct);
					PickedItem->Pnext = PickedList;
					PickedList = PickedItem;
				}
				#undef JUNCTION
			}
		}

		// Delete labels attached to wires
		wxPoint pos = GetScreen()->m_Curseur;
		for ( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL; DelStruct = DelStruct->Pnext)
		{
			if ( DelStruct->m_Flags & STRUCT_DELETED ) continue;
			if ( DelStruct->m_StructType != DRAW_LABEL_STRUCT_TYPE ) continue;
			GetScreen()->m_Curseur = ((DrawTextStruct*)DelStruct)->m_Pos;
			EDA_BaseStruct * TstStruct =
				PickStruct(GetScreen()->m_Curseur, GetScreen()->EEDrawList,WIREITEM|BUSITEM);
			if ( TstStruct && TstStruct->m_Flags & STRUCT_DELETED )
			{
				DelStruct->m_Flags |= STRUCT_DELETED;
				/* Put this structure in the picked list: */
				PickedItem = new DrawPickedStruct(DelStruct);
				PickedItem->Pnext = PickedList;
				PickedList = PickedItem;
			}
		}
		GetScreen()->m_Curseur = pos;
	}

	for ( DelStruct = GetScreen()->EEDrawList; DelStruct != NULL; DelStruct = DelStruct->Pnext)
		DelStruct->m_Flags = 0;

	if ( PickedList )
	{
		DeleteStruct(DrawPanel, DC, PickedList);
		GetScreen()->SetModify();
	}

}


/*****************************************************************/
void LocateAndDeleteItem(WinEDA_SchematicFrame * frame, wxDC * DC)
/*****************************************************************/

/* Routine d'effacement d'un element de schema ( et placement en "undelete" )
	si plusieurs elements sont superposes: ordre de priorite:
	1 : MARQUEUR
	2 : JUNCTION
	2 : NOCONNECT
	3 : WIRE ou BUS
	4 : DRAWITEM
	5 : TEXT
	6 : COMPOSANT
	7 : SHEET
*/

{
EDA_BaseStruct * DelStruct;
SCH_SCREEN * screen = frame->GetScreen();
	
	DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, MARKERITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, JUNCTIONITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, NOCONNECTITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, RACCORDITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, WIREITEM|BUSITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, DRAWITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, TEXTITEM|LABELITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, LIBITEM);
	if( DelStruct == NULL ) DelStruct = PickStruct(screen->m_Curseur,
			screen->EEDrawList, SHEETITEM);

	if (DelStruct)
	{
		g_ItemToRepeat = NULL;
		DeleteStruct(frame->DrawPanel, DC, DelStruct);
		frame->TestDanglingEnds(frame->m_CurrentScreen->EEDrawList, DC);
		frame->GetScreen()->SetModify();
	}
}




/***************************************************************/
void EraseStruct(EDA_BaseStruct *DrawStruct, SCH_SCREEN * Screen)
/***************************************************************/
/* Suppression definitive d'une structure dans une liste chainee
	d'elements de dessin
	DrawStruct = pointeur sur la structure
	Screen = pointeur sur l'ecran d'appartenance
	Le chainage de la liste est modifie.

 Remarque:
	pour les structures DRAW_SHEET_STRUCT_TYPE, l'ecran et les structures
		correspondantes ne sont pas touches.
	Ils doivent etre traites separement
*/
{
EDA_BaseStruct	*DrawList;
DrawPickedStruct	*PickedList = NULL;
DrawSheetLabelStruct* SheetLabel, *NextLabel;

	if( DrawStruct == NULL ) return;
	if( Screen == NULL ) return;

		Screen->SetModify();

	if (DrawStruct->m_StructType == DRAW_SHEETLABEL_STRUCT_TYPE)
	{	/* Cette stucture est rattachee a une feuille, et n'est pas
		accessible par la liste globale directement */
		DrawList = Screen->EEDrawList;
		for ( ; DrawList != NULL; DrawList = DrawList->Pnext )
		{
			if(DrawList->m_StructType != DRAW_SHEET_STRUCT_TYPE) continue;
			/* Examen de la Sheet */
			SheetLabel = ((DrawSheetStruct *) DrawList)->m_Label;
			if (SheetLabel == NULL) continue;
			if (SheetLabel == (DrawSheetLabelStruct*) DrawStruct)
			{
				((DrawSheetStruct *) DrawList)->m_Label =
							(DrawSheetLabelStruct *)SheetLabel->Pnext;
				delete DrawStruct;
				return;
			}
			else while( SheetLabel->Pnext )/* Examen de la liste dependante */
			{
				NextLabel = (DrawSheetLabelStruct *)SheetLabel->Pnext;
				if( NextLabel == (DrawSheetLabelStruct*) DrawStruct )
				{
					SheetLabel->Pnext = (EDA_BaseStruct *)NextLabel->Pnext;
					delete DrawStruct;
					return;
				}
				SheetLabel = NextLabel;
			}
		}
		return;
	}


	if (DrawStruct->m_StructType == DRAW_PICK_ITEM_STRUCT_TYPE)
	{
		PickedList = (DrawPickedStruct *) DrawStruct;
		while (PickedList)
		{
			if (PickedList->m_PickedStruct == Screen->EEDrawList)
			{
				Screen->EEDrawList = Screen->EEDrawList->Pnext;
				delete DrawStruct;
			}
			else
			{
				DrawList = Screen->EEDrawList;
				while ( DrawList && DrawList->Pnext)
				{
					if (DrawList->Pnext == PickedList->m_PickedStruct)
					{
						DrawList->Pnext = DrawList->Pnext->Pnext;
						delete DrawStruct;
						return;
					}
					DrawList = DrawList->Pnext;
				}
			}
			PickedList = (DrawPickedStruct *)PickedList->Pnext;
		}
	}
	else	// structure usuelle */
	{
		if (DrawStruct == Screen->EEDrawList)
		{
			Screen->EEDrawList = DrawStruct->Pnext;
			delete DrawStruct;
		}
		else
		{
			DrawList = Screen->EEDrawList;
			while (DrawList && DrawList->Pnext)
			{
				if (DrawList->Pnext == DrawStruct)
				{
					DrawList->Pnext = DrawStruct->Pnext;
					delete DrawStruct;
					return;
				}
				DrawList = DrawList->Pnext;
			}
		}
	}
}



/********************************/
void DeleteAllMarkers(int type)
/********************************/
/* Effacement des marqueurs du type "type" */
{
SCH_SCREEN * screen;
EDA_BaseStruct * DrawStruct, * NextStruct;
DrawMarkerStruct * Marker;

	EDA_ScreenList ScreenList(NULL);
	for ( screen = ScreenList.GetFirst(); screen != NULL; screen = ScreenList.GetNext() )
	{
		for ( DrawStruct = screen->EEDrawList; DrawStruct != NULL; DrawStruct = NextStruct)
		{
			NextStruct = DrawStruct->Pnext;
			if(DrawStruct->m_StructType != DRAW_MARKER_STRUCT_TYPE ) continue;
			/* Marqueur trouve */
			Marker = (DrawMarkerStruct * ) DrawStruct;
			if( Marker->m_Type != type ) continue;
			/* Suppression du marqueur */
			EraseStruct( DrawStruct, screen);
		}
	}
}

/********************************************************************/
void DeleteOneLibraryDrawStruct(WinEDA_DrawPanel * panel, wxDC * DC,
				EDA_LibComponentStruct * LibEntry,
				LibEDA_BaseStruct * DrawItem, int Affiche)
/********************************************************************/
/* Routine d'effacement d'un "LibraryDrawStruct"
	(d'un element de dessin d'un composant )

	Parametres d'entree
		Pointeur sur le composant comportant la structure
			(Si NULL la structure a effacer est supposee non rattachee
			a un composant)
		Pointeur sur la structure a effacer

	Efface egalement le graphique correspondant de l'ecran
*/
{
LibEDA_BaseStruct *PreviousDrawItem;

	/* Effacement du graphique  */
	if( Affiche && DC)
		DrawLibraryDrawStruct(panel, DC, LibEntry, 0 , 0, DrawItem, CurrentUnit, g_XorMode);

	/* Effacement de la structure en memoire */
	if( LibEntry ) /* Recherche du predecesseur */
	{
		PreviousDrawItem = LibEntry->m_Drawings;

		/* Cas du 1er symbole graphique = struct a supprimer */
		if( LibEntry->m_Drawings == DrawItem )
		{
			LibEntry->m_Drawings = DrawItem->Next();
			delete DrawItem;
		}

		else	/* Cas des autres items */
		while(PreviousDrawItem)
		{
			if(PreviousDrawItem->Pnext == DrawItem)
			{
				PreviousDrawItem->Pnext = DrawItem->Pnext;
				delete DrawItem; break;
			}
			PreviousDrawItem = PreviousDrawItem->Next();
		}
	}

	else /* Structure non reliee a un composant */
	{
		delete DrawItem;
	}
}


