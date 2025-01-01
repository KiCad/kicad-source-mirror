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

#ifndef SCH_EASYEDAPRO_PARSER_H_
#define SCH_EASYEDAPRO_PARSER_H_

#include <io/easyedapro/easyedapro_parser.h>

#include <sch_io/sch_io_mgr.h>
#include <pin_type.h>
#include <layer_ids.h>
#include <plotters/plotter.h>


class EDA_TEXT;
class SCH_SYMBOL;

namespace EASYEDAPRO
{
struct PIN_INFO
{
    EASYEDAPRO::SYM_PIN pin;
    wxString            number;
    wxString            name;
};

struct SYM_INFO
{
    EASYEDAPRO::SYM_HEAD                head;
    std::vector<PIN_INFO>               pins;
    std::unique_ptr<LIB_SYMBOL>         libSymbol;
    std::optional<EASYEDAPRO::SCH_ATTR> symbolAttr;
    std::map<wxString, int>             partUnits;
};
} // namespace EASYEDAPRO


class SCH_EASYEDAPRO_PARSER
{

public:
    explicit SCH_EASYEDAPRO_PARSER( SCHEMATIC* aSchematic, PROGRESS_REPORTER* aProgressReporter );
    ~SCH_EASYEDAPRO_PARSER();

    /*void Parse( const ALTIUM_COMPOUND_FILE&                  aAltiumPcbFile,
                const std::map<ALTIUM_PCB_DIR, std::string>& aFileMapping );*/

    static double Convert( wxString aValue );

    template <typename T>
    static T ScaleSize( T aValue )
    {
        return KiROUND( schIUScale.MilsToIU( aValue * 10 ) );
    }

    template <typename T>
    static VECTOR2<T> ScaleSize( VECTOR2<T> aValue )
    {
        return VECTOR2<T>( ScaleSize( aValue.x ), ScaleSize( aValue.y ) );
    }

    template <typename T>
    static VECTOR2<T> ScalePos( VECTOR2<T> aValue )
    {
        return VECTOR2<T>( ScaleSize( aValue.x ), -ScaleSize( aValue.y ) );
    }

    template <typename T>
    static VECTOR2<T> ScalePosSym( VECTOR2<T> aValue )
    {
        return VECTOR2<T>( ScaleSize( aValue.x ), -ScaleSize( aValue.y ) );
    }

    double SizeToKi( wxString units );

    EASYEDAPRO::SYM_INFO ParseSymbol( const std::vector<nlohmann::json>&  aLines,
                                      const std::map<wxString, wxString>& aDeviceAttributes );

    void ParseSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                         const nlohmann::json&                       aProject,
                         std::map<wxString, EASYEDAPRO::SYM_INFO>&   aSymbolMap,
                         const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap,
                         const std::vector<nlohmann::json>& aLines, const wxString& aLibName );

protected:
    SCHEMATIC* m_schematic;

    wxString ResolveFieldVariables( const wxString                      aInput,
                                    const std::map<wxString, wxString>& aDeviceAttributes );

    template <typename T>
    void ApplyFontStyle( const std::map<wxString, nlohmann::json>& fontStyles, T& text,
                         const wxString& styleStr );

    template <typename T>
    void ApplyLineStyle( const std::map<wxString, nlohmann::json>& lineStyles, T& shape,
                         const wxString& styleStr );

    template <typename T>
    void ApplyAttrToField( const std::map<wxString, nlohmann::json>& fontStyles, T* text,
                           const EASYEDAPRO::SCH_ATTR& aAttr, bool aIsSym, bool aToSym,
                           const std::map<wxString, wxString>& aDeviceAttributes = {},
                           SCH_SYMBOL*                         aParent = nullptr );
};


#endif // SCH_EASYEDAPRO_PARSER_H_
