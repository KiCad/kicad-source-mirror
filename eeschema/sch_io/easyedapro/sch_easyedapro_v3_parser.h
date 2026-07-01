/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef SCH_EASYEDAPRO_V3_PARSER_H_
#define SCH_EASYEDAPRO_V3_PARSER_H_

#include <io/easyedapro/easyedapro_parser.h>
#include <io/easyedapro/easyedapro_v3_parser.h>

#include "sch_easyedapro_parser.h"


namespace EASYEDAPRO
{
/// Map an EasyEDA "HORIZ_VERT" alignment token to a KiCad justify code (0 top/left, 1 center, 2 bottom/right).
int AlignToFontV( const wxString& aAlign );
int AlignToFontH( const wxString& aAlign );
} // namespace EASYEDAPRO


class SCH_EASYEDAPRO_V3_PARSER
{
public:
    explicit SCH_EASYEDAPRO_V3_PARSER( SCHEMATIC*         aSchematic,
                                       PROGRESS_REPORTER* aProgressReporter );

    EASYEDAPRO::SYM_INFO ParseSymbol(
            const EASYEDAPRO::V3_DOC_RAW&       aDoc,
            const std::map<wxString, wxString>&  aDeviceAttributes,
            const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap = {} );

    void ParseSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                         const nlohmann::json&                       aProject,
                         std::map<wxString, EASYEDAPRO::SYM_INFO>&   aSymbolMap,
                         const std::map<wxString, EASYEDAPRO::BLOB>& aBlobMap,
                         const EASYEDAPRO::V3_DOC_RAW&               aDoc,
                         const wxString&                              aLibName );

private:
    SCHEMATIC* m_schematic;

    template <typename T>
    void ApplyV3FontStyle( T& text, const nlohmann::json& aInner,
                           const char* aAlignField = "align" );

    template <typename T>
    void ApplyV3LineStyle( T& shape, const nlohmann::json& aInner );
};


#endif // SCH_EASYEDAPRO_V3_PARSER_H_
