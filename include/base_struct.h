/*********************************************************************/
/*	base_struct.h :  Basic classes for most kicad item descriptions  */
/*********************************************************************/

#ifndef BASE_STRUCT_H
#define BASE_STRUCT_H


/* Id for class identification, at run time */
enum DrawStructureType {
	TYPE_NOT_INIT = 0,
	TYPEPCB,
	// Items in pcb
	PCB_EQUIPOT_STRUCT_TYPE,
	TYPEMODULE,
	TYPEPAD,
	TYPEDRAWSEGMENT,
	TYPETEXTE,
	TYPETEXTEMODULE,
	TYPEEDGEMODULE,
	TYPETRACK,
	TYPEZONE,
	TYPEVIA,
	TYPEMARQUEUR,
	TYPECOTATION,
	TYPEMIRE,
	TYPESCREEN,
	TYPEBLOCK,
	TYPEEDGEZONE,
	// Draw Items in schematic
	DRAW_POLYLINE_STRUCT_TYPE,
	DRAW_JUNCTION_STRUCT_TYPE,
	DRAW_TEXT_STRUCT_TYPE,
	DRAW_LABEL_STRUCT_TYPE,
	DRAW_GLOBAL_LABEL_STRUCT_TYPE,
	DRAW_LIB_ITEM_STRUCT_TYPE,
	DRAW_PICK_ITEM_STRUCT_TYPE,
	DRAW_SEGMENT_STRUCT_TYPE,
	DRAW_BUSENTRY_STRUCT_TYPE,
	DRAW_SHEET_STRUCT_TYPE,
	DRAW_SHEETLABEL_STRUCT_TYPE,
	DRAW_MARKER_STRUCT_TYPE,
	DRAW_NOCONNECT_STRUCT_TYPE,
	DRAW_PART_TEXT_STRUCT_TYPE,
	// General
	SCREEN_STRUCT_TYPE,
	BLOCK_LOCATE_STRUCT_TYPE,
	// Draw Items in library component
	LIBCOMPONENT_STRUCT_TYPE,
	COMPONENT_ARC_DRAW_TYPE,
	COMPONENT_CIRCLE_DRAW_TYPE,
	COMPONENT_GRAPHIC_TEXT_DRAW_TYPE,
	COMPONENT_RECT_DRAW_TYPE,
	COMPONENT_POLYLINE_DRAW_TYPE,
	COMPONENT_LINE_DRAW_TYPE,
	COMPONENT_PIN_DRAW_TYPE,
	COMPONENT_FIELD_DRAW_TYPE,
	// End value
	MAX_STRUCT_TYPE_ID
};


/********************************************************************/
/* Classes de base: servent a deriver les classes reellement utiles */
/********************************************************************/

class EDA_BaseStruct		/* Basic class, not directly used */
{
public:
	int m_StructType;			/* Struct ident for run time identification */
	EDA_BaseStruct * Pnext;		/* Linked list: Link (next struct) */
	EDA_BaseStruct * Pback;		/* Linked list: Link (previous struct) */
	EDA_BaseStruct * m_Parent;	/* Linked list: Link (parent struct) */
	EDA_BaseStruct *m_Son;		/* Linked list: Link (son struct) */
	EDA_BaseStruct *m_Image;	/* Link to an image copy for undelete or abort command */
	int m_Flags;				// flags for editions and other
	unsigned long m_TimeStamp;			// Time stamp used for logical links
	int m_Selected;				/* Used by block commands, and selective editing */

private:
	int m_Status;

private:
	void InitVars(void);

public:

	EDA_BaseStruct(EDA_BaseStruct * parent, int idType);
	EDA_BaseStruct(int struct_type);
	virtual ~EDA_BaseStruct() {};
	EDA_BaseStruct * Next(void) { return Pnext; }
	/* Gestion de l'etat (status) de la structure (active, deleted..) */
	int GetState(int type);
	void SetState(int type, int state );
	int ReturnStatus(void) const
		{
		return(m_Status);
		}
	void SetStatus(int new_status)
		{
		m_Status = new_status;
		}
	wxString ReturnClassName(void);
	/* addition d'une nouvelle struct a la liste chainée */
	void AddToChain(EDA_BaseStruct * laststruct);
	/* fonction de placement */
	virtual void Place(WinEDA_DrawFrame * frame, wxDC * DC);
	virtual void Draw(WinEDA_DrawPanel * panel, wxDC * DC, const wxPoint & offset, int draw_mode, int Color = -1);

};

// Text justify:
// Values -1,0,1 are used in computations, do not change them
typedef enum {
	GR_TEXT_HJUSTIFY_LEFT = -1,
	GR_TEXT_HJUSTIFY_CENTER = 0,
	GR_TEXT_HJUSTIFY_RIGHT = 1
} GRTextHorizJustifyType;

typedef enum {
	GR_TEXT_VJUSTIFY_TOP = -1,
	GR_TEXT_VJUSTIFY_CENTER = 0,
	GR_TEXT_VJUSTIFY_BOTTOM = 1
} GRTextVertJustifyType;

/* controle des remplissages a l'ecran (Segments textes...)*/
#define FILAIRE  0
#define FILLED  1
#define SKETCH  2


#define DEFAULT_SIZE_TEXT 60		/* Hauteur (en 1/000" par defaut des textes */

/* classe de gestion des textes (labels, textes composants ..)
	(Non utilisee seule) */
class EDA_TextStruct
{
public:
	wxString m_Text;				/* text! */
	int m_Layer;					/* couche d'appartenance */
	wxPoint m_Pos;					/* XY position of anchor text. */
	wxSize m_Size;					/* XY size of text */
	int m_Width;					/* epaisseur du trait */
	int m_Orient;                   /* Orient in 0.1 degrees */
	int m_Miroir;					// Display Normal / mirror
	int m_Attributs;				/* controle visibilite */
	int m_CharType;					/* normal, bold, italic ... */
	int m_HJustify, m_VJustify;		/* Justifications Horiz et Vert du texte */
	int m_ZoomLevelDrawable;		/* Niveau de zoom acceptable pour affichage normal */
	int * m_TextDrawings;			/* pointeur sur la liste des segments de dessin */
	int m_TextDrawingsSize;			/* nombre de segments a dessiner */


public:
	EDA_TextStruct(const wxString & text = wxEmptyString);
	virtual ~EDA_TextStruct(void);
	void CreateDrawData(void);
	int GetLength(void) { return m_Text.Length(); };
	int Pitch(void);	/* retourne le pas entre 2 caracteres */
	void Draw(WinEDA_DrawPanel * panel, wxDC * DC,
				const wxPoint & offset, int color,
				int draw_mode, int display_mode = FILAIRE, int anchor_color = -1);
	/* locate functions */
	int Locate(const wxPoint & posref);
	int Len_Size(void);	// Return the text lenght in internal units
};



/* Basic class for build items like lines, which have 1 start point and 1 end point.
   Arc and circles can use this class.
*/
class EDA_BaseLineStruct: public EDA_BaseStruct
{
public:
	int m_Layer;				// Layer number
	int m_Width;                // 0 = line, > 0 = tracks, bus ...
	wxPoint m_Start;			// Line start point
	wxPoint m_End;				// Line end point

public:
	EDA_BaseLineStruct(EDA_BaseStruct * StructFather, DrawStructureType idtype);
};


	/**************************/
	/* class DrawPickedStruct */
	/**************************/

/* Class to hold structures picked by pick events (like block selection)
	This class has only one useful member: .m_PickedStruct, used as a link.
	It does not describe really an item.
	It is used to create a linked list of selected items (in block selection).
	Each DrawPickedStruct item has is member: .m_PickedStruct pointing the
	real selected item
*/
class DrawPickedStruct: public EDA_BaseStruct
{
public:
	EDA_BaseStruct *m_PickedStruct;

public:
	DrawPickedStruct(EDA_BaseStruct *pickedstruct = NULL);
	~DrawPickedStruct(void);
	void Place(WinEDA_DrawFrame * frame, wxDC * DC) {};
	void DeleteWrapperList(void);
	DrawPickedStruct * Next(void) { return (DrawPickedStruct*) Pnext; }
};


/* class to handle component boundary box.
	This class is similar to wxRect, but some wxRect functions are very curious,
	so I prefer this suitable class
*/
class EDA_Rect
{
public:
	wxPoint m_Pos;		// Rectangle Origin
	wxSize m_Size;		// Rectangle Size 

public:
	EDA_Rect(void) {};
	wxPoint Centre(void)
	{
		return wxPoint(m_Pos.x + (m_Size.x>>1), m_Pos.y + (m_Size.y>>1) );
	}

	void Normalize(void);	// Ensure the height ant width are >= 0
	bool Inside(const wxPoint & point);	// Return TRUE if point is in Rect
	bool Inside(int x, int y) { return Inside(wxPoint(x,y)); }
	wxSize GetSize(void) { return m_Size; }
	int GetX(void) { return m_Pos.x;}
	int GetY(void) { return m_Pos.y;}
	wxPoint GetOrigin(void) { return m_Pos;}
	wxPoint GetPosition(void) { return m_Pos;}
	wxPoint GetEnd(void) { return wxPoint(GetRight(),GetBottom());}
	int GetWidth(void) { return m_Size.x; }
	int GetHeight(void) { return m_Size.y; }
	int GetRight(void) {return m_Pos.x + m_Size.x; }
	int GetBottom(void) {return m_Pos.y + m_Size.y; }
	void SetOrigin(const wxPoint & pos) { m_Pos = pos; }
	void SetOrigin(int x, int y) { m_Pos.x = x; m_Pos.y = y; }
	void SetSize(const wxSize & size) { m_Size = size; }
	void SetSize(int w, int h) { m_Size.x = w; m_Size.y = h; }
	void Offset(int dx, int dy) { m_Pos.x += dx; m_Pos.y += dy; }
	void Offset(const wxPoint & offset) { m_Pos.x += offset.x; m_Pos.y += offset.y; }
	void SetX(int val) { m_Pos.x = val; }
	void SetY(int val) { m_Pos.y = val; }
	void SetWidth(int val) { m_Size.x = val; }
	void SetHeight(int val) { m_Size.y = val; }
	void SetEnd(const wxPoint & pos)
	{
		m_Size.x = pos.x - m_Pos.x; m_Size.y = pos.y - m_Pos.y;
	}
	EDA_Rect& Inflate(wxCoord dx, wxCoord dy);
};

#endif /* BASE_STRUCT_H */
