#ifndef _SCH_LEGACY_PLUGIN_H_
#define _SCH_LEGACY_PLUGIN_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see change_log.txt for contributors.
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
#include <stack>
#include <general.h>


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
class SELECTION;
class SCH_LEGACY_PLUGIN_CACHE;
class LIB_PART;
class PART_LIB;
class LIB_ALIAS;
class BUS_ALIAS;


/**
 * A #SCH_PLUGIN derivation for loading schematic files created before the new s-expression
 * file format.
 *
 * The legacy parser and formatter attempt to be compatible with the legacy file format.
 * The original parser was very forgiving in that it would parse only part of a keyword.
 * So "$C", "$Co", and "$Com" could be used for "$Comp" and the old parser would allow
 * this.  This parser is not that forgiving and sticks to the legacy file format document.
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_LEGACY_PLUGIN : public SCH_PLUGIN
{
public:

    SCH_LEGACY_PLUGIN();
    virtual ~SCH_LEGACY_PLUGIN();

    const wxString GetName() const override
    {
        return wxT( "Eeschema-Legacy" );
    }

    const wxString GetFileExtension() const override
    {
        return wxT( "sch" );
    }

    /**
     * const char* PropBuffering
     *
     * is a property used internally by the plugin to enable cache buffering which prevents
     * the library file from being written every time the cache is changed.  This is useful
     * when writing the schematic cache library file or saving a library to a new file name.
     */
    static const char* PropBuffering;

    /**
     * const char* PropBuffering
     *
     * is a property used internally by the plugin to disable writing the library
     * documentation (.dcm) file when saving the library cache.
     */
    static const char* PropNoDocFile;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, KIWAY* aKiway,
                     SCH_SHEET* aAppendToMe = nullptr, 
                     const PROPERTIES* aProperties = nullptr ) override;

    void LoadContent( LINE_READER& aReader, SCH_SCREEN* aScreen,
                      int version = EESCHEMA_VERSION );

    void Save( const wxString& aFileName, SCH_SCREEN* aScreen, KIWAY* aKiway,
               const PROPERTIES* aProperties = nullptr ) override;

    void Format( SCH_SCREEN* aScreen );

    void Format( SELECTION* aSelection, OUTPUTFORMATTER* aFormatter );

    void EnumerateSymbolLib( wxArrayString&    aAliasNameList,
                             const wxString&   aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;
    void EnumerateSymbolLib( std::vector<LIB_ALIAS*>& aAliasList,
                             const wxString&   aLibraryPath,
                             const PROPERTIES* aProperties = nullptr ) override;
    LIB_ALIAS* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                           const PROPERTIES* aProperties = nullptr ) override;
    void SaveSymbol( const wxString& aLibraryPath, const LIB_PART* aSymbol,
                     const PROPERTIES* aProperties = nullptr ) override;
    void DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
                      const PROPERTIES* aProperties = nullptr ) override;
    void DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
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

    static LIB_PART* ParsePart( LINE_READER& aReader, int majorVersion = 0, int minorVersion = 0 );
    static void FormatPart( LIB_PART* aPart, OUTPUTFORMATTER& aFormatter );

private:
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
    SCH_COMPONENT* loadComponent( LINE_READER& aReader );
    std::shared_ptr<BUS_ALIAS> loadBusAlias( LINE_READER& aReader, SCH_SCREEN* aScreen );

    void saveComponent( SCH_COMPONENT* aComponent );
    void saveField( SCH_FIELD* aField );
    void saveBitmap( SCH_BITMAP* aBitmap );
    void saveSheet( SCH_SHEET* aSheet );
    void saveJunction( SCH_JUNCTION* aJunction );
    void saveNoConnect( SCH_NO_CONNECT* aNoConnect );
    void saveBusEntry( SCH_BUS_ENTRY_BASE* aBusEntry );
    void saveLine( SCH_LINE* aLine );
    void saveText( SCH_TEXT* aText );
    void saveBusAlias( std::shared_ptr<BUS_ALIAS> aAlias );

    void cacheLib( const wxString& aLibraryFileName );
    bool writeDocFile( const PROPERTIES* aProperties );
    bool isBuffering( const PROPERTIES* aProperties );

protected:
    int                  m_version;    ///< Version of file being loaded.

    /** For throwing exceptions or errors on partial schematic loads. */
    wxString             m_error;

    wxString             m_path;       ///< Root project path for loading child sheets.
    std::stack<wxString> m_currentPath;///< Stack to maintain nested sheet paths
    const PROPERTIES*    m_props;      ///< Passed via Save() or Load(), no ownership, may be nullptr.
    KIWAY*               m_kiway;      ///< Required for path to legacy component libraries.
    SCH_SHEET*           m_rootSheet;  ///< The root sheet of the schematic being loaded..
    OUTPUTFORMATTER*     m_out;        ///< The output formatter for saving SCH_SCREEN objects.
    SCH_LEGACY_PLUGIN_CACHE* m_cache;

    /// initialize PLUGIN like a constructor would.
    void init( KIWAY* aKiway, const PROPERTIES* aProperties = nullptr );
};

#endif  // _SCH_LEGACY_PLUGIN_H_
