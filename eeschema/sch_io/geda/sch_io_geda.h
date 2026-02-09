/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This file contains file format knowledge derived from the gEDA and
 * Lepton EDA projects:
 *
 *   gEDA/gaf  - Copyright (C) 1998-2010 Ales Hvezda
 *               Copyright (C) 1998-2016 gEDA Contributors
 *   Lepton EDA - Copyright (C) 2017-2024 Lepton EDA Contributors
 *
 * Both projects are licensed under the GNU General Public License v2 or later.
 * See https://github.com/lepton-eda/lepton-eda and
 *     https://github.com/rlutz/geda-gaf
 */

#ifndef SCH_IO_GEDA_H_
#define SCH_IO_GEDA_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <eda_shape.h>
#include <stroke_params.h>

#include <wx/filename.h>
#include <wx/textfile.h>

#include <map>
#include <set>
#include <vector>
#include <memory>

class LIB_SYMBOL;
class SCH_SHEET;
class SCH_SCREEN;
class SCH_SYMBOL;
class SCH_LINE;
class SCH_TEXT;
class SCH_SHAPE;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_PIN;
class SCHEMATIC;


/**
 * A #SCH_IO derivation for loading gEDA/gschem schematic files (.sch).
 *
 * gEDA schematics are ASCII text files with a simple line-oriented format.
 * Each object type is identified by the first character of its definition line:
 *   C=component, N=net, U=bus, P=pin, T=text, L=line, B=box, V=circle,
 *   A=arc, H=path, G=picture, {=attributes, [=embedded component.
 *
 * Symbol loading follows the sch-rnd model: .sym files are discovered by
 * scanning standard gEDA library directories and loaded on first reference.
 * Embedded symbols (defined inline via [] blocks) are also supported.
 */
class SCH_IO_GEDA : public SCH_IO
{
public:
    SCH_IO_GEDA();
    ~SCH_IO_GEDA() override;

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "gEDA / Lepton EDA schematic files" ), { "sch" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;

    int GetModifyHash() const override { return 0; }

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*                            aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>*    aProperties = nullptr ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    /// Return the map of built-in gEDA symbol definitions (symbol name -> .sym content).
    static const std::map<wxString, wxString>& getBuiltinSymbols();

private:
    // -----------------------------------------------------------------------
    // Data types
    // -----------------------------------------------------------------------

    /// Parsed attribute from a gEDA T line inside a { } block.
    struct GEDA_ATTR
    {
        wxString name;
        wxString value;
        int      x        = 0;
        int      y        = 0;
        int      size     = 10;
        int      angle    = 0;
        int      align    = 0;
        bool     visible  = false;
        int      showNV   = 0;   ///< 0=name+value, 1=value, 2=name
    };

    /// Pending component waiting for symbol resolution.
    /// A gEDA C line defers actual symbol loading until we know whether
    /// the definition is embedded ([] block) or from the library.
    struct PENDING_COMPONENT
    {
        wxString                basename;
        int                     x           = 0;
        int                     y           = 0;
        int                     angle       = 0;
        int                     mirror      = 0;
        int                     selectable  = 1;
        std::vector<GEDA_ATTR>  attrs;
        bool                    embedded    = false;
        std::unique_ptr<LIB_SYMBOL> embeddedSym;
    };

    /// Entry in the symbol library search cache.
    struct SYM_CACHE_ENTRY
    {
        wxString                    path;       ///< Full path to .sym file
        std::unique_ptr<LIB_SYMBOL> symbol;     ///< Loaded symbol (null if not yet loaded)
        wxString                    symversion;  ///< symversion from the .sym file (if any)
        wxString                    netAttr;    ///< net= attribute (e.g. "GND:1") identifying power symbols
    };

    // -----------------------------------------------------------------------
    // Coordinate transformation
    // -----------------------------------------------------------------------

    /// gEDA coordinates are mils with Y-up. KiCad uses 100nm IU with Y-down.
    /// During parsing we store raw gEDA coordinates, then post-process to flip Y.
    static constexpr int MILS_TO_IU = 254;

    /// Convert a gEDA distance in mils to KiCad IU.
    int toKiCadDist( int aMils ) const;

    /// Apply the Y-flip and scale to transform gEDA coords to KiCad.
    /// Call only after m_maxY has been computed in the post-processing pass.
    VECTOR2I toKiCad( int aGedaX, int aGedaY ) const;

    // -----------------------------------------------------------------------
    // File parsing
    // -----------------------------------------------------------------------

    /// Parse the "v YYYYMMDD N" version line and validate.
    bool parseVersionLine( const wxString& aLine );

    /// Read a { } attribute block starting after the '{' line.
    std::vector<GEDA_ATTR> parseAttributes( wxTextFile& aFile, size_t& aLineIdx );

    /// Find an attribute by name, returns empty string if not found.
    wxString findAttr( const std::vector<GEDA_ATTR>& aAttrs, const wxString& aName ) const;

    /// Find a GEDA_ATTR struct by name, returns nullptr if not found.
    const GEDA_ATTR* findAttrStruct( const std::vector<GEDA_ATTR>& aAttrs,
                                      const wxString& aName ) const;

    /// Check for and consume a { } attribute block at the current line position.
    std::vector<GEDA_ATTR> maybeParseAttributes( wxTextFile& aFile, size_t& aLineIdx );

    // -----------------------------------------------------------------------
    // Object parsers - schematics
    // -----------------------------------------------------------------------

    void parseComponent( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseNet( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseBus( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseText( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseLine( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseBox( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseCircle( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseArc( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parsePath( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parsePicture( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parsePin( const wxString& aLine, wxTextFile& aFile, size_t& aLineIdx );
    void parseEmbeddedComponent( wxTextFile& aFile, size_t& aLineIdx );

    // -----------------------------------------------------------------------
    // Symbol loading
    // -----------------------------------------------------------------------

    /// Ensure the symbol library hash is built by scanning gEDA library dirs.
    void initSymbolLibrary();

    /// Recursively scan a directory for .sym files and populate m_symLibrary.
    void scanSymbolDir( const wxString& aDir );

    /// Parse an RC file (gafrc or gschemrc) for component-library directives.
    void parseRcFileForLibraries( const wxString& aPath, const wxString& aBaseDir );

    /// Load a .sym file and return a LIB_SYMBOL. Parses the symbol format
    /// which is the same as schematic format but contains P/L/A/V/B/T/H
    /// objects within a symbol context.
    /// If aSymversion is non-null, the symbol's symversion attribute (if present) is written to it.
    /// If aNetAttr is non-null, the symbol's net= attribute (if present) is written to it.
    std::unique_ptr<LIB_SYMBOL> loadSymbolFile( const wxString& aPath,
                                                 wxString* aSymversion = nullptr,
                                                 wxString* aNetAttr = nullptr );

    /// Parse objects from a .sym file or [] block into a LIB_SYMBOL.
    void parseSymbolObjects( wxTextFile& aFile, size_t& aLineIdx, LIB_SYMBOL& aSymbol,
                             size_t aEndLine );

    /// Create a pin on a LIB_SYMBOL from gEDA P line data and its attributes.
    void addSymbolPin( LIB_SYMBOL& aSymbol, int aX1, int aY1, int aX2, int aY2,
                       int aWhichEnd, const std::vector<GEDA_ATTR>& aAttrs );

    /// Add a graphical item (line/box/circle/arc/path) to a LIB_SYMBOL.
    void addSymbolGraphic( LIB_SYMBOL& aSymbol, const wxString& aLine, wxTextFile& aFile,
                           size_t& aLineIdx, wxChar aType );

    /// Instantiate the pending component: look up or load the symbol,
    /// create SCH_SYMBOL, apply transforms and attributes, and emit to screen.
    void flushPendingComponent();

    /// Import a gEDA hierarchical sub-schematic as a KiCad SCH_SHEET.
    /// Called when a component has a source= attribute.
    void importHierarchicalSheet( const wxString& aSourceFile );

    /// Get or load a cached symbol by gEDA basename.
    LIB_SYMBOL* getOrLoadSymbol( const wxString& aBasename );

    /// Try loading a symbol from the built-in standard library embedded in the importer.
    std::unique_ptr<LIB_SYMBOL> loadBuiltinSymbol( const wxString& aBasename );

    /// Create a fallback rectangular symbol when the .sym file is not found.
    std::unique_ptr<LIB_SYMBOL> createFallbackSymbol( const wxString& aBasename );

    /// Map gEDA angle (0/90/180/270) + mirror to KiCad symbol orientation.
    int toKiCadOrientation( int aAngle, int aMirror ) const;

    /// Derive the KiCad import library name from the schematic file.
    wxString getLibName() const;

    // -----------------------------------------------------------------------
    // Style mapping
    // -----------------------------------------------------------------------

    /// Map gEDA dashstyle (0-4) to KiCad LINE_STYLE.
    static LINE_STYLE toLineStyle( int aDashStyle );

    /// Map gEDA filltype (0-4) to KiCad FILL_T.
    static FILL_T toFillType( int aFillType );

    // -----------------------------------------------------------------------
    // Post-processing
    // -----------------------------------------------------------------------

    /// Track a point as a net or pin endpoint for junction detection.
    void trackEndpoint( int aGedaX, int aGedaY );

    /// Run the full post-processing pipeline after parsing is complete:
    /// 1. Flush any pending component
    /// 2. Compute bounding box and Y-flip all coordinates
    /// 3. Process net= attributes for implicit connections
    /// 4. Place junctions where 3+ endpoints meet
    void postProcess();

    /// For each component with net= attributes, create global labels at pin positions.
    void processNetAttributes();

    /// Place junctions where 3+ net/pin endpoints coincide.
    void addJunctions();

    /// Create SCH_BUS_WIRE_ENTRY objects where net endpoints touch bus segments.
    void addBusEntries();

    /// Load sub-schematics for any SCH_SHEET objects created during parsing.
    void loadDeferredSheets();

    /// Enlarge the page if needed and center all content on the sheet.
    void fitPageToContent();

    // -----------------------------------------------------------------------
    // State
    // -----------------------------------------------------------------------

    SCH_SCREEN*  m_screen;
    SCH_SHEET*   m_rootSheet;
    SCHEMATIC*   m_schematic;
    wxFileName   m_filename;

    /// The maximum Y coordinate seen during parsing (gEDA coords, before flip).
    int m_maxY;

    /// gEDA file version fields from the "v YYYYMMDD N" header line.
    /// Used for version-aware parsing of old format files.
    long m_releaseVersion;
    long m_fileFormatVersion;

    /// Pending component awaiting symbol resolution.
    std::unique_ptr<PENDING_COMPONENT> m_pendingComp;

    /// Symbol library cache: gEDA basename -> cache entry.
    std::map<wxString, SYM_CACHE_ENTRY> m_symLibrary;
    bool m_symLibraryInitialized;

    /// Loaded symbols for this import session, keyed by basename.
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_libSymbols;

    /// Sequential counter for auto-generated #PWR references.
    int m_powerCounter;

    /// Net endpoint positions in raw gEDA coordinates for junction detection.
    std::map<std::pair<int,int>, int> m_netEndpoints;

    /// Components with net= attributes that need post-processing.
    struct NET_ATTR_RECORD
    {
        wxString netname;
        wxString pinnumber;
        SCH_SYMBOL* symbol;
    };

    std::vector<NET_ATTR_RECORD> m_netAttrRecords;

    /// Parsed bus segment in KiCad coordinates with gEDA ripper direction.
    struct BUS_SEGMENT
    {
        VECTOR2I start;
        VECTOR2I end;
        int      ripperDir;  ///< -1, 0, or 1 from gEDA U line
    };

    std::vector<BUS_SEGMENT> m_busSegments;

    /// Deferred hierarchical sheet loads. Populated during parsing when components
    /// with source= attributes are found, processed after the main parse completes.
    struct DEFERRED_SHEET
    {
        SCH_SHEET* sheet;
        wxString   sourceFile;  ///< Resolved full path to the sub-schematic
    };

    std::vector<DEFERRED_SHEET> m_deferredSheets;

    /// Set of fully-resolved file paths currently in the import call stack,
    /// used to detect and prevent circular hierarchy references.
    std::set<wxString> m_importStack;

    /// Properties passed from the import framework (search paths, etc.)
    const std::map<std::string, UTF8>* m_properties;
};

#endif // SCH_IO_GEDA_H_
