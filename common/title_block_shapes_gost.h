/**
 * @file title_block_shape_gost.h
 * @brief description of graphic items and texts to build a title block
 * using GOST standard
 */

/*
 * This file should be included only in worksheet.cpp
 * This is not an usual .h file, it is more a .cpp file
 * it creates a lot of structures to define the shape of a title block
 * and frame references
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


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

void EDA_DRAW_FRAME::TraceWorkSheet( wxDC* aDC, wxSize& aPageSize,
                                     wxPoint& aLTmargin, wxPoint& aRBmargin,
                                     wxString& aPaperFormat, wxString& aFileName,
                                     TITLE_BLOCK& aTitleBlock,
                                     int aSheetCount, int aSheetNumber,
                                     int aPenWidth, double aScalar,
                                     EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
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
    refx = aLTmargin.x;
    refy = aLTmargin.y;

    // lower right corner
    int xg, yg;
    xg   = aPageSize.x - aRBmargin.x;
    yg   = aPageSize.y - aRBmargin.y;

    int lnMsg, ln;
    int lnWosn = aPenWidth * 2;
    int lnWtonk = aPenWidth;
    wxSize sz;
    wxSize size0_8( SIZETEXT * aScalar * 0.8, SIZETEXT * aScalar * 1 );
    wxSize size1_5( SIZETEXT * aScalar * 1.5, SIZETEXT * aScalar * 1.5 );
    wxSize size2( SIZETEXT * aScalar * 2, SIZETEXT * aScalar * 2 );
    wxSize size3( SIZETEXT * aScalar * 3, SIZETEXT * aScalar * 3 );

    // Draw the border.
    GRRect( m_canvas->GetClipBox(), aDC, refx * aScalar, refy * aScalar,
            xg * aScalar, yg * aScalar, lnWosn, aLineColor );

    // Center - right bottom corner
    refx = aPageSize.x - aRBmargin.x;
    refy = aPageSize.y - aRBmargin.y;

    // First page
    if( aSheetNumber == 1 )
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
                        lnWosn, aLineColor );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aLineColor );
                break;

            case WS_TEXT:
                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn1_Text1 )
                        DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                         msg, TEXT_ORIENT_HORIZ, size0_8,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aPenWidth, false, false );
                    else
                        DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                         msg, TEXT_ORIENT_HORIZ, size,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aPenWidth, false, false );
                }
                break;

            case WS_TEXTL:
                if( !msg.IsEmpty() )
                    DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aPenWidth, false, false );
                break;

            }
        }

        // Sheet number
        if( aSheetCount > 1 )
        {
            pos.x = (refx - Mm2mils( 36 )) * aScalar;
            pos.y = (refy - Mm2mils( 17.5 )) * aScalar;
            msg.Empty();
            msg << aSheetNumber;
            DrawGraphicText( m_canvas, aDC, pos, aLineColor, msg,
                             TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_CENTER,
                             GR_TEXT_VJUSTIFY_CENTER, aPenWidth, false, false );
        }

        // Count of sheets
        pos.x = (refx - Mm2mils( 10 )) * aScalar;
        pos.y = (refy - Mm2mils( 17.5 )) * aScalar;
        msg.Empty();
        msg << aSheetCount;
        DrawGraphicText( m_canvas, aDC, pos, aLineColor, msg,
                         TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_LEFT,
                         GR_TEXT_VJUSTIFY_CENTER, aPenWidth, false, false );

        // Company name
        msg = aTitleBlock.GetCompany();
        if( !msg.IsEmpty() )
        {
            sz = size1_5;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 49 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 25 )) * aScalar;
            pos.y = (refy - Mm2mils( 7.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }

        // Title
        msg = aTitleBlock.GetTitle();
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
                DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                                 msg, TEXT_ORIENT_HORIZ, sz,
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
                pos.y += titleHeight * aScalar;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size3;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 119 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 60 )) * aScalar;
            pos.y = (refy - Mm2mils( 47.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }

        // Developer
        msg = aTitleBlock.GetComment2();
        if( !msg.IsEmpty() )
        {
            sz = size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167.5 )) * aScalar;
            pos.y = (refy - Mm2mils( 27.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }

        // Verifier
        msg = aTitleBlock.GetComment3();
        if( !msg.IsEmpty() )
        {
            sz = size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167 )) * aScalar;
            pos.y = (refy - Mm2mils( 22.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }

        // Approver
        msg = aTitleBlock.GetComment4();
        if( !msg.IsEmpty() )
        {
            sz = size;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 22 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 167 )) * aScalar;
            pos.y = (refy - Mm2mils( 2.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
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
                        lnWosn, aLineColor );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aLineColor );
                break;

            case WS_TEXT:
                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn2a_Text1 )
                        DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                         msg, TEXT_ORIENT_HORIZ, size0_8,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aPenWidth, false, false );
                    else
                        DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                         msg, TEXT_ORIENT_HORIZ, size,
                                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                         aPenWidth, false, false );
                }
                break;

            case WS_TEXTL:
                if( !msg.IsEmpty() )
                    DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                     msg, TEXT_ORIENT_HORIZ, size,
                                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                                     aPenWidth, false, false );
                break;
            }
        }

        // Sheet number
        pos.x = (refx - Mm2mils( 5 )) * aScalar;
        pos.y = (refy - Mm2mils( 4 )) * aScalar;
        msg.Empty();
        msg << aSheetNumber;
        DrawGraphicText( m_canvas, aDC, pos, aLineColor, msg,
                         TEXT_ORIENT_HORIZ, size, GR_TEXT_HJUSTIFY_CENTER,
                         GR_TEXT_VJUSTIFY_CENTER, aPenWidth, false, false );

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size3;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 109 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 65 )) * aScalar;
            pos.y = (refy - Mm2mils( 7.5 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_HORIZ, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }
    }

    // Format
    pos.x = (refx - Mm2mils( 23 )) * aScalar;
    pos.y = (refy + Mm2mils( 2.5 )) * aScalar;
    msg.Empty();
    msg << aPaperFormat;
    DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                     msg, TEXT_ORIENT_HORIZ, size,
                     GR_TEXT_HJUSTIFY_LEFT, GR_TEXT_VJUSTIFY_CENTER,
                     aPenWidth, false, false );

    // Center - left bottom corner
    refx = aLTmargin.x;
    refy = aPageSize.y - aRBmargin.y;
    for( WsItem = &WS_DopLeft_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        if( aSheetNumber > 1 && WsItem == &WS_DopLeft_Line9 ) // Some fields for first page only
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
                    lnWosn, aLineColor );
            break;

        case WS_TEXT:
            if( !msg.IsEmpty() )
                DrawGraphicText( m_canvas, aDC, pos, aLineColor,
                                 msg, TEXT_ORIENT_VERT, size,
                                 GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                                 aPenWidth, false, false );
            break;
        }
    }

    if( aPaperFormat == PAGE_INFO::A4 || aPageSize.x > aPageSize.y ) // A4 or Landscape
    {
        // Center - left top corner
        refx = aLTmargin.x;
        refy = aLTmargin.y;
        for( WsItem = &WS_DopTop_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            if( aSheetNumber > 1 && WsItem == &WS_DopTop_Line3 ) // Some fields for first page only
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
                        lnWosn, aLineColor );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aLineColor );
                break;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size2;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 69 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx + Mm2mils( 35 )) * aScalar;
            pos.y = (refy + Mm2mils( 7 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, 1800, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }
    }
    else // Portrait
    {
        // Center - right top corner
        // Lines are used from the upper left corner by the change of coordinates
        refx = aPageSize.x - aRBmargin.x;
        refy = aLTmargin.y;
        for( WsItem = &WS_DopTop_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            if( aSheetNumber > 1 && WsItem == &WS_DopTop_Line3 ) // Some fields for first page only
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
                        lnWosn, aLineColor );
                break;

            case WS_TONK:
                GRLine( m_canvas->GetClipBox(), aDC, pos.x, pos.y, end.x, end.y,
                        lnWtonk, aLineColor );
                break;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();
        if( !msg.IsEmpty() )
        {
            sz = size2;
            lnMsg = ReturnGraphicTextWidth( msg, sz.x, false, false ) / aScalar;
            ln = Mm2mils( 69 );
            if( lnMsg > ln )
                sz.x *= float( ln ) / lnMsg;
            pos.x = (refx - Mm2mils( 7 )) * aScalar;
            pos.y = (refy + Mm2mils( 35 )) * aScalar;
            DrawGraphicText( m_canvas, aDC, pos, aTextColor,
                             msg, TEXT_ORIENT_VERT, sz,
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             aPenWidth, false, false );
        }
    }
}
