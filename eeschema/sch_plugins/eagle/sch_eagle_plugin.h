/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef _SCH_EAGLE_PLUGIN_H_
#define _SCH_EAGLE_PLUGIN_H_

#include <sch_line.h>
#include <sch_io_mgr.h>
#include <plugins/eagle/eagle_parser.h>
#include <lib_item.h>
#include <geometry/seg.h>

#include <boost/ptr_container/ptr_map.hpp>

class EDA_TEXT;
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
class SCH_GLOBALLABEL;
class SCH_SYMBOL;
class SCH_FIELD;
class PROPERTIES;
class SCH_EAGLE_PLUGIN_CACHE;
class LIB_SYMBOL;
class PART_LIB;
class LIB_CIRCLE;
class LIB_FIELD;
class LIB_RECTANGLE;
class LIB_POLYLINE;
class LIB_PIN;
class LIB_TEXT;
class wxXmlNode;


typedef struct EAGLE_LIBRARY
{
    wxString name;
    boost::ptr_map<wxString, LIB_SYMBOL> KiCadSymbols;
    std::unordered_map<wxString, wxXmlNode*> SymbolNodes;
    std::unordered_map<wxString, int> GateUnit;
    std::unordered_map<wxString, wxString> package;
} EAGLE_LIBRARY;

typedef boost::ptr_map<wxString, EPART> EPART_LIST;


/**
 * A #SCH_PLUGIN derivation for loading 6.x+ Eagle schematic files.
 *
 * As with all SCH_PLUGINs there is no UI dependencies i.e. windowing calls allowed.
 */
class SCH_EAGLE_PLUGIN : public SCH_PLUGIN
{
public:
    SCH_EAGLE_PLUGIN();
    ~SCH_EAGLE_PLUGIN();

    const wxString GetName() const override;

    void SetReporter( REPORTER* aReporter ) override { m_reporter = aReporter; }

    const wxString GetFileExtension() const override;

    const wxString GetLibraryFileExtension() const override;

    int GetModifyHash() const override;

    SCH_SHEET* Load( const wxString& aFileName, SCHEMATIC* aSchematic,
                     SCH_SHEET* aAppendToMe = nullptr,
                     const PROPERTIES* aProperties = nullptr ) override;

    bool CheckHeader( const wxString& aFileName ) override;


private:
    void loadDrawing( wxXmlNode* aDrawingNode );
    void loadLayerDefs( wxXmlNode* aLayers );
    void loadSchematic( wxXmlNode* aSchematicNode );
    void loadSheet( wxXmlNode* aSheetNode, int sheetcount );
    void loadInstance( wxXmlNode* aInstanceNode );
    EAGLE_LIBRARY* loadLibrary( wxXmlNode* aLibraryNode, EAGLE_LIBRARY* aEagleLib );
    void countNets( wxXmlNode* aSchematicNode );

    /// Move any labels on the wire to the new end point of the wire.
    void moveLabels( SCH_ITEM* aWire, const wxPoint& aNewEndPoint );

    /// This function finds best way to place a bus entry symbol for when an Eagle wire segment
    /// ends on an Eagle bus segment.
    void addBusEntries();

    /// Return the matching layer or return LAYER_NOTES
    SCH_LAYER_ID kiCadLayer( int aEagleLayer );

    std::pair<VECTOR2I, const SEG*> findNearestLinePoint( const wxPoint& aPoint,
            const std::vector<SEG>& aLines ) const;

    void                loadSegments( wxXmlNode* aSegmentsNode, const wxString& aNetName,
                                      const wxString& aNetClass );
    SCH_LINE*           loadWire( wxXmlNode* aWireNode );
    SCH_TEXT*           loadLabel( wxXmlNode* aLabelNode, const wxString& aNetName );
    SCH_JUNCTION*       loadJunction( wxXmlNode* aJunction );
    SCH_TEXT*           loadPlainText( wxXmlNode* aSchText );
    void                loadFrame( wxXmlNode* aFrameNode, std::vector<SCH_LINE*>& aLines );

    bool            loadSymbol( wxXmlNode* aSymbolNode, std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                EDEVICE* aDevice, int aGateNumber, const wxString& aGateName );
    LIB_CIRCLE*     loadSymbolCircle( std::unique_ptr<LIB_SYMBOL>& aSymbol, wxXmlNode* aCircleNode,
                                      int aGateNumber );
    LIB_RECTANGLE*  loadSymbolRectangle( std::unique_ptr<LIB_SYMBOL>& aSymbol, wxXmlNode* aRectNode,
                                         int aGateNumber );
    LIB_POLYLINE*   loadSymbolPolyLine( std::unique_ptr<LIB_SYMBOL>& aSymbol,
                                        wxXmlNode* aPolygonNode, int aGateNumber );
    LIB_ITEM*       loadSymbolWire( std::unique_ptr<LIB_SYMBOL>& aSymbol, wxXmlNode* aWireNode,
                                    int aGateNumber );
    LIB_PIN*        loadPin( std::unique_ptr<LIB_SYMBOL>& aSymbol, wxXmlNode*, EPIN* epin,
                             int aGateNumber );
    LIB_TEXT*       loadSymbolText( std::unique_ptr<LIB_SYMBOL>& aSymbol, wxXmlNode* aLibText,
                                    int aGateNumber );
    void            loadFrame( wxXmlNode* aFrameNode, std::vector<LIB_ITEM*>& aLines );

    void            loadTextAttributes( EDA_TEXT* aText, const ETEXT& aAttribs ) const;
    void            loadFieldAttributes( LIB_FIELD* aField, const LIB_TEXT* aText ) const;

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
    bool checkConnections( const SCH_SYMBOL* aSymbol, const LIB_PIN* aPin ) const;

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

    /**
     * Fix invalid characters in Eagle symbol names.
     *
     * It changes invalid characters to underscores.
     *
     * @param aName is the symbol name to be fixed.
     * @return Fixed symbol name.
     */
    static wxString fixSymbolName( const wxString& aName );

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

    REPORTER*   m_reporter;       ///< Reporter for warnings/errors

    ///< Map references to missing symbol units data
    std::map<wxString, EAGLE_MISSING_CMP> m_missingCmps;

    SCH_SHEET*  m_rootSheet;      ///< The root sheet of the schematic being loaded
    SCH_SHEET*  m_currentSheet;   ///< The current sheet of the schematic being loaded
    wxString    m_version;        ///< Eagle file version.
    wxFileName  m_filename;
    wxString    m_libName;        ///< Library name to save symbols
    SCHEMATIC*  m_schematic;      ///< Passed to Load(), the schematic object being loaded

    EPART_MAP                         m_partlist;
    std::map<wxString, EAGLE_LIBRARY> m_eagleLibs;

    SCH_PLUGIN::SCH_PLUGIN_RELEASER   m_pi;         ///< Plugin to create the KiCad symbol library.
    std::unique_ptr< PROPERTIES >     m_properties; ///< Library plugin properties.

    std::map<wxString, int>           m_netCounts;
    std::map<int, SCH_LAYER_ID>       m_layerMap;

    ///< Wire intersection points, used for quick checks whether placing a net label in a particular
    ///< place would short two nets.
    std::vector<VECTOR2I> m_wireIntersections;

    ///< Wires and labels of a single connection (segment in Eagle nomenclature)
    typedef struct SEG_DESC_STRUCT
    {
        ///< Test if a particular label is attached to any of the stored segments
        const SEG* LabelAttached( const SCH_TEXT* aLabel ) const;

        std::vector<SCH_TEXT*> labels;
        std::vector<SEG> segs;
    } SEG_DESC;

    ///< Segments representing wires for intersection checking
    std::vector<SEG_DESC> m_segments;

    ///< Nets as defined in the <nets> sections of an Eagle schematic file.
    std::map<wxString, ENET> m_nets;

    ///< Positions of pins and wire endings mapped to its parent
    std::map<wxPoint, std::set<const EDA_ITEM*>> m_connPoints;
};

#endif  // _SCH_EAGLE_PLUGIN_H_
