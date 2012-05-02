/************************************************/
/*	Local definitions of the EELibs?.c modules.	*/
/************************************************/

#ifndef EELIBSL_H
#define EELIBSL_H

#ifndef global
#define global extern
#endif

#define DEFAULT_SIZE_TEXT 60		/* Hauteur (en 1/000" par defaut des textes */

#define BooleanType char

// TODO get rid of PART_NAME_LEN, use char * && strncpy
#define PART_NAME_LEN	64		/* Maximum length of part name. */
#define PREFIX_NAME_LEN	5		/* Maximum length of prefix (IC, R, SW etc.). */
#define FOOT_NAME_LEN	40		/* Maximum length of footprint name. */
#define MFG_NAME_LEN	30		/* Maximum length of manufacture name. */
#define MFG_PART_LEN	40		/* Maximum length of manf part name. */
#define PIN_SEPERATOR	"\n"	/* See Pins in LibraryEntryStruct. */
#define FILE_IDENT "EESchema-LIBRARY Version 2.0"  /* Must be at the lib file start. */
#define PIN_WIDTH	100			   /* Width between 2 pins in internal units. */
#define PIN_LENGTH	300			/* Default Length of each pin to be drawn. */

#define INVERT_PIN_RADIUS 40			/* Radius of inverted pin circle. */
#define CLOCK_PIN_DIM 40				/* Dim of clock pin symbol. */
#define IEEE_SYMBOL_PIN_DIM 40			/* Dim of special pin symbol. */

/* Normalize angle to be in the 0..360 range: */
#define	NORMALIZE_ANGLE(Angle)	{ while (Angle < 0) Angle += 3600; \
				  while (Angle > 3600) Angle -= 3600; }

/* definition des types des structures d'elements de librairie */
typedef enum {
	ROOT,		/* La structure est du type LibraryEntryStruct */
	ALIAS		/* La structure est un alias */
	} LibrEntryType;

/* Definitions des Pins */

typedef enum {		/* Type des Pins */
	PIN_INPUT,
	PIN_OUTPUT,
	PIN_BIDI,
	PIN_TRISTATE,
	PIN_PASSIVE,
	PIN_UNSPECIFIED,
	PIN_POWER,
	PIN_OPENCOLLECTOR,
	PIN_OPENEMITTER
} ElectricPinType;

/* Autres bits: bits du membre .Flag des Pins */
#define PINNOTDRAW 1		/* si 1: pin invisible */

typedef enum {		/* Forme des Pins */
	NONE = 0,
	INVERT = 1,
	CLOCK = 2,
	LOWLEVEL_IN = 4,
	LOWLEVEL_OUT = 8
	} DrawPinShape;

typedef enum {			/* Orientation des Pins */
	PIN_N     = 'N',  // normal, no rotation
	PIN_RIGHT = 'R',
	PIN_LEFT  = 'L',
	PIN_UP    = 'U',
	PIN_DOWN  = 'D',
	} DrawPinOrient;

typedef enum
	{
	REFERENCE = 0,		/* Champ Reference of part, i.e. "IC21" */
	VALUE,			/* Champ Value of part, i.e. "3.3K" */
	FIELD1,
	FIELD2,
	FIELD3,
	FIELD4,
	FIELD5,
	FIELD6,
	FIELD7,
	FIELD8,
	MODULE_PCB,		/* Champ Name Module PCB, i.e. "16DIP300" */
	SHEET_NAME		/* Champ Name Schema componant, i.e. "cnt16.sch" */
	} NumFieldType;

typedef struct LibraryFieldEntry	/* Fields auxiliaires identiques aux fields
						des composants, pouvant etre predefinis en lib */
	{
	int StructType;
	int FieldId;			/* 1 a 11, mais usuellement MODULE_PCB et SHEET_PART */
	int PosX, PosY, Size;
	int Orient;			 /* Orientation */
	int Flags;			 /* Attributs (Non visible ...) */
	char *Text;			 /* Pointeur sur le texte */
	struct LibraryFieldEntry *nxt;
	}LibraryFieldEntry;

/* Structures de dessin des composants : */

typedef struct LibraryAliasStruct {
	LibrEntryType Type;						/* Type = ALIAS pour cette struct */
	char Name[PART_NAME_LEN + 1];			/* Alias Part name. */
	char RootName[PART_NAME_LEN + 1];		/* Part name pour le composant de reference */
	struct LibraryEntryStruct * RootEntry;	/* Pointeur sur le composant de ref */
} LibraryAliasStruct;

typedef enum {
	ARC_DRAW_TYPE,
	CIRCLE_DRAW_TYPE,
	TEXT_DRAW_TYPE,
	SQUARE_DRAW_TYPE,
	LINE_DRAW_TYPE,
	POLYLINE_DRAW_TYPE,
	SEGMENT_DRAW_TYPE,
	PIN_DRAW_TYPE
} DrawStructType;

typedef struct LibraryDrawArc {
	int x, y, r, t1, t2, width;
	} LibraryDrawArc;

typedef struct LibraryDrawCircle {
	int x, y, r, width;
} LibraryDrawCircle;

typedef struct LibraryDrawText {
	int Horiz, x, y, size, type;
	char *Text;
} LibraryDrawText;

typedef struct LibraryDrawSquare {
	int x1, y1, x2, y2, width;
} LibraryDrawSquare;

typedef struct LibraryDrawPolyline {
	int n, *PolyList;
	BooleanType Fill;
	int width;
} LibraryDrawPolyline;

typedef struct LibraryDrawSegment {
	int x1, y1, x2, y2, width;
} LibraryDrawSegment;

typedef struct LibraryDrawPin {
	int posX, posY;			/* Position du point de reference de la Pin */
	short Len;			/* longueur de la Pin */
	short Orient;			/* Orientation de la Pin (Up, Down, Left, Right) */
	short PinShape;			/* Bit a bit: forme de la pin (voir enum prec) */
	char PinType;			/* type electrique de la pin */
	char Flags;			/* bit 0 != 0: pin invisible */
	char  Num[5];			/* numero / ref grid array, si .Num < 0  4 chars */
	char * Name;
	char * ReName;
	short SizeNum, SizeName;	/* taille des num pin et name pin */
	} LibraryDrawPin;

typedef struct LibraryDrawEntryStruct {
	int DrawType;
	short Unit;			/* identification de l'unite */
	short Convert;			/* identification de la forme en multiples rep. */
	union {
	LibraryDrawArc Arc;
	LibraryDrawCircle Circ;
	LibraryDrawText Text;
	LibraryDrawSquare Sqr;
	LibraryDrawPolyline Poly;
	LibraryDrawSegment Segm;
	LibraryDrawPin Pin;
	} U;
	struct LibraryDrawEntryStruct *nxt;
} LibraryDrawEntryStruct;

typedef struct LibraryEntryStruct {
	LibrEntryType Type;						/* Type = ROOT;
							   					    = ALIAS pour struct LibraryAliasType */
	char Name[PART_NAME_LEN + 1];			/* Part name. */
	char Prefix[PREFIX_NAME_LEN + 1];		/* Prefix ( U, IC ... ) */
	char * AliasList;						/* Pointeur sur la liste des ALIAS de ce composant */
	int NamePosX,   NamePosY,   NameOrient,   NameSize;
	int PrefixPosX, PrefixPosY, PrefixOrient, PrefixSize;
	int NumOfUnits;
	int TextInside;
	BooleanType DrawPinNum, DrawPinName, DrawName, DrawPrefix;
	LibraryFieldEntry *Fields;			/* Liste des Champs auxiliaires */
	LibraryDrawEntryStruct *Drawings;		/* How to draw this part */
	int BBoxMinX, BBoxMaxX, BBoxMinY, BBoxMaxY;	/* BBox around the part. */
	struct LibraryEntryStruct *nxt;
} LibraryEntryStruct;

typedef struct LibraryStruct {
	char 		*Name;					/* Name of library loaded. */
	int			isSheet;
	int 		NumOfParts;				/* Number of parts this library has. */
	struct LibraryEntryStruct *Entries;	/* Parts themselves are saved here. */
	struct LibraryStruct *nxt;			/* Point on next lib in chain. */
} LibraryStruct;

global LibraryStruct          	*Libs,  *CurrentLib, *DesignName, *LSptr;
global LibraryEntryStruct 	*LibEntry, *LEptr;
global LibraryDrawEntryStruct 	*Drawing, *LDptr;
global LibraryFieldEntry	*InsEntry;
global char 			 schName[50];

#endif // EELIBSL_H

/*
ViewRef :       VIEWREF ViewNameRef _ViewRef PopC
                {
                $$=$2; if(bug>2)fprintf(Error,"ViewRef: %25s ", $3);
                iptr = (struct inst *)Malloc(sizeof (struct inst));
                iptr->sym = $3;
                iptr->nxt = insts;
                insts = iptr;
                }
*/
