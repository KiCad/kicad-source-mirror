/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SCH_EASYEDA_PARSER_H_
#define SCH_EASYEDA_PARSER_H_

#include <io/easyeda/easyeda_parser_base.h>
#include <io/easyeda/easyeda_parser_structs.h>

#include <sch_io/sch_io_mgr.h>


class EDA_TEXT;

class SCH_EASYEDA_PARSER: public EASYEDA_PARSER_BASE
{
public:
    explicit SCH_EASYEDA_PARSER( SCHEMATIC* aSchematic, PROGRESS_REPORTER* aProgressReporter );
    ~SCH_EASYEDA_PARSER();

    double ScaleSize( double aValue ) override
    {
        return KiROUND( schIUScale.MilsToIU( aValue * 10 ) );
    }

    template <typename T>
    VECTOR2<T> RelPosSym( const VECTOR2<T>& aVec )
    {
        return VECTOR2<T>( RelPosX( aVec.x ), RelPosY( aVec.y ) );
    }

    std::pair<LIB_SYMBOL*, bool> MakePowerSymbol( const wxString& aFlagTypename,
                                                  const wxString& aNetname );

    void ParseSymbolShapes( LIB_SYMBOL* aContainer, std::map<wxString, wxString> paramMap,
                            wxArrayString aShapes );

    LIB_SYMBOL* ParseSymbol( const VECTOR2D& aOrigin, std::map<wxString, wxString> aParams,
                             wxArrayString aShapes );

    void ParseSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet, const wxString& aFileName,
                         wxArrayString aShapes );

private:
    SCHEMATIC* m_schematic;
};


#endif // SCH_EASYEDA_PARSER_H_
