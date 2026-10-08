/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <memory>
#include <vector>
#include <wx/string.h>

class BUS_ALIAS;
class SCHEMATIC;
class SCH_SHEET;
class SCH_SHEET_LIST;
class SCH_SHEET_PATH;
struct SCH_SHEET_INSTANCE;
class SCH_SYMBOL;
class SCH_FIELD;
class SCH_BITMAP;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_BUS_ENTRY_BASE;
class SCH_SHAPE;
class SCH_RULE_AREA;
class SCH_LINE;
class SCH_TEXT;
class SCH_TEXTBOX;
class SCH_TABLE;
class LIB_SYMBOL;
class OUTPUTFORMATTER;

/// Writes a schematic or symbol library in the KiCad 9.0 file format. Extracted from the
/// 9.0.0 release, so it can only emit what that release can read back.
class SCH_WRITER_V9
{
public:
    static constexpr int FORMAT_VERSION = 20250114;
    static constexpr int SYMBOL_LIB_VERSION = 20241209;

    SCH_WRITER_V9() :
            m_schematic( nullptr ),
            m_out( nullptr )
    {
    }

    void SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic );

    void SaveSymbolLibrary( const wxString& aFileName, const std::vector<LIB_SYMBOL*>& aSymbols );

private:
    void Format( SCH_SHEET* aSheet );

    void saveSymbol( SCH_SYMBOL* aSymbol, const SCHEMATIC& aSchematic, const SCH_SHEET_LIST& aSheetList,
                     bool aForClipboard, const SCH_SHEET_PATH* aRelativePath = nullptr );

    void saveField( SCH_FIELD* aField );

    void saveBitmap( const SCH_BITMAP& aBitmap );

    void saveSheet( SCH_SHEET* aSheet, const SCH_SHEET_LIST& aSheetList );

    void saveJunction( SCH_JUNCTION* aJunction );

    void saveNoConnect( SCH_NO_CONNECT* aNoConnect );

    void saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry );

    void saveShape( SCH_SHAPE* aShape );

    void saveRuleArea( SCH_RULE_AREA* aRuleArea );

    void saveLine( SCH_LINE* aLine );

    void saveText( SCH_TEXT* aText );

    void saveTextBox( SCH_TEXTBOX* aTextBox );

    void saveTable( SCH_TABLE* aTable );

    void saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias );

    void saveInstances( const std::vector<SCH_SHEET_INSTANCE>& aInstances );

    SCHEMATIC*       m_schematic;
    OUTPUTFORMATTER* m_out;
};
