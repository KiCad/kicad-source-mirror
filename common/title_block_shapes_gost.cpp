/**
 * @file title_block_shapes_gost.cpp
 * @brief description of graphic items and texts to build a title block
 * using GOST standard
 */

/*
 * This file creates a lot of structures to define the shape of a title block
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

#include <fctsys.h>
#include <drawtxt.h>
#include <appl_wxstruct.h>
#include <worksheet.h>
#include <class_title_block.h>
#include <build_version.h>
#include <worksheet_shape_builder.h>

#define TEXTSIZE        100             // worksheet text size

// Work sheet structure type definitions.
enum TypeKi_WorkSheetData {
    WS_OSN,
    WS_TONK,
    WS_TEXT,
    WS_TEXTL
};

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


// Center - right bottom corner
Ki_WorkSheetData    WS_Osn1_Line1 =
{
    WS_OSN,
    &WS_Osn1_Line2,
    Mm2mils( 185 ),Mm2mils( 55 ),
    0,             Mm2mils( 55 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line2 =
{
    WS_OSN,
    &WS_Osn1_Line3,
    Mm2mils( 120 ),Mm2mils( 40 ),
    0,             Mm2mils( 40 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line3 =
{
    WS_OSN,
    &WS_Osn1_Line4,
    Mm2mils( 185 ),Mm2mils( 35 ),
    Mm2mils( 120 ),Mm2mils( 35 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line4 =
{
    WS_OSN,
    &WS_Osn1_Line5,
    Mm2mils( 50 ), Mm2mils( 35 ),
    0,             Mm2mils( 35 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line5 =
{
    WS_OSN,
    &WS_Osn1_Line6,
    Mm2mils( 185 ),Mm2mils( 30 ),
    Mm2mils( 120 ),Mm2mils( 30 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line6 =
{
    WS_OSN,
    &WS_Osn1_Line7,
    Mm2mils( 50 ), Mm2mils( 20 ),
    0,             Mm2mils( 20 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line7 =
{
    WS_OSN,
    &WS_Osn1_Line8,
    Mm2mils( 120 ),Mm2mils( 15 ),
    0,             Mm2mils( 15 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line8 =
{
    WS_OSN,
    &WS_Osn1_Line9,
    Mm2mils( 185 ),Mm2mils( 55 ),
    Mm2mils( 185 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line9 =
{
    WS_OSN,
    &WS_Osn1_Line10,
    Mm2mils( 178 ), Mm2mils( 55 ),
    Mm2mils( 178 ), Mm2mils( 30 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line10 =
{
    WS_OSN,
    &WS_Osn1_Line11,
    Mm2mils( 168 ), Mm2mils( 55 ),
    Mm2mils( 168 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line11 =
{
    WS_OSN,
    &WS_Osn1_Line12,
    Mm2mils( 145 ), Mm2mils( 55 ),
    Mm2mils( 145 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line12 =
{
    WS_OSN,
    &WS_Osn1_Line13,
    Mm2mils( 130 ), Mm2mils( 55 ),
    Mm2mils( 130 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line13 =
{
    WS_OSN,
    &WS_Osn1_Line14,
    Mm2mils( 120 ), Mm2mils( 55 ),
    Mm2mils( 120 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line14 =
{
    WS_OSN,
    &WS_Osn1_Line15,
    Mm2mils( 50 ),  Mm2mils( 40 ),
    Mm2mils( 50 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line15 =
{
    WS_OSN,
    &WS_Osn1_Line16,
    Mm2mils( 35 ),  Mm2mils( 40 ),
    Mm2mils( 35 ),  Mm2mils( 20 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line16 =
{
    WS_OSN,
    &WS_Osn1_Line17,
    Mm2mils( 30 ),  Mm2mils( 20 ),
    Mm2mils( 30 ),  Mm2mils( 15 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line17 =
{
    WS_OSN,
    &WS_Osn1_Line18,
    Mm2mils( 18 ),  Mm2mils( 40 ),
    Mm2mils( 18 ),  Mm2mils( 20 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line18 =
{
    WS_TONK,
    &WS_Osn1_Line19,
    Mm2mils( 185 ), Mm2mils( 50 ),
    Mm2mils( 120 ), Mm2mils( 50 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line19 =
{
    WS_TONK,
    &WS_Osn1_Line20,
    Mm2mils( 185 ), Mm2mils( 45 ),
    Mm2mils( 120 ), Mm2mils( 45 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line20 =
{
    WS_TONK,
    &WS_Osn1_Line21,
    Mm2mils( 185 ), Mm2mils( 40 ),
    Mm2mils( 120 ), Mm2mils( 40 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line21 =
{
    WS_TONK,
    &WS_Osn1_Line22,
    Mm2mils( 185 ), Mm2mils( 25 ),
    Mm2mils( 120 ), Mm2mils( 25 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line22 =
{
    WS_TONK,
    &WS_Osn1_Line23,
    Mm2mils( 185 ), Mm2mils( 20 ),
    Mm2mils( 120 ), Mm2mils( 20 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line23 =
{
    WS_TONK,
    &WS_Osn1_Line24,
    Mm2mils( 185 ), Mm2mils( 15 ),
    Mm2mils( 120 ), Mm2mils( 15 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line24 =
{
    WS_TONK,
    &WS_Osn1_Line25,
    Mm2mils( 185 ), Mm2mils( 10 ),
    Mm2mils( 120 ), Mm2mils( 10 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line25 =
{
    WS_TONK,
    &WS_Osn1_Line26,
    Mm2mils( 185 ), Mm2mils( 5 ),
    Mm2mils( 120 ), Mm2mils( 5 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line26 =
{
    WS_TONK,
    &WS_Osn1_Line27,
    Mm2mils( 45 ),  Mm2mils( 35 ),
    Mm2mils( 45 ),  Mm2mils( 20 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Line27 =
{
    WS_TONK,
    &WS_Osn1_Text1,
    Mm2mils( 40 ), Mm2mils( 35 ),
    Mm2mils( 40 ), Mm2mils( 20 ),
    NULL
};

Ki_WorkSheetData    WS_Osn1_Text1 =
{
    WS_TEXT,
    &WS_Osn1_Text2,
    Mm2mils( 181.5 ),Mm2mils( 32.5 ),
    0,                              0,
    wxT( "Изм." )
};

Ki_WorkSheetData    WS_Osn1_Text2 =
{
    WS_TEXTL,
    &WS_Osn1_Text3,
    Mm2mils( 184 ),  Mm2mils( 27.5 ),
    0,                              0,
    wxT( "Разраб." )
};

Ki_WorkSheetData    WS_Osn1_Text3 =
{
    WS_TEXTL,
    &WS_Osn1_Text4,
    Mm2mils( 184 ),Mm2mils( 22.5 ),
    0,                            0,
    wxT( "Пров." )
};

Ki_WorkSheetData    WS_Osn1_Text4 =
{
    WS_TEXTL,
    &WS_Osn1_Text5,
    Mm2mils( 184 ),   Mm2mils( 17.5 ),
    0,                               0,
    wxT( "Т.контр." )
};

Ki_WorkSheetData    WS_Osn1_Text5 =
{
    WS_TEXTL,
    &WS_Osn1_Text6,
    Mm2mils( 184 ),   Mm2mils( 7.5 ),
    0,                              0,
    wxT( "Н.контр." )
};

Ki_WorkSheetData    WS_Osn1_Text6 =
{
    WS_TEXTL,
    &WS_Osn1_Text7,
    Mm2mils( 184 ),Mm2mils( 2.5 ),
    0,                           0,
    wxT( "Утв." )
};

Ki_WorkSheetData    WS_Osn1_Text7 =
{
    WS_TEXT,
    &WS_Osn1_Text8,
    Mm2mils( 173 ),Mm2mils( 32.5 ),
    0,                            0,
    wxT( "Лист" )
};

Ki_WorkSheetData    WS_Osn1_Text8 =
{
    WS_TEXT,
    &WS_Osn1_Text9,
    Mm2mils( 156.5 ), Mm2mils( 32.5 ),
    0,                               0,
    wxT( "N докум." )
};

Ki_WorkSheetData    WS_Osn1_Text9 =
{
    WS_TEXT,
    &WS_Osn1_Text10,
    Mm2mils( 137.5 ),Mm2mils( 32.5 ),
    0,                              0,
    wxT( "Подп." )
};

Ki_WorkSheetData    WS_Osn1_Text10 =
{
    WS_TEXT,
    &WS_Osn1_Text11,
    Mm2mils( 125 ), Mm2mils( 32.5 ),
    0,                            0,
    wxT( "Дата" )
};

Ki_WorkSheetData    WS_Osn1_Text11 =
{
    WS_TEXT,
    &WS_Osn1_Text12,
    Mm2mils( 42.5 ),Mm2mils( 37.5 ),
    0,                             0,
    wxT( "Лит." )
};

Ki_WorkSheetData    WS_Osn1_Text12 =
{
    WS_TEXT,
    &WS_Osn1_Text13,
    Mm2mils( 26.5 ),Mm2mils( 37.5 ),
    0,                             0,
    wxT( "Масса" )
};

Ki_WorkSheetData    WS_Osn1_Text13 =
{
    WS_TEXT,
    &WS_Osn1_Text14,
    Mm2mils( 9 ),    Mm2mils( 37.5 ),
    0,                           0,
    wxT( "Масштаб" )
};

Ki_WorkSheetData    WS_Osn1_Text14 =
{
    WS_TEXTL,
    &WS_Osn1_Text15,
    Mm2mils( 49 ),  Mm2mils( 17.5 ),
    0,                           0,
    wxT( "Лист" )
};

Ki_WorkSheetData    WS_Osn1_Text15 =
{
    WS_TEXTL,
    &WS_Osn1_Text16,
    Mm2mils( 29 ),  Mm2mils( 17.5 ),
    0,                           0,
    wxT( "Листов" )
};

Ki_WorkSheetData    WS_Osn1_Text16 =
{
    WS_TEXTL,
    &WS_Osn1_Text17,
    Mm2mils( 40 ), -Mm2mils( 2.5 ),
    0,                           0,
    wxT( "Формат" )
};

Ki_WorkSheetData    WS_Osn1_Text17 =
{
    WS_TEXTL,
    NULL,
    Mm2mils( 110 ),     -Mm2mils( 2.5 ),
    0,                                0,
    wxT( "Копировал" )
};

Ki_WorkSheetData    WS_Osn2a_Line1 =
{
    WS_OSN,
    &WS_Osn2a_Line2,
    Mm2mils( 185 ), Mm2mils( 15 ),
    0,              Mm2mils( 15 ),
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line2 =
{
    WS_OSN,
    &WS_Osn2a_Line3,
    Mm2mils( 185 ), Mm2mils( 5 ),
    Mm2mils( 120 ), Mm2mils( 5 ),
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line3 =
{
    WS_OSN,
    &WS_Osn2a_Line4,
    Mm2mils( 10 ),  Mm2mils( 8 ),
    0,              Mm2mils( 8 ),
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line4 =
{
    WS_OSN,
    &WS_Osn2a_Line5,
    Mm2mils( 185 ), Mm2mils( 15 ),
    Mm2mils( 185 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line5 =
{
    WS_OSN,
    &WS_Osn2a_Line6,
    Mm2mils( 178 ), Mm2mils( 15 ),
    Mm2mils( 178 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line6 =
{
    WS_OSN,
    &WS_Osn2a_Line7,
    Mm2mils( 168 ), Mm2mils( 15 ),
    Mm2mils( 168 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line7 =
{
    WS_OSN,
    &WS_Osn2a_Line8,
    Mm2mils( 145 ), Mm2mils( 15 ),
    Mm2mils( 145 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line8 =
{
    WS_OSN,
    &WS_Osn2a_Line9,
    Mm2mils( 130 ), Mm2mils( 15 ),
    Mm2mils( 130 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line9 =
{
    WS_OSN,
    &WS_Osn2a_Line10,
    Mm2mils( 120 ),  Mm2mils( 15 ),
    Mm2mils( 120 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line10 =
{
    WS_OSN,
    &WS_Osn2a_Line11,
    Mm2mils( 10 ),   Mm2mils( 15 ),
    Mm2mils( 10 ),             0,
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Line11 =
{
    WS_TONK,
    &WS_Osn2a_Text1,
    Mm2mils( 185 ), Mm2mils( 10 ),
    Mm2mils( 120 ), Mm2mils( 10 ),
    NULL
};

Ki_WorkSheetData    WS_Osn2a_Text1 =
{
    WS_TEXT,
    &WS_Osn2a_Text2,
    Mm2mils( 181.5 ),Mm2mils( 2.5 ),
    0,                             0,
    wxT( "Изм." )
};

Ki_WorkSheetData    WS_Osn2a_Text2 =
{
    WS_TEXT,
    &WS_Osn2a_Text3,
    Mm2mils( 173 ), Mm2mils( 2.5 ),
    0,                           0,
    wxT( "Лист" )
};

Ki_WorkSheetData    WS_Osn2a_Text3 =
{
    WS_TEXT,
    &WS_Osn2a_Text4,
    Mm2mils( 156.5 ), Mm2mils( 2.5 ),
    0,                              0,
    wxT( "N докум." )
};

Ki_WorkSheetData    WS_Osn2a_Text4 =
{
    WS_TEXT,
    &WS_Osn2a_Text5,
    Mm2mils( 137.5 ),Mm2mils( 2.5 ),
    0,                             0,
    wxT( "Подп." )
};

Ki_WorkSheetData    WS_Osn2a_Text5 =
{
    WS_TEXT,
    &WS_Osn2a_Text6,
    Mm2mils( 125 ), Mm2mils( 2.5 ),
    0,                           0,
    wxT( "Дата" )
};

Ki_WorkSheetData    WS_Osn2a_Text6 =
{
    WS_TEXT,
    &WS_Osn2a_Text7,
    Mm2mils( 5 ),   Mm2mils( 11.5 ),
    0,                          0,
    wxT( "Лист" )
};

Ki_WorkSheetData    WS_Osn2a_Text7 =
{
    WS_TEXTL,
    &WS_Osn2a_Text8,
    Mm2mils( 40 ), -Mm2mils( 2.5 ),
    0,                           0,
    wxT( "Формат" )
};

Ki_WorkSheetData    WS_Osn2a_Text8 =
{
    WS_TEXTL,
    NULL,
    Mm2mils( 110 ),     -Mm2mils( 2.5 ),
    0,                                0,
    wxT( "Копировал" )
};

// Center - left bottom corner

Ki_WorkSheetData    WS_DopLeft_Line1 =
{
    WS_OSN,
    &WS_DopLeft_Line2,
    Mm2mils( 12 ),    Mm2mils( 145 ),
    0,                Mm2mils( 145 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line2 =
{
    WS_OSN,
    &WS_DopLeft_Line3,
    Mm2mils( 12 ),    Mm2mils( 110 ),
    0,                Mm2mils( 110 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line3 =
{
    WS_OSN,
    &WS_DopLeft_Line4,
    Mm2mils( 12 ),    Mm2mils( 85 ),
    0,                Mm2mils( 85 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line4 =
{
    WS_OSN,
    &WS_DopLeft_Line5,
    Mm2mils( 12 ),    Mm2mils( 60 ),
    0,                Mm2mils( 60 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line5 =
{
    WS_OSN,
    &WS_DopLeft_Line6,
    Mm2mils( 12 ),    Mm2mils( 25 ),
    0,                Mm2mils( 25 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line6 =
{
    WS_OSN,
    &WS_DopLeft_Line7,
    Mm2mils( 12 ),     0,
    0,                 0,
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line7 =
{
    WS_OSN,
    &WS_DopLeft_Line8,
    Mm2mils( 12 ),    Mm2mils( 145 ),
    Mm2mils( 12 ),              0,
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line8 =
{
    WS_OSN,
    &WS_DopLeft_Text1,
    Mm2mils( 7 ),     Mm2mils( 145 ),
    Mm2mils( 7 ),              0,
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Text1 =
{
    WS_TEXT,
    &WS_DopLeft_Text2,
    Mm2mils( 9.5 ),      Mm2mils( 12.5 ),
    0,                               0,
    wxT( "Инв.N подл." )
};

Ki_WorkSheetData    WS_DopLeft_Text2 =
{
    WS_TEXT,
    &WS_DopLeft_Text3,
    Mm2mils( 9.5 ),       Mm2mils( 42.5 ),
    0,                                0,
    wxT( "Подп. и дата" )
};

Ki_WorkSheetData    WS_DopLeft_Text3 =
{
    WS_TEXT,
    &WS_DopLeft_Text4,
    Mm2mils( 9.5 ),     Mm2mils( 72.5 ),
    0,                              0,
    wxT( "Взам.инв.N" )
};

Ki_WorkSheetData    WS_DopLeft_Text4 =
{
    WS_TEXT,
    &WS_DopLeft_Text5,
    Mm2mils( 9.5 ),      Mm2mils( 97.5 ),
    0,                               0,
    wxT( "Инв.N дубл." )
};

Ki_WorkSheetData    WS_DopLeft_Text5 =
{
    WS_TEXT,
    &WS_DopLeft_Line9,
    Mm2mils( 9.5 ),       Mm2mils( 127.5 ),
    0,                                 0,
    wxT( "Подп. и дата" )
};

Ki_WorkSheetData    WS_DopLeft_Line9 =
{
    WS_OSN,
    &WS_DopLeft_Line10,
    Mm2mils( 7 ),      Mm2mils( 287 ),
    Mm2mils( 7 ),      Mm2mils( 167 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line10 =
{
    WS_OSN,
    &WS_DopLeft_Line11,
    Mm2mils( 12 ),     Mm2mils( 287 ),
    Mm2mils( 12 ),     Mm2mils( 167 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line11 =
{
    WS_OSN,
    &WS_DopLeft_Line12,
    Mm2mils( 12 ),     Mm2mils( 287 ),
    Mm2mils( 12 ),     Mm2mils( 167 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line12 =
{
    WS_OSN,
    &WS_DopLeft_Line13,
    Mm2mils( 12 ),     Mm2mils( 167 ),
    0,                 Mm2mils( 167 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line13 =
{
    WS_OSN,
    &WS_DopLeft_Line14,
    Mm2mils( 12 ),     Mm2mils( 227 ),
    0,                 Mm2mils( 227 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Line14 =
{
    WS_OSN,
    &WS_DopLeft_Text6,
    Mm2mils( 12 ),    Mm2mils( 287 ),
    0,                Mm2mils( 287 ),
    NULL
};

Ki_WorkSheetData    WS_DopLeft_Text6 =
{
    WS_TEXT,
    &WS_DopLeft_Text7,
    Mm2mils( 9.5 ),   Mm2mils( 197 ),
    0,                           0,
    wxT( "Справ. N" )
};

Ki_WorkSheetData    WS_DopLeft_Text7 =
{
    WS_TEXT,
    NULL,
    Mm2mils( 9.5 ),        Mm2mils( 257 ),
    0,                                   0,
    wxT( "Перв. примен." )
};

// Center - left top corner

Ki_WorkSheetData    WS_DopTop_Line1 =
{
    WS_OSN,
    &WS_DopTop_Line2,
    Mm2mils( 70 ),                0,
    Mm2mils( 70 ),   Mm2mils( 14 ),
    NULL
};

Ki_WorkSheetData    WS_DopTop_Line2 =
{
    WS_OSN,
    &WS_DopTop_Line3,
    Mm2mils( 70 ),   Mm2mils( 14 ),
    0,               Mm2mils( 14 ),
    NULL
};

Ki_WorkSheetData    WS_DopTop_Line3 =
{
    WS_OSN,
    &WS_DopTop_Line4,
    Mm2mils( 70 ),   Mm2mils( 14 ),
    Mm2mils( 137 ),  Mm2mils( 14 ),
    NULL
};

Ki_WorkSheetData    WS_DopTop_Line4 =
{
    WS_OSN,
    &WS_DopTop_Line5,
    Mm2mils( 84 ),   Mm2mils( 7 ),
    Mm2mils( 137 ),  Mm2mils( 7 ),
    NULL
};

Ki_WorkSheetData    WS_DopTop_Line5 =
{
    WS_OSN,
    &WS_DopTop_Line6,
    Mm2mils( 84 ),   Mm2mils( 14 ),
    Mm2mils( 84 ),             0,
    NULL
};

Ki_WorkSheetData    WS_DopTop_Line6 =
{
    WS_OSN,
    NULL,
    Mm2mils( 137 ),Mm2mils( 14 ),
    Mm2mils( 137 ),             0,
    NULL
};

#include <worksheet_shape_builder.h>

void WS_DRAW_ITEM_LIST::BuildWorkSheetGraphicList(
                           const wxString& aPaperFormat,
                           const wxString& aFileName,
                           const wxString& aSheetPathHumanReadable,
                           const TITLE_BLOCK& aTitleBlock,
                           EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor )
{
    wxPoint             pos;
    wxPoint             end;
    int                 refx, refy;
    Ki_WorkSheetData*   WsItem;
    wxSize              size( TEXTSIZE * m_milsToIu, TEXTSIZE * m_milsToIu );
    wxString            msg;
    WS_DRAW_ITEM_TEXT*  gtext;

    // Upper left corner
    refx    = m_LTmargin.x;
    refy    = m_LTmargin.y;

    // lower right corner
    int xg, yg;
    xg  = m_pageSize.x - m_RBmargin.x;
    yg  = m_pageSize.y - m_RBmargin.y;

    int     lnMsg, ln;
    int     lnWosn  = m_penSize * 2;
    int     lnWtonk = m_penSize;
    wxSize  sz;
    wxSize  size0_8( TEXTSIZE * m_milsToIu * 0.8, TEXTSIZE * m_milsToIu * 1 );
    wxSize  size1_5( TEXTSIZE * m_milsToIu * 1.5, TEXTSIZE * m_milsToIu * 1.5 );
    wxSize  size2( TEXTSIZE * m_milsToIu * 2, TEXTSIZE * m_milsToIu * 2 );
    wxSize  size3( TEXTSIZE * m_milsToIu * 3, TEXTSIZE * m_milsToIu * 3 );

    // Draw the border.
    Append( new WS_DRAW_ITEM_RECT(
                wxPoint( refx * m_milsToIu, refy * m_milsToIu ),
                wxPoint( xg * m_milsToIu, yg * m_milsToIu ),
                lnWosn, aLineColor ) );

    // Center - right bottom corner
    refx    = m_pageSize.x - m_RBmargin.x;
    refy    = m_pageSize.y - m_RBmargin.y;

    // First page
    if( m_sheetNumber == 1 )
    {
        for( WsItem = &WS_Osn1_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            pos.x   = (refx - WsItem->m_Posx) * m_milsToIu;
            pos.y   = (refy - WsItem->m_Posy) * m_milsToIu;
            end.x   = (refx - WsItem->m_Endx) * m_milsToIu;
            end.y   = (refy - WsItem->m_Endy) * m_milsToIu;
            msg     = WsItem->m_TextBase;

            switch( WsItem->m_Type )
            {
            case WS_OSN:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWosn, aLineColor ) );
                break;

            case WS_TONK:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWtonk, aLineColor ) );
                break;

            case WS_TEXT:

                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn1_Text1 )
                        Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                                       size0_8, m_penSize,
                                                       aLineColor ) );
                    else
                        Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                                       size, m_penSize, aLineColor ) );
                }

                break;

            case WS_TEXTL:

                if( !msg.IsEmpty() )
                {
                    Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                           size, m_penSize, aLineColor ) );
                    gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                }

                break;
            }
        }

        // Sheet number
        if( m_sheetCount > 1 )
        {
            pos.x   = ( refx - Mm2mils( 36 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 17.5 ) ) * m_milsToIu;
            msg.Empty();
            msg << m_sheetNumber;
            Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                           size, m_penSize, aLineColor ) );
        }

        // Count of sheets
        pos.x   = ( refx - Mm2mils( 10 ) ) * m_milsToIu;
        pos.y   = ( refy - Mm2mils( 17.5 ) ) * m_milsToIu;
        msg.Empty();
        msg << m_sheetCount;
        Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                               size, m_penSize, aLineColor ) );
        gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );

        // Company name
        msg = aTitleBlock.GetCompany();

        if( !msg.IsEmpty() )
        {
            sz      = size1_5;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 49 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 25 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 7.5 ) ) * m_milsToIu;
            Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                           sz, m_penSize, aLineColor ) );
        }

        // Title
        msg = aTitleBlock.GetTitle();

        if( !msg.IsEmpty() )
        {
            sz = size1_5;
            wxArrayString   lines;
            int             titleWidth          = 0;
            int             titleHeight         = (sz.y + sz.y * 0.5) / m_milsToIu;
            int             titleFieldWidth     = Mm2mils( 69 );
            int             titleFieldHeight    = Mm2mils( 24 );
            int             index   = 0;
            wxString        fullMsg = msg;

            while( 1 )      // Reduce the height of wrapped title until the fit
            {
                 while( 1 ) // Wrap the title
                {
                    titleWidth = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;

                    if( titleWidth > titleFieldWidth )
                    {
                        index = 0;

                        while( 1 )
                        {
                            msg = msg.Left( msg.Length() - 1 );

                            if( msg.Length() == 0 )
                            {
                                lines.Clear();
                                msg     = fullMsg;
                                sz.x    -= m_milsToIu;
                                break;
                            }
                            else
                            {
                                index++;
                                titleWidth =
                                    ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;

                                wxString ch = wxString( msg.Last() );

                                if( titleWidth < titleFieldWidth && ch == wxT( " " ) )
                                {
                                    // New sentence on a new line
                                    int dot = msg.Index( wxT( ". " ) );

                                    if( dot != wxNOT_FOUND )
                                    {
                                        index   += msg.Length() - dot - 2;
                                        msg     = msg.Left( dot + 1 );
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
                        }
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
                }

                if( titleFieldHeight < (int) ( titleHeight * lines.Count() ) )
                {
                    sz.y    -= m_milsToIu;
                    sz.x    -= m_milsToIu;
                    msg     = fullMsg;
                    lines.Clear();
                }
                else
                    break;
            }

            pos.x   = ( refx - Mm2mils( 85 ) ) * m_milsToIu;
            pos.y   =
                ( refy - Mm2mils( 27.5 ) - (titleHeight * (lines.Count() - 1) / 2) ) * m_milsToIu;

            for( unsigned curLn = 0; curLn < lines.Count(); curLn++ )
            {
                msg = lines[curLn];
                Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                               sz, m_penSize, aTextColor ) );
                pos.y += titleHeight * m_milsToIu;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();

        if( !msg.IsEmpty() )
        {
            sz      = size3;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 119 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 60 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 47.5 ) ) * m_milsToIu;
            Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                           sz, m_penSize, aTextColor ) );
        }

        // Developer
        msg = aTitleBlock.GetComment2();

        if( !msg.IsEmpty() )
        {
            sz      = size;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 22 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 167.5 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 27.5 ) ) * m_milsToIu;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                   sz, m_penSize, aTextColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        }

        // Verifier
        msg = aTitleBlock.GetComment3();

        if( !msg.IsEmpty() )
        {
            sz      = size;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 22 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 167 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 22.5 ) ) * m_milsToIu;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                   sz, m_penSize, aTextColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        }

        // Approver
        msg = aTitleBlock.GetComment4();

        if( !msg.IsEmpty() )
        {
            sz      = size;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 22 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 167 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 2.5 ) ) * m_milsToIu;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                   sz, m_penSize, aTextColor ) );
            gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
        }
    }
    else    // other pages
    {
        for( WsItem = &WS_Osn2a_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            pos.x   = (refx - WsItem->m_Posx) * m_milsToIu;
            pos.y   = (refy - WsItem->m_Posy) * m_milsToIu;
            end.x   = (refx - WsItem->m_Endx) * m_milsToIu;
            end.y   = (refy - WsItem->m_Endy) * m_milsToIu;
            msg     = WsItem->m_TextBase;

            switch( WsItem->m_Type )
            {
            case WS_OSN:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWosn, aLineColor ) );
                break;

            case WS_TONK:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWtonk, aLineColor ) );
                break;

            case WS_TEXT:

                if( !msg.IsEmpty() )
                {
                    if( WsItem == &WS_Osn2a_Text1 )
                        Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                                       size0_8, m_penSize,
                                                       aLineColor ) );
                    else
                        Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                                       size, m_penSize, aLineColor ) );
                }

                break;

            case WS_TEXTL:

                if( !msg.IsEmpty() )
                {
                    Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                           size, m_penSize,
                                                           aLineColor ) );
                    gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );
                }

                break;
            }
        }

        // Sheet number
        pos.x   = ( refx - Mm2mils( 5 ) ) * m_milsToIu;
        pos.y   = ( refy - Mm2mils( 4 ) ) * m_milsToIu;
        msg.Empty();
        msg << m_sheetNumber;
        Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                               size, m_penSize, aLineColor ) );

        // Decimal number
        msg = aTitleBlock.GetComment1();

        if( !msg.IsEmpty() )
        {
            sz      = size3;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 109 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 65 ) ) * m_milsToIu;
            pos.y   = ( refy - Mm2mils( 7.5 ) ) * m_milsToIu;
            Append( new WS_DRAW_ITEM_TEXT( msg, pos,
                                           sz, m_penSize, aTextColor ) );
        }
    }

    // Format
    pos.x   = ( refx - Mm2mils( 23 ) ) * m_milsToIu;
    pos.y   = ( refy + Mm2mils( 2.5 ) ) * m_milsToIu;
    msg.Empty();
    msg << aPaperFormat;
    Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                           size, m_penSize, aLineColor ) );
    gtext->SetHorizJustify( GR_TEXT_HJUSTIFY_LEFT );

    // Center - left bottom corner
    refx    = m_LTmargin.x;
    refy    = m_pageSize.y - m_RBmargin.y;

    for( WsItem = &WS_DopLeft_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
    {
        if( m_sheetNumber > 1 && WsItem == &WS_DopLeft_Line9 ) // Some fields for first page only
            break;

        pos.x   = (refx - WsItem->m_Posx) * m_milsToIu;
        pos.y   = (refy - WsItem->m_Posy) * m_milsToIu;
        end.x   = (refx - WsItem->m_Endx) * m_milsToIu;
        end.y   = (refy - WsItem->m_Endy) * m_milsToIu;
        msg     = WsItem->m_TextBase;

        switch( WsItem->m_Type )
        {
        case WS_OSN:
            Append( new WS_DRAW_ITEM_LINE( pos, end,
                                           lnWosn, aLineColor ) );
            break;

        case WS_TEXT:

            if( !msg.IsEmpty() )
            {
                Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                       size, m_penSize, aLineColor ) );
                gtext->SetOrientation( TEXT_ORIENT_VERT );
            }

            break;
        }
    }

    if( aPaperFormat == PAGE_INFO::A4 || m_pageSize.x > m_pageSize.y )    // A4 or Landscape
    {
        // Center - left top corner
        refx    = m_LTmargin.x;
        refy    = m_LTmargin.y;

        for( WsItem = &WS_DopTop_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            if( m_sheetNumber > 1 && WsItem == &WS_DopTop_Line3 ) // Some fields for first page only
                break;

            pos.x   = (refx + WsItem->m_Posx) * m_milsToIu;
            pos.y   = (refy + WsItem->m_Posy) * m_milsToIu;
            end.x   = (refx + WsItem->m_Endx) * m_milsToIu;
            end.y   = (refy + WsItem->m_Endy) * m_milsToIu;
            msg     = WsItem->m_TextBase;

            switch( WsItem->m_Type )
            {
            case WS_OSN:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWosn, aLineColor ) );
                break;

            case WS_TONK:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWtonk, aLineColor ) );
                break;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();

        if( !msg.IsEmpty() )
        {
            sz      = size2;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 69 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx + Mm2mils( 35 ) ) * m_milsToIu;
            pos.y   = ( refy + Mm2mils( 7 ) ) * m_milsToIu;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                   sz, m_penSize, aTextColor ) );
            gtext->SetOrientation( 1800.0 );
        }
    }
    else    // Portrait
    {
        // Center - right top corner
        // Lines are used from the upper left corner by the change of coordinates
        refx    = m_pageSize.x - m_RBmargin.x;
        refy    = m_LTmargin.y;

        for( WsItem = &WS_DopTop_Line1; WsItem != NULL; WsItem = WsItem->Pnext )
        {
            if( m_sheetNumber > 1 && WsItem == &WS_DopTop_Line3 ) // Some fields for first page only
                break;

            pos.x   = (refx - WsItem->m_Posy) * m_milsToIu;
            pos.y   = (refy + WsItem->m_Posx) * m_milsToIu;
            end.x   = (refx - WsItem->m_Endy) * m_milsToIu;
            end.y   = (refy + WsItem->m_Endx) * m_milsToIu;
            msg     = WsItem->m_TextBase;

            switch( WsItem->m_Type )
            {
            case WS_OSN:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWosn, aLineColor ) );
                break;

            case WS_TONK:
                Append( new WS_DRAW_ITEM_LINE( pos, end,
                                               lnWtonk, aLineColor ) );
                break;
            }
        }

        // Decimal number
        msg = aTitleBlock.GetComment1();

        if( !msg.IsEmpty() )
        {
            sz      = size2;
            lnMsg   = ReturnGraphicTextWidth( msg, sz.x, false, false ) / m_milsToIu;
            ln      = Mm2mils( 69 );

            if( lnMsg > ln )
                sz.x *= float(ln) / lnMsg;

            pos.x   = ( refx - Mm2mils( 7 ) ) * m_milsToIu;
            pos.y   = ( refy + Mm2mils( 35 ) ) * m_milsToIu;
            Append( gtext = new WS_DRAW_ITEM_TEXT( msg, pos,
                                                   sz, m_penSize, aTextColor ) );
            gtext->SetOrientation( TEXT_ORIENT_VERT );
        }
    }
}
