/***************/
/* worksheet.h */
/***************/

// Values are in 1/1000 inch

#ifndef WORKSHEET_H_
#define WORKSHEET_H_

#include <common.h>                     // Mm2mils()

#define GRID_REF_W       70             // height of the band reference grid

#if defined(KICAD_GOST)
#define SIZETEXT         100             // worksheet text size
#else
#define SIZETEXT         60             // worksheet text size
#endif

#define SIZETEXT_REF     50             // worksheet frame reference text size
#define PAS_REF          2000           // no reference markings on worksheet frame

#if !defined(KICAD_GOST)

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
#define BLOCK_COMPANY_Y               (SIZETEXT * 11)
#define BLOCK_COMMENT1_Y              (SIZETEXT * 13)
#define BLOCK_COMMENT2_Y              (SIZETEXT * 15)
#define BLOCK_COMMENT3_Y              (SIZETEXT * 17)
#define BLOCK_COMMENT4_Y              (SIZETEXT * 19)

#endif

#define VARIABLE_BLOCK_START_POSITION (SIZETEXT * 10)

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
#if defined(KICAD_GOST)
    WS_OSN,
    WS_TONK,
    WS_TEXT,
    WS_TEXTL
#else
    WS_DATE,
    WS_REV,
    WS_KICAD_VERSION,
    WS_SIZESHEET,
    WS_IDENTSHEET,
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
    WS_CADRE
#endif
};

#if defined(KICAD_GOST)

extern Ki_WorkSheetData WS_Osn1_Line1;
extern Ki_WorkSheetData WS_Osn1_Line2;
extern Ki_WorkSheetData WS_Osn1_Line3;
extern Ki_WorkSheetData WS_Osn1_Line4;
extern Ki_WorkSheetData WS_Osn1_Line5;
extern Ki_WorkSheetData WS_Osn1_Line6;
extern Ki_WorkSheetData WS_Osn1_Line7;
extern Ki_WorkSheetData WS_Osn1_Line8;
extern Ki_WorkSheetData WS_Osn1_Line9;
extern Ki_WorkSheetData WS_Osn1_Line10;
extern Ki_WorkSheetData WS_Osn1_Line11;
extern Ki_WorkSheetData WS_Osn1_Line12;
extern Ki_WorkSheetData WS_Osn1_Line13;
extern Ki_WorkSheetData WS_Osn1_Line14;
extern Ki_WorkSheetData WS_Osn1_Line15;
extern Ki_WorkSheetData WS_Osn1_Line16;
extern Ki_WorkSheetData WS_Osn1_Line17;
extern Ki_WorkSheetData WS_Osn1_Line18;
extern Ki_WorkSheetData WS_Osn1_Line19;
extern Ki_WorkSheetData WS_Osn1_Line20;
extern Ki_WorkSheetData WS_Osn1_Line21;
extern Ki_WorkSheetData WS_Osn1_Line22;
extern Ki_WorkSheetData WS_Osn1_Line23;
extern Ki_WorkSheetData WS_Osn1_Line24;
extern Ki_WorkSheetData WS_Osn1_Line25;
extern Ki_WorkSheetData WS_Osn1_Line26;
extern Ki_WorkSheetData WS_Osn1_Line27;

extern Ki_WorkSheetData WS_Osn1_Text1;
extern Ki_WorkSheetData WS_Osn1_Text2;
extern Ki_WorkSheetData WS_Osn1_Text3;
extern Ki_WorkSheetData WS_Osn1_Text4;
extern Ki_WorkSheetData WS_Osn1_Text5;
extern Ki_WorkSheetData WS_Osn1_Text6;
extern Ki_WorkSheetData WS_Osn1_Text7;
extern Ki_WorkSheetData WS_Osn1_Text8;
extern Ki_WorkSheetData WS_Osn1_Text9;
extern Ki_WorkSheetData WS_Osn1_Text10;
extern Ki_WorkSheetData WS_Osn1_Text11;
extern Ki_WorkSheetData WS_Osn1_Text12;
extern Ki_WorkSheetData WS_Osn1_Text13;
extern Ki_WorkSheetData WS_Osn1_Text14;
extern Ki_WorkSheetData WS_Osn1_Text15;
extern Ki_WorkSheetData WS_Osn1_Text16;
extern Ki_WorkSheetData WS_Osn1_Text17;

extern Ki_WorkSheetData WS_Osn2a_Line1;
extern Ki_WorkSheetData WS_Osn2a_Line2;
extern Ki_WorkSheetData WS_Osn2a_Line3;
extern Ki_WorkSheetData WS_Osn2a_Line4;
extern Ki_WorkSheetData WS_Osn2a_Line5;
extern Ki_WorkSheetData WS_Osn2a_Line6;
extern Ki_WorkSheetData WS_Osn2a_Line7;
extern Ki_WorkSheetData WS_Osn2a_Line8;
extern Ki_WorkSheetData WS_Osn2a_Line9;
extern Ki_WorkSheetData WS_Osn2a_Line10;
extern Ki_WorkSheetData WS_Osn2a_Line11;

extern Ki_WorkSheetData WS_Osn2a_Text1;
extern Ki_WorkSheetData WS_Osn2a_Text2;
extern Ki_WorkSheetData WS_Osn2a_Text3;
extern Ki_WorkSheetData WS_Osn2a_Text4;
extern Ki_WorkSheetData WS_Osn2a_Text5;
extern Ki_WorkSheetData WS_Osn2a_Text6;
extern Ki_WorkSheetData WS_Osn2a_Text7;
extern Ki_WorkSheetData WS_Osn2a_Text8;

extern Ki_WorkSheetData WS_DopLeft_Line1;
extern Ki_WorkSheetData WS_DopLeft_Line2;
extern Ki_WorkSheetData WS_DopLeft_Line3;
extern Ki_WorkSheetData WS_DopLeft_Line4;
extern Ki_WorkSheetData WS_DopLeft_Line5;
extern Ki_WorkSheetData WS_DopLeft_Line6;
extern Ki_WorkSheetData WS_DopLeft_Line7;
extern Ki_WorkSheetData WS_DopLeft_Line8;
extern Ki_WorkSheetData WS_DopLeft_Line9;
extern Ki_WorkSheetData WS_DopLeft_Line10;
extern Ki_WorkSheetData WS_DopLeft_Line11;
extern Ki_WorkSheetData WS_DopLeft_Line12;
extern Ki_WorkSheetData WS_DopLeft_Line13;
extern Ki_WorkSheetData WS_DopLeft_Line14;

extern Ki_WorkSheetData WS_DopLeft_Text1;
extern Ki_WorkSheetData WS_DopLeft_Text2;
extern Ki_WorkSheetData WS_DopLeft_Text3;
extern Ki_WorkSheetData WS_DopLeft_Text4;
extern Ki_WorkSheetData WS_DopLeft_Text5;
extern Ki_WorkSheetData WS_DopLeft_Text6;
extern Ki_WorkSheetData WS_DopLeft_Text7;

extern Ki_WorkSheetData WS_DopTop_Line1;
extern Ki_WorkSheetData WS_DopTop_Line2;
extern Ki_WorkSheetData WS_DopTop_Line3;
extern Ki_WorkSheetData WS_DopTop_Line4;
extern Ki_WorkSheetData WS_DopTop_Line5;
extern Ki_WorkSheetData WS_DopTop_Line6;

#else
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
#endif

#endif // WORKSHEET_H_
