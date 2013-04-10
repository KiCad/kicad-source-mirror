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

#if defined(KICAD_GOST)

// Center - right bottom corner

Ki_WorkSheetData WS_Osn1_Line1 =
{
    WS_OSN,
    &WS_Osn1_Line2,
    Mm2mils( 185 ), Mm2mils( 55 ),
    0,              Mm2mils( 55 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line2 =
{
    WS_OSN,
    &WS_Osn1_Line3,
    Mm2mils( 120 ), Mm2mils( 40 ),
    0,              Mm2mils( 40 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line3 =
{
    WS_OSN,
    &WS_Osn1_Line4,
    Mm2mils( 185 ), Mm2mils( 35 ),
    Mm2mils( 120 ), Mm2mils( 35 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line4 =
{
    WS_OSN,
    &WS_Osn1_Line5,
    Mm2mils( 50 ),  Mm2mils( 35 ),
    0,              Mm2mils( 35 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line5 =
{
    WS_OSN,
    &WS_Osn1_Line6,
    Mm2mils( 185 ), Mm2mils( 30 ),
    Mm2mils( 120 ), Mm2mils( 30 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line6 =
{
    WS_OSN,
    &WS_Osn1_Line7,
    Mm2mils( 50 ),  Mm2mils( 20 ),
    0,              Mm2mils( 20 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line7 =
{
    WS_OSN,
    &WS_Osn1_Line8,
    Mm2mils( 120 ), Mm2mils( 15 ),
    0,              Mm2mils( 15 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line8 =
{
    WS_OSN,
    &WS_Osn1_Line9,
    Mm2mils( 185 ), Mm2mils( 55 ),
    Mm2mils( 185 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line9 =
{
    WS_OSN,
    &WS_Osn1_Line10,
    Mm2mils( 178 ), Mm2mils( 55 ),
    Mm2mils( 178 ), Mm2mils( 30 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line10 =
{
    WS_OSN,
    &WS_Osn1_Line11,
    Mm2mils( 168 ), Mm2mils( 55 ),
    Mm2mils( 168 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line11 =
{
    WS_OSN,
    &WS_Osn1_Line12,
    Mm2mils( 145 ), Mm2mils( 55 ),
    Mm2mils( 145 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line12 =
{
    WS_OSN,
    &WS_Osn1_Line13,
    Mm2mils( 130 ), Mm2mils( 55 ),
    Mm2mils( 130 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line13 =
{
    WS_OSN,
    &WS_Osn1_Line14,
    Mm2mils( 120 ), Mm2mils( 55 ),
    Mm2mils( 120 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line14 =
{
    WS_OSN,
    &WS_Osn1_Line15,
    Mm2mils( 50 ),  Mm2mils( 40 ),
    Mm2mils( 50 ),  0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line15 =
{
    WS_OSN,
    &WS_Osn1_Line16,
    Mm2mils( 35 ),  Mm2mils( 40 ),
    Mm2mils( 35 ),  Mm2mils( 20 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line16 =
{
    WS_OSN,
    &WS_Osn1_Line17,
    Mm2mils( 30 ),  Mm2mils( 20 ),
    Mm2mils( 30 ),  Mm2mils( 15 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line17 =
{
    WS_OSN,
    &WS_Osn1_Line18,
    Mm2mils( 18 ),  Mm2mils( 40 ),
    Mm2mils( 18 ),  Mm2mils( 20 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line18 =
{
    WS_TONK,
    &WS_Osn1_Line19,
    Mm2mils( 185 ), Mm2mils( 50 ),
    Mm2mils( 120 ), Mm2mils( 50 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line19 =
{
    WS_TONK,
    &WS_Osn1_Line20,
    Mm2mils( 185 ), Mm2mils( 45 ),
    Mm2mils( 120 ), Mm2mils( 45 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line20 =
{
    WS_TONK,
    &WS_Osn1_Line21,
    Mm2mils( 185 ), Mm2mils( 40 ),
    Mm2mils( 120 ), Mm2mils( 40 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line21 =
{
    WS_TONK,
    &WS_Osn1_Line22,
    Mm2mils( 185 ), Mm2mils( 25 ),
    Mm2mils( 120 ), Mm2mils( 25 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line22 =
{
    WS_TONK,
    &WS_Osn1_Line23,
    Mm2mils( 185 ), Mm2mils( 20 ),
    Mm2mils( 120 ), Mm2mils( 20 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line23 =
{
    WS_TONK,
    &WS_Osn1_Line24,
    Mm2mils( 185 ), Mm2mils( 15 ),
    Mm2mils( 120 ), Mm2mils( 15 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line24 =
{
    WS_TONK,
    &WS_Osn1_Line25,
    Mm2mils( 185 ), Mm2mils( 10 ),
    Mm2mils( 120 ), Mm2mils( 10 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line25 =
{
    WS_TONK,
    &WS_Osn1_Line26,
    Mm2mils( 185 ), Mm2mils( 5 ),
    Mm2mils( 120 ), Mm2mils( 5 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line26 =
{
    WS_TONK,
    &WS_Osn1_Line27,
    Mm2mils( 45 ),  Mm2mils( 35 ),
    Mm2mils( 45 ),  Mm2mils( 20 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Line27 =
{
    WS_TONK,
    &WS_Osn1_Text1,
    Mm2mils( 40 ),  Mm2mils( 35 ),
    Mm2mils( 40 ),  Mm2mils( 20 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn1_Text1 =
{
    WS_TEXT,
    &WS_Osn1_Text2,
    Mm2mils( 181.5 ),   Mm2mils( 32.5 ),
    0,                  0,
    wxT( "Изм." ),      NULL
};

Ki_WorkSheetData WS_Osn1_Text2 =
{
    WS_TEXTL,
    &WS_Osn1_Text3,
    Mm2mils( 184 ),     Mm2mils( 27.5 ),
    0,                  0,
    wxT( "Разраб." ),   NULL
};

Ki_WorkSheetData WS_Osn1_Text3 =
{
    WS_TEXTL,
    &WS_Osn1_Text4,
    Mm2mils( 184 ),     Mm2mils( 22.5 ),
    0,                  0,
    wxT( "Пров." ),     NULL
};

Ki_WorkSheetData WS_Osn1_Text4 =
{
    WS_TEXTL,
    &WS_Osn1_Text5,
    Mm2mils( 184 ),    Mm2mils( 17.5 ),
    0,                      0,
    wxT( "Т.контр." ),      NULL
};

Ki_WorkSheetData WS_Osn1_Text5 =
{
    WS_TEXTL,
    &WS_Osn1_Text6,
    Mm2mils( 184 ),     Mm2mils( 7.5 ),
    0,                  0,
    wxT( "Н.контр." ),  NULL
};

Ki_WorkSheetData WS_Osn1_Text6 =
{
    WS_TEXTL,
    &WS_Osn1_Text7,
    Mm2mils( 184 ),     Mm2mils( 2.5 ),
    0,                      0,
    wxT( "Утв." ),          NULL
};

Ki_WorkSheetData WS_Osn1_Text7 =
{
    WS_TEXT,
    &WS_Osn1_Text8,
    Mm2mils( 173 ),     Mm2mils( 32.5 ),
    0,                      0,
    wxT( "Лист" ),          NULL
};

Ki_WorkSheetData WS_Osn1_Text8 =
{
    WS_TEXT,
    &WS_Osn1_Text9,
    Mm2mils( 156.5 ),   Mm2mils( 32.5 ),
    0,                  0,
    wxT( "N докум." ),  NULL
};

Ki_WorkSheetData WS_Osn1_Text9 =
{
    WS_TEXT,
    &WS_Osn1_Text10,
    Mm2mils( 137.5 ),   Mm2mils( 32.5 ),
    0,                  0,
    wxT( "Подп." ),     NULL
};

Ki_WorkSheetData WS_Osn1_Text10 =
{
    WS_TEXT,
    &WS_Osn1_Text11,
    Mm2mils( 125 ),     Mm2mils( 32.5 ),
    0,                  0,
    wxT( "Дата" ),      NULL
};

Ki_WorkSheetData WS_Osn1_Text11 =
{
    WS_TEXT,
    &WS_Osn1_Text12,
    Mm2mils( 42.5 ),    Mm2mils( 37.5 ),
    0,                  0,
    wxT( "Лит." ),      NULL
};

Ki_WorkSheetData WS_Osn1_Text12 =
{
    WS_TEXT,
    &WS_Osn1_Text13,
    Mm2mils( 26.5 ),    Mm2mils( 37.5 ),
    0,                      0,
    wxT( "Масса" ),         NULL
};

Ki_WorkSheetData WS_Osn1_Text13 =
{
    WS_TEXT,
    &WS_Osn1_Text14,
    Mm2mils( 9 ),       Mm2mils( 37.5 ),
    0,                  0,
    wxT( "Масштаб" ),   NULL
};

Ki_WorkSheetData WS_Osn1_Text14 =
{
    WS_TEXTL,
    &WS_Osn1_Text15,
    Mm2mils( 49 ),      Mm2mils( 17.5 ),
    0,                  0,
    wxT( "Лист" ),      NULL
};

Ki_WorkSheetData WS_Osn1_Text15 =
{
    WS_TEXTL,
    &WS_Osn1_Text16,
    Mm2mils( 29 ),      Mm2mils( 17.5 ),
    0,                  0,
    wxT( "Листов" ),    NULL
};

Ki_WorkSheetData WS_Osn1_Text16 =
{
    WS_TEXTL,
    &WS_Osn1_Text17,
    Mm2mils( 40 ),     -Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Формат" ),    NULL
};

Ki_WorkSheetData WS_Osn1_Text17 =
{
    WS_TEXTL,
    NULL,
    Mm2mils( 110 ),    -Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Копировал" ), NULL
};

Ki_WorkSheetData WS_Osn2a_Line1 =
{
    WS_OSN,
    &WS_Osn2a_Line2,
    Mm2mils( 185 ), Mm2mils( 15 ),
    0,              Mm2mils( 15 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line2 =
{
    WS_OSN,
    &WS_Osn2a_Line3,
    Mm2mils( 185 ), Mm2mils( 5 ),
    Mm2mils( 120 ), Mm2mils( 5 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line3 =
{
    WS_OSN,
    &WS_Osn2a_Line4,
    Mm2mils( 10 ),  Mm2mils( 8 ),
    0,              Mm2mils( 8 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line4 =
{
    WS_OSN,
    &WS_Osn2a_Line5,
    Mm2mils( 185 ), Mm2mils( 15 ),
    Mm2mils( 185 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line5 =
{
    WS_OSN,
    &WS_Osn2a_Line6,
    Mm2mils( 178 ), Mm2mils( 15 ),
    Mm2mils( 178 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line6 =
{
    WS_OSN,
    &WS_Osn2a_Line7,
    Mm2mils( 168 ), Mm2mils( 15 ),
    Mm2mils( 168 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line7 =
{
    WS_OSN,
    &WS_Osn2a_Line8,
    Mm2mils( 145 ), Mm2mils( 15 ),
    Mm2mils( 145 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line8 =
{
    WS_OSN,
    &WS_Osn2a_Line9,
    Mm2mils( 130 ), Mm2mils( 15 ),
    Mm2mils( 130 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line9 =
{
    WS_OSN,
    &WS_Osn2a_Line10,
    Mm2mils( 120 ), Mm2mils( 15 ),
    Mm2mils( 120 ), 0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line10 =
{
    WS_OSN,
    &WS_Osn2a_Line11,
    Mm2mils( 10 ),  Mm2mils( 15 ),
    Mm2mils( 10 ),  0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Line11 =
{
    WS_TONK,
    &WS_Osn2a_Text1,
    Mm2mils( 185 ), Mm2mils( 10 ),
    Mm2mils( 120 ), Mm2mils( 10 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_Osn2a_Text1 =
{
    WS_TEXT,
    &WS_Osn2a_Text2,
    Mm2mils( 181.5 ),   Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Изм." ),      NULL
};

Ki_WorkSheetData WS_Osn2a_Text2 =
{
    WS_TEXT,
    &WS_Osn2a_Text3,
    Mm2mils( 173 ),     Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Лист" ),      NULL
};

Ki_WorkSheetData WS_Osn2a_Text3 =
{
    WS_TEXT,
    &WS_Osn2a_Text4,
    Mm2mils( 156.5 ),   Mm2mils( 2.5 ),
    0,                  0,
    wxT( "N докум." ),  NULL
};

Ki_WorkSheetData WS_Osn2a_Text4 =
{
    WS_TEXT,
    &WS_Osn2a_Text5,
    Mm2mils( 137.5 ),   Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Подп." ),     NULL
};

Ki_WorkSheetData WS_Osn2a_Text5 =
{
    WS_TEXT,
    &WS_Osn2a_Text6,
    Mm2mils( 125 ),     Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Дата" ),      NULL
};

Ki_WorkSheetData WS_Osn2a_Text6 =
{
    WS_TEXT,
    &WS_Osn2a_Text7,
    Mm2mils( 5 ),       Mm2mils( 11.5 ),
    0,                  0,
    wxT( "Лист" ),      NULL
};

Ki_WorkSheetData WS_Osn2a_Text7 =
{
    WS_TEXTL,
    &WS_Osn2a_Text8,
    Mm2mils( 40 ),     -Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Формат" ),    NULL
};

Ki_WorkSheetData WS_Osn2a_Text8 =
{
    WS_TEXTL,
    NULL,
    Mm2mils( 110 ),    -Mm2mils( 2.5 ),
    0,                  0,
    wxT( "Копировал" ), NULL
};

// Center - left bottom corner

Ki_WorkSheetData WS_DopLeft_Line1 =
{
    WS_OSN,
    &WS_DopLeft_Line2,
    Mm2mils( 12 ),  Mm2mils( 145 ),
    0,              Mm2mils( 145 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line2 =
{
    WS_OSN,
    &WS_DopLeft_Line3,
    Mm2mils( 12 ),  Mm2mils( 110 ),
    0,              Mm2mils( 110 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line3 =
{
    WS_OSN,
    &WS_DopLeft_Line4,
    Mm2mils( 12 ),  Mm2mils( 85 ),
    0,              Mm2mils( 85 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line4 =
{
    WS_OSN,
    &WS_DopLeft_Line5,
    Mm2mils( 12 ),  Mm2mils( 60 ),
    0,              Mm2mils( 60 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line5 =
{
    WS_OSN,
    &WS_DopLeft_Line6,
    Mm2mils( 12 ),  Mm2mils( 25 ),
    0,              Mm2mils( 25 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line6 =
{
    WS_OSN,
    &WS_DopLeft_Line7,
    Mm2mils( 12 ),  0,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line7 =
{
    WS_OSN,
    &WS_DopLeft_Line8,
    Mm2mils( 12 ),  Mm2mils( 145 ),
    Mm2mils( 12 ),  0,
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line8 =
{
    WS_OSN,
    &WS_DopLeft_Text1,
    Mm2mils( 7 ),   Mm2mils( 145 ),
    Mm2mils( 7 ),   0,
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Text1 =
{
    WS_TEXT,
    &WS_DopLeft_Text2,
    Mm2mils( 9.5 ),         Mm2mils( 12.5 ),
    0,                      0,
    wxT( "Инв.N подл." ),   NULL
};

Ki_WorkSheetData WS_DopLeft_Text2 =
{
    WS_TEXT,
    &WS_DopLeft_Text3,
    Mm2mils( 9.5 ),         Mm2mils( 42.5 ),
    0,                      0,
    wxT( "Подп. и дата" ),  NULL
};

Ki_WorkSheetData WS_DopLeft_Text3 =
{
    WS_TEXT,
    &WS_DopLeft_Text4,
    Mm2mils( 9.5 ),         Mm2mils( 72.5 ),
    0,                      0,
    wxT( "Взам.инв.N" ),    NULL
};

Ki_WorkSheetData WS_DopLeft_Text4 =
{
    WS_TEXT,
    &WS_DopLeft_Text5,
    Mm2mils( 9.5 ),         Mm2mils( 97.5 ),
    0,                      0,
    wxT( "Инв.N дубл." ),   NULL
};

Ki_WorkSheetData WS_DopLeft_Text5 =
{
    WS_TEXT,
    &WS_DopLeft_Line9,
    Mm2mils( 9.5 ),         Mm2mils( 127.5 ),
    0,                      0,
    wxT( "Подп. и дата" ),  NULL
};

Ki_WorkSheetData WS_DopLeft_Line9 =
{
    WS_OSN,
    &WS_DopLeft_Line10,
    Mm2mils( 7 ),  Mm2mils( 287 ),
    Mm2mils( 7 ),  Mm2mils( 167 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line10 =
{
    WS_OSN,
    &WS_DopLeft_Line11,
    Mm2mils( 12 ),  Mm2mils( 287 ),
    Mm2mils( 12 ),  Mm2mils( 167 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line11 =
{
    WS_OSN,
    &WS_DopLeft_Line12,
    Mm2mils( 12 ),  Mm2mils( 287 ),
    Mm2mils( 12 ),  Mm2mils( 167 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line12 =
{
    WS_OSN,
    &WS_DopLeft_Line13,
    Mm2mils( 12 ),  Mm2mils( 167 ),
    0,              Mm2mils( 167 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line13 =
{
    WS_OSN,
    &WS_DopLeft_Line14,
    Mm2mils( 12 ),  Mm2mils( 227 ),
    0,              Mm2mils( 227 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Line14 =
{
    WS_OSN,
    &WS_DopLeft_Text6,
    Mm2mils( 12 ),  Mm2mils( 287 ),
    0,              Mm2mils( 287 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopLeft_Text6 =
{
    WS_TEXT,
    &WS_DopLeft_Text7,
    Mm2mils( 9.5 ),         Mm2mils( 197 ),
    0,                      0,
    wxT( "Справ. N" ),      NULL
};

Ki_WorkSheetData WS_DopLeft_Text7 =
{
    WS_TEXT,
    NULL,
    Mm2mils( 9.5 ),         Mm2mils( 257 ),
    0,                      0,
    wxT( "Перв. примен." ), NULL
};

// Center - left top corner

Ki_WorkSheetData WS_DopTop_Line1 =
{
    WS_OSN,
    &WS_DopTop_Line2,
    Mm2mils( 70 ),  0,
    Mm2mils( 70 ),  Mm2mils( 14 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopTop_Line2 =
{
    WS_OSN,
    &WS_DopTop_Line3,
    Mm2mils( 70 ),  Mm2mils( 14 ),
    0,              Mm2mils( 14 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopTop_Line3 =
{
    WS_OSN,
    &WS_DopTop_Line4,
    Mm2mils( 70 ),  Mm2mils( 14 ),
    Mm2mils( 137 ), Mm2mils( 14 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopTop_Line4 =
{
    WS_OSN,
    &WS_DopTop_Line5,
    Mm2mils( 84 ),  Mm2mils( 7 ),
    Mm2mils( 137 ), Mm2mils( 7 ),
    NULL,           NULL
};

Ki_WorkSheetData WS_DopTop_Line5 =
{
    WS_OSN,
    &WS_DopTop_Line6,
    Mm2mils( 84 ),  Mm2mils( 14 ),
    Mm2mils( 84 ),  0,
    NULL,           NULL
};

Ki_WorkSheetData WS_DopTop_Line6 =
{
    WS_OSN,
    NULL,
    Mm2mils( 137 ), Mm2mils( 14 ),
    Mm2mils( 137 ), 0,
    NULL,           NULL
};

#else

Ki_WorkSheetData WS_Date =
{
    WS_DATE,
    &WS_Licence,
    BLOCK_DATE_X,   BLOCK_DATE_Y,
    0,              0,
    wxT( "Date: " ),NULL
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
    BLOCK_REV_X,     BLOCK_REV_Y,
    0,               0,
    wxT( "Rev: " ),  NULL
};

Ki_WorkSheetData WS_SizeSheet =
{
    WS_SIZESHEET,
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

Ki_WorkSheetData WS_Title =
{
    WS_TITLE,
    &WS_SheetFilename,
    BLOCK_TITLE_X,    BLOCK_TITLE_Y,
    0,                0,
    wxT( "Title: " ), NULL
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
    BLOCK_COMMENT_X,BLOCK_COMPANY_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment1 =
{
    WS_COMMENT1,
    &WS_Comment2,
    BLOCK_COMMENT_X,BLOCK_COMMENT1_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment2 =
{
    WS_COMMENT2,
    &WS_Comment3,
    BLOCK_COMMENT_X,BLOCK_COMMENT2_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment3 =
{
    WS_COMMENT3,
    &WS_Comment4,
    BLOCK_COMMENT_X,BLOCK_COMMENT3_Y,
    0,              0,
    NULL,           NULL
};

Ki_WorkSheetData WS_Comment4 =
{
    WS_COMMENT4,
    &WS_MostLeftLine,
    BLOCK_COMMENT_X, BLOCK_COMMENT4_Y,
    0,               0,
    NULL,            NULL
};


/// Left vertical segment
Ki_WorkSheetData WS_MostLeftLine =
{
    WS_LEFT_SEGMENT,
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
    NULL,             NULL
};


/// superior horizontal segment
Ki_WorkSheetData WS_MostUpperLine =
{
    WS_UPPER_SEGMENT,
    &WS_Segm3,
    BLOCK_OX,        SIZETEXT * 16,
    0,               SIZETEXT * 16,
    NULL,            NULL
};


/// horizontal segment above COMPANY NAME
Ki_WorkSheetData WS_Segm3 =
{
    WS_SEGMENT,
    &WS_Segm4,
    BLOCK_OX,   SIZETEXT * 6,
    0,          SIZETEXT * 6,
    NULL,       NULL
};


/// vertical segment of the left REV and SHEET
Ki_WorkSheetData WS_Segm4 =
{
    WS_SEGMENT,
    &WS_Segm5,
    BLOCK_REV_X + SIZETEXT,SIZETEXT * 4,
    BLOCK_REV_X + SIZETEXT,0,
    NULL,                  NULL
};


Ki_WorkSheetData WS_Segm5 =
{
    WS_SEGMENT,
    &WS_Segm6,
    BLOCK_OX,  SIZETEXT * 2,
    0,         SIZETEXT * 2,
    NULL,      NULL
};


Ki_WorkSheetData WS_Segm6 =
{
    WS_SEGMENT,
    &WS_Segm7,
    BLOCK_OX,   SIZETEXT * 4,
    0,          SIZETEXT * 4,
    NULL,       NULL
};


Ki_WorkSheetData WS_Segm7 =
{
    WS_SEGMENT,
    NULL,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 4,
    BLOCK_OX - (SIZETEXT * 11),SIZETEXT * 2,
    NULL,                      NULL
};

#endif

void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* aDC, BASE_SCREEN* aScreen, int aLineWidth,
                                     double aScalar, const wxString &aFilename )
{
    if( !m_showBorderAndTitleBlock )
        return;

    const PAGE_INFO&  pageInfo = GetPageSettings();
    wxSize  pageSize = pageInfo.GetSizeMils();

    // if not printing, draw the page limits:
    if( !aScreen->m_IsPrinting && g_ShowPageLimits )
    {
        GRSetDrawMode( aDC, GR_COPY );
        GRRect( m_canvas->GetClipBox(), aDC, 0, 0,
                pageSize.x * aScalar, pageSize.y * aScalar, aLineWidth,
                g_DrawBgColor == WHITE ? LIGHTGRAY : DARKDARKGRAY );
    }

    wxPoint margin_left_top( pageInfo.GetLeftMarginMils(), pageInfo.GetTopMarginMils() );
    wxPoint margin_right_bottom( pageInfo.GetRightMarginMils(), pageInfo.GetBottomMarginMils() );
    wxString paper = pageInfo.GetType();
    wxString file = aFilename;
    TITLE_BLOCK t_block = GetTitleBlock();
    int number_of_screens = aScreen->m_NumberOfScreens;
    int screen_to_draw = aScreen->m_ScreenNumber;

    TraceWorkSheet( aDC, pageSize, margin_left_top, margin_right_bottom,
                    paper, file, t_block, number_of_screens, screen_to_draw,
                    aLineWidth, aScalar );
}


void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* aDC, wxSize& aSz, wxPoint& aLT, wxPoint& aRB,
                                     wxString& aType, wxString& aFlNm, TITLE_BLOCK& aTb,
                                     int aNScr, int aScr, int aLnW, double aScalar,
                                     EDA_COLOR_T aClr1, EDA_COLOR_T aClr2 )
{
    wxPoint pos;
    wxPoint end;
    int refx, refy;
    wxString Line;
    Ki_WorkSheetData* WsItem;
    wxSize size( SIZETEXT * aScalar, SIZETEXT * aScalar );
    wxSize size_ref( SIZETEXT_REF * aScalar, SIZETEXT_REF * aScalar );
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

    int lnMsg, ln;
    int lnWosn = aLnW * 2;
    int lnWtonk = aLnW;
    wxSize sz;
    wxSize size0_8( SIZETEXT * aScalar * 0.8, SIZETEXT * aScalar * 1 );
    wxSize size1_5( SIZETEXT * aScalar * 1.5, SIZETEXT * aScalar * 1.5 );
    wxSize size2( SIZETEXT * aScalar * 2, SIZETEXT * aScalar * 2 );
    wxSize size3( SIZETEXT * aScalar * 3, SIZETEXT * aScalar * 3 );

    // Draw the border.
    GRRect( m_canvas->GetClipBox(), aDC, refx * aScalar, refy * aScalar,
            xg * aScalar, yg * aScalar, lnWosn, aClr1 );

    // Center - right bottom corner
    refx = aSz.x - aRB.x;
    refy = aSz.y - aRB.y;

    // First page
    if( aScr == 1 )
    {
        for( WsItem = &WS_Osn1_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            pos.x = (refx - WsItem->m_Posx) * aScalar;
            pos.y = (refy - WsItem->m_Posy) * aScalar;
            end.x = (refx - WsItem->m_Endx) * aScalar;
            end.y = (refy - WsItem->m_Endy) * aScalar;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWosn, aClr1 );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aClr1 );
                break;

            case WS_TEXT:
                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn1_Text1 )
                        DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                         msg, TEXT_ORIENT_HORIZ, size0_8,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aLnW, false, false );
                    else
                        DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                         msg, TEXT_ORIENT_HORIZ, size,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aLnW, false, false );
                }
                break;

            case WS_TEXTL:
                if( !msg.IsEmpty() )
                    DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                break;

            }
        }

        // Sheet number
        if( aNScr > 1 )
        {
            pos.x = (refx - Mm2mils( 36 )) * aScalar;
            pos.y = (refy - Mm2mils( 17.5 )) * aScalar;
            msg.Empty();
            msg << aScr;
            DrawGraphicText( m_canvas, aDC, pos, aClr1, msg,
                             TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_CENTER,
                             GR_TEXT_VJUSTIFY_CENTER, aLnW, false, false );
        }

        // Count of sheets
        pos.x = (refx - Mm2mils( 10 )) * aScalar;
        pos.y = (refy - Mm2mils( 17.5 )) * aScalar;
        msg.Empty();
        msg << aNScr;
        DrawGraphicText( m_canvas, aDC, pos, aClr1, msg,
                         TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_LEFT,
                         GR_TEXT_VJUSTIFY_CENTER, aLnW, false, false );

        // Company name
        msg = aTb.GetCompany();
        if( !msg.IsEmpty() )
        {
            sz = size1_5;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 49 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 25 )) * aScalar;
            pos.y = (refy - Mm2mils( 7.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }

        // Title
        msg = aTb.GetTitle();
        if( !msg.IsEmpty() )
        {
            sz = size1_5;
            wxArrayString lines;
            int titleWidth = 0;
            int titleHeight = (sz.y + sz.y * 0.5) / aScalar;
            int titleFieldWidth = Mm2mils( 69 );
            int titleFieldHeight = Mm2mils( 24 );
            int index = 0;
            wxString fullMsg = msg;
            do // Reduce the height of wrapped title until the fit
            {
                do // Wrap the title
                {
                    titleWidth = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
                    if( titleWidth > titleFieldWidth )
                    {
                        index = 0;
                        do
                        {
                            msg = msg.Left( msg.Length() - 1 );
                            if( msg.Length() == 0 )
                            {
                                lines.Clear();
                                msg = fullMsg;
                                sz.x -= aScalar;
                                break;
                            }
                            else
                            {
                                index++;
                                titleWidth = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;

                                wxString ch = wxString( msg.Last() );
                                if( titleWidth < titleFieldWidth && ch == wxT( " " ) )
                                {
                                    // New sentence on a new line
                                    int dot = msg.Index( wxT( ". " ) );
                                    if( dot != wxNOT_FOUND )
                                    {
                                        index += msg.Length() - dot - 2;
                                        msg = msg.Left( dot + 1 );
                                        lines.Add( msg );
                                        msg = fullMsg.Right( index );
                                        break;
                                    }
                                    else
                                    {
                                        msg = msg.Left( msg.Length() - 1 );
                                        lines.Add( msg );
                                        msg = fullMsg.Right( index );
                                        break;
                                    }
                                }
                            }
                        }while( 1 );
                    }
                    else
                    {
                        // New sentence on a new line
                        int dot = msg.Index( wxT( ". " ) );
                        if( dot != wxNOT_FOUND )
                        {
                            lines.Add( msg.Left( dot + 1 ) );
                            lines.Add( fullMsg.Right( msg.Length() - dot - 2 ) );
                        }
                        else
                            lines.Add( msg );
                        break;
                    }
                }while( 1 );

                if( titleFieldHeight < titleHeight * lines.Count() )
                {
                    sz.y -= aScalar;
                    sz.x -= aScalar;
                    msg = fullMsg;
                    lines.Clear();
                }
                else
                    break;
            }while( 1 );

            pos.x = (refx - Mm2mils( 85 )) * aScalar;
            pos.y = (refy - Mm2mils( 27.5 ) - (titleHeight * (lines.Count() - 1) / 2)) * aScalar;

            for( int curLn = 0; curLn < lines.Count(); curLn++ )
            {
                msg = lines[curLn];
                DrawGraphicText( m_canvas, aDC, pos, aClr2,
                                 msg, TEXT_ORIENT_HORIZ, sz,
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
                pos.y += titleHeight * aScalar;
            }
        }

        // Decimal number
        msg = aTb.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size3;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 119 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 60 )) * aScalar;
            pos.y = (refy - Mm2mils( 47.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }

        // Developer
        msg = aTb.GetComment2();
        if( !msg.IsEmpty() )
        {
            sz = size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167.5 )) * aScalar;
            pos.y = (refy - Mm2mils( 27.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }

        // Verifier
        msg = aTb.GetComment3();
        if( !msg.IsEmpty() )
        {
            sz = size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167 )) * aScalar;
            pos.y = (refy - Mm2mils( 22.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }

        // Approver
        msg = aTb.GetComment4();
        if( !msg.IsEmpty() )
        {
            sz = size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167 )) * aScalar;
            pos.y = (refy - Mm2mils( 2.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }
    }
    else // other pages
    {
        for( WsItem = &WS_Osn2a_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            pos.x = (refx - WsItem->m_Posx) * aScalar;
            pos.y = (refy - WsItem->m_Posy) * aScalar;
            end.x = (refx - WsItem->m_Endx) * aScalar;
            end.y = (refy - WsItem->m_Endy) * aScalar;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWosn, aClr1 );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aClr1 );
                break;

            case WS_TEXT:
                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn2a_Text1 )
                        DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                         msg, TEXT_ORIENT_HORIZ, size0_8,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aLnW, false, false );
                    else
                        DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                         msg, TEXT_ORIENT_HORIZ, size,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aLnW, false, false );
                }
                break;

            case WS_TEXTL:
                if( !msg.IsEmpty() )
                    DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aLnW, false, false );
                break;
            }
        }

        // Sheet number
        pos.x = (refx - Mm2mils( 5 )) * aScalar;
        pos.y = (refy - Mm2mils( 4 )) * aScalar;
        msg.Empty();
        msg << aScr;
        DrawGraphicText( m_canvas, aDC, pos, aClr1, msg,
                         TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_CENTER,
                         GR_TEXT_VJUSTIFY_CENTER, aLnW, false, false );

        // Decimal number
        msg = aTb.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size3;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 109 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 65 )) * aScalar;
            pos.y = (refy - Mm2mils( 7.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }
    }

    // Format
    pos.x = (refx - Mm2mils( 23 )) * aScalar;
    pos.y = (refy + Mm2mils( 2.5 )) * aScalar;
    msg.Empty();
    msg << aType;
    DrawGraphicText( m_canvas, aDC, pos, aClr1,
                     msg, TEXT_ORIENT_HORIZ, size,
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                     aLnW, false, false );

    // Center - left bottom corner
    refx = aLT.x;
    refy = aSz.y - aRB.y;
    for( WsItem = &WS_DopLeft_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        if( aScr > 1 && WsItem == &WS_DopLeft_Line9 ) // Some fields for first page only
            break;

        pos.x = (refx - WsItem->m_Posx) * aScalar;
        pos.y = (refy - WsItem->m_Posy) * aScalar;
        end.x = (refx - WsItem->m_Endx) * aScalar;
        end.y = (refy - WsItem->m_Endy) * aScalar;
        msg = WsItem->m_Legende;
        switch( WsItem->m_Type )
        {
        case WS_OSN:
            GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                    lnWosn, aClr1 );
            break;

        case WS_TEXT:
            if( !msg.IsEmpty() )
                DrawGraphicText( m_canvas, aDC, pos, aClr1,
                                 msg, TEXT_ORIENT_VERT, size,
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                 aLnW, false, false );
            break;
        }
    }

    if( aType == PAGE_INFO::A4 || aSz.x > aSz.y ) // A4 or Landscape
    {
        // Center - left top corner
        refx = aLT.x;
        refy = aLT.y;
        for( WsItem = &WS_DopTop_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            if( aScr > 1 && WsItem == &WS_DopTop_Line3 ) // Some fields for first page only
                break;

            pos.x = (refx + WsItem->m_Posx) * aScalar;
            pos.y = (refy + WsItem->m_Posy) * aScalar;
            end.x = (refx + WsItem->m_Endx) * aScalar;
            end.y = (refy + WsItem->m_Endy) * aScalar;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWosn, aClr1 );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aClr1 );
                break;
            }
        }

        // Decimal number
        msg = aTb.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size2;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 69 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx + Mm2mils( 35 )) * aScalar;
            pos.y = (refy + Mm2mils( 7 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, 1800, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }
    }
    else // Portrait
    {
        // Center - right top corner
        // Lines are used from the upper left corner by the change of coordinates
        refx = aSz.x - aRB.x;
        refy = aLT.y;
        for( WsItem = &WS_DopTop_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            if( aScr > 1 && WsItem == &WS_DopTop_Line3 ) // Some fields for first page only
                break;

            pos.x = (refx - WsItem->m_Posy) * aScalar;
            pos.y = (refy + WsItem->m_Posx) * aScalar;
            end.x = (refx - WsItem->m_Endy) * aScalar;
            end.y = (refy + WsItem->m_Endx) * aScalar;
            msg = WsItem->m_Legende;
            switch( WsItem->m_Type )
            {
            case WS_OSN:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWosn, aClr1 );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aClr1 );
                break;
            }
        }

        // Decimal number
        msg = aTb.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size2;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 69 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 7 )) * aScalar;
            pos.y = (refy + Mm2mils( 35 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aClr2,
                             msg, TEXT_ORIENT_VERT, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aLnW, false, false );
        }
    }

#else

    // Draw the border.
    int ii, jj, ipas, gxpas, gypas;
    for( ii = 0; ii < 2; ii++ )
    {
        GRRect( m_canvas->GetClipBox(), aDC, refx * aScalar, refy * aScalar,
                xg * aScalar, yg * aScalar, aLnW, aClr1 );

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
            GRLine( m_canvas->GetClipBox(), aDC, ii * aScalar, refy * aScalar,
                    ii * aScalar, ( refy + GRID_REF_W ) * aScalar, aLnW, aClr1 );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( ii - gxpas / 2 ) * aScalar,
                                  ( refy + GRID_REF_W / 2 ) * aScalar ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );

        if( ii < xg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, ii * aScalar, yg * aScalar,
                    ii * aScalar, ( yg - GRID_REF_W ) * aScalar, aLnW, aClr1 );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( ii - gxpas / 2 ) * aScalar,
                                  ( yg - GRID_REF_W / 2) * aScalar ),
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
            GRLine( m_canvas->GetClipBox(), aDC, refx * aScalar, ii * aScalar,
                    ( refx + GRID_REF_W ) * aScalar, ii * aScalar, aLnW, aClr1 );
        }

        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( refx + GRID_REF_W / 2 ) * aScalar,
                                  ( ii - gypas / 2 ) * aScalar ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );

        if( ii < yg - PAS_REF / 2 )
        {
            GRLine( m_canvas->GetClipBox(), aDC, xg * aScalar, ii * aScalar,
                    ( xg - GRID_REF_W ) * aScalar, ii * aScalar, aLnW, aClr1 );
        }
        DrawGraphicText( m_canvas, aDC,
                         wxPoint( ( xg - GRID_REF_W / 2 ) * aScalar,
                                  ( ii - gxpas / 2 ) * aScalar ),
                         aClr1, Line, TEXT_ORIENT_HORIZ, size_ref,
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         aLnW, false, false );
    }

    int UpperLimit = VARIABLE_BLOCK_START_POSITION;
    refx = aSz.x - aRB.x  - GRID_REF_W;
    refy = aSz.y - aRB.y - GRID_REF_W;

    for( WsItem = &WS_Date; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        pos.x = (refx - WsItem->m_Posx) * aScalar;
        pos.y = (refy - WsItem->m_Posy) * aScalar;
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
                                 GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
             }
            msg = aTb.GetRevision();
            DrawGraphicText( m_canvas, aDC, pos, aClr2, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
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
                wxFileName fn( aFlNm );

                if( WsItem->m_Legende )
                    msg = WsItem->m_Legende;

                msg << fn.GetFullName();
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
                                 GetPenSizeForBold( std::min( size.x, size.y ) ),
                                 false, true );
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_TITLE:
            if( WsItem->m_Legende )
            {
                msg = WsItem->m_Legende;
                DrawGraphicText( m_canvas, aDC, pos, aClr1, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                 GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
                pos.x += ReturnGraphicTextWidth( msg, size.x, false, false );
            }
            msg = aTb.GetTitle();
            DrawGraphicText( m_canvas, aDC, pos, aClr2, msg, TEXT_ORIENT_HORIZ, size,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             GetPenSizeForBold( std::min( size.x, size.y ) ), false, true );
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
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
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
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
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
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
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
                UpperLimit = std::max( UpperLimit, WsItem->m_Posy + SIZETEXT );
            }
            break;

        case WS_UPPER_SEGMENT:
            if( UpperLimit == 0 )
                break;

        case WS_LEFT_SEGMENT:
            WS_MostUpperLine.m_Posy =
                WS_MostUpperLine.m_Endy    =
                    WS_MostLeftLine.m_Posy = UpperLimit;
            pos.y = (refy - WsItem->m_Posy) * aScalar;

        case WS_SEGMENT:
            xg = aSz.x - GRID_REF_W - aRB.x - WsItem->m_Endx;
            yg = aSz.y - GRID_REF_W - aRB.y - WsItem->m_Endy;
            GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y,
                    xg * aScalar, yg * aScalar, aLnW, aClr1 );
            break;
        }
    }

#endif

}


const wxString EDA_DRAW_FRAME::GetXYSheetReferences( const wxPoint& aPosition ) const
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
        << GetScreen()->m_NumberOfScreens;
    return msg;
}


void TITLE_BLOCK::Format( OUTPUTFORMATTER* aFormatter, int aNestLevel, int aControlBits ) const
    throw( IO_ERROR )
{
    // Don't write the title block information if there is nothing to write.
    if(  !m_title.IsEmpty() || /* !m_date.IsEmpty() || */ !m_revision.IsEmpty()
      || !m_company.IsEmpty() || !m_comment1.IsEmpty() || !m_comment2.IsEmpty()
      || !m_comment3.IsEmpty() || !m_comment4.IsEmpty()  )
    {
        aFormatter->Print( aNestLevel, "(title_block \n" );

        if( !m_title.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(title %s)\n",
                               aFormatter->Quotew( m_title ).c_str() );

        /* version control users were complaining, see mailing list.
        if( !m_date.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(date %s)\n",
                               aFormatter->Quotew( m_date ).c_str() );
        */

        if( !m_revision.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(rev %s)\n",
                               aFormatter->Quotew( m_revision ).c_str() );

        if( !m_company.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(company %s)\n",
                               aFormatter->Quotew( m_company ).c_str() );

        if( !m_comment1.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 1 %s)\n",
                               aFormatter->Quotew( m_comment1 ).c_str() );

        if( !m_comment2.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 2 %s)\n",
                               aFormatter->Quotew( m_comment2 ).c_str() );

        if( !m_comment3.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 3 %s)\n",
                               aFormatter->Quotew( m_comment3 ).c_str() );

        if( !m_comment4.IsEmpty() )
            aFormatter->Print( aNestLevel+1, "(comment 4 %s)\n",
                               aFormatter->Quotew( m_comment4 ).c_str() );

        aFormatter->Print( aNestLevel, ")\n\n" );
    }
}
