#ifndef _SCH_SEXPR_PLUGIN_H_
#define _SCH_SEXPR_PLUGIN_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#include <memory>
#include <sch_io_mgr.h>
#include <sch_file_versions.h>
#include <stack>


class KIWAY;
class LINE_READER;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_BITMAP;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_LINE;
class SCH_BUS_ENTRY_BASE;
class SCH_TEXT;
class SCH_COMPONENT;
class SCH_FIELD;
class PROPERTIES;
class EE_SELECTION;
class SCH_SEXPR_PLUGIN_CACHE;
class LIB_PART;
class PART_LIB;
class BUS_ALIAS;

/**
 * A #SCH_PLUGIN derivation for loading schematic files using the new s-expression
 * file format.
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_SEXPR_PLUGIN : public SCH_PLUGIN
{
public:

    SCH_SEXPR_PLUGIN();
    virtual ~SCH_SEXPR_PLUGIN();

    const wxString GetName() const override
    {
        return wxT( "Eeschema s-expression" );
    }

    const wxString GetFileExtension() const override
    {
        return wxT( "kicad_sch" );
    }

    const wxString GetLibraryFileExtension() const override
    {
        return wxT( "kicad_sym" );
    }

    /**
     * The property used internally by the plugin to enable cache buffering which prevents
     * the library file from being written every time the cache is changed.  This is useful
     * when writing the schematic cache library file or saving a library to a new file name.
     */
    static const char* PropBuffering;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                     SCH_SHEET* aAppendToMe = nullptr,
                     const PROPERTIES* aProperties = nullptr ) override;

    void LoadContent( LINE_READER& aReader, SCH_SHEET* aSheet,
                      int aVersion = SEXPR_SCHEMATIC_FILE_VERSION );

    void Save( const wxString& aFileName, SCH_SHEET* aSheet, SCHEMATIC* aSchematic,
               const PROPERTIES* aProperties = nullptr ) override;

    void Format( SCH_SHEET* aSheet );

    void Format( EE_SELECTION* aSelection, SCH_SHEET_PATH* aSheetPath,
                 OUTPUTFORMATTER* aFormatter );

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;
    void EnumerateSymbolLib( std::vector<LIB_PART*>& aSymbolList,
                             const wxString&   aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;
    LIB_PART* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                           const PROPERTIES* aProperties = nullptr ) override;
    void SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
                     const PROPERTIES* aProperties = nullptr ) override;
    void DeleteSymbol( const wxString& aLibraryPath, const wxString& aSymbolName,
                       const PROPERTIES* aProperties = nullptr ) override;
    void CreateSymbolLib( const wxString& aLibraryPath,
                          const PROPERTIES* aProperties = nullptr ) override;
    bool DeleteSymbolLib( const wxString& aLibraryPath,
                          const PROPERTIES* aProperties = nullptr ) override;
    void SaveLibrary( const wxString& aLibraryPath,
                      const PROPERTIES* aProperties = nullptr ) override;

    bool CheckHeader( const wxString& aFileName ) override;
    bool IsSymbolLibWritable( const wxString& aLibraryPath ) override;

    const wxString& GetError() const override { return m_error; }

    static LIB_PART* ParsePart( LINE_READER& aReader,
                                int aVersion = SEXPR_SCHEMATIC_FILE_VERSION );
    static void FormatPart( LIB_PART* aPart, OUTPUTFORMATTER& aFormatter );

private:
    void loadHierarchy( SCH_SHEET* aSheet );
    void loadFile( const wxString& aFileName, SCH_SHEET* aSheet );

    void saveSymbol( SCH_COMPONENT* aComponent, SCH_SHEET_PATH* aSheetPath, int aNestLevel );
    void saveField( SCH_FIELD* aField, int aNestLevel );
    void saveBitmap( SCH_BITMAP* aBitmap, int aNestLevel );
    void saveSheet( SCH_SHEET* aSheet, int aNestLevel );
    void saveJunction( SCH_JUNCTION* aJunction, int aNestLevel );
    void saveNoConnect( SCH_NO_CONNECT* aNoConnect, int aNestLevel );
    void saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry, int aNestLevel );
    void saveLine( SCH_LINE* aLine, int aNestLevel );
    void saveText( SCH_TEXT* aText, int aNestLevel );
    void saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias, int aNestLevel );

    void cacheLib( const wxString& aLibraryFileName );
    bool isBuffering( const PROPERTIES* aProperties );

protected:
    int                  m_version;    ///< Version of file being loaded.
    int                  m_fieldId;    ///< Non-mandatory schematic field ID counter.

    /** For throwing exceptions or errors on partial schematic loads. */
    wxString             m_error;

    wxString             m_path;       ///< Root project path for loading child sheets.
    std::stack<wxString> m_currentPath;///< Stack to maintain nested sheet paths
    const PROPERTIES*    m_props;      ///< Passed via Save() or Load(), no ownership, may be nullptr.
    SCH_SHEET*           m_rootSheet;  ///< The root sheet of the schematic being loaded..
    SCHEMATIC*           m_schematic;  ///< Passed to Load(), the schematic object being loaded
    OUTPUTFORMATTER*     m_out;        ///< The output formatter for saving SCH_SCREEN objects.
    SCH_SEXPR_PLUGIN_CACHE* m_cache;

    /// initialize PLUGIN like a constructor would.
    void init( SCHEMATIC* aSchematic, const PROPERTIES* aProperties = nullptr );
};

#endif  // _SCH_SEXPR_PLUGIN_H_
