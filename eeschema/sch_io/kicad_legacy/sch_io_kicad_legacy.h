/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#ifndef SCH_IO_KICAD_LEGACY_H_
#define SCH_IO_KICAD_LEGACY_H_

#include <memory>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <stack>
#include <general.h>        // for EESCHEMA_VERSION definition
#include <wildcards_and_files_ext.h>


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
class SCH_SYMBOL;
class SCH_FIELD;
class SELECTION;
class SCH_IO_KICAD_LEGACY_LIB_CACHE;
class LIB_SYMBOL;
class LEGACY_SYMBOL_LIB;
class BUS_ALIAS;
class OUTPUTFORMATTER;


/**
 * A #SCH_IO derivation for loading schematic files created before the new s-expression
 * file format.
 *
 * The legacy parser and formatter attempt to be compatible with the legacy file format.
 * The original parser was very forgiving in that it would parse only part of a keyword.
 * So "$C", "$Co", and "$Com" could be used for "$Comp" and the old parser would allow
 * this.  This parser is not that forgiving and sticks to the legacy file format document.
 *
 * As with all SCH_IO there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_IO_KICAD_LEGACY : public SCH_IO
{
public:

    SCH_IO_KICAD_LEGACY();
    virtual ~SCH_IO_KICAD_LEGACY();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad legacy schematic files" ),
                                      { FILEEXT::LegacySchematicFileExtension } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "KiCad legacy symbol library files" ),
                                      { FILEEXT::LegacySymbolLibFileExtension } );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    bool CanReadLibrary( const wxString& aFileName ) const override;

    /**
     * The property used internally by the plugin to enable cache buffering which prevents
     * the library file from being written every time the cache is changed.  This is useful
     * when writing the schematic cache library file or saving a library to a new file name.
     */
    static const char* PropBuffering;

    /**
     * The property used internally by the plugin to disable writing the library
     * documentation (.dcm) file when saving the library cache.
     */
    static const char* PropNoDocFile;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void LoadContent( LINE_READER& aReader, SCH_SCREEN* aScreen,
                      int version = EESCHEMA_VERSION );

    void SaveSchematicFile( const wxString& aFileName, SCH_SHEET* aScreen, SCHEMATIC* aSchematic,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void Format( SCH_SHEET* aSheet );

    void Format( SELECTION* aSelection, OUTPUTFORMATTER* aFormatter );

    void EnumerateSymbolLib( wxArrayString&    aSymbolNameList,
                             const wxString&   aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;
    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&   aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;
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

    bool IsLibraryWritable( const wxString& aLibraryPath ) override;

    const wxString& GetError() const override { return m_error; }

    static LIB_SYMBOL* ParsePart( LINE_READER& aReader, int majorVersion = 0,
                                  int minorVersion = 0 );
    static void FormatPart( LIB_SYMBOL* aSymbol, OUTPUTFORMATTER& aFormatter );

private:
    void checkpoint();
    void loadHierarchy( SCH_SHEET* aSheet );
    void loadHeader( LINE_READER& aReader, SCH_SCREEN* aScreen );
    void loadPageSettings( LINE_READER& aReader, SCH_SCREEN* aScreen );
    void loadFile( const wxString& aFileName, SCH_SCREEN* aScreen );
    SCH_SHEET* loadSheet( LINE_READER& aReader );
    SCH_BITMAP* loadBitmap( LINE_READER& aReader );
    SCH_JUNCTION* loadJunction( LINE_READER& aReader );
    SCH_NO_CONNECT* loadNoConnect( LINE_READER& aReader );
    SCH_LINE* loadWire( LINE_READER& aReader );
    SCH_BUS_ENTRY_BASE* loadBusEntry( LINE_READER& aReader );
    SCH_TEXT* loadText( LINE_READER& aReader );
    SCH_SYMBOL* loadSymbol( LINE_READER& aReader );
    std::shared_ptr<BUS_ALIAS> loadBusAlias( LINE_READER& aReader, SCH_SCREEN* aScreen );

    void saveSymbol( SCH_SYMBOL* aSymbol );
    void saveField( SCH_FIELD* aField, int aLegacyId );
    void saveBitmap( const SCH_BITMAP& aBitmap );
    void saveSheet( SCH_SHEET* aSheet );
    void saveJunction( SCH_JUNCTION* aJunction );
    void saveNoConnect( SCH_NO_CONNECT* aNoConnect );
    void saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry );
    void saveLine( SCH_LINE* aLine );
    void saveText( SCH_TEXT* aText );
    void saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias );

    void cacheLib( const wxString& aLibraryFileName, const std::map<std::string, UTF8>* aProperties );
    bool writeDocFile( const std::map<std::string, UTF8>* aProperties );
    bool isBuffering( const std::map<std::string, UTF8>* aProperties );

protected:
    int                      m_version;          ///< Version of file being loaded.

    ///< Indicate if we are appending the loaded schemitic or loading a full project.
    bool                     m_appending;

    wxString                 m_error;            ///< For throwing exceptions or errors on partial
                                                 ///<  schematic loads.
    LINE_READER*             m_lineReader;       ///< for progress reporting
    unsigned                 m_lastProgressLine;
    unsigned                 m_lineCount;        ///< for progress reporting

    wxString                 m_path;             ///< Root project path for loading child sheets.
    std::stack<wxString>     m_currentPath;      ///< Stack to maintain nested sheet paths
    SCH_SHEET*               m_rootSheet;        ///< The root sheet of the schematic being loaded.
    SCH_SHEET*               m_currentSheet;     ///< The sheet currently being loaded.
    OUTPUTFORMATTER*         m_out;              ///< The formatter for saving SCH_SCREEN objects.
    SCH_IO_KICAD_LEGACY_LIB_CACHE* m_cache;
    SCHEMATIC*               m_schematic;

    /// initialize PLUGIN like a constructor would.
    void init( SCHEMATIC* aSchematic, const std::map<std::string, UTF8>* aProperties = nullptr );
};

#endif  // SCH_IO_KICAD_LEGACY_H_
