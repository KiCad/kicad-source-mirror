	/*********************/
	/* dangling_ends.cpp */
	/*********************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "netlist.h"	/* Definitions generales liees au calcul de netliste */
#include "protos.h"

enum End_Type
{
	UNKNOWN = 0,
	WIRE_START_END,
	WIRE_END_END,
	BUS_START_END,
	BUS_END_END,
	JUNCTION_END,
	PIN_END,
	LABEL_END,
	ENTRY_END,
	SHEET_LABEL_END
};

class DanglingEndHandle
{
public:
	const void * m_Item;
	wxPoint m_Pos;
	int m_Type;
	DanglingEndHandle * m_Pnext;
	
	DanglingEndHandle(int type)
	{
		m_Item = NULL;
		m_Type = type;
		m_Pnext = NULL;
	}
};

DanglingEndHandle * ItemList;

static void TestWireForDangling(EDA_DrawLineStruct * DrawRef,
				WinEDA_SchematicFrame * frame, wxDC * DC);
void TestLabelForDangling(DrawTextStruct * label,
				WinEDA_SchematicFrame * frame, wxDC * DC);
DanglingEndHandle * RebuildEndList(EDA_BaseStruct *DrawList);

/**********************************************************/
bool SegmentIntersect(int Sx1, int Sy1, int Sx2, int Sy2, 
								int Px1, int Py1)
/**********************************************************/
/* Retourne TRUE si le point P est sur le segment S.
	Le segment est suppose horizontal ou vertical.
*/
{
int Sxmin, Sxmax, Symin, Symax;

	if (Sx1 == Sx2)			 /* Line S is vertical. */
		{
		Symin = MIN(Sy1, Sy2); Symax = MAX(Sy1, Sy2);
		if (Px1 != Sx1) return FALSE;
		if (Py1 >= Symin  &&  Py1 <= Symax)  return TRUE;
		else  return FALSE;
		}

	else if (Sy1 == Sy2)	/* Line S is horizontal. */
		{
		Sxmin = MIN(Sx1, Sx2); Sxmax = MAX(Sx1, Sx2);
		if (Py1 != Sy1) return FALSE;
		if (Px1 >= Sxmin  &&  Px1 <= Sxmax)  return TRUE;
		else  return FALSE;
		}
	else return FALSE;	// Segments quelconques
}



/******************************************************************************/
void WinEDA_SchematicFrame::TestDanglingEnds(EDA_BaseStruct *DrawList, wxDC *DC)
/******************************************************************************/
/* Met a jour les membres m_Dangling des wires, bus, labels
*/
{
EDA_BaseStruct * DrawItem;
const DanglingEndHandle * DanglingItem, * nextitem;
	
	if ( ItemList )
		for ( DanglingItem = ItemList; DanglingItem != NULL; DanglingItem = nextitem)
		{
			nextitem = DanglingItem->m_Pnext;
			delete DanglingItem;
		}
		
	ItemList = RebuildEndList(DrawList);
	// Controle des elements
	for ( DrawItem = DrawList; DrawItem != NULL; DrawItem= DrawItem->Pnext)
	{
		switch( DrawItem->m_StructType )
		{
			case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
			case DRAW_LABEL_STRUCT_TYPE:
				#undef STRUCT
				#define STRUCT ((DrawLabelStruct*)DrawItem)
					TestLabelForDangling(STRUCT, this, DC);
					break;
				break;
			
			case DRAW_SEGMENT_STRUCT_TYPE:
				#undef STRUCT
				#define STRUCT ((EDA_DrawLineStruct*)DrawItem)
				if( STRUCT->m_Layer == LAYER_WIRE)
				{
					TestWireForDangling(STRUCT, this, DC);
					break;
				}
				if( STRUCT->m_Layer == LAYER_NOTES) break;
				if( STRUCT->m_Layer == LAYER_BUS)
				{
					STRUCT->m_StartIsDangling = 
						STRUCT->m_EndIsDangling = FALSE;
					break;
				}
				break;
		}
	}
}

/********************************************************************/
LibDrawPin * WinEDA_SchematicFrame::LocatePinEnd(EDA_BaseStruct *DrawList,
		const wxPoint & pos)
/********************************************************************/
/* Teste si le point de coordonnées pos est sur l'extrémité d'une PIN
	retourne un pointeur sur la pin
	NULL sinon
*/
{
EDA_SchComponentStruct * DrawLibItem;
LibDrawPin * Pin;
wxPoint pinpos;
	
	Pin = LocateAnyPin(DrawList,pos, &DrawLibItem);
	if( ! Pin ) return NULL;
		
	pinpos = Pin->m_Pos;

	if(DrawLibItem == NULL ) pinpos.y = -pinpos.y;

	else
		{
		int x1 = pinpos.x, y1 = pinpos.y;
		pinpos.x = DrawLibItem->m_Pos.x + DrawLibItem->m_Transform[0][0] * x1
								+ DrawLibItem->m_Transform[0][1] * y1;
		pinpos.y = DrawLibItem->m_Pos.y + DrawLibItem->m_Transform[1][0] * x1
								+ DrawLibItem->m_Transform[1][1] * y1;
		}

	if( (pos.x == pinpos.x) && (pos.y == pinpos.y) )	return Pin;
	return NULL;
}


/****************************************************************************/
void TestWireForDangling(EDA_DrawLineStruct * DrawRef,
				WinEDA_SchematicFrame * frame, wxDC * DC)
/****************************************************************************/
{
DanglingEndHandle * terminal_item;
bool Sdangstate = TRUE, Edangstate = TRUE;
	
	for ( terminal_item = ItemList; terminal_item != NULL;
				terminal_item = terminal_item->m_Pnext)
	{
		if ( terminal_item->m_Item == DrawRef ) continue;
			
		if ( (DrawRef->m_Start.x == terminal_item->m_Pos.x) &&
			 (DrawRef->m_Start.y == terminal_item->m_Pos.y) )
			Sdangstate = FALSE;

		if ( (DrawRef->m_End.x == terminal_item->m_Pos.x) &&
			 (DrawRef->m_End.y == terminal_item->m_Pos.y) )
			Edangstate = FALSE;

		if ( (Sdangstate == FALSE) && (Edangstate == FALSE) )
			break;
	}

	if ( (Sdangstate != DrawRef->m_StartIsDangling) ||
		 (Edangstate != DrawRef->m_EndIsDangling) )
	{
		if ( DC )
			RedrawOneStruct(frame->DrawPanel,DC, DrawRef, g_XorMode);
		DrawRef->m_StartIsDangling = Sdangstate;
		DrawRef->m_EndIsDangling = Edangstate;
		if ( DC )
			RedrawOneStruct(frame->DrawPanel,DC, DrawRef, GR_DEFAULT_DRAWMODE);
	}
}

/********************************************************/
void TestLabelForDangling(DrawTextStruct * label,
				WinEDA_SchematicFrame * frame, wxDC * DC)
/********************************************************/
{
DanglingEndHandle * terminal_item;
bool dangstate = TRUE;
	
	for ( terminal_item = ItemList; terminal_item != NULL;
				terminal_item = terminal_item->m_Pnext)
	{
		if ( terminal_item->m_Item == label ) continue;
		switch(	terminal_item->m_Type )
		{
			case PIN_END:
			case LABEL_END:
			case SHEET_LABEL_END:
				if ( (label->m_Pos.x == terminal_item->m_Pos.x) &&
					 (label->m_Pos.y == terminal_item->m_Pos.y) )
					dangstate = FALSE;
				break;

			case WIRE_START_END:
			case BUS_START_END:
				dangstate = ! SegmentIntersect(terminal_item->m_Pos.x,
							terminal_item->m_Pos.y,
							terminal_item->m_Pnext->m_Pos.x,
							terminal_item->m_Pnext->m_Pos.y, 
							label->m_Pos.x, label->m_Pos.y);
				terminal_item = terminal_item->m_Pnext;
				break;

			case UNKNOWN:
			case JUNCTION_END:
			case ENTRY_END:
			case WIRE_END_END:
			case BUS_END_END:
				break;
		}

		if (dangstate == FALSE) break;
	}

	if ( dangstate != label->m_IsDangling )
	{
		if ( DC )
			RedrawOneStruct(frame->DrawPanel,DC, label, g_XorMode);
		label->m_IsDangling = dangstate;
		if ( DC )
			RedrawOneStruct(frame->DrawPanel,DC, label, GR_DEFAULT_DRAWMODE);
	}
}


/****************************************************/
wxPoint ReturnPinPhysicalPosition( LibDrawPin * Pin,
			EDA_SchComponentStruct * DrawLibItem)
/****************************************************/
/* Retourne la position physique de la pin, qui dépend de l'orientation
du composant */
{
wxPoint PinPos = Pin->m_Pos;
	
	if(DrawLibItem == NULL ) PinPos.y = -PinPos.y;

	else
		{
		int x = Pin->m_Pos.x, y = Pin->m_Pos.y;
		PinPos.x = DrawLibItem->m_Pos.x + DrawLibItem->m_Transform[0][0] * x
								+ DrawLibItem->m_Transform[0][1] * y;
		PinPos.y = DrawLibItem->m_Pos.y + DrawLibItem->m_Transform[1][0] * x
								+ DrawLibItem->m_Transform[1][1] * y;
		}

	return PinPos;
}


/***********************************************************/
DanglingEndHandle * RebuildEndList(EDA_BaseStruct *DrawList)
/***********************************************************/
{
DanglingEndHandle * StartList = NULL, *item, *lastitem = NULL;
EDA_BaseStruct * DrawItem;
	
	for ( DrawItem = DrawList; DrawItem != NULL; DrawItem = DrawItem->Pnext)
	{
		switch( DrawItem->m_StructType )
		{
			case DRAW_LABEL_STRUCT_TYPE:
				break;
			
			case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
				#undef STRUCT
				#define STRUCT ((DrawGlobalLabelStruct*)DrawItem)
				item = new DanglingEndHandle(LABEL_END);
				item->m_Item = DrawItem;
				item->m_Pos = STRUCT->m_Pos;
				if ( lastitem ) lastitem->m_Pnext = item;
				else StartList = item;
				lastitem = item;
				break;
			
			case DRAW_SEGMENT_STRUCT_TYPE:
				#undef STRUCT
				#define STRUCT ((EDA_DrawLineStruct*)DrawItem)
				if( STRUCT->m_Layer == LAYER_NOTES ) break;
				if( (STRUCT->m_Layer == LAYER_BUS) || (STRUCT->m_Layer == LAYER_WIRE) )
				{
					item = new DanglingEndHandle((STRUCT->m_Layer == LAYER_BUS) ?
						BUS_START_END : WIRE_START_END);
					item->m_Item = DrawItem;
					item->m_Pos = STRUCT->m_Start;
					if ( lastitem ) lastitem->m_Pnext = item;
					else StartList = item;
					lastitem = item;
					item = new DanglingEndHandle((STRUCT->m_Layer == LAYER_BUS) ?
						BUS_END_END : WIRE_END_END);
					item->m_Item = DrawItem;
					item->m_Pos = STRUCT->m_End;
					lastitem->m_Pnext = item;
					lastitem = item;
				}
				break;
				
			case DRAW_JUNCTION_STRUCT_TYPE:
				#undef STRUCT
				#define STRUCT ((DrawJunctionStruct*)DrawItem)
					item = new DanglingEndHandle(JUNCTION_END);
					item->m_Item = DrawItem;
					item->m_Pos = STRUCT->m_Pos;
					if ( lastitem ) lastitem->m_Pnext = item;
					else StartList = item;
					lastitem = item;
				break;
				
			case DRAW_BUSENTRY_STRUCT_TYPE:
				#undef STRUCT
				#define STRUCT ((DrawBusEntryStruct*)DrawItem)
					item = new DanglingEndHandle(ENTRY_END);
					item->m_Item = DrawItem;
					item->m_Pos = STRUCT->m_Pos;
					if ( lastitem ) lastitem->m_Pnext = item;
					else StartList = item;
					lastitem = item;
					item = new DanglingEndHandle(ENTRY_END);
					item->m_Item = DrawItem;
					item->m_Pos = STRUCT->m_End();
					lastitem->m_Pnext = item;
					lastitem = item;
				break;
				
			case DRAW_LIB_ITEM_STRUCT_TYPE:
			{
				#undef STRUCT
				#define STRUCT ((EDA_SchComponentStruct*)DrawItem)
				EDA_LibComponentStruct * Entry;
				Entry = FindLibPart( STRUCT->m_ChipName, wxEmptyString, FIND_ROOT);
				if( Entry == NULL ) break;
				LibEDA_BaseStruct * DrawLibItem = Entry->m_Drawings;
				for ( ; DrawLibItem != NULL; DrawLibItem = DrawLibItem->Next())
				{
					if(DrawLibItem->m_StructType != COMPONENT_PIN_DRAW_TYPE) continue;

					LibDrawPin * Pin = (LibDrawPin *) DrawLibItem;

					if( Pin->m_Unit && DrawLibItem->m_Unit &&
						(DrawLibItem->m_Unit != Pin->m_Unit) )
						continue;

					if( Pin->m_Convert && DrawLibItem->m_Convert &&
						(DrawLibItem->m_Convert != Pin->m_Convert) )
						continue;

					item = new DanglingEndHandle(PIN_END);
					item->m_Item = Pin;
					item->m_Pos = ReturnPinPhysicalPosition( Pin,STRUCT);
					if ( lastitem ) lastitem->m_Pnext = item;
					else StartList = item;
					lastitem = item;
				}
				break;

			}

			case DRAW_SHEET_STRUCT_TYPE:
			{
				#undef STRUCT
				#define STRUCT ((DrawSheetStruct*)DrawItem)
				DrawSheetLabelStruct * pinsheet = STRUCT->m_Label;
				while(pinsheet)
				{
					item = new DanglingEndHandle(SHEET_LABEL_END);
					item->m_Item = pinsheet;
					item->m_Pos = pinsheet->m_Pos;
					if ( lastitem ) lastitem->m_Pnext = item;
					else StartList = item;
					lastitem = item;
					pinsheet = (DrawSheetLabelStruct*)pinsheet->Pnext;
				}
				break;
			}
		}
	}
	return StartList;
}
