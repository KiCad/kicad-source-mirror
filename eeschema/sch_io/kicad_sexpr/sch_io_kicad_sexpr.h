/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Wayne Stambaugh <stambaughw@gmail.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_IO_KICAD_SEXPR_H_
#define SCH_IO_KICAD_SEXPR_H_

#include <memory>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_file_versions.h>
#include <sch_sheet_path.h>
#include <stack>
#include <wildcards_and_files_ext.h>
#include <wx/string.h>


class KIWAY;
class LINE_READER;
class SCH_SCREEN;
class SCH_SHEET;
struct SCH_SHEET_INSTANCE;
class SCH_BITMAP;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_LINE;
class SCH_SHAPE;
class SCH_RULE_AREA;
class SCH_BUS_ENTRY_BASE;
class SCH_TEXT;
class SCH_TEXTBOX;
class SCH_TABLE;
class SCH_GROUP;
class SCH_SYMBOL;
class SCH_FIELD;
struct SCH_SYMBOL_INSTANCE;
class SCH_SELECTION;
class SCH_IO_KICAD_SEXPR_LIB_CACHE;
class LIB_SYMBOL;
class LEGACY_SYMBOL_LIB;
class BUS_ALIAS;

/**
 * A #SCH_IO derivation for loading schematic files using the new s-expression
 * file format.
 *
 * As with all SCH_IOs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_IO_KICAD_SEXPR : public SCH_IO
{
public:

    SCH_IO_KICAD_SEXPR();
    virtual ~SCH_IO_KICAD_SEXPR();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad s-expression schematic files" ),
                                      { FILEEXT::KiCadSchematicFileExtension } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad symbol library files" ),
                                      { FILEEXT::KiCadSymbolLibFileExtension } );
    }

    /**
     * The property used internally by the plugin to enable cache buffering which prevents
     * the library file from being written every time the cache is changed.  This is useful
     * when writing the schematic cache library file or saving a library to a new file name.
     */
    static const char* PropBuffering;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void LoadContent( LINE_READER& aReader, SCH_SHEET* aSheet,
                      int aVersion = SEXPR_SCHEMATIC_FILE_VERSION );

    void SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void Format( SCH_SHEET* aSheet );

    void Format( SCH_SELECTION* aSelection, SCH_SHEET_PATH* aSelectionPath,
                 SCHEMATIC& aSchematic, OUTPUTFORMATTER* aFormatter, bool aForClipboard );

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const std::map<std::string, UTF8>*         aProperties = nullptr ) override;
    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
                     const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    void DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                       const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    void CreateLibrary( const wxString& aLibraryPath,
                        const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    bool DeleteLibrary( const wxString& aLibraryPath,
                        const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    void SaveLibrary( const wxString& aLibraryPath,
                      const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool CanReadLibrary( const wxString& aLibraryPath ) const override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override;

    void GetAvailableSymbolFields( std::vector<wxString>& aNames ) override;
    void GetDefaultSymbolFields( std::vector<wxString>& aNames ) override;

    const wxString& GetError() const override { return m_error; }

    static std::vector<LIB_SYMBOL*> ParseLibSymbols( std::string& aSymbolText,
                                                     std::string  aSource,
                                                     int aFileVersion = SEXPR_SCHEMATIC_FILE_VERSION );
    static void FormatLibSymbol( LIB_SYMBOL* aPart, OUTPUTFORMATTER& aFormatter );

private:
    void loadHierarchy( const SCH_SHEET_PATH& aParentSheetPath, SCH_SHEET* aSheet );
    void loadFile( const wxString& aFileName, SCH_SHEET* aSheet );

    void saveSymbol( SCH_SYMBOL* aSymbol, const SCHEMATIC& aSchematic,
                     const SCH_SHEET_LIST& aSheetList, bool aForClipboard,
                     const SCH_SHEET_PATH* aRelativePath = nullptr );
    void saveField( SCH_FIELD* aField );
    void saveBitmap( const SCH_BITMAP& aBitmap );
    void saveSheet( SCH_SHEET* aSheet, const SCH_SHEET_LIST& aSheetList );
    void saveJunction( SCH_JUNCTION* aJunction );
    void saveNoConnect( SCH_NO_CONNECT* aNoConnect );
    void saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry );
    void saveLine( SCH_LINE* aLine );
    void saveShape( SCH_SHAPE* aShape );
    void saveRuleArea( SCH_RULE_AREA* aRuleArea );
    void saveText( SCH_TEXT* aText );
    void saveTextBox( SCH_TEXTBOX* aText );
    void saveTable( SCH_TABLE* aTable );
    void saveGroup( SCH_GROUP* aGroup );
    void saveInstances( const std::vector<SCH_SHEET_INSTANCE>& aSheets );

    void cacheLib( const wxString& aLibraryFileName, const std::map<std::string, UTF8>* aProperties );
    bool isBuffering( const std::map<std::string, UTF8>* aProperties );

protected:
    int                     m_version;          ///< Version of file being loaded.
    bool                    m_appending;        ///< Schematic load append status.
    wxString                m_error;            ///< For throwing exceptions or errors on partial
                                                ///<  loads.

    wxString                m_path;             ///< Root project path for loading child sheets.
    std::stack<wxString>    m_currentPath;      ///< Stack to maintain nested sheet paths
    SCH_SHEET*              m_rootSheet;        ///< The root sheet of the schematic being loaded.
    SCH_SHEET_PATH          m_currentSheetPath;
    SCHEMATIC*              m_schematic;
    OUTPUTFORMATTER*        m_out;              ///< The formatter for saving SCH_SCREEN objects.
    SCH_IO_KICAD_SEXPR_LIB_CACHE* m_cache;

    /// initialize PLUGIN like a constructor would.
    void init( SCHEMATIC* aSchematic, const std::map<std::string, UTF8>* aProperties = nullptr );

    std::function<bool( wxString aTitle, int aIcon, wxString aMsg, wxString aAction )> m_queryUserCallback;
};

#endif  // SCH_IO_KICAD_SEXPR_H_
