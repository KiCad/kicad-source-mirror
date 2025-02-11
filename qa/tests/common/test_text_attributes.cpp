/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <font/text_attributes.h>
#include <font/font.h>


BOOST_AUTO_TEST_SUITE( TextAttributes )


BOOST_AUTO_TEST_CASE( Compare )
{
    std::hash<TEXT_ATTRIBUTES> hasher;
    TEXT_ATTRIBUTES a;
    TEXT_ATTRIBUTES b;

    BOOST_CHECK_EQUAL( a, b );
    BOOST_CHECK_EQUAL( hasher( a ), hasher( b ) );

    a.m_Font = KIFONT::FONT::GetFont();
    BOOST_CHECK_GT( a, b );
    BOOST_CHECK_NE( hasher( a ), hasher( b ) );

    a.m_Font = nullptr;
    b.m_Font = KIFONT::FONT::GetFont();
    BOOST_CHECK_LT( a, b );

    b.m_Font = nullptr;
    a.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
    BOOST_CHECK_GT( a, b );

    a.m_Halign = GR_TEXT_H_ALIGN_CENTER;
    b.m_Halign = GR_TEXT_H_ALIGN_RIGHT;
    BOOST_CHECK_LT( a, b );

    b.m_Halign = GR_TEXT_H_ALIGN_CENTER;
    a.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    BOOST_CHECK_GT( a, b );

    a.m_Valign = GR_TEXT_V_ALIGN_CENTER;
    b.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    BOOST_CHECK_LT( a, b );

    b.m_Valign = GR_TEXT_V_ALIGN_CENTER;
    a.m_Angle = EDA_ANGLE( 90.0, DEGREES_T );
    BOOST_CHECK_GT( a, b );

    a.m_Angle = EDA_ANGLE( 0.0, DEGREES_T );
    b.m_Angle = EDA_ANGLE( 90.0, DEGREES_T );
    BOOST_CHECK_LT( a, b );

    b.m_Angle = EDA_ANGLE( 0.0, DEGREES_T );
    a.m_StrokeWidth = 1;
    BOOST_CHECK_GT( a, b );

    a.m_StrokeWidth = 0;
    b.m_StrokeWidth = 1;
    BOOST_CHECK_LT( a, b );

    b.m_StrokeWidth = 0;
    a.m_Italic = true;
    BOOST_CHECK_GT( a, b );

    a.m_Italic = false;
    b.m_Italic = true;
    BOOST_CHECK_LT( a, b );

    b.m_Italic = false;
    a.m_Bold = true;
    BOOST_CHECK_GT( a, b );

    a.m_Bold = false;
    b.m_Bold = true;
    BOOST_CHECK_LT( a, b );

    b.m_Bold = false;
    a.m_Underlined = true;
    BOOST_CHECK_GT( a, b );

    a.m_Underlined = false;
    b.m_Underlined = true;
    BOOST_CHECK_LT( a, b );

    b.m_Underlined = false;
    a.m_Color = KIGFX::COLOR4D( RED );
    BOOST_CHECK_GT( a, b );

    a.m_Color = KIGFX::COLOR4D( UNSPECIFIED_COLOR );
    b.m_Color = KIGFX::COLOR4D( RED );
    BOOST_CHECK_LT( a, b );

    b.m_Color = KIGFX::COLOR4D( UNSPECIFIED_COLOR );
    a.m_Mirrored = true;
    BOOST_CHECK_GT( a, b );

    a.m_Mirrored = false;
    b.m_Mirrored = true;
    BOOST_CHECK_LT( a, b );

    b.m_Mirrored = false;
    b.m_Multiline = false;
    BOOST_CHECK_GT( a, b );

    b.m_Multiline = true;
    a.m_Multiline = false;
    BOOST_CHECK_LT( a, b );

    a.m_Multiline = true;
    a.m_Size.x = 1;
    BOOST_CHECK_GT( a, b );

    a.m_Size.x = 0;
    b.m_Size.x = 1;
    BOOST_CHECK_LT( a, b );

    b.m_Size.x = 0;
    a.m_KeepUpright = true;
    BOOST_CHECK_GT( a, b );

    a.m_KeepUpright = false;
    b.m_KeepUpright = true;
    BOOST_CHECK_LT( a, b );

    b.m_KeepUpright = false;
}


BOOST_AUTO_TEST_SUITE_END()
