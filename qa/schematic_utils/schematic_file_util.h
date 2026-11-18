/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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
#ifndef QA_QA_UTILS_SCHEMATIC_SCHEMATIC_FILE_UTIL_H_
#define QA_QA_UTILS_SCHEMATIC_SCHEMATIC_FILE_UTIL_H_

#include <string>
#include <memory>

#include <wx/string.h>

class LIBRARY_TABLE_ROW;
class PROJECT;
class SYMBOL_LIBRARY_ADAPTER;
class SCH_SHEET;
class SCHEMATIC;
class SETTINGS_MANAGER;

namespace KI_TEST
{
    void DumpSchematicToFile( SCHEMATIC& aSchematic, SCH_SHEET& aSheet,
                              const std::string& aFilename );

    std::unique_ptr<SCHEMATIC> ReadSchematicFromStream( std::istream& aStream, PROJECT* aProject );

    std::unique_ptr<SCHEMATIC> ReadSchematicFromFile( const std::string& aFilename,
                                                      PROJECT* aProject );

    void LoadSchematic( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                        std::unique_ptr<SCHEMATIC>& aSchematic );

    /**
     * Serve one symbol library to a project through a temporary project library table.
     *
     * The previous project symbol table is restored and the temporary directory removed on destruction.
     */
    struct SCOPED_PROJECT_SYMBOL_LIBRARY
    {
        SCOPED_PROJECT_SYMBOL_LIBRARY( PROJECT& aProject, const wxString& aNickname, const wxString& aUri );

        struct TABLE_RESTORE
        {
            wxString previousDirectory;
            wxString directory;
            ~TABLE_RESTORE();
        };

        SYMBOL_LIBRARY_ADAPTER* adapter;
        TABLE_RESTORE           restore;
        LIBRARY_TABLE_ROW*      row = nullptr;
    };
}
#endif /* QA_QA_UTILS_SCHEMATIC_SCHEMATIC_FILE_UTIL_H_ */
