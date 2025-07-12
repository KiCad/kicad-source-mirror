/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#ifndef _SCH_IO_ALTIUM_H_
#define _SCH_IO_ALTIUM_H_

#include <memory>
#include <sch_sheet_path.h>
#include <vector>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <wx/filename.h>
#include <wx/gdicmn.h>
#include <case_insensitive_map.h>

#include "altium_parser_sch.h"

class SYMBOL;
class SCH_SYMBOL;
class SCH_SHEET;
class TITLE_BLOCK;

class ALTIUM_COMPOUND_FILE;

/**
 * SCH_IO_ALTIUM
 * is a #SCH_IO derivation for loading Altium .SchDoc schematic files.
 *
 * As with all SCH_IO there is no UI dependencies i.e. windowing calls allowed.
 */

static std::vector<LIB_SYMBOL*> nullsym;
static std::vector<int> nullint;

struct HARNESS
{
    struct HARNESS_PORT
    {
        VECTOR2I              m_location;
        VECTOR2I              m_entryLocation;
        ASCH_SHEET_ENTRY_SIDE m_harnessConnectorSide;
        int                   m_primaryConnectionPosition;
        wxString              m_name;
    };

    wxString                  m_name;
    VECTOR2I                  m_location;
    VECTOR2I                  m_size;
    std::vector<HARNESS_PORT> m_ports;
    HARNESS_PORT              m_entry;
};


class SCH_IO_ALTIUM : public SCH_IO
{
public:
    SCH_IO_ALTIUM();
    ~SCH_IO_ALTIUM();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Altium schematic files" ), { "SchDoc" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Altium Schematic Library or Integrated Library" ),
                                      { "SchLib", "IntLib" } );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    SCH_SHEET* LoadSchematicProject( SCHEMATIC*                         aSchematic,
                                     const std::map<std::string, UTF8>* aProperties );

    // unimplemented functions. Will trigger a not_implemented IO error.
    //void SaveLibrary( const wxString& aFileName, const PROPERTIES* aProperties = NULL ) override;

    //void Save( const wxString& aFileName, SCH_SCREEN* aSchematic, KIWAY* aKiway,
    //           const PROPERTIES* aProperties = NULL ) override;


    void EnumerateSymbolLib( wxArrayString&         aSymbolNameList,
                             const wxString&        aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList,
                             const wxString&           aLibraryPath,
                             const std::map<std::string, UTF8>*    aProperties = nullptr ) override;

    LIB_SYMBOL* LoadSymbol( const wxString&        aLibraryPath,
                            const wxString&        aAliasName,
                            const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    //void SaveSymbol( const wxString& aLibraryPath, const LIB_SYMBOL* aSymbol,
    //                 const PROPERTIES* aProperties = NULL ) override;

    //void DeleteAlias( const wxString& aLibraryPath, const wxString& aAliasName,
    //                  const PROPERTIES* aProperties = NULL ) override;

    //void DeleteSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
    //                   const PROPERTIES* aProperties = NULL ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    wxString   getLibName();
    wxFileName getLibFileName();

    void ParseAltiumSch( const wxString& aFileName );
    void ParseStorage( const ALTIUM_COMPOUND_FILE& aAltiumSchFile );
    void ParseAdditional( const ALTIUM_COMPOUND_FILE& aAltiumSchFile );
    void ParseFileHeader( const ALTIUM_COMPOUND_FILE& aAltiumSchFile );

    void ParseASCIISchematic( const wxString& aFileName );

    void ParseRecord( int index, std::map<wxString, wxString>& properties,
                      const wxString& aSectionName );

private:
    SCH_SCREEN* getCurrentScreen();
    SCH_SHEET* getCurrentSheet();

    bool ShouldPutItemOnSheet( int aOwnerindex );
    bool IsComponentPartVisible( const ASCH_OWNER_INTERFACE& aElem ) const;
    const ASCH_STORAGE_FILE* GetFileFromStorage( const wxString& aFilename ) const;
    void CreateAliases();
    void AddTextBox( const ASCH_TEXT_FRAME* aElem );
    void AddLibTextBox( const ASCH_TEXT_FRAME* aElem, std::vector<LIB_SYMBOL*>& aSymbol = nullsym,
                        std::vector<int>& aFontSize = nullint );

    void ParseComponent( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParseTemplate( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParsePin( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol = nullsym);
    void ParseLabel( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol = nullsym,
                     std::vector<int>& aFontSize = nullint );
    void ParseTextFrame( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol = nullsym,
                         std::vector<int>& aFontSize = nullint );
    void ParseNote( const std::map<wxString, wxString>& aProperties );
    void ParseBezier( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParsePolyline( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParsePolygon( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseRoundRectangle( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseArc( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseEllipticalArc( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseEllipse( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseCircle( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseLine( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseSignalHarness( const std::map<wxString, wxString>& aProperties );
    void ParseHarnessConnector( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParseHarnessEntry( const std::map<wxString, wxString>& aProperties );
    void ParseHarnessType( const std::map<wxString, wxString>& aProperties );
    void ParseHarnessPort( const ASCH_PORT& aElem );
    void ParseHyperlink( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParsePieChart( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseRectangle( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym);
    void ParseSheetSymbol( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParseSheetEntry( const std::map<wxString, wxString>& aProperties );
    void ParsePowerPort( const std::map<wxString, wxString>& aProperties );
    void ParsePort( const ASCH_PORT& aElem );
    void ParsePortHelper( const ASCH_PORT& aElem );
    void ParseNoERC( const std::map<wxString, wxString>& aProperties );
    void ParseNetLabel( const std::map<wxString, wxString>& aProperties );
    void ParseBus( const std::map<wxString, wxString>& aProperties );
    void ParseWire( const std::map<wxString, wxString>& aProperties );
    void ParseJunction( const std::map<wxString, wxString>& aProperties );
    void ParseImage( const std::map<wxString, wxString>& aProperties );
    void ParseSheet( const std::map<wxString, wxString>& aProperties );
    void ParseSheetName( const std::map<wxString, wxString>& aProperties );
    void ParseFileName( const std::map<wxString, wxString>& aProperties );
    void ParseDesignator( const std::map<wxString, wxString>& aProperties );
    void ParseLibDesignator( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym, std::vector<int>& aFontSize = nullint );
    void ParseBusEntry( const std::map<wxString, wxString>& aProperties );
    void ParseParameter( const std::map<wxString, wxString>& aProperties );
    void ParseLibParameter( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym, std::vector<int>& aFontSize = nullint );
    void ParseImplementationList( int aIndex, const std::map<wxString, wxString>& aProperties );
    void ParseImplementation( const std::map<wxString, wxString>& aProperties, std::vector<LIB_SYMBOL*>& aSymbol  = nullsym );

    void ParseLibHeader( const ALTIUM_COMPOUND_FILE& aAltiumSchFile, std::vector<int>& aFontSizes );
    CASE_INSENSITIVE_MAP<LIB_SYMBOL*> ParseLibFile( const ALTIUM_COMPOUND_FILE& aAltiumSchFile );
    std::vector<LIB_SYMBOL*> ParseLibComponent( const std::map<wxString, wxString>& aProperties );

    void doEnumerateSymbolLib( const wxString& aLibraryPath, const std::map<std::string, UTF8>* aProperties,
                               std::function<void(const wxString&, LIB_SYMBOL*)> aInserter );

    void fixupSymbolPinNameNumbers( SYMBOL* aSymbol );

    // Symbol caching
    void ensureLoadedLibrary( const wxString& aLibraryPath, const std::map<std::string, UTF8>* aProperties );
    long long getLibraryTimestamp( const wxString& aLibraryPath ) const;

    static bool isBinaryFile( const wxString& aFileName );
    static bool isASCIIFile( const wxString& aFileName );
    static bool checkFileHeader( const wxString& aFileName );

private:
    wxString       m_rootFilepath;      // The file path of the root sheet being imported
    SCH_SHEET*     m_rootSheet;         // The root sheet of the schematic being loaded..
    SCH_SHEET_PATH m_sheetPath;
    SCHEMATIC*     m_schematic;         // Passed to Load(), the schematic object being loaded
    wxString       m_libName;           // Library name to save symbols
    bool           m_isIntLib;          // Flag to indicate Integrated Library

    std::unique_ptr<TITLE_BLOCK>       m_currentTitleBlock;   // Will be assigned at the end of parsing
                                                              //   a sheet

    VECTOR2I                           m_sheetOffset;
    std::unique_ptr<ASCH_SHEET>        m_altiumSheet;
    std::map<int, SCH_SYMBOL*>         m_symbols;
    std::map<int, SCH_SHEET*>          m_sheets;
    std::map<int, LIB_SYMBOL*>         m_libSymbols;        // every symbol has its unique lib_symbol

    std::map<wxString, LIB_SYMBOL*>    m_powerSymbols;
    std::vector<ASCH_STORAGE_FILE>     m_altiumStorage;
    std::vector<ASCH_ADDITIONAL_FILE>  m_altiumAdditional;

    std::map<int, ASCH_SYMBOL>         m_altiumComponents;
    std::map<int, ASCH_TEMPLATE>       m_altiumTemplates;
    std::map<int, int>                 m_altiumImplementationList;
    std::vector<ASCH_PORT>             m_altiumPortsCurrentSheet; // we require all connections first

    // parse harness ports after "FileHeader" was parsed, in 2nd run.
    std::vector<ASCH_PORT>             m_altiumHarnessPortsCurrentSheet;
    std::map<int, HARNESS>             m_altiumHarnesses;

    // Add offset to all harness ownerIndex'es after parsing FileHeader.
    int m_harnessOwnerIndexOffset;
    int m_harnessEntryParent; // used to identify harness connector for harness entry element

    std::map<wxString, long long> m_timestamps;
    std::map<wxString, CASE_INSENSITIVE_MAP<LIB_SYMBOL*>> m_libCache;

    // List of available fonts with font name and font size in pt
    std::vector<std::pair<wxString, int>> m_fonts;

    // Cache the error messages to avoid duplicate messages
    std::unordered_map<wxString, SEVERITY > m_errorMessages;
};

#endif // _SCH_IO_ALTIUM_H_
