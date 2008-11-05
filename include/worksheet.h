	/***************************************************/
	/* WORKSHEET.H: constantes pour trace du cartouche */
	/***************************************************/

	/****************************/
	/* Description du cartouche */
	/****************************/

/* Values are in 1/1000 inch */

#define GRID_REF_W 70	/* hauteur de la bande de reference grille */
#define SIZETEXT 60		/* Dimension des textes du cartouche */
#define SIZETEXT_REF 50	/* Dimension des lettres du marquage des reperes */
#define PAS_REF 2000	/* pas des marquages de reference des reperes */
#define TEXT_VTAB_HEIGHT SIZETEXT*2

#if defined(KICAD_GOST)
/* Shtamp */
#define STAMP_OX	185 * 10000 / 254
#define STAMP_OY	 55 * 10000 / 254

#define STAMP_Y_0	0
#define STAMP_Y_5	5 * 10000 / 254
#define STAMP_Y_8	8 * 10000 / 254
#define STAMP_Y_7	7 * 10000 / 254
#define STAMP_Y_10	10 * 10000 / 254
#define STAMP_Y_14	14 * 10000 / 254
#define STAMP_Y_15	15 * 10000 / 254
#define STAMP_Y_20	20 * 10000 / 254
#define STAMP_Y_25	25 * 10000 / 254
#define STAMP_Y_30	30 * 10000 / 254
#define STAMP_Y_35	35 * 10000 / 254
#define STAMP_Y_40	40 * 10000 / 254
#define STAMP_Y_45	45 * 10000 / 254
#define STAMP_Y_50	50 * 10000 / 254
#define STAMP_Y_55	55 * 10000 / 254

#define STAMP_X_0	0
#define STAMP_X_10	10 * 10000 / 254
#define STAMP_X_14	14 * 10000 / 254
#define STAMP_X_18	18 * 10000 / 254
#define STAMP_X_30	30 * 10000 / 254
#define STAMP_X_35	35 * 10000 / 254
#define STAMP_X_40	40 * 10000 / 254
#define STAMP_X_45	45 * 10000 / 254
#define STAMP_X_50	50 * 10000 / 254
#define STAMP_X_53	53 * 10000 / 254
#define STAMP_X_70	70 * 10000 / 254
#define STAMP_X_84	84 * 10000 / 254
#define STAMP_X_120	120 * 10000 / 254
#define STAMP_X_130	130 * 10000 / 254
#define STAMP_X_137	137 * 10000 / 254
#define STAMP_X_145	145 * 10000 / 254
#define STAMP_X_168	168 * 10000 / 254
#define STAMP_X_178	178 * 10000 / 254
#define STAMP_X_185	185 * 10000 / 254

#define STAMP_5		5 * 10000 / 254
#define STAMP_7		7 * 10000 / 254
#define STAMP_12	12 * 10000 / 254

#define STAMP_145	145 * 10000 / 254
#define STAMP_110	110 * 10000 / 254
#define STAMP_85	85 * 10000 / 254
#define STAMP_60	60 * 10000 / 254
#define STAMP_25	25 * 10000 / 254
#endif

/* Les coord ci dessous sont relatives au coin bas - droit de la feuille, et
seront soustraires de cette origine
*/
#define BLOCK_OX	4200
#define BLOCK_KICAD_VERSION_X BLOCK_OX - SIZETEXT
#define BLOCK_KICAD_VERSION_Y SIZETEXT
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
#define BLOCK_FULLSHEETNAME_X BLOCK_OX - SIZETEXT
#define BLOCK_FULLSHEETNAME_Y (SIZETEXT * 7)
#define BLOCK_FILENAME_X BLOCK_OX - SIZETEXT
#define BLOCK_FILENAME_Y (SIZETEXT * 9)
#define BLOCK_COMMENT_X BLOCK_OX - SIZETEXT
#define VARIABLE_BLOCK_START_POSITION (SIZETEXT * 10)
#define BLOCK_COMPANY_Y (SIZETEXT*11)
#define BLOCK_COMMENT1_Y (SIZETEXT*13)
#define BLOCK_COMMENT2_Y (SIZETEXT*15)
#define BLOCK_COMMENT3_Y (SIZETEXT*17)
#define BLOCK_COMMENT4_Y (SIZETEXT*19)

struct Ki_WorkSheetData
{
public:
	int m_Type;				/* nombre permettant de reconnaitre la description */
	Ki_WorkSheetData * Pnext;
	int m_Posx, m_Posy;		/* position de l'element ou point de depart du segment */
	int m_Endx, m_Endy;		/* extremite d'un element type segment ou cadre */
	const wxChar * m_Legende;		/* Pour m_Textes: texte a afficher avant le texte lui meme */
	const wxChar * m_Text;		/* Pour m_Textes:pointeur sur le texte a afficher */
};

/* Type des descriptions Ki_WorkSheetData */
enum TypeKi_WorkSheetData
{
	WS_DATE,
	WS_REV,
	WS_KICAD_VERSION,
#if defined(KICAD_GOST)
	WS_PODPIS,
#endif
	WS_SIZESHEET,
	WS_IDENTSHEET,
#if defined(KICAD_GOST)
	WS_SHEETS,
#endif
	WS_TITLE,
	WS_FILENAME,
	WS_FULLSHEETNAME,
	WS_COMPANY_NAME,
	WS_COMMENT1,
	WS_COMMENT2,
	WS_COMMENT3,
	WS_COMMENT4,
	WS_SEGMENT,
	WS_UPPER_SEGMENT,
	WS_LEFT_SEGMENT,
#if defined(KICAD_GOST)
	WS_CADRE,
	WS_LEFT_SEGMENT_D,
	WS_SEGMENT_D,
	WS_PODPIS_D,
	WS_IDENTSHEET_D,
	WS_SEGMENT_LU,
	WS_SEGMENT_LT,
	WS_PODPIS_LU
#else
	WS_CADRE
#endif
};

extern Ki_WorkSheetData WS_Date;
extern Ki_WorkSheetData WS_Revision;
extern Ki_WorkSheetData WS_Licence;
extern Ki_WorkSheetData WS_SizeSheet;
extern Ki_WorkSheetData WS_IdentSheet;
extern Ki_WorkSheetData WS_FullSheetName;
extern Ki_WorkSheetData WS_SheetFilename;
extern Ki_WorkSheetData WS_Title;
extern Ki_WorkSheetData WS_Company;
extern Ki_WorkSheetData WS_Comment1;
extern Ki_WorkSheetData WS_Comment2;
extern Ki_WorkSheetData WS_Comment3;
extern Ki_WorkSheetData WS_Comment4;
extern Ki_WorkSheetData WS_SeparatorLine;
extern Ki_WorkSheetData WS_MostLeftLine;
extern Ki_WorkSheetData WS_MostUpperLine;
extern Ki_WorkSheetData WS_Segm3;
extern Ki_WorkSheetData WS_Segm4;
extern Ki_WorkSheetData WS_Segm5;
extern Ki_WorkSheetData WS_Segm6;
extern Ki_WorkSheetData WS_Segm7;

#if defined(KICAD_GOST)
extern Ki_WorkSheetData WS_Izm;
extern Ki_WorkSheetData WS_Razr;
extern Ki_WorkSheetData WS_Prov;
extern Ki_WorkSheetData WS_TKon;
extern Ki_WorkSheetData WS_NKon;
extern Ki_WorkSheetData WS_Utv;
extern Ki_WorkSheetData WS_List;
extern Ki_WorkSheetData WS_NDoc;
extern Ki_WorkSheetData WS_Podp;
extern Ki_WorkSheetData WS_Data;
extern Ki_WorkSheetData WS_Art;
extern Ki_WorkSheetData WS_Mass;
extern Ki_WorkSheetData WS_Msht;
extern Ki_WorkSheetData WS_List1;
extern Ki_WorkSheetData WS_List2;
extern Ki_WorkSheetData WS_Segm8;
extern Ki_WorkSheetData WS_Segm9;
extern Ki_WorkSheetData WS_Segm10;
extern Ki_WorkSheetData WS_Segm11;
extern Ki_WorkSheetData WS_Segm12;
extern Ki_WorkSheetData WS_Segm13;
extern Ki_WorkSheetData WS_Segm14;
extern Ki_WorkSheetData WS_Segm15;
extern Ki_WorkSheetData WS_Segm16;
extern Ki_WorkSheetData WS_Segm17;
extern Ki_WorkSheetData WS_Segm18;
extern Ki_WorkSheetData WS_Segm19;
extern Ki_WorkSheetData WS_Segm20;
extern Ki_WorkSheetData WS_Segm21;
extern Ki_WorkSheetData WS_Segm22;
extern Ki_WorkSheetData WS_Segm23;
extern Ki_WorkSheetData WS_Segm24;
extern Ki_WorkSheetData WS_Segm25;
extern Ki_WorkSheetData WS_CADRE_D;
extern Ki_WorkSheetData WS_Segm1_D;
extern Ki_WorkSheetData WS_Segm2_D;
extern Ki_WorkSheetData WS_Segm3_D;
extern Ki_WorkSheetData WS_Segm4_D;
extern Ki_WorkSheetData WS_Segm5_D;
extern Ki_WorkSheetData WS_Segm6_D;
extern Ki_WorkSheetData WS_Segm7_D;
extern Ki_WorkSheetData WS_Segm8_D;
extern Ki_WorkSheetData WS_Segm9_D;
extern Ki_WorkSheetData WS_Segm10_D;
extern Ki_WorkSheetData WS_Segm11_D;
extern Ki_WorkSheetData WS_Izm_D;
extern Ki_WorkSheetData WS_List_D;
extern Ki_WorkSheetData WS_NDoc_D;
extern Ki_WorkSheetData WS_Podp_D;
extern Ki_WorkSheetData WS_Date_D;
extern Ki_WorkSheetData WS_List1_D;
extern Ki_WorkSheetData WS_ListN_D;
extern Ki_WorkSheetData WS_Segm1_LU;
extern Ki_WorkSheetData WS_Segm2_LU;
extern Ki_WorkSheetData WS_Segm3_LU;
extern Ki_WorkSheetData WS_Segm4_LU;
extern Ki_WorkSheetData WS_Segm5_LU;
extern Ki_WorkSheetData WS_Segm6_LU;
extern Ki_WorkSheetData WS_Segm7_LU;
extern Ki_WorkSheetData WS_Segm8_LU;
extern Ki_WorkSheetData WS_Podp1_LU;
extern Ki_WorkSheetData WS_Podp2_LU;
extern Ki_WorkSheetData WS_Podp3_LU;
extern Ki_WorkSheetData WS_Podp4_LU;
extern Ki_WorkSheetData WS_Podp5_LU;
extern Ki_WorkSheetData WS_Segm1_LT;
extern Ki_WorkSheetData WS_Segm2_LT;
extern Ki_WorkSheetData WS_Segm3_LT;
extern Ki_WorkSheetData WS_Segm4_LT;
extern Ki_WorkSheetData WS_Segm5_LT;
#endif

#ifdef EDA_BASE

Ki_WorkSheetData WS_Date =
	{
	WS_DATE,
	&WS_Licence,
	BLOCK_DATE_X, BLOCK_DATE_Y,
	0,0,
#if defined(KICAD_GOST)
	NULL, NULL
#else
 	wxT("Date: "), NULL
#endif
	};

Ki_WorkSheetData WS_Licence =
	{
	WS_KICAD_VERSION,
	&WS_Revision,
	BLOCK_KICAD_VERSION_X, BLOCK_KICAD_VERSION_Y,
	0,0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Revision =
	{
	WS_REV,
	&WS_SizeSheet,
#if defined(KICAD_GOST)
	STAMP_X_185 - 30, STAMP_Y_30 + 90,
#else
 	BLOCK_REV_X, BLOCK_REV_Y,
#endif
 	0,0,
#if defined(KICAD_GOST)
	NULL, NULL
#else
 	wxT("Rev: "), NULL
#endif
	};

Ki_WorkSheetData WS_SizeSheet =
	{
	WS_SIZESHEET,
#if defined(KICAD_GOST)
	&WS_Title,
	BLOCK_SIZE_SHEET_X, BLOCK_SIZE_SHEET_Y,
	0,0,
	NULL, NULL
	};
#else
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
	wxT("Id: "),NULL
	};
#endif

Ki_WorkSheetData WS_Title =
	{
	WS_TITLE,
	&WS_SheetFilename,
	BLOCK_TITLE_X, BLOCK_TITLE_Y,
	0,0,
#if defined(KICAD_GOST)
	NULL, NULL
#else
	wxT("Title: "),NULL
#endif
	};

Ki_WorkSheetData WS_SheetFilename =
	{
	WS_FILENAME,
	&WS_FullSheetName,
	BLOCK_FILENAME_X, BLOCK_FILENAME_Y,
	0,0,
	wxT("File: "),NULL
	};

Ki_WorkSheetData WS_FullSheetName =
	{
	WS_FULLSHEETNAME,
	&WS_Company,
	BLOCK_FULLSHEETNAME_X, BLOCK_FULLSHEETNAME_Y,
	0,0,
	wxT("Sheet: "),NULL
	};

Ki_WorkSheetData WS_Company =
	{
	WS_COMPANY_NAME,
	&WS_Comment1,
	BLOCK_COMMENT_X, BLOCK_COMPANY_Y,
	0,0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment1 =
	{
	WS_COMMENT1,
	&WS_Comment2,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_OY,
	STAMP_OX, 0,
#else
	BLOCK_COMMENT_X, BLOCK_COMMENT1_Y,
	0,0,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment2 =
	{
	WS_COMMENT2,
	&WS_Comment3,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_OY,
	STAMP_OX, 0,
#else
	BLOCK_COMMENT_X, BLOCK_COMMENT2_Y,
	0,0,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment3 =
	{
	WS_COMMENT3,
	&WS_Comment4,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_OY,
	STAMP_OX, 0,
#else
	BLOCK_COMMENT_X, BLOCK_COMMENT3_Y,
	0,0,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Comment4 =
	{
	WS_COMMENT4,
	&WS_MostLeftLine,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_OY,
	STAMP_OX, 0,
#else
	BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
	0,0,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_MostLeftLine =   /* segment vertical gauche */
	{
	WS_LEFT_SEGMENT,
#if defined(KICAD_GOST)
	&WS_MostUpperLine,
	STAMP_OX, STAMP_OY,
	STAMP_OX, 0,
#else
	&WS_SeparatorLine,
	BLOCK_OX, SIZETEXT * 16,
	BLOCK_OX, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_SeparatorLine =	/* horizontal segment between filename and comments*/
	{
	WS_SEGMENT,
	&WS_MostUpperLine,
	BLOCK_OX, VARIABLE_BLOCK_START_POSITION,
	0, VARIABLE_BLOCK_START_POSITION,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_MostUpperLine =	/* segment horizontal superieur */
	{
	WS_UPPER_SEGMENT,
	&WS_Segm3,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_OY,
	0, STAMP_OY,
#else
	BLOCK_OX, SIZETEXT * 16,
	0, SIZETEXT * 16,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm3 =		/* segment horizontal au dessus de NAME COMPANY */
	{
	WS_SEGMENT,
	&WS_Segm4,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_Y_50,
	STAMP_X_120, STAMP_Y_50,
#else
	BLOCK_OX, SIZETEXT * 6,
	0, SIZETEXT * 6,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm4 =		/* segment vertical a gauche de SHEET et REV */
	{
	WS_SEGMENT,
	&WS_Segm5,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_Y_45,
	STAMP_X_120, STAMP_Y_45,
#else
	BLOCK_REV_X + SIZETEXT, SIZETEXT * 4,
	BLOCK_REV_X + SIZETEXT, 0,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm5 =		/* 1er segment horizontal */
	{
	WS_SEGMENT,
	&WS_Segm6,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_Y_40,
	0, STAMP_Y_40,
#else
	BLOCK_OX, SIZETEXT * 2,
	0, SIZETEXT * 2,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm6 =		/* 2eme segment horizontal */
	{
	WS_SEGMENT,
	&WS_Segm7,
#if defined(KICAD_GOST)
	STAMP_OX, STAMP_Y_35,
	STAMP_X_120, STAMP_Y_35,
#else
	BLOCK_OX, SIZETEXT * 4,
	0, SIZETEXT * 4,
#endif
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm7 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
#if defined(KICAD_GOST)
	&WS_Segm8,
	STAMP_X_50, STAMP_Y_35,
	0, STAMP_Y_35,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm8 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm9,
	STAMP_OX, STAMP_Y_30,
	STAMP_X_120, STAMP_Y_30,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm9 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm10,
	STAMP_OX, STAMP_Y_25,
	STAMP_X_120, STAMP_Y_25,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm10 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm11,
	STAMP_OX, STAMP_Y_20,
	STAMP_X_120, STAMP_Y_20,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm11 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm12,
	STAMP_X_50, STAMP_Y_20,
	0, STAMP_Y_20,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm12 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm13,
	STAMP_OX, STAMP_Y_15,
	0, STAMP_Y_15,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm13 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm14,
	STAMP_OX, STAMP_Y_10,
	STAMP_X_120, STAMP_Y_10,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm14 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm15,
	STAMP_OX, STAMP_Y_5,
	STAMP_X_120, STAMP_Y_5,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm15 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm16,
	STAMP_X_178, STAMP_OY,
	STAMP_X_178, STAMP_Y_30,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm16 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm17,
	STAMP_X_168, STAMP_OY,
	STAMP_X_168, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm17 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm18,
	STAMP_X_145, STAMP_OY,
	STAMP_X_145, 0,
 	NULL,NULL
 	};
 
Ki_WorkSheetData WS_Segm18 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm19,
	STAMP_X_130, STAMP_OY,
	STAMP_X_130, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm19 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm20,
	STAMP_X_120, STAMP_OY,
	STAMP_X_120, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm20 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm21,
	STAMP_X_50, STAMP_Y_40,
	STAMP_X_50, 0,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm21 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm22,
	STAMP_X_45, STAMP_Y_35,
	STAMP_X_45, STAMP_Y_20,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm22 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm23,
	STAMP_X_40, STAMP_Y_35,
	STAMP_X_40, STAMP_Y_20,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm23 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm24,
	STAMP_X_35, STAMP_Y_40,
	STAMP_X_35, STAMP_Y_20,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm24 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Segm25,
	STAMP_X_30, STAMP_Y_20,
	STAMP_X_30, STAMP_Y_15,
	NULL,NULL
	};

Ki_WorkSheetData WS_Segm25 =		/* segment vertical apres SIZE */
	{
	WS_SEGMENT,
	&WS_Izm,
	STAMP_X_18, STAMP_Y_40,
	STAMP_X_18, STAMP_Y_20,
	NULL,NULL
	};

Ki_WorkSheetData WS_Izm = 
	{
	WS_PODPIS,
	&WS_Razr,
	STAMP_X_185 - 30,STAMP_Y_30 + 90,
	0,0,
	wxT("Изм."), NULL
	};

Ki_WorkSheetData WS_Razr =
	{
	WS_PODPIS,
	&WS_Prov,
	STAMP_X_185 - 30,STAMP_Y_25 + 90,
	0,0,
	wxT("Разраб."), NULL
	};

Ki_WorkSheetData WS_Prov =
	{
	WS_PODPIS,
	&WS_TKon,
	STAMP_X_185 - 30,STAMP_Y_20 + 90,
	0,0,
	wxT("Пров."), NULL
	};

Ki_WorkSheetData WS_TKon =
	{
	WS_PODPIS,
	&WS_NKon,
	STAMP_X_185 - 30,STAMP_Y_15 + 90,
	0,0,
	wxT("Т.контр."), NULL
	};

Ki_WorkSheetData WS_NKon =
	{
	WS_PODPIS,
	&WS_Utv,
	STAMP_X_185 - 30,STAMP_Y_5 + 90,
	0,0,
	wxT("Н.контр."), NULL
	};

Ki_WorkSheetData WS_Utv =
	{
	WS_PODPIS,
	&WS_List,
	STAMP_X_185 - 30,STAMP_Y_0 + 90,
	0,0,
	wxT("Утв."), NULL
	};

Ki_WorkSheetData WS_List =
	{
	WS_PODPIS,
	&WS_NDoc,
	STAMP_X_178 - 30,STAMP_Y_30 + 90,
	0,0,
	wxT("Лист"), NULL
	};

Ki_WorkSheetData WS_NDoc =
	{
	WS_PODPIS,
	&WS_Podp,
	STAMP_X_168 - 30,STAMP_Y_30 + 90,
	0,0,
	wxT("N докум."), NULL
	};

Ki_WorkSheetData WS_Podp =
	{
	WS_PODPIS,
	&WS_Data,
	STAMP_X_145 - 30,STAMP_Y_30 + 90,
	0,0,
	wxT("Подп."), NULL
	};

Ki_WorkSheetData WS_Data =
	{
	WS_PODPIS,
	&WS_Art,
	STAMP_X_130 - 30,STAMP_Y_30 + 90,
	0,0,
	wxT("Дата"), NULL
	};

Ki_WorkSheetData WS_Art =
	{
	WS_PODPIS,
	&WS_Mass,
	STAMP_X_50 - 30,STAMP_Y_35 + 90,
	0,0,
	wxT("Арт."), NULL
	};

Ki_WorkSheetData WS_Mass =
	{
	WS_PODPIS,
	&WS_Msht,
	STAMP_X_35 - 30,STAMP_Y_35 + 90,
	0,0,
	wxT("Масса"), NULL
	};

Ki_WorkSheetData WS_Msht =
	{
	WS_PODPIS,
	&WS_List1,
	STAMP_X_18 - 30,STAMP_Y_35 + 90,
	0,0,
	wxT("Масштаб"), NULL
	};

Ki_WorkSheetData WS_List1 =
	{
	WS_IDENTSHEET,
	&WS_List2,
	STAMP_X_50 - 30,STAMP_Y_15 + 90,
	0,0,
	wxT("Лист "), NULL
	};

Ki_WorkSheetData WS_List2 =
	{
	WS_SHEETS,
	NULL,
	STAMP_X_30 - 30,STAMP_Y_15 + 90,
	0,0,
	wxT("Листов "), NULL
	};

Ki_WorkSheetData WS_CADRE_D =
	{
	WS_CADRE,
	&WS_Segm1_D,
	STAMP_OX, 0,
	0,0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm1_D =
	{
	WS_LEFT_SEGMENT_D,
	&WS_Segm2_D,
	STAMP_OX, STAMP_Y_15,
	STAMP_OX, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm2_D =
	{
	WS_SEGMENT_D,
	&WS_Segm3_D,
	STAMP_X_178, STAMP_Y_15,
	STAMP_X_178, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm3_D =
	{
	WS_SEGMENT_D,
	&WS_Segm4_D,
	STAMP_X_168, STAMP_Y_15,
	STAMP_X_168, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm4_D =
	{
	WS_SEGMENT_D,
	&WS_Segm5_D,
	STAMP_X_145, STAMP_Y_15,
	STAMP_X_145, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm5_D =
	{
	WS_SEGMENT_D,
	&WS_Segm6_D,
	STAMP_X_130, STAMP_Y_15,
	STAMP_X_130, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm6_D =
	{
	WS_SEGMENT_D,
	&WS_Segm7_D,
	STAMP_X_120, STAMP_Y_15,
	STAMP_X_120, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm7_D =
	{
	WS_SEGMENT_D,
	&WS_Segm8_D,
	STAMP_X_10, STAMP_Y_15,
	STAMP_X_10, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm8_D =
	{
	WS_SEGMENT_D,
	&WS_Segm9_D,
	STAMP_X_185, STAMP_Y_10,
	STAMP_X_120, STAMP_Y_10,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm9_D =
	{
	WS_SEGMENT_D,
	&WS_Segm10_D,
	STAMP_X_185, STAMP_Y_5,
	STAMP_X_120, STAMP_Y_5,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm10_D =
	{
	WS_SEGMENT_D,
	&WS_Segm11_D,
	STAMP_X_10, STAMP_Y_8,
	0, STAMP_Y_8,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm11_D =
	{
	WS_SEGMENT_D,
	&WS_Izm_D,
	STAMP_X_185, STAMP_Y_15,
	0, STAMP_Y_15,
	NULL, NULL
	};

Ki_WorkSheetData WS_Izm_D =
	{
	WS_PODPIS_D,
	&WS_List_D,
	STAMP_X_185 - 30, STAMP_Y_0 + 90,
	0, 0,
	wxT("Изм."), NULL
	};

Ki_WorkSheetData WS_List_D =
	{
	WS_PODPIS_D,
	&WS_NDoc_D,
	STAMP_X_178 - 30, STAMP_Y_0 + 90,
	0, 0,
	wxT("Лист"), NULL
	};

Ki_WorkSheetData WS_NDoc_D =
	{
	WS_PODPIS_D,
	&WS_Podp_D,
	STAMP_X_168 - 30, STAMP_Y_0 + 90,
	0, 0,
	wxT("N докум."), NULL
	};

Ki_WorkSheetData WS_Podp_D =
	{
	WS_PODPIS_D,
	&WS_Date_D,
	STAMP_X_145 - 30, STAMP_Y_0 + 90,
	0, 0,
	wxT("Подп."), NULL
	};

Ki_WorkSheetData WS_Date_D =
	{
	WS_PODPIS_D,
	&WS_List1_D,
	STAMP_X_130 - 30, STAMP_Y_0 + 90,
	0, 0,
	wxT("Дата"), NULL
	};

Ki_WorkSheetData WS_List1_D =
	{
	WS_PODPIS_D,
	&WS_ListN_D,
	STAMP_X_10 - 30, STAMP_Y_8 + 90,
	0, 0,
	wxT("Лист"), NULL
	};

Ki_WorkSheetData WS_ListN_D =
	{
	WS_IDENTSHEET_D,
	NULL,
	STAMP_Y_0 + 196, STAMP_Y_0 + 90,
	0, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm1_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm2_LU,
	STAMP_12, STAMP_145,
	STAMP_12, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm2_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm3_LU,
	STAMP_7, STAMP_145,
	STAMP_7, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm3_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm4_LU,
	STAMP_12, STAMP_145,
	0, STAMP_145,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm4_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm5_LU,
	STAMP_12, STAMP_110,
	0, STAMP_110,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm5_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm6_LU,
	STAMP_12, STAMP_85,
	0, STAMP_85,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm6_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm7_LU,
	STAMP_12, STAMP_60,
	0, STAMP_60,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm7_LU =
	{
	WS_SEGMENT_LU,
	&WS_Segm8_LU,
	STAMP_12, STAMP_25,
	0, STAMP_25,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm8_LU =
	{
	WS_SEGMENT_LU,
	&WS_Podp1_LU,
	STAMP_12, 0,
	0, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Podp1_LU =
	{
	WS_PODPIS_LU,
	&WS_Podp2_LU,
	STAMP_7 + 90, 0 + 30,
	0, 0,
	wxT("Инв.N подл."), NULL
	};

Ki_WorkSheetData WS_Podp2_LU =
	{
	WS_PODPIS_LU,
	&WS_Podp3_LU,
	STAMP_7 + 90, STAMP_25 + 30,
	0, 0,
	wxT("Подп. и дата"), NULL
	};

Ki_WorkSheetData WS_Podp3_LU =
	{
	WS_PODPIS_LU,
	&WS_Podp4_LU,
	STAMP_7 + 90, STAMP_60 + 30,
	0, 0,
	wxT("Взам.инв.N"), NULL
	};

Ki_WorkSheetData WS_Podp4_LU =
	{
	WS_PODPIS_LU,
	&WS_Podp5_LU,
	STAMP_7 + 90, STAMP_85 + 30,
	0, 0,
	wxT("Инв.N дубл."), NULL
	};

Ki_WorkSheetData WS_Podp5_LU =
	{
	WS_PODPIS_LU,
	NULL,
	STAMP_7 + 90, STAMP_110 + 30,
	0, 0,
	wxT("Подп. и дата"), NULL
	};

Ki_WorkSheetData WS_Segm1_LT =
	{
	WS_SEGMENT_LT,
	&WS_Segm2_LT,
	STAMP_X_0, STAMP_Y_14,
	STAMP_X_137, STAMP_Y_14,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm2_LT =
	{
	WS_SEGMENT_LT,
	&WS_Segm3_LT,
	STAMP_X_137, STAMP_Y_14,
	STAMP_X_137, 0,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm3_LT =
	{
	WS_SEGMENT_LT,
	&WS_Segm4_LT,
	STAMP_X_137, STAMP_Y_7,
	STAMP_X_84, STAMP_Y_7,
	NULL, NULL
	};

Ki_WorkSheetData WS_Segm4_LT =
	{
	WS_SEGMENT_LT,
	&WS_Segm5_LT,
	STAMP_X_84, STAMP_Y_14,
	STAMP_X_84, 0,
	NULL, NULL
	};
Ki_WorkSheetData WS_Segm5_LT =
	{
	WS_SEGMENT_LT,
	NULL,
	STAMP_X_70, STAMP_Y_14,
	STAMP_X_70, 0,
#else
	NULL,
	BLOCK_OX - (SIZETEXT * 11), SIZETEXT * 4,
	BLOCK_OX - (SIZETEXT * 11), SIZETEXT * 2,
#endif
	NULL, NULL
	};

#endif
