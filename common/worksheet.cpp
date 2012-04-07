/*****************/
/* WORKSHEET.CPP */
/*****************/

#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <class_base_screen.h>
#include <drawtxt.h>
#include <confirm.h>
#include <wxstruct.h>
#include <appl_wxstruct.h>
#include <kicad_string.h>
#include <worksheet.h>
#include <class_title_block.h>

#include <build_version.h>

// Must be defined in main applications:

Ki_WorkSheetData WS_Date =
{
    WS_DATE,
    &WS_Licence,
    BLOCK_DATE_X,   BLOCK_DATE_Y,
    0,              0,
#if defined(KICAD_GOST)
    NULL,           NULL
#else
    wxT( "Date: " ),NULL
#endif
};

Ki_WorkSheetData WS_Licence =
{
    WS_KICAD_VERSION,
    &WS_Revision,
    BLOCK_KICAD_VERSION_X,BLOCK_KICAD_VERSION_Y,
    0,                    0,
    NULL,                 NULL
};

Ki_WorkSheetData WS_Revision =
{
    WS_REV,
    &WS_SizeSheet,
#if defined(KICAD_GOST)
    STAMP_X_185 - 30,STAMP_Y_30 + 90,
#else
    BLOCK_REV_X,     BLOCK_REV_Y,
#endif
    0,               0,
#if defined(KICAD_GOST)
    NULL,            NULL
#else
    wxT( "Rev: " ),  NULL
#endif
};

Ki_WorkSheetData WS_SizeSheet =
{
    WS_SIZESHEET,
#if defined(KICAD_GOST)
    &WS_Title,
    BLOCK_SIZE_SHEET_X,BLOCK_SIZE_SHEET_Y,
    0,                 0,
    NULL,              NULL
};
#else
    &WS_IdentSheet,
    BLOCK_SIZE_SHEET_X, BLOCK_SIZE_SHEET_Y,
    0, 0,
    wxT( "Size: " ), NULL
};

Ki_WorkSheetData WS_IdentSheet =
{
    WS_IDENTSHEET,
    &WS_Title,
    BLOCK_ID_SHEET_X,BLOCK_ID_SHEET_Y,
    0,               0,
    wxT( "Id: " ),   NULL
};
#endif

Ki_WorkSheetData WS_Title =
{
    WS_TITLE,
    &WS_SheetFilename,
#if defined(KICAD_GOST)
    STAMP_X_85,       STAMP_Y_25 + 90,
    0,                0,
    NULL,             NULL
#else
    BLOCK_TITLE_X,    BLOCK_TITLE_Y,
    0,                0,
    wxT( "Title: " ), NULL
#endif
};

Ki_WorkSheetData WS_SheetFilename =
{
    WS_FILENAME,
    &WS_FullSheetName,
    BLOCK_FILENAME_X, BLOCK_FILENAME_Y,
    0,                0,
    wxT( "File: " ),  NULL
};

Ki_WorkSheetData WS_FullSheetName =
{
    WS_FULLSHEETNAME,
    &WS_Company,
    BLOCK_FULLSHEETNAME_X,BLOCK_FULLSHEETNAME_Y,
    0,                    0,
    wxT( "Sheet: " ),     NULL
};

Ki_WorkSheetData WS_Company =
{
    WS_COMPANY_NAME,
    &WS_Comment1,
#if defined(KICAD_GOST)
    STAMP_X_50 / 2, STAMP_Y_0 + 270,
    0,              0,
#else
    BLOCK_COMMENT_X,BLOCK_COMPANY_Y,
    0,              0,
#endif
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment1 =
{
    WS_COMMENT1,
    &WS_Comment2,
#if defined(KICAD_GOST)
    STAMP_X_120 / 2,STAMP_Y_40 + 270,
    STAMP_OX,       0,
#else
    BLOCK_COMMENT_X,BLOCK_COMMENT1_Y,
    0,              0,
#endif
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment2 =
{
    WS_COMMENT2,
    &WS_Comment3,
#if defined(KICAD_GOST)
    STAMP_X_168 - 30,      STAMP_Y_25 + 90,
    STAMP_OX,       0,
#else
    BLOCK_COMMENT_X,BLOCK_COMMENT2_Y,
    0,              0,
#endif
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment3 =
{
    WS_COMMENT3,
    &WS_Comment4,
#if defined(KICAD_GOST)
    STAMP_X_168 - 30,      STAMP_Y_20 + 90,
    STAMP_OX,       0,
#else
    BLOCK_COMMENT_X,BLOCK_COMMENT3_Y,
    0,              0,
#endif
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment4 =
{
    WS_COMMENT4,
    &WS_MostLeftLine,
#if defined(KICAD_GOST)
    STAMP_X_168 - 30,      STAMP_Y_0 + 90,
    STAMP_OX,        0,
#else
    BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
    0,               0,
#endif
    NULL,            NULL
};


/// Left vertical segment
Ki_WorkSheetData WS_MostLeftLine =
{
    WS_LEFT_SEGMENT,
#if defined(KICAD_GOST)
    &WS_MostUpperLine,
    STAMP_OX,         STAMP_OY,
    STAMP_OX,         0,
#else
    &WS_SeparatorLine,
    BLOCK_OX,         SIZETEXT * 16,
    BLOCK_OX,         0,
    NULL,             NULL
};


/// horizontal segment between filename and comments
Ki_WorkSheetData WS_SeparatorLine =
{
    WS_SEGMENT,
    &WS_MostUpperLine,
    BLOCK_OX,         VARIABLE_BLOCK_START_POSITION,
    0,                VARIABLE_BLOCK_START_POSITION,
#endif
    NULL,             NULL
};


/// superior horizontal segment
Ki_WorkSheetData WS_MostUpperLine =
{
    WS_UPPER_SEGMENT,
    &WS_Segm3,
#if defined(KICAD_GOST)
    STAMP_OX,        STAMP_OY,
    0,               STAMP_OY,
#else
    BLOCK_OX,        SIZETEXT * 16,
    0,               SIZETEXT * 16,
#endif
    NULL,            NULL
};


/// horizontal segment above COMPANY NAME
Ki_WorkSheetData WS_Segm3 =
{
    WS_SEGMENT,
    &WS_Segm4,
#if defined(KICAD_GOST)
    STAMP_OX,   STAMP_Y_50,
    STAMP_X_120,STAMP_Y_50,
#else
    BLOCK_OX,   SIZETEXT * 6,
    0,          SIZETEXT * 6,
#endif
    NULL,       NULL
};


/// vertical segment of the left REV and SHEET
Ki_WorkSheetData WS_Segm4 =
{
    WS_SEGMENT,
    &WS_Segm5,
#if defined(KICAD_GOST)
    STAMP_OX,              STAMP_Y_45,
    STAMP_X_120,           STAMP_Y_45,
#else
    BLOCK_REV_X + SIZETEXT,SIZETEXT * 4,
    BLOCK_REV_X + SIZETEXT,0,
#endif
    NULL,                  NULL
};


Ki_WorkSheetData WS_Segm5 =
{
    WS_SEGMENT,
    &WS_Segm6,
#if defined(KICAD_GOST)
    STAMP_OX,  STAMP_Y_40,
    0,         STAMP_Y_40,
#else
    BLOCK_OX,  SIZETEXT * 2,
    0,         SIZETEXT * 2,
#endif
    NULL,      NULL
};


Ki_WorkSheetData WS_Segm6 =
{
    WS_SEGMENT,
    &WS_Segm7,
#if defined(KICAD_GOST)
    STAMP_OX,   STAMP_Y_35,
    STAMP_X_120,STAMP_Y_35,
#else
    BLOCK_OX,   SIZETEXT * 4,
    0,          SIZETEXT * 4,
#endif
    NULL,       NULL
};


Ki_WorkSheetData WS_Segm7 =
{
    WS_SEGMENT,
#if defined(KICAD_GOST)
    &WS_Segm8,
    STAMP_X_50,STAMP_Y_35,
    0,         STAMP_Y_35,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm8 =
{
    WS_SEGMENT,
    &WS_Segm9,
    STAMP_OX,   STAMP_Y_30,
    STAMP_X_120,STAMP_Y_30,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm9 =
{
    WS_SEGMENT,
    &WS_Segm10,
    STAMP_OX,   STAMP_Y_25,
    STAMP_X_120,STAMP_Y_25,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm10 =
{
    WS_SEGMENT,
    &WS_Segm11,
    STAMP_OX,   STAMP_Y_20,
    STAMP_X_120,STAMP_Y_20,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm11 =
{
    WS_SEGMENT,
    &WS_Segm12,
    STAMP_X_50,STAMP_Y_20,
    0,         STAMP_Y_20,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm12 =
{
    WS_SEGMENT,
    &WS_Segm13,
    STAMP_OX,  STAMP_Y_15,
    0,         STAMP_Y_15,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm13 =
{
    WS_SEGMENT,
    &WS_Segm14,
    STAMP_OX,   STAMP_Y_10,
    STAMP_X_120,STAMP_Y_10,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm14 =
{
    WS_SEGMENT,
    &WS_Segm15,
    STAMP_OX,   STAMP_Y_5,
    STAMP_X_120,STAMP_Y_5,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm15 =
{
    WS_SEGMENT,
    &WS_Segm16,
    STAMP_X_178,STAMP_OY,
    STAMP_X_178,STAMP_Y_30,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm16 =
{
    WS_SEGMENT,
    &WS_Segm17,
    STAMP_X_168,STAMP_OY,
    STAMP_X_168,0,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm17 =
{
    WS_SEGMENT,
    &WS_Segm18,
    STAMP_X_145,STAMP_OY,
    STAMP_X_145,0,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm18 =
{
    WS_SEGMENT,
    &WS_Segm19,
    STAMP_X_130,STAMP_OY,
    STAMP_X_130,0,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm19 =
{
    WS_SEGMENT,
    &WS_Segm20,
    STAMP_X_120,STAMP_OY,
    STAMP_X_120,0,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm20 =
{
    WS_SEGMENT,
    &WS_Segm21,
    STAMP_X_50,STAMP_Y_40,
    STAMP_X_50,0,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm21 =
{
    WS_SEGMENT,
    &WS_Segm22,
    STAMP_X_45,STAMP_Y_35,
    STAMP_X_45,STAMP_Y_20,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm22 =
{
    WS_SEGMENT,
    &WS_Segm23,
    STAMP_X_40,STAMP_Y_35,
    STAMP_X_40,STAMP_Y_20,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm23 =
{
    WS_SEGMENT,
    &WS_Segm24,
    STAMP_X_35,STAMP_Y_40,
    STAMP_X_35,STAMP_Y_20,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm24 =
{
    WS_SEGMENT,
    &WS_Segm25,
    STAMP_X_30,STAMP_Y_20,
    STAMP_X_30,STAMP_Y_15,
    NULL,      NULL
};

Ki_WorkSheetData WS_Segm25 =
{
    WS_SEGMENT,
    &WS_Izm,
    STAMP_X_18,STAMP_Y_40,
    STAMP_X_18,STAMP_Y_20,
    NULL,      NULL
};

Ki_WorkSheetData WS_Izm =
{
    WS_PODPIS,
    &WS_Razr,
    STAMP_X_185 - 30,STAMP_Y_30 + 90,
    0,               0,
    wxT( "Изм." ),NULL
};

Ki_WorkSheetData WS_Razr =
{
    WS_PODPIS,
    &WS_Prov,
    STAMP_X_185 - 30,      STAMP_Y_25 + 90,
    0,                     0,
    wxT( "Разраб." ),NULL
};

Ki_WorkSheetData WS_Prov =
{
    WS_PODPIS,
    &WS_TKon,
    STAMP_X_185 - 30,  STAMP_Y_20 + 90,
    0,                 0,
    wxT( "Пров." ),NULL
};

Ki_WorkSheetData WS_TKon =
{
    WS_PODPIS,
    &WS_NKon,
    STAMP_X_185 - 30,       STAMP_Y_15 + 90,
    0,                      0,
    wxT( "Т.контр." ),NULL
};

Ki_WorkSheetData WS_NKon =
{
    WS_PODPIS,
    &WS_Utv,
    STAMP_X_185 - 30,       STAMP_Y_5 + 90,
    0,                      0,
    wxT( "Н.контр." ),NULL
};

Ki_WorkSheetData WS_Utv =
{
    WS_PODPIS,
    &WS_List,
    STAMP_X_185 - 30,STAMP_Y_0 + 90,
    0,               0,
    wxT( "Утв." ),NULL
};

Ki_WorkSheetData WS_List =
{
    WS_PODPIS,
    &WS_NDoc,
    STAMP_X_178 - 30, STAMP_Y_30 + 90,
    0,                0,
    wxT( "Лист" ),NULL
};

Ki_WorkSheetData WS_NDoc =
{
    WS_PODPIS,
    &WS_Podp,
    STAMP_X_168 - 30,      STAMP_Y_30 + 90,
    0,                     0,
    wxT( "N докум." ),NULL
};

Ki_WorkSheetData WS_Podp =
{
    WS_PODPIS,
    &WS_Data,
    STAMP_X_145 - 30,  STAMP_Y_30 + 90,
    0,                 0,
    wxT( "Подп." ),NULL
};

Ki_WorkSheetData WS_Data =
{
    WS_PODPIS,
    &WS_Art,
    STAMP_X_130 - 30, STAMP_Y_30 + 90,
    0,                0,
    wxT( "Дата" ),NULL
};

Ki_WorkSheetData WS_Art =
{
    WS_PODPIS,
    &WS_Mass,
    STAMP_X_50 - 30, STAMP_Y_35 + 90,
    0,               0,
    wxT( "Лит." ),NULL
};

Ki_WorkSheetData WS_Mass =
{
    WS_PODPIS,
    &WS_Msht,
    STAMP_X_35 - 30,    STAMP_Y_35 + 90,
    0,                  0,
    wxT( "Масса" ),NULL
};

Ki_WorkSheetData WS_Msht =
{
    WS_PODPIS,
    &WS_List1,
    STAMP_X_18 - 30,        STAMP_Y_35 + 90,
    0,                      0,
    wxT( "Масштаб" ),NULL
};

Ki_WorkSheetData WS_List1 =
{
    WS_IDENTSHEET,
    &WS_List2,
    STAMP_X_50 - 30,   STAMP_Y_15 + 90,
    0,                 0,
    wxT( "Лист " ),NULL
};

Ki_WorkSheetData WS_List2 =
{
    WS_SHEETS,
    NULL,
    STAMP_X_30 - 30,       STAMP_Y_15 + 90,
    0,                     0,
    wxT( "Листов " ),NULL
};

Ki_WorkSheetData WS_CADRE_D =
{
    WS_CADRE,
    &WS_Segm1_D,
    STAMP_X_65,      STAMP_Y_0 + 270,
    0,          0,
    NULL,       NULL
};

Ki_WorkSheetData WS_Segm1_D =
{
    WS_LEFT_SEGMENT_D,
    &WS_Segm2_D,
    STAMP_OX,         STAMP_Y_15,
    STAMP_OX,         0,
    NULL,             NULL
};

Ki_WorkSheetData WS_Segm2_D =
{
    WS_SEGMENT_D,
    &WS_Segm3_D,
    STAMP_X_178, STAMP_Y_15,
    STAMP_X_178, 0,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm3_D =
{
    WS_SEGMENT_D,
    &WS_Segm4_D,
    STAMP_X_168, STAMP_Y_15,
    STAMP_X_168, 0,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm4_D =
{
    WS_SEGMENT_D,
    &WS_Segm5_D,
    STAMP_X_145, STAMP_Y_15,
    STAMP_X_145, 0,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm5_D =
{
    WS_SEGMENT_D,
    &WS_Segm6_D,
    STAMP_X_130, STAMP_Y_15,
    STAMP_X_130, 0,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm6_D =
{
    WS_SEGMENT_D,
    &WS_Segm7_D,
    STAMP_X_120, STAMP_Y_15,
    STAMP_X_120, 0,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm7_D =
{
    WS_SEGMENT_D,
    &WS_Segm8_D,
    STAMP_X_10,  STAMP_Y_15,
    STAMP_X_10,  0,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm8_D =
{
    WS_SEGMENT_D,
    &WS_Segm9_D,
    STAMP_X_185, STAMP_Y_10,
    STAMP_X_120, STAMP_Y_10,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm9_D =
{
    WS_SEGMENT_D,
    &WS_Segm10_D,
    STAMP_X_185, STAMP_Y_5,
    STAMP_X_120, STAMP_Y_5,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm10_D =
{
    WS_SEGMENT_D,
    &WS_Segm11_D,
    STAMP_X_10,  STAMP_Y_8,
    0,           STAMP_Y_8,
    NULL,        NULL
};

Ki_WorkSheetData WS_Segm11_D =
{
    WS_SEGMENT_D,
    &WS_Izm_D,
    STAMP_X_185, STAMP_Y_15,
    0,           STAMP_Y_15,
    NULL,        NULL
};

Ki_WorkSheetData WS_Izm_D =
{
    WS_PODPIS_D,
    &WS_List_D,
    STAMP_X_185 - 30,STAMP_Y_0 + 90,
    0,               0,
    wxT( "Изм." ),NULL
};

Ki_WorkSheetData WS_List_D =
{
    WS_PODPIS_D,
    &WS_NDoc_D,
    STAMP_X_178 - 30, STAMP_Y_0 + 90,
    0,                0,
    wxT( "Лист" ),NULL
};

Ki_WorkSheetData WS_NDoc_D =
{
    WS_PODPIS_D,
    &WS_Podp_D,
    STAMP_X_168 - 30,      STAMP_Y_0 + 90,
    0,                     0,
    wxT( "N докум." ),NULL
};

Ki_WorkSheetData WS_Podp_D =
{
    WS_PODPIS_D,
    &WS_Date_D,
    STAMP_X_145 - 30,  STAMP_Y_0 + 90,
    0,                 0,
    wxT( "Подп." ),NULL
};

Ki_WorkSheetData WS_Date_D =
{
    WS_PODPIS_D,
    &WS_List1_D,
    STAMP_X_130 - 30, STAMP_Y_0 + 90,
    0,                0,
    wxT( "Дата" ),NULL
};

Ki_WorkSheetData WS_List1_D =
{
    WS_PODPIS_D,
    &WS_ListN_D,
    STAMP_X_10 - 30,  STAMP_Y_8 + 90,
    0,                0,
    wxT( "Лист" ),NULL
};

Ki_WorkSheetData WS_ListN_D =
{
    WS_IDENTSHEET_D,
    NULL,
    STAMP_Y_0 + 196,STAMP_Y_0 + 90,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Segm1_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm2_LU,
    STAMP_12,     STAMP_145,
    STAMP_12,     0,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm2_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm3_LU,
    STAMP_7,      STAMP_145,
    STAMP_7,      0,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm3_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm4_LU,
    STAMP_12,     STAMP_145,
    0,            STAMP_145,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm4_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm5_LU,
    STAMP_12,     STAMP_110,
    0,            STAMP_110,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm5_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm6_LU,
    STAMP_12,     STAMP_85,
    0,            STAMP_85,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm6_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm7_LU,
    STAMP_12,     STAMP_60,
    0,            STAMP_60,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm7_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm8_LU,
    STAMP_12,     STAMP_25,
    0,            STAMP_25,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm8_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm9_LU,
    STAMP_12,     0,
    0,            0,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm9_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm10_LU,
    STAMP_12,     STAMP_287,
    STAMP_12,     STAMP_167,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm10_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm11_LU,
    STAMP_7,     STAMP_287,
    STAMP_7,     STAMP_167,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm11_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm12_LU,
    STAMP_12,     STAMP_287,
    0,            STAMP_287,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm12_LU =
{
    WS_SEGMENT_LU,
    &WS_Segm13_LU,
    STAMP_12,     STAMP_227,
    0,            STAMP_227,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm13_LU =
{
    WS_SEGMENT_LU,
    &WS_Podp1_LU,
    STAMP_12,     STAMP_167,
    0,            STAMP_167,
    NULL,         NULL
};

Ki_WorkSheetData WS_Podp1_LU =
{
    WS_PODPIS_LU,
    &WS_Podp2_LU,
    STAMP_7 + 90, 0 + 492,
    0,            0,
    wxT( "Инв.N подл." ),NULL
};

Ki_WorkSheetData WS_Podp2_LU =
{
    WS_PODPIS_LU,
    &WS_Podp3_LU,
    STAMP_7 + 90, STAMP_25 + 688,
    0,            0,
    wxT( "Подп. и дата" ),NULL
};

Ki_WorkSheetData WS_Podp3_LU =
{
    WS_PODPIS_LU,
    &WS_Podp4_LU,
    STAMP_7 + 90, STAMP_60 + 492,
    0,            0,
    wxT( "Взам.инв.N" ),NULL
};

Ki_WorkSheetData WS_Podp4_LU =
{
    WS_PODPIS_LU,
    &WS_Podp5_LU,
    STAMP_7 + 90,  STAMP_85 + 492,
    0,             0,
    wxT( "Инв.N дубл." ),NULL
};

Ki_WorkSheetData WS_Podp5_LU =
{
    WS_PODPIS_LU,
    &WS_Podp6_LU,
    STAMP_7 + 90, STAMP_110 + 688,
    0,            0,
    wxT( "Подп. и дата" ),NULL
};

Ki_WorkSheetData WS_Podp6_LU =
{
    WS_PODPIS_LU,
    &WS_Podp7_LU,
    STAMP_7 + 90, STAMP_167 + 1180,
    0,            0,
    wxT( "Справ. N" ),NULL
};

Ki_WorkSheetData WS_Podp7_LU =
{
    WS_PODPIS_LU,
    NULL,
    STAMP_7 + 90, STAMP_227 + 1180,
    0,            0,
    wxT( "Перв. примен." ),NULL
};

Ki_WorkSheetData WS_Segm1_LT =
{
    WS_SEGMENT_LT,
    &WS_Segm2_LT,
    STAMP_X_0,    STAMP_Y_14,
    STAMP_X_137,  STAMP_Y_14,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm2_LT =
{
    WS_SEGMENT_LT,
    &WS_Segm3_LT,
    STAMP_X_137,  STAMP_Y_14,
    STAMP_X_137,  0,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm3_LT =
{
    WS_SEGMENT_LT,
    &WS_Segm4_LT,
    STAMP_X_137,  STAMP_Y_7,
    STAMP_X_84,   STAMP_Y_7,
    NULL,         NULL
};

Ki_WorkSheetData WS_Segm4_LT =
{
    WS_SEGMENT_LT,
    &WS_Segm5_LT,
    STAMP_X_84,   STAMP_Y_14,
    STAMP_X_84,   0,
    NULL,         NULL
};
Ki_WorkSheetData WS_Segm5_LT =
{
    WS_SEGMENT_LT,
    NULL,
    STAMP_X_70,   STAMP_Y_14,
    STAMP_X_70,   0,
#else
    NULL,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 4,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 2,
#endif
    NULL,                      NULL
};


void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* DC, BASE_SCREEN* screen, int line_width )
{
    if( !m_showBorderAndTitleBlock )
        return;

    const PAGE_INFO&  pageInfo = GetPageSettings();
    wxSize  pageSize = pageInfo.GetSizeMils();

    // if not printing, draw the page limits:
    if( !screen->m_IsPrinting && g_ShowPageLimits )
    {
        int scale = m_internalUnits / 1000;
        GRSetDrawMode( DC, GR_COPY );
        GRRect( m_canvas->GetClipBox(), DC, 0, 0,
                pageSize.x * scale, pageSize.y * scale, line_width,
                g_DrawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    wxPoint margin_left_top( pageInfo.GetLeftMarginMils(), pageInfo.GetTopMarginMils() );
    wxPoint margin_right_bottom( pageInfo.GetRightMarginMils(), pageInfo.GetBottomMarginMils() );
    wxString paper = pageInfo.GetType();
    wxString file = screen->GetFileName();
    TITLE_BLOCK t_block = GetTitleBlock();
    int number_of_screens = screen->m_NumberOfScreen;
    int screen_to_draw = screen->m_ScreenNumber;

    TraceWorkSheet( ( wxDC* )DC, pageSize, margin_left_top, margin_right_bottom,
                      paper, file, t_block, number_of_screens, screen_to_draw,
                      ( int )line_width );
}


void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* aDC, wxSize& aSz, wxPoint& aLT, wxPoint& aRB,
                                       wxString& aType, wxString& aFlNm, TITLE_BLOCK& aTb,
                                       int aNScr, int aScr, int aLnW, EDA_COLOR_T aClr1,
                                       EDA_COLOR_T aClr2 )
{
    wxPoint pos;
    int refx, refy;
    wxString Line;
    Ki_WorkSheetData* WsItem;
    int scale = m_internalUnits / 1000;
    wxSize size( SIZETEXT * scale, SIZETEXT * scale );
    wxSize size_ref( SIZETEXT_REF * scale, SIZETEXT_REF * scale );
    wxString msg;

    GRSetDrawMode( aDC, GR_COPY );

    // Upper left corner
    refx = aLT.x;
    refy = aLT.y;

    // lower right corner
    int xg, yg;
    xg   = aSz.x - aRB.x;
    yg   = aSz.y - aRB.y;

#if defined(KICAD_GOST)
    // Draw the border.
    GRRect( m_canvas->GetClipBox(), aDC, refx * scale, refy * scale,
            xg * scale, yg * scale, aLnW, aClr1 );

    refx = aLT.x;
    refy = aSz.y - aRB.y; // Lower left corner
    for( WsItem = &WS_Segm1_LU; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x = ( refx - WsItem->m_Posx ) * scale;
        pos.y = ( refy - WsItem->m_Posy ) * scale;
        msg.Empty();
        switch( WsItem->m_Type )
        {
        case WS_CADRE:
            break;

        case WS_PODPIS_LU:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            DrawGraphicText( m_canvas, aDC, pos, aClr1,
                             msg, TEXT_ORIENT_VERT, size,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_BOTTOM,
                             aLnW, false, false );
            break;

        case WS_SEGMENT_LU:
            xg = aLT.x - WsItem->m_Endx;
            yg = aSz.y - aRB.y - WsItem->m_Endy;
            GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                    xg * scale, yg * scale, aLnW, aClr1 );
            break;
        }
    }

    refy = aRB.y; // Left Top corner
    for( WsItem = &WS_Segm1_LT; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x = ( refx + WsItem->m_Posx ) * scale;
        pos.y = ( refy + WsItem->m_Posy ) * scale;
        msg.Empty();
        switch( WsItem->m_Type )
        {
        case WS_SEGMENT_LT:
            xg = aLT.x + WsItem->m_Endx;
            yg = aRB.y + WsItem->m_Endy;
            GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                    xg * scale, yg * scale, aLnW, aClr1 );
            break;
        }
    }

    wxSize size2( SIZETEXT * scale * 2, SIZETEXT * scale * 2);
    wxSize size3( SIZETEXT * scale * 3, SIZETEXT * scale * 3);
    wxSize size1_5( SIZETEXT * scale * 1.5, SIZETEXT * scale * 1.5);
    // lower right corner
    refx = aSz.x - aRB.x;
    refy = aSz.y - aRB.y;

    if( aScr == 1 )
    {
        for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            pos.x = (refx - WsItem->m_Posx) * scale;
            pos.y = (refy - WsItem->m_Posy) * scale;
            msg.Empty();
            switch( WsItem->m_Type )
            {
            case WS_DATE:
                break;

            case WS_REV:
                break;

            case WS_KICAD_VERSION:
                break;

            case WS_PODPIS:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                break;

            case WS_SIZESHEET:
                break;

            case WS_IDENTSHEET:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                if( aNScr > 1 )
                    msg << aScr;
                DrawGraphicText( m_canvas, aDC, pos, aClr1, msg,
                                 TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_LEFT,
                                 GR_TEXT_VJUSTIFY_CENTER, aLnW, false, false );
                break;

            case WS_SHEETS:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                msg << aNScr;
                DrawGraphicText( m_canvas, aDC, pos, aClr1, msg,
                                 TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_LEFT,
                                 GR_TEXT_VJUSTIFY_CENTER, aLnW, false, false );
                break;

            case WS_COMPANY_NAME:
            msg = aTb.GetCompany();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size1_5,
                                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_TITLE:
            msg = aTb.GetTitle();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size1_5,
                                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_COMMENT1:
            msg = aTb.GetComment1();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size3,
                                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                    pos.x = (aLT.x + 1260) * scale;
                    pos.y = (aLT.y + 270) * scale;
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, 1800, size2,
                                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_COMMENT2:
            msg = aTb.GetComment2();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_COMMENT3:
            msg = aTb.GetComment3();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_COMMENT4:
            msg = aTb.GetComment4();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_UPPER_SEGMENT:
            case WS_LEFT_SEGMENT:
                WS_MostUpperLine.m_Posy = WS_MostUpperLine.m_Endy =
                    WS_MostLeftLine.m_Posy = STAMP_OY;
                pos.y = ( refy - WsItem->m_Posy ) * scale;

            case WS_SEGMENT:
                xg = aSz.x - aRB.x - WsItem->m_Endx;
                yg = aSz.y - aRB.y - WsItem->m_Endy;
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                        xg * scale, yg * scale, aLnW, aClr1 );
                break;
            }
        }
    }
    else
    {
        for( WsItem = &WS_CADRE_D; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            pos.x = ( refx - WsItem->m_Posx ) * scale;
            pos.y = ( refy - WsItem->m_Posy ) * scale;
            msg.Empty();

            switch( WsItem->m_Type )
            {
            case WS_CADRE:
            // Begin list number > 1
            msg = aTb.GetComment1();
                if( !msg.IsEmpty() )
                {
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, TEXT_ORIENT_HORIZ, size3,
                                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                    pos.x = (aLT.x + 1260) * scale;
                    pos.y = (aLT.y + 270) * scale;
                    DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                     msg, 1800, size2,
                                     GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                }
                break;

            case WS_PODPIS_D:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                break;

            case WS_IDENTSHEET_D:
                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;
                msg << aScr;
                DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                break;

            case WS_LEFT_SEGMENT_D:
                pos.y = ( refy - WsItem->m_Posy ) * scale;

            case WS_SEGMENT_D:
                xg = aSz.x - aRB.x - WsItem->m_Endx;
                yg = aSz.y - aRB.y - WsItem->m_Endy;
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                        xg * scale, yg * scale, aLnW, aClr1 );
                break;
            }
        }
    }

#else
    // Draw the border.
    int ii, jj, ipas, gxpas, gypas;
    for( ii = 0; ii < 2; ii++ )
    {
        GRRect( m_canvas->GetClipBox(), aDC, refx * scale, refy * scale,
                xg * scale, yg * scale, aLnW, aClr1 );

        refx += GRID_REF_W; refy += GRID_REF_W;
        xg   -= GRID_REF_W; yg -= GRID_REF_W;
    }

    // Upper left corner
    refx = aLT.x;
    refy = aLT.y;

    // lower right corner
    xg   = aSz.x - aRB.x;
    yg   = aSz.y - aRB.y;

    ipas  = ( xg - refx ) / PAS_REF;
    gxpas = ( xg - refx ) / ipas;
    for( ii = refx + gxpas, jj = 1; ipas > 0; ii += gxpas, jj++, ipas-- )
    {
        Line.Printf( wxT( "%d" ), jj );

        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, ii * scale, refy * scale,
                    ii * scale, ( refy + GRID_REF_W ) * scale, aLnW, aClr1 );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( ii - gxpas / 2 ) * scale,
                                  ( refy + GRID_REF_W / 2 ) * scale ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );

        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, ii * scale, yg * scale,
                    ii * scale, ( yg - GRID_REF_W ) * scale, aLnW, aClr1 );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( ii - gxpas / 2 ) * scale,
                                  ( yg - GRID_REF_W / 2) * scale ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );
    }

    ipas  = ( yg - refy ) / PAS_REF;
    gypas = ( yg - refy ) / ipas;

    for( ii = refy + gypas, jj = 0; ipas > 0; ii += gypas, jj++, ipas-- )
    {
        if( jj < 26 )
            Line.Printf( wxT( "%c" ), jj + 'A' );
        else    // I hope 52 identifiers are enough...
            Line.Printf( wxT( "%c" ), 'a' + jj - 26 );

        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, refx * scale, ii * scale,
                    ( refx + GRID_REF_W ) * scale, ii * scale, aLnW, aClr1 );
        }

        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( refx + GRID_REF_W / 2 ) * scale,
                                  ( ii - gypas / 2 ) * scale ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );

        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, xg * scale, ii * scale,
                    ( xg - GRID_REF_W ) * scale, ii * scale, aLnW, aClr1 );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( xg - GRID_REF_W / 2 ) * scale,
                                  ( ii - gxpas / 2 ) * scale ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );
    }

    int UpperLimit = VARIABLE_BLOCK_START_POSITION;
    refx = aSz.x - aRB.x  - GRID_REF_W;
    refy = aSz.y - aRB.y - GRID_REF_W;

    for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x = (refx - WsItem->m_Posx) * scale;
        pos.y = (refy - WsItem->m_Posy) * scale;
        msg.Empty();

        switch( WsItem->m_Type )
        {
        case WS_DATE:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTb.GetDate();
            DrawGraphicText( m_canvas, aDC, pos, aClr1,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, true );
            break;

        case WS_REV:
            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aClr1, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( MIN( size.x, size.y ) ), false, true );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
             }
            msg = aTb.GetRevision();
            DrawGraphicText( m_canvas, aDC, pos, aClr2, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             GetPenSizeForBold( MIN( size.x, size.y ) ), false, true );
            break;

        case WS_KICAD_VERSION:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += g_ProductName + wxGetApp().GetAppName();
            msg += wxT( " " ) + GetBuildVersion();
            DrawGraphicText( m_canvas, aDC, pos, aClr1,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
            break;

        case WS_SIZESHEET:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aType;
            DrawGraphicText( m_canvas, aDC, pos, aClr1,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
            break;


        case WS_IDENTSHEET:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg << aScr << wxT( "/" ) << aNScr;
            DrawGraphicText( m_canvas, aDC, pos, aClr1,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
            break;

        case WS_FILENAME:
            {
                wxString fname, fext;
                wxFileName::SplitPath( aFlNm, (wxString*) NULL, &fname, &fext );

                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;

                msg << fname << wxT( "." ) << fext;
                DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
            }
            break;

        case WS_FULLSHEETNAME:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += GetScreenDesc();
            DrawGraphicText( m_canvas, aDC, pos, aClr1,
                             msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
            break;


        case WS_COMPANY_NAME:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTb.GetCompany();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( MIN( size.x, size.y ) ),
                                 false, true );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_TITLE:
            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aClr1, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( MIN( size.x, size.y ) ), false, true );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
            }
            msg = aTb.GetTitle();
            DrawGraphicText( m_canvas, aDC, pos, aClr2, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             GetPenSizeForBold( MIN( size.x, size.y ) ), false, true );
            break;

        case WS_COMMENT1:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTb.GetComment1();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT2:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTb.GetComment2();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT3:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTb.GetComment3();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_COMMENT4:
            if( WsItem->m_Legende )
                msg = WsItem->m_Legende;
            msg += aTb.GetComment4();
            if( !msg.IsEmpty() )
            {
                DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                 msg, TEXT_ORIENT_HORIZ, size,
                                 GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                UpperLimit = MAX( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_UPPER_SEGMENT:
            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy =
                WS_MostUpperLine.m_Endy    =
                    WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (refy - WsItem->m_Posy) * scale;

        case WS_SEGMENT:
            xg = aSz.x - GRID_REF_W - aRB.x - WsItem->m_Endx;
            yg = aSz.y - GRID_REF_W - aRB.y - WsItem->m_Endy;
            GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                    xg * scale, yg * scale, aLnW, aClr1 );
            break;
        }
    }

#endif
}


const wxString EDA_DRAW_FRAME::GetXYSheetReferences( const wxPoint& aPosition )
{
    const PAGE_INFO& pageInfo = GetPageSettings();

    int         ii;
    int         xg, yg;
    int         ipas;
    int         gxpas, gypas;
    int         refx, refy;
    wxString    msg;

    // Upper left corner
    refx = pageInfo.GetLeftMarginMils();
    refy = pageInfo.GetTopMarginMils();

    // lower right corner
    xg   = pageInfo.GetSizeMils().x - pageInfo.GetRightMarginMils();
    yg   = pageInfo.GetSizeMils().y - pageInfo.GetBottomMarginMils();

    // Get the Y axis identifier (A symbol A ... Z)
    if( aPosition.y < refy || aPosition.y > yg )  // Outside of Y limits
        msg << wxT( "?" );
    else
    {
        ipas  = ( yg - refy ) / PAS_REF;        // ipas = Y count sections
        gypas = ( yg - refy ) / ipas;           // gypas = Y section size
        ii    = ( aPosition.y - refy ) / gypas;
        msg.Printf( wxT( "%c" ), 'A' + ii );
    }

    // Get the X axis identifier (A number 1 ... n)
    if( aPosition.x < refx || aPosition.x > xg )  // Outside of X limits
        msg << wxT( "?" );
    else
    {
        ipas  = ( xg - refx ) / PAS_REF;        // ipas = X count sections
        gxpas = ( xg - refx ) / ipas;           // gxpas = X section size

        ii = ( aPosition.x - refx ) / gxpas;
        msg << ii + 1;
    }

    return msg;
}


wxString EDA_DRAW_FRAME::GetScreenDesc()
{
    wxString msg;

    msg << GetScreen()->m_ScreenNumber << wxT( "/" )
        << GetScreen()->m_NumberOfScreen;
    return msg;
}


void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    // Don't write the title block information if there is nothing to write.
    if(  !m_title.IsEmpty() || !m_date.IsEmpty() || !m_revision.IsEmpty()
      || !m_company.IsEmpty() || !m_comment1.IsEmpty() || !m_comment2.IsEmpty()
      || !m_comment3.IsEmpty() || !m_comment4.IsEmpty()  )
    {
        aFormatter->Print( aNestLevel, "(title_block\n" );

        if( !m_title.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n", EscapedUTF8( m_title ).c_str() );

        if( !m_date.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n", EscapedUTF8( m_date ).c_str() );

        if( !m_revision.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(rev %s)\n", EscapedUTF8( m_revision ).c_str() );

        if( !m_company.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(company %s)\n", EscapedUTF8( m_company ).c_str() );

        if( !m_comment1.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment1 %s)\n", EscapedUTF8( m_comment1 ).c_str() );

        if( !m_comment2.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment2 %s)\n", EscapedUTF8( m_comment2 ).c_str() );

        if( !m_comment3.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment3 %s)\n", EscapedUTF8( m_comment3 ).c_str() );

        if( !m_comment4.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment4 %s)\n", EscapedUTF8( m_comment4 ).c_str() );

        aFormatter->Print( aNestLevel, ")\n\n" );
    }
}
