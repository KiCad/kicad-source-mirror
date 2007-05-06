/*********************************************************/
/* Modules de creations de Traits, Wires, Bus, Junctions */
/*********************************************************/

#include "fctsys.h"
#include "gr_basic.h"

#include "common.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


/* Routines Locales */
static void Polyline_in_Ghost(WinEDA_DrawPanel * panel, wxDC * DC, bool erase);
static void Segment_in_Ghost(WinEDA_DrawPanel * panel, wxDC * DC, bool erase);
static void ExitTrace(WinEDA_DrawFrame * frame, wxDC * DC);
static bool IsTerminalPoint(SCH_SCREEN * screen, const wxPoint & pos, int layer );


/*************************************************************/
void WinEDA_SchematicFrame::BeginSegment(wxDC * DC, int type)
/*************************************************************/
/* Routine de Trace de segments ( WIRES, BUS ) pour lesquels chaque segment
est une structure.
*/
{
DrawSegmentStruct * oldsegment, * newsegment;
wxPoint pos = GetScreen()->m_Curseur;

	if ( GetScreen()->m_CurrentItem &&
		 (GetScreen()->m_CurrentItem->m_Flags == 0) )
		GetScreen()->m_CurrentItem = NULL;

	if ( GetScreen()->m_CurrentItem )
		{
		switch (GetScreen()->m_CurrentItem->m_StructType )
			{
			case DRAW_SEGMENT_STRUCT_TYPE:
			case DRAW_POLYLINE_STRUCT_TYPE:
				break;

			default:
				return;
			}
		}

	oldsegment = newsegment =
			(DrawSegmentStruct *) GetScreen()->m_CurrentItem;

	if (!newsegment)  /* 1er point : creation de la 1ere structure */
	{

		switch(type)
		{
			default:
				newsegment = new DrawSegmentStruct(pos, LAYER_NOTES);
				break;
			case LAYER_WIRE:
				newsegment = new DrawSegmentStruct(pos, LAYER_WIRE);
				if ( LocatePinEnd(GetScreen()->EEDrawList, pos) )
					newsegment->m_StartIsDangling = FALSE;
				break;
			case LAYER_BUS:
				newsegment = new DrawSegmentStruct(pos, LAYER_BUS);
				break;
		}

		newsegment->m_Flags = IS_NEW;
		GetScreen()->m_CurrentItem = newsegment;
		GetScreen()->ManageCurseur = Segment_in_Ghost;
		GetScreen()->ForceCloseManageCurseur = ExitTrace;
		g_ItemToRepeat = NULL;
	}

	else	/* Trace en cours: Placement d'un point supplementaire */
	{
		if( (oldsegment->m_Start.x == oldsegment->m_End.x) &&
			(oldsegment->m_Start.y == oldsegment->m_End.y) )	/* Structure inutile */
			return;
		GetScreen()->ManageCurseur(DrawPanel, DC, FALSE);
		oldsegment->m_EndIsDangling = FALSE;

		/* Creation du segment suivant ou fin de tracé si point sur pin, jonction ...*/
		if ( IsTerminalPoint(GetScreen(), oldsegment->m_End, oldsegment->m_Layer) )
		{
			EndSegment(DC); return;
		}

		/* Placement en liste generale */
		oldsegment->Pnext = GetScreen()->EEDrawList;
		g_ItemToRepeat = GetScreen()->EEDrawList = oldsegment;
		GetScreen()->CursorOff(DrawPanel, DC);	// Erase schematic cursor
		RedrawOneStruct(DrawPanel,DC, oldsegment, GR_DEFAULT_DRAWMODE);
		GetScreen()->CursorOn(DrawPanel, DC);	// Display schematic cursor

		/* Creation du segment suivant */
		newsegment = oldsegment->GenCopy();
		newsegment->m_Start = oldsegment->m_End;
		newsegment->m_End = pos;
		oldsegment->m_Flags = 0;
		newsegment->m_Flags = IS_NEW;
		GetScreen()->m_CurrentItem = newsegment;
		GetScreen()->ManageCurseur(DrawPanel, DC, FALSE);
		newsegment->m_StartIsDangling = FALSE;
		newsegment->m_EndIsDangling = TRUE;
	}
}


/*************************************************************/
/*	 Routine de fin de trace d'une struct segment (Wire, Bus */
/*************************************************************/
void WinEDA_SchematicFrame::EndSegment(wxDC *DC)
{
DrawSegmentStruct * segment = (DrawSegmentStruct *)GetScreen()->m_CurrentItem;

	if ( GetScreen()->ManageCurseur == NULL ) return;
	if ( segment == NULL ) return;
	if ( (segment->m_Flags & IS_NEW) == 0) return;

	if( (segment->m_Start.x == segment->m_End.x) &&
		(segment->m_Start.y == segment->m_End.y) )	/* Structure inutile */
	{
		EraseStruct(segment, (SCH_SCREEN*)GetScreen());
		segment = NULL;
	}

	else
	{		/* Placement en liste generale */
		GetScreen()->ManageCurseur(DrawPanel, DC, FALSE);
		segment->Pnext = GetScreen()->EEDrawList;
		g_ItemToRepeat = GetScreen()->EEDrawList = segment;
		segment->m_Flags = 0;
	}

	/* Fin de trace */
	GetScreen()->ManageCurseur = NULL;
	GetScreen()->ForceCloseManageCurseur = NULL;
	GetScreen()->m_CurrentItem = NULL;

	TestDanglingEnds(GetScreen()->EEDrawList, DC);
	SetFlagModify(GetScreen());
	if( segment )
	{
		GetScreen()->CursorOff(DrawPanel, DC);	// Erase schematic cursor
		RedrawOneStruct(DrawPanel,DC, segment, GR_DEFAULT_DRAWMODE);
		GetScreen()->CursorOn(DrawPanel, DC);	// Display schematic cursor
	}
}

/****************************************************************************/
static void Segment_in_Ghost(WinEDA_DrawPanel * panel, wxDC * DC, bool erase)
/****************************************************************************/
/*  Dessin du Segment Fantome lors des deplacements du curseur
*/
{
DrawSegmentStruct * segment =
		(DrawSegmentStruct *) panel->m_Parent->GetScreen()->m_CurrentItem;
wxPoint endpos;
int color;

	if ( segment == NULL ) return;

	color = ReturnLayerColor(segment->m_Layer) ^ HIGHT_LIGHT_FLAG;

	endpos = panel->m_Parent->GetScreen()->m_Curseur;

	if( g_HVLines )	/* Coerce the line to vertical or horizontal one: */
		{
		if (ABS(endpos.x - segment->m_Start.x) < ABS(endpos.y - segment->m_Start.y))
			endpos.x = segment->m_Start.x;
		else
			endpos.y = segment->m_Start.y;
		}

	if( erase )		// Redraw if segment lengtht != 0
	{
		if ( (segment->m_Start.x != segment->m_End.x) ||
			 (segment->m_Start.y != segment->m_End.y) )
		RedrawOneStruct(panel,DC, segment, XOR_MODE, color);
	}
	segment->m_End = endpos;
	// Redraw if segment lengtht != 0

	if ( (segment->m_Start.x != segment->m_End.x) ||
			(segment->m_Start.y != segment->m_End.y) )
	RedrawOneStruct(panel,DC, segment, XOR_MODE,color);
}

/*****************************************************************************/
static void Polyline_in_Ghost(WinEDA_DrawPanel * panel, wxDC * DC, bool erase)
/*****************************************************************************/
/*  Dessin du du Polyline Fantome lors des deplacements du curseur
*/
{
DrawPolylineStruct * NewPoly =
		(DrawPolylineStruct *)panel->m_Parent->GetScreen()->m_CurrentItem;
int color;
wxPoint endpos;

	endpos = panel->m_Parent->GetScreen()->m_Curseur;
	color = ReturnLayerColor(NewPoly->m_Layer);

	GRSetDrawMode(DC, XOR_MODE);

	if( g_HVLines )
		{
		/* Coerce the line to vertical or horizontal one: */
		if (ABS(endpos.x - NewPoly->m_Points[NewPoly->m_NumOfPoints * 2 - 2]) <
			 ABS(endpos.y - NewPoly->m_Points[NewPoly->m_NumOfPoints * 2 - 1]))
			endpos.x = NewPoly->m_Points[NewPoly->m_NumOfPoints * 2 - 2];
		else
			endpos.y = NewPoly->m_Points[NewPoly->m_NumOfPoints * 2 - 1];
		}

	NewPoly->m_NumOfPoints++;
	if( erase )
		RedrawOneStruct(panel,DC, NewPoly, XOR_MODE, color);

	NewPoly->m_Points[NewPoly->m_NumOfPoints * 2 - 2] = endpos.x;
	NewPoly->m_Points[NewPoly->m_NumOfPoints * 2 - 1] = endpos.y;
	RedrawOneStruct(panel,DC, NewPoly, XOR_MODE, color);
	NewPoly->m_NumOfPoints--;
}

/**********************************************************/
void WinEDA_SchematicFrame::DeleteCurrentSegment(wxDC * DC)
/**********************************************************/
/*
Routine effacant le dernier trait trace, ou l'element pointe par la souris
*/
{

	g_ItemToRepeat = NULL;

	if( (GetScreen()->m_CurrentItem == NULL) ||
		((GetScreen()->m_CurrentItem->m_Flags & IS_NEW) == 0) )
		{
		return;
		}

	/* Trace en cours: annulation */
	if (GetScreen()->m_CurrentItem->m_StructType == DRAW_POLYLINE_STRUCT_TYPE)
		{
		Polyline_in_Ghost(DrawPanel, DC, FALSE); /* Effacement du trace en cours */
		}

	else
		{
		Segment_in_Ghost(DrawPanel, DC, FALSE); /* Effacement du trace en cours */
		}

	EraseStruct(GetScreen()->m_CurrentItem, GetScreen());
	GetScreen()->ManageCurseur = NULL;
	GetScreen()->m_CurrentItem = NULL;
}


/***************************************************************************/
EDA_BaseStruct * WinEDA_SchematicFrame::CreateNewJunctionStruct(wxDC * DC)
/***************************************************************************/
/* Routine to create new connection struct.
*/
{
DrawJunctionStruct *NewConnect;

	NewConnect = new DrawJunctionStruct(GetScreen()->m_Curseur);

	g_ItemToRepeat = NewConnect;

	GetScreen()->CursorOff(DrawPanel, DC);	// Erase schematic cursor
	RedrawOneStruct(DrawPanel,DC, NewConnect, GR_DEFAULT_DRAWMODE);
	GetScreen()->CursorOn(DrawPanel, DC);	// Display schematic cursor

	NewConnect->Pnext = GetScreen()->EEDrawList;
	GetScreen()->EEDrawList = NewConnect;
	SetFlagModify(GetScreen());
	return(NewConnect);
}

/*************************************************************************/
EDA_BaseStruct *WinEDA_SchematicFrame::CreateNewNoConnectStruct(wxDC * DC)
/*************************************************************************/
/*Routine to create new NoConnect struct. ( Symbole de Non Connexion)
*/
{
DrawNoConnectStruct *NewNoConnect;

	NewNoConnect = new DrawNoConnectStruct(GetScreen()->m_Curseur);
	g_ItemToRepeat = NewNoConnect;

	GetScreen()->CursorOff(DrawPanel, DC);	// Erase schematic cursor
	RedrawOneStruct(DrawPanel,DC, NewNoConnect,  GR_DEFAULT_DRAWMODE);
	GetScreen()->CursorOn(DrawPanel, DC);	// Display schematic cursor

	NewNoConnect->Pnext = GetScreen()->EEDrawList;
	GetScreen()->EEDrawList = NewNoConnect;
	SetFlagModify(GetScreen());
	return(NewNoConnect);
}


/**********************************************************/
static void ExitTrace(WinEDA_DrawFrame * frame, wxDC * DC)
/**********************************************************/
/* Routine de sortie des menus de trace */
{
BASE_SCREEN * Screen = frame->GetScreen();

	if( Screen->m_CurrentItem)  /* trace en cours */
		{
		Screen->ManageCurseur(frame->DrawPanel, DC, FALSE);
		Screen->ManageCurseur = NULL;
		Screen->ForceCloseManageCurseur = NULL;
		EraseStruct(Screen->m_CurrentItem,(SCH_SCREEN*) Screen);
		Screen->m_CurrentItem = NULL;
		return;
		}

	else g_ItemToRepeat = NULL;	// Fin de commande generale
}


/***************************************************/
void WinEDA_SchematicFrame::RepeatDrawItem(wxDC *DC)
/***************************************************/
/* Routine de recopie du dernier element dessine
	Les elements duplicables sont
		fils, bus, traits, textes, labels
		Les labels termines par un nombre seront incrementes 
*/
{
char Line[256];
int ox = 0, oy = 0;

	if( g_ItemToRepeat == NULL ) return;

	switch( g_ItemToRepeat->m_StructType )
		{
		case DRAW_JUNCTION_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawJunctionStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Pos.x += g_RepeatStep.x; ox = STRUCT->m_Pos.x;
			STRUCT->m_Pos.y += g_RepeatStep.y; oy = STRUCT->m_Pos.y;
			break;

		case DRAW_NOCONNECT_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawNoConnectStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Pos.x += g_RepeatStep.x; ox = STRUCT->m_Pos.x;
			STRUCT->m_Pos.y += g_RepeatStep.y; oy = STRUCT->m_Pos.y;
			break;

		case DRAW_TEXT_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawTextStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Pos.x += g_RepeatStep.x; ox = STRUCT->m_Pos.x;
			STRUCT->m_Pos.y += g_RepeatStep.y; oy = STRUCT->m_Pos.y;
			/*** Increment du numero de label ***/
			strcpy(Line,STRUCT->GetText());
			IncrementLabelMember(Line);
			STRUCT->m_Text = Line;
			break;

 
		case DRAW_LABEL_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawLabelStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Pos.x += g_RepeatStep.x; ox = STRUCT->m_Pos.x;
			STRUCT->m_Pos.y += g_RepeatStep.y; oy = STRUCT->m_Pos.y;
			/*** Increment du numero de label ***/
			strcpy(Line,STRUCT->GetText());
			IncrementLabelMember(Line);
			STRUCT->m_Text = Line;
			break;


		case DRAW_GLOBAL_LABEL_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawGlobalLabelStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Pos.x += g_RepeatStep.x; ox = STRUCT->m_Pos.x;
			STRUCT->m_Pos.y += g_RepeatStep.y; oy = STRUCT->m_Pos.y;
			/*** Increment du numero de label ***/
			strcpy(Line,STRUCT->GetText());
			IncrementLabelMember(Line);
			STRUCT->m_Text = Line;
			break;

		case DRAW_SEGMENT_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawSegmentStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Start.x += g_RepeatStep.x; ox = STRUCT->m_Start.x;
			STRUCT->m_Start.y += g_RepeatStep.y; oy = STRUCT->m_Start.y;
			STRUCT->m_End.x += g_RepeatStep.x;
			STRUCT->m_End.y += g_RepeatStep.y;
			break;

		case DRAW_RACCORD_STRUCT_TYPE:
			#undef STRUCT
			#define STRUCT ((DrawRaccordStruct*) g_ItemToRepeat)
			g_ItemToRepeat = STRUCT->GenCopy();
			STRUCT->m_Pos.x += g_RepeatStep.x; ox = STRUCT->m_Pos.x;
			STRUCT->m_Pos.y += g_RepeatStep.y; oy = STRUCT->m_Pos.y;
			break;

		default:
			g_ItemToRepeat = NULL;
			DisplayError(this, "Repeat Type Error", 10);
			break;
		}

	if ( g_ItemToRepeat )
		{
		g_ItemToRepeat->Pnext = GetScreen()->EEDrawList;
		GetScreen()->EEDrawList = g_ItemToRepeat;
		TestDanglingEnds(GetScreen()->EEDrawList, NULL);
		RedrawOneStruct(DrawPanel,DC, g_ItemToRepeat, GR_DEFAULT_DRAWMODE);
//		GetScreen()->Curseur.x = ox; GetScreen()->Curseur.x = oy; 
//		GRMouseWarp(DrawPanel, DrawPanel->CursorScreenPosition() );
		}
}


/******************************************/
void IncrementLabelMember(char * Line)
/******************************************/
/* Routine incrementant les labels, c'est a dire pour les textes finissant
par un nombre, ajoutant <RepeatDeltaLabel> a ce nombre
*/
{
char * strnum;
int ii;

 	strnum = Line + strlen(Line) - 1;
	if( !isdigit(*strnum) ) return;

	while( (strnum >= Line) && isdigit(*strnum) ) strnum--;
	strnum++;	/* pointe le debut de la chaine des digits */
	ii = atoi(strnum) + g_RepeatDeltaLabel;
	sprintf(strnum, "%d", ii);
}

/***************************************************************************/
static bool IsTerminalPoint(SCH_SCREEN * screen, const wxPoint & pos, int layer)
/***************************************************************************/
/* Returne TRUE si pos est un point possible pour terminer automatiquement un
segment, c'est a dire pour
	- type WIRE, si il y a
		- une jonction
		- ou une pin
		- ou une extrémité unique de fil

	- type BUS, si il y a
		- ou une extrémité unique de BUS
*/
{
EDA_BaseStruct * item;
LibDrawPin * pin;
DrawLibItemStruct * LibItem = NULL;
DrawSheetLabelStruct * pinsheet;
wxPoint itempos;

	switch ( layer )
		{
		case LAYER_BUS:
			item = PickStruct(screen, BUSITEM);
			if ( item ) return TRUE;
			pinsheet = LocateAnyPinSheet(pos, screen->EEDrawList );
			if ( pinsheet && IsBusLabel(pinsheet->GetText()) )
				{
				itempos = pinsheet->m_Pos;
				if ( (itempos.x == pos.x) && (itempos.y == pos.y) )	return TRUE;
				}
			break;

		case LAYER_NOTES:
			item = PickStruct(screen, DRAWITEM);
			if ( item )
				return TRUE;
			break;

		case LAYER_WIRE:
			item = PickStruct(screen, RACCORDITEM |JUNCTIONITEM);
			if ( item ) return TRUE;

			pin = LocateAnyPin( screen->EEDrawList, pos, &LibItem );
			if ( pin && LibItem )
				{
				// calcul de la position exacte du point de connexion de la pin,
				// selon orientation du composant:
				itempos = LibItem->GetScreenCoord(pin->m_Pos);
				itempos.x += LibItem->m_Pos.x;
				itempos.y += LibItem->m_Pos.y;
				if ( (itempos.x == pos.x) && (itempos.y == pos.y) )	return TRUE;
				}

			item = PickStruct(screen, WIREITEM);
			if ( item ) return TRUE;

			item = PickStruct(screen, LABELITEM);
			if ( item && (item->m_StructType != DRAW_TEXT_STRUCT_TYPE) &&
				( ((DrawGlobalLabelStruct*)item)->m_Pos.x == pos.x) &&
				( ((DrawGlobalLabelStruct*)item)->m_Pos.y == pos.y) )
					return TRUE;

			pinsheet = LocateAnyPinSheet( pos, screen->EEDrawList );
			if ( pinsheet && ! IsBusLabel(pinsheet->GetText()) )
				{
				itempos = pinsheet->m_Pos;
				if ( (itempos.x == pos.x) && (itempos.y == pos.y) )	return TRUE;
				}

			break;

		default:
			break;
		}

	return FALSE;
}


