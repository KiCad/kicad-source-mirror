/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Russell Oliver <roliver8143@gmail.com>
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
 */

#ifndef SCH_IO_EAGLE_H_
#define SCH_IO_EAGLE_H_

#include <sch_line.h>
#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>
#include <io/eagle/eagle_parser.h>
#include <geometry/seg.h>


class EDA_TEXT;
class KIWAY;
class LINE_READER;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_BITMAP;
class SCH_JUNCTION;
class SCH_NO_CONNECT;
class SCH_SHAPE;
class SCH_LINE;
class SCH_BUS_ENTRY_BASE;
class SCH_TEXT;
class SCH_GLOBALLABEL;
class SCH_SYMBOL;
class SCH_FIELD;
class LIB_SYMBOL;
class LEGACY_SYMBOL_LIB;
class SCH_PIN;
class wxXmlNode;


struct EAGLE_LIBRARY
{
    wxString name;
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> KiCadSymbols;

    /**
     * Map Eagle gate unit number (which are strings) to KiCad library symbol unit number.
     *
     * The look up name is #EDEVICE name + #EDEVICE name + #EGATE name separated by underscores.
     * Hashing would be faster but it would be nearly impossible to debug so use string look up
     * for now.
     */
    std::unordered_map<wxString, int> GateToUnitMap;
    std::unordered_map<wxString, wxString> package;
};


/**
 * A #SCH_IO derivation for loading 6.x+ Eagle schematic files.
 *
 * As with all #SCH_IO objects there are no UI dependencies i.e. windowing calls allowed.
 */
class SCH_IO_EAGLE : public SCH_IO
{
public:
    const int ARC_ACCURACY = KiROUND( SCH_IU_PER_MM * 0.01 ); // 0.01mm

    SCH_IO_EAGLE();
    ~SCH_IO_EAGLE();

    const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Eagle XML schematic files" ), { "sch" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Eagle XML library files" ), { "lbr" } );
    }

    bool CanReadSchematicFile( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;

    int GetModifyHash() const override;

    SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                  SCH_SHEET*             aAppendToMe = nullptr,
                                  const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void EnumerateSymbolLib( wxArrayString& aSymbolNameList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties ) override;

    void EnumerateSymbolLib( std::vector<LIB_SYMBOL*>& aSymbolList, const wxString& aLibraryPath,
                             const std::map<std::string, UTF8>* aProperties ) override;

    LIB_SYMBOL* LoadSymbol( const wxString& aLibraryPath, const wxString& aAliasName,
                            const std::map<std::string, UTF8>* aProperties ) override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

private:
    bool          checkHeader( const wxString& aFileName ) const;
    wxXmlDocument loadXmlDocument( const wxString& aFileName );
    long long     getLibraryTimestamp( const wxString& aLibraryPath ) const;
    void          ensureLoadedLibrary( const wxString& aLibraryPath );

    void loadDrawing( const std::unique_ptr<EDRAWING>& aDrawing );
    void loadLayerDefs( const std::vector<std::unique_ptr<ELAYER>>& aLayers );
    void loadSchematic( const ESCHEMATIC& aSchematic );
    void loadSheet( const std::unique_ptr<ESHEET>& aSheet );
    void loadInstance( const std::unique_ptr<EINSTANCE>& aInstance,
                       const std::map<wxString, std::unique_ptr<EPART>>& aParts );

    void loadModuleInstance( const std::unique_ptr<EMODULEINST>& aModuleInstance );

    EAGLE_LIBRARY* loadLibrary( const ELIBRARY* aLibrary, EAGLE_LIBRARY* aEagleLib );
    void countNets( const ESCHEMATIC& aSchematic );

    /// Move any labels on the wire to the new end point of the wire.
    void moveLabels( SCH_LINE* aWire, const VECTOR2I& aNewEndPoint );

    /// This function finds best way to place a bus entry symbol for when an Eagle wire segment
    /// ends on an Eagle bus segment.
    void addBusEntries();

    /// Return the matching layer or return LAYER_NOTES
    SCH_LAYER_ID kiCadLayer( int aEagleLayer );

    std::pair<VECTOR2I, const SEG*> findNearestLinePoint( const VECTOR2I&         aPoint,
                                                          const std::vector<SEG>& aLines ) const;

    void          loadSegments( const std::vector<std::unique_ptr<ESEGMENT>>& aSegments,
                                const wxString& aNetName,
                                const wxString& aNetClass );
    SCH_SHAPE*    loadPolyLine( const std::unique_ptr<EPOLYGON>& aPolygon );
    SCH_ITEM*     loadWire( const std::unique_ptr<EWIRE>& aWire, SEG& endpoints );
    SCH_SHAPE*    loadCircle( const std::unique_ptr<ECIRCLE>& aCircle );
    SCH_SHAPE*    loadRectangle( const std::unique_ptr<ERECT>& aRect );
    SCH_TEXT*     loadLabel( const std::unique_ptr<ELABEL>& aLabel, const wxString& aNetName );
    SCH_JUNCTION* loadJunction( const std::unique_ptr<EJUNCTION>&  aJunction );
    SCH_TEXT*     loadPlainText( const std::unique_ptr<ETEXT>& aSchText );
    void          loadFrame( const std::unique_ptr<EFRAME>& aFrame,
                             std::vector<SCH_ITEM*>& aItems );

    bool          loadSymbol( const std::unique_ptr<ESYMBOL>& aEsymbol,
                              std::unique_ptr<LIB_SYMBOL>& aSymbol,
                              const std::unique_ptr<EDEVICE>& aDevice, int aGateNumber,
                              const wxString& aGateName );
    SCH_SHAPE*    loadSymbolCircle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                    const std::unique_ptr<ECIRCLE>& aCircle,
                                    int aGateNumber );
    SCH_SHAPE*    loadSymbolRectangle( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                       const std::unique_ptr<ERECT>& aRectangle,
                                       int aGateNumber );
    SCH_SHAPE*    loadSymbolPolyLine( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                      const std::unique_ptr<EPOLYGON>& aPolygon, int aGateNumber );
    SCH_ITEM*     loadSymbolWire( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                  const std::unique_ptr<EWIRE>& aWire,
                                  int aGateNumber );
    SCH_PIN*      loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol, const std::unique_ptr<EPIN>& aPin,
                           int aGateNumber );
    SCH_TEXT*     loadSymbolText( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                  const std::unique_ptr<ETEXT>& aText,
                                  int aGateNumber );
    void          loadTextAttributes( EDA_TEXT* aText,
                                      const std::unique_ptr<ETEXT>& aAttributes ) const;
    void          loadFieldAttributes( SCH_FIELD* aField, const SCH_TEXT* aText ) const;

    ///< Move net labels that are detached from any wire to the nearest wire
    void adjustNetLabels();

    /**
     * Translate an Eagle-style bus name into one that is KiCad-compatible.
     *
     * For vector buses such as A[7..0] this has no impact.  For group buses, we translate from
     * Eagle-style to KiCad-style.
     *
     * @param aEagleName is the name of the bus from the Eagle schematic
     */
    wxString translateEagleBusName( const wxString& aEagleName ) const;

    wxString        getLibName();
    wxFileName      getLibFileName();

    ///< Checks if there are other wires or pins at the position of the tested pin
    bool checkConnections( const SCH_SYMBOL* aSymbol, const SCH_PIN* aPin ) const;

    /**
     * Create net labels to emulate implicit connections in Eagle.
     *
     * Each named power input pin creates an implicit connection in Eagle. To emulate this behavior
     * one needs to attach global net labels to the mentioned pins. This is is also expected for the
     * units that are not instantiated in the schematics, therefore such units need to be stored
     * in order to create them at later stage.
     *
     * @param aSymbol is the symbol to process.
     * @param aScreen is the screen where net labels should be added.
     * @param aUpdateSet decides whether the missing units data should be updated.
     */
    void addImplicitConnections( SCH_SYMBOL* aSymbol, SCH_SCREEN* aScreen, bool aUpdateSet );

    bool netHasPowerDriver( SCH_LINE* aLine, const wxString& aNetName ) const;

    void getEagleSymbolFieldAttributes( const std::unique_ptr<EINSTANCE>& aInstance,
                                        const wxString& aEagleFieldName,
                                        SCH_FIELD* aField );
    const ESYMBOL* getEagleSymbol( const std::unique_ptr<EINSTANCE>& aInstance );

    SCH_SHEET* getCurrentSheet();
    SCH_SCREEN* getCurrentScreen();

    // Describe missing units containing pins creating implicit connections
    // (named power pins in Eagle).
    struct EAGLE_MISSING_CMP
    {
        EAGLE_MISSING_CMP( const SCH_SYMBOL* aSymbol = nullptr )
            : cmp( aSymbol )
        {
        }

        ///< Link to the parent symbol
        const SCH_SYMBOL* cmp;

        /* Map of the symbol units: for each unit there is a flag saying
         * whether the unit needs to be instantiated with appropriate net labels to
         * emulate implicit connections as is done in Eagle.
         */
        std::map<int, bool> units;
    };

    ///< Map references to missing symbol units data
    std::map<wxString, EAGLE_MISSING_CMP> m_missingCmps;

    SCH_SHEET*  m_rootSheet;      ///< The root sheet of the schematic being loaded
    SCH_SHEET_PATH  m_sheetPath;  ///< The current sheet path of the schematic being loaded.
    wxString    m_version;        ///< Eagle file version.
    wxFileName  m_filename;
    wxString    m_libName;        ///< Library name to save symbols
    SCHEMATIC*  m_schematic;      ///< Passed to Load(), the schematic object being loaded
    std::vector<EMODULE*> m_modules; ///< The current module stack being loaded.
    std::vector<EMODULEINST*> m_moduleInstances;

    std::map<wxString, const EPART*>   m_partlist;
    std::map<wxString, long long>      m_timestamps;
    std::map<wxString, EAGLE_LIBRARY>  m_eagleLibs;
    std::map<wxString, std::unique_ptr<EMODULE>> m_eagleModules;

    std::unordered_map<wxString, bool> m_userValue; ///< deviceset/@uservalue for device.

    IO_RELEASER<SCH_IO>                m_pi;                ///< PI to create KiCad symbol library.

    int                                m_sheetIndex;
    std::map<wxString, int>            m_netCounts;
    std::map<int, SCH_LAYER_ID>        m_layerMap;
    std::map<wxString, wxString>       m_powerPorts;        ///< map from symbol reference to global
                                                            ///<   label equivalent

    ///< Wire intersection points, used for quick checks whether placing a net label in a particular
    ///< place would short two nets.
    std::vector<VECTOR2I> m_wireIntersections;

    ///< Wires and labels of a single connection (segment in Eagle nomenclature)
    struct SEG_DESC
    {
        ///< Test if a particular label is attached to any of the stored segments
        const SEG* LabelAttached( const SCH_TEXT* aLabel ) const;

        std::vector<SCH_TEXT*> labels;
        std::vector<SEG> segs;
    };

    ///< Segments representing wires for intersection checking
    std::vector<SEG_DESC> m_segments;

    ///< Nets as defined in the <nets> sections of an Eagle schematic file.
    std::map<wxString, ENET> m_nets;

    ///< Positions of pins and wire endings mapped to its parent
    std::map<VECTOR2I, std::set<const EDA_ITEM*>> m_connPoints;

    ///< The fully parsed Eagle schematic file.
    std::unique_ptr<EAGLE_DOC> m_eagleDoc;
};

#endif  // SCH_IO_EAGLE_H_
