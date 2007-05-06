	/***************************************************/
	/* WORKSHEET.H: constantes pour trace du cartouche */
	/***************************************************/

	/****************************/
	/* Description du cartouche */
	/****************************/
#define GRID_REF_W 70	/* hauteur de la bande de reference grille */
#define SIZETEXT 60		/* Dimension des textes du cartouche */
#define SIZETEXT_REF 50	/* Dimension des lettres du marquage des reperes */
#define PAS_REF 2000	/* pas des marquages de reference des reperes */
#define TEXT_VTAB_HEIGHT SIZETEXT*2

/* Les coord ci dessous sont relatives au coin bas - droit de la feuille, et
seront soustraires de cette origine
*/
#define BLOCK_OX	4200
#define BLOCK_LICENCE_X BLOCK_OX - SIZETEXT
#define BLOCK_LICENCE_Y SIZETEXT
#define BLOCK_REV_X 820
#define BLOCK_REV_Y (SIZETEXT*3)
#define BLOCK_DATE_X BLOCK_OX - (SIZETEXT*15)
#define BLOCK_DATE_Y (SIZETEXT*3)
#define BLOCK_ID_SHEET_X 820
#define BLOCK_ID_SHEET_Y SIZETEXT
#define BLOCK_SIZE_SHEET_X BLOCK_OX - SIZETEXT
#define BLOCK_SIZE_SHEET_Y (SIZETEXT*3)
#define BLOCK_TITLE_X BLOCK_OX - SIZETEXT
#define BLOCK_TITLE_Y (SIZETEXT*5)
#define BLOCK_COMMENT_X BLOCK_OX - SIZETEXT
#define VARIABLE_BLOCK_START_POSITION (SIZETEXT * 6)
#define BLOCK_COMPANY_Y (SIZETEXT*7)
#define BLOCK_COMMENT1_Y (SIZETEXT*9)
#define BLOCK_COMMENT2_Y (SIZETEXT*11)
#define BLOCK_COMMENT3_Y (SIZETEXT*13)
#define BLOCK_COMMENT4_Y (SIZETEXT*15)

struct Ki_WorkSheetData
{
public:
	int m_Type;				/* nombre permettant de reconnaitre la description */
	Ki_WorkSheetData * Pnext;
	int m_Posx, m_Posy;		/* position de l'element ou point de depart du segment */
	int m_Endx, m_Endy;		/* extremite d'un element type segment ou cadre */
	wxChar * m_Legende;		/* Pour m_Textes: texte a afficher avant le texte lui meme */
	wxChar * m_Text;		/* Pour m_Textes:pointeur sur le texte a afficher */
};

/* Type des descriptions Ki_WorkSheetData */
enum TypeKi_WorkSheetData
{
	WS_DATE,
	WS_REV,
	WS_LICENCE,
	WS_SIZESHEET,
	WS_IDENTSHEET,
	WS_TITLE,
	WS_COMPANY_NAME,
	WS_COMMENT1,
	WS_COMMENT2,
	WS_COMMENT3,
	WS_COMMENT4,
	WS_SEGMENT,
	WS_UPPER_SEGMENT,
	WS_LEFT_SEGMENT,
	WS_CADRE
};

extern Ki_WorkSheetData WS_Date;
extern Ki_WorkSheetData WS_Revision;
extern Ki_WorkSheetData WS_Licence;
extern Ki_WorkSheetData WS_SizeSheet;
extern Ki_WorkSheetData WS_IdentSheet;
extern Ki_WorkSheetData WS_Title;
extern Ki_WorkSheetData WS_Company;
extern Ki_WorkSheetData WS_Comment1;
extern Ki_WorkSheetData WS_Comment2;
extern Ki_WorkSheetData WS_Comment3;
extern Ki_WorkSheetData WS_Comment4;
extern Ki_WorkSheetData WS_MostLeftLine;
extern Ki_WorkSheetData WS_MostUpperLine;
extern Ki_WorkSheetData WS_Segm3;
extern Ki_WorkSheetData WS_Segm4;
extern Ki_WorkSheetData WS_Segm5;
extern Ki_WorkSheetData WS_Segm6;
extern Ki_WorkSheetData WS_Segm7;

#ifdef EDA_BASE

Ki_WorkSheetData WS_Date =
	{
	WS_DATE,
	&WS_Licence,
	BLOCK_DATE_X, BLOCK_DATE_Y,
	0,0,
	wxT("Date: "), NULL
	};

Ki_WorkSheetData WS_Licence =
	{
	WS_LICENCE,
	&WS_Revision,
	BLOCK_LICENCE_X, BLOCK_LICENCE_Y,
	0,0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Revision =
	{
	WS_REV,
	&WS_SizeSheet,
	BLOCK_REV_X, BLOCK_REV_Y,
	0,0,
	wxT("Rev: "), NULL
	};

Ki_WorkSheetData WS_SizeSheet =
	{
	WS_SIZESHEET,
	&WS_IdentSheet,
	BLOCK_SIZE_SHEET_X, BLOCK_SIZE_SHEET_Y,
	0,0,
	wxT("Size: "),NULL
	};

Ki_WorkSheetData WS_IdentSheet =
	{
	WS_IDENTSHEET,
	&WS_Title,
	BLOCK_ID_SHEET_X, BLOCK_ID_SHEET_Y,
	0,0,
	wxT("Sheet: "),NULL
	};

Ki_WorkSheetData WS_Title =
	{
	WS_TITLE,
	&WS_Company,
	BLOCK_TITLE_X, BLOCK_TITLE_Y,
	0,0,
	wxT("Title: "),NULL
	};

Ki_WorkSheetData WS_Company =
	{
	WS_COMPANY_NAME,
	&WS_Comment1,
	BLOCK_COMMENT_X, BLOCK_COMPANY_Y,
	0,0,
	NULL,NULL,
	};

Ki_WorkSheetData WS_Comment1 =
	{
	WS_COMMENT1,
	&WS_Comment2,
	BLOCK_COMMENT_X, BLOCK_COMMENT1_Y,
	0,0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment2 =
	{
	WS_COMMENT2,
	&WS_Comment3,
	BLOCK_COMMENT_X, BLOCK_COMMENT2_Y,
	0,0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment3 =
	{
	WS_COMMENT3,
	&WS_Comment4,
	BLOCK_COMMENT_X, BLOCK_COMMENT3_Y,
	0,0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment4 =
	{
	WS_COMMENT4,
	&WS_MostLeftLine,
	BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
	0,0,
	NULL,NULL
	};

Ki_WorkSheetData WS_MostLeftLine =   /* segment vertical gauche */
	{
	WS_LEFT_SEGMENT,
	&WS_MostUpperLine,
	BLOCK_OX, SIZETEXT * 16,
	BLOCK_OX, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_MostUpperLine =	/* segment horizontal superieur */
	{
	WS_UPPER_SEGMENT,
	&WS_Segm3,
	BLOCK_OX, SIZETEXT * 16,
	0, SIZETEXT * 16,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm3 =		/* segment horizontal au dessus de NAME COMPANY */
	{
	WS_SEGMENT,
	&WS_Segm4,
	BLOCK_OX, SIZETEXT * 6,
	0, SIZETEXT * 6,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm4 =		/* segment vertical a gauche de SHEET et REV */
	{
	WS_SEGMENT,
	&WS_Segm5,
	BLOCK_REV_X + SIZETEXT, SIZETEXT * 4,
	BLOCK_REV_X + SIZETEXT, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm5 =		/* 1er segment horizontal */
	{
	WS_SEGMENT,
	&WS_Segm6,
	BLOCK_OX, SIZETEXT * 2,
	0, SIZETEXT * 2,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm6 =		/* 2eme segment horizontal */
	{
	WS_SEGMENT,
	&WS_Segm7,
	BLOCK_OX, SIZETEXT * 4,
	0, SIZETEXT * 4,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm7 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	NULL,
	BLOCK_OX - (SIZETEXT * 11), SIZETEXT * 4,
	BLOCK_OX - (SIZETEXT * 11), SIZETEXT * 2,
	NULL,NULL
	};

#endif
