/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2021 Ola Rinta-Koski <gitlab@rinta-koski.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <font/version_info.h>
#include <font/fontconfig.h>
#include <harfbuzz/hb.h>
#ifdef _MSC_VER
#include <ft2build.h>
#else
#include <freetype2/ft2build.h>
#endif
#include FT_FREETYPE_H

using namespace KIFONT;

wxString VERSION_INFO::FreeType()
{
    FT_Library library;

    FT_Int major = 0;
    FT_Int minor = 0;
    FT_Int patch = 0;
    FT_Init_FreeType( &library );
    FT_Library_Version( library, &major, &minor, &patch );
    FT_Done_FreeType( library );

    return wxString::Format( "%d.%d.%d", major, minor, patch );
}


wxString VERSION_INFO::HarfBuzz()
{
    return wxString::FromUTF8( HB_VERSION_STRING );
}


wxString VERSION_INFO::FontConfig()
{
    return fontconfig::FONTCONFIG::Version();
}


wxString VERSION_INFO::FontLibrary()
{
    return wxString::Format( "FreeType %s HarfBuzz %s", FreeType(), HarfBuzz() );
}