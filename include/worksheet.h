/***************/
/* worksheet.h */
/***************/

/* Values are in 1/1000 inch */

#ifndef __WORKSHEET_H__
#define __WORKSHEET_H__

#define GRID_REF_W       70     /* height of the band reference grid */
#define SIZETEXT         60     /* worksheet text size */
#define SIZETEXT_REF     50     /* worksheet frame reference text size */
#define PAS_REF          2000   /* no reference markings on worksheet frame */
#define TEXT_VTAB_HEIGHT SIZETEXT * 2

#if defined(KICAD_GOST)
#define STAMP_OX 185 * 10000 / 254
#define STAMP_OY 55 * 10000 / 254

#define STAMP_Y_0  0
#define STAMP_Y_5  5 * 10000 / 254
#define STAMP_Y_8  8 * 10000 / 254
#define STAMP_Y_7  7 * 10000 / 254
#define STAMP_Y_10 10 * 10000 / 254
#define STAMP_Y_14 14 * 10000 / 254
#define STAMP_Y_15 15 * 10000 / 254
#define STAMP_Y_20 20 * 10000 / 254
#define STAMP_Y_25 25 * 10000 / 254
#define STAMP_Y_30 30 * 10000 / 254
#define STAMP_Y_35 35 * 10000 / 254
#define STAMP_Y_40 40 * 10000 / 254
#define STAMP_Y_45 45 * 10000 / 254
#define STAMP_Y_50 50 * 10000 / 254
#define STAMP_Y_55 55 * 10000 / 254

#define STAMP_X_0   0
#define STAMP_X_10  10 * 10000 / 254
#define STAMP_X_14  14 * 10000 / 254
#define STAMP_X_18  18 * 10000 / 254
#define STAMP_X_30  30 * 10000 / 254
#define STAMP_X_35  35 * 10000 / 254
#define STAMP_X_40  40 * 10000 / 254
#define STAMP_X_45  45 * 10000 / 254
#define STAMP_X_50  50 * 10000 / 254
#define STAMP_X_53  53 * 10000 / 254
#define STAMP_X_70  70 * 10000 / 254
#define STAMP_X_84  84 * 10000 / 254
#define STAMP_X_120 120 * 10000 / 254
#define STAMP_X_130 130 * 10000 / 254
#define STAMP_X_137 137 * 10000 / 254
#define STAMP_X_145 145 * 10000 / 254
#define STAMP_X_168 168 * 10000 / 254
#define STAMP_X_178 178 * 10000 / 254
#define STAMP_X_185 185 * 10000 / 254

#define STAMP_5  5 * 10000 / 254
#define STAMP_7  7 * 10000 / 254
#define STAMP_12 12 * 10000 / 254

#define STAMP_145 145 * 10000 / 254
#define STAMP_110 110 * 10000 / 254
#define STAMP_85  85 * 10000 / 254
#define STAMP_60  60 * 10000 / 254
#define STAMP_25  25 * 10000 / 254
#endif

/* The coordinates below are relative to the bottom right corner of page and
 * will be subtracted from this origin.
 */
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

/* Work sheet structure type definitions. */
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

#endif /* __WORKSHEET_H__ */
