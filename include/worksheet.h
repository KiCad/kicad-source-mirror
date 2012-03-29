/***************/
/* worksheet.h */
/***************/

// Values are in 1/1000 inch

#ifndef WORKSHEET_H_
#define WORKSHEET_H_

#include <common.h>                     // Mm2mils()

#define GRID_REF_W       70             // height of the band reference grid
#define SIZETEXT         60             // worksheet text size
#define SIZETEXT_REF     50             // worksheet frame reference text size
#define PAS_REF          2000           // no reference markings on worksheet frame
#define TEXT_VTAB_HEIGHT (SIZETEXT * 2)

#if defined(KICAD_GOST)

// There is a page layout minor issue in GOST mode.
// This is the rounding-off error of 1 mil exactly.
// I hope this problem will go away when we will go
// to nanometers (zaka62).
#define STAMP_OX    Mm2mils( 185 ) + 1
#define STAMP_OY    Mm2mils(  55 )

#define STAMP_Y_0   0
#define STAMP_Y_5   Mm2mils(   5 )
#define STAMP_Y_8   Mm2mils(   8 )
#define STAMP_Y_7   Mm2mils(   7 )
#define STAMP_Y_10  Mm2mils(  10 )
#define STAMP_Y_14  Mm2mils(  14 )
#define STAMP_Y_15  Mm2mils(  15 )
#define STAMP_Y_20  Mm2mils(  20 )
#define STAMP_Y_25  Mm2mils(  25 )
#define STAMP_Y_30  Mm2mils(  30 )
#define STAMP_Y_35  Mm2mils(  35 )
#define STAMP_Y_40  Mm2mils(  40 )
#define STAMP_Y_45  Mm2mils(  45 )
#define STAMP_Y_50  Mm2mils(  50 )
#define STAMP_Y_55  Mm2mils(  55 )

#define STAMP_X_0   0
#define STAMP_X_10  Mm2mils(  10 )
#define STAMP_X_14  Mm2mils(  14 )
#define STAMP_X_18  Mm2mils(  18 )
#define STAMP_X_30  Mm2mils(  30 )
#define STAMP_X_35  Mm2mils(  35 )
#define STAMP_X_40  Mm2mils(  40 )
#define STAMP_X_45  Mm2mils(  45 )
#define STAMP_X_50  Mm2mils(  50 )
#define STAMP_X_53  Mm2mils(  53 )
#define STAMP_X_65  Mm2mils(  65 )
#define STAMP_X_70  Mm2mils(  70 )
#define STAMP_X_84  Mm2mils(  84 )
#define STAMP_X_85  Mm2mils(  85 )
#define STAMP_X_120 Mm2mils( 120 )
#define STAMP_X_130 Mm2mils( 130 )
#define STAMP_X_137 Mm2mils( 137 )
#define STAMP_X_145 Mm2mils( 145 )
#define STAMP_X_168 Mm2mils( 168 )
#define STAMP_X_178 Mm2mils( 178 )
#define STAMP_X_185 Mm2mils( 185 )

#define STAMP_5     Mm2mils(   5 )
#define STAMP_7     Mm2mils(   7 )
#define STAMP_12    Mm2mils(  12 )

#define STAMP_145   Mm2mils( 145 )
#define STAMP_110   Mm2mils( 110 )
#define STAMP_85    Mm2mils(  85 )
#define STAMP_60    Mm2mils(  60 )
#define STAMP_25    Mm2mils(  25 )

#define STAMP_287   Mm2mils( 287 )
#define STAMP_227   Mm2mils( 227 )
#define STAMP_167   Mm2mils( 167 )
#endif


// The coordinates below are relative to the bottom right corner of page and
// will be subtracted from this origin.
#define BLOCK_OX                      4200
#define BLOCK_KICAD_VERSION_X         BLOCK_OX - SIZETEXT
#define BLOCK_KICAD_VERSION_Y         SIZETEXT
#define BLOCK_REV_X                   820
#define BLOCK_REV_Y                   (SIZETEXT * 3)
#define BLOCK_DATE_X                  BLOCK_OX - (SIZETEXT * 15)
#define BLOCK_DATE_Y                  (SIZETEXT * 3)
#define BLOCK_ID_SHEET_X              820
#define BLOCK_ID_SHEET_Y              SIZETEXT
#define BLOCK_SIZE_SHEET_X            BLOCK_OX - SIZETEXT
#define BLOCK_SIZE_SHEET_Y            (SIZETEXT * 3)
#define BLOCK_TITLE_X                 BLOCK_OX - SIZETEXT
#define BLOCK_TITLE_Y                 (SIZETEXT * 5)
#define BLOCK_FULLSHEETNAME_X         BLOCK_OX - SIZETEXT
#define BLOCK_FULLSHEETNAME_Y         (SIZETEXT * 7)
#define BLOCK_FILENAME_X              BLOCK_OX - SIZETEXT
#define BLOCK_FILENAME_Y              (SIZETEXT * 9)
#define BLOCK_COMMENT_X               BLOCK_OX - SIZETEXT
#define VARIABLE_BLOCK_START_POSITION (SIZETEXT * 10)
#define BLOCK_COMPANY_Y               (SIZETEXT * 11)
#define BLOCK_COMMENT1_Y              (SIZETEXT * 13)
#define BLOCK_COMMENT2_Y              (SIZETEXT * 15)
#define BLOCK_COMMENT3_Y              (SIZETEXT * 17)
#define BLOCK_COMMENT4_Y              (SIZETEXT * 19)


struct Ki_WorkSheetData
{
public:
    int               m_Type;
    Ki_WorkSheetData* Pnext;
    int               m_Posx, m_Posy;
    int               m_Endx, m_Endy;
    const wxChar*     m_Legende;
    const wxChar*     m_Text;
};


/// Work sheet structure type definitions.
enum TypeKi_WorkSheetData {
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
extern Ki_WorkSheetData WS_Segm9_LU;
extern Ki_WorkSheetData WS_Segm10_LU;
extern Ki_WorkSheetData WS_Segm11_LU;
extern Ki_WorkSheetData WS_Segm12_LU;
extern Ki_WorkSheetData WS_Segm13_LU;
extern Ki_WorkSheetData WS_Podp1_LU;
extern Ki_WorkSheetData WS_Podp2_LU;
extern Ki_WorkSheetData WS_Podp3_LU;
extern Ki_WorkSheetData WS_Podp4_LU;
extern Ki_WorkSheetData WS_Podp5_LU;
extern Ki_WorkSheetData WS_Podp6_LU;
extern Ki_WorkSheetData WS_Podp7_LU;
extern Ki_WorkSheetData WS_Segm1_LT;
extern Ki_WorkSheetData WS_Segm2_LT;
extern Ki_WorkSheetData WS_Segm3_LT;
extern Ki_WorkSheetData WS_Segm4_LT;
extern Ki_WorkSheetData WS_Segm5_LT;
#endif

#endif // WORKSHEET_H_
