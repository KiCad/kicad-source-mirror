/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#ifndef PCB_IO_EAGLE_H_
#define PCB_IO_EAGLE_H_

#include <io/eagle/eagle_parser.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>

#include <eda_units.h>
#include <layer_ids.h>
#include <lset.h>
#include <netclass.h>

#include <map>
#include <tuple>
#include <wx/xml/xml.h>

class PAD;
class PCB_TEXT;
class ZONE;

typedef std::map<wxString, FOOTPRINT*> FOOTPRINT_MAP;
typedef std::vector<ZONE*>             ZONES;
typedef std::map<wxString, ENET>        NET_MAP;
typedef NET_MAP::const_iterator         NET_MAP_CITER;


/// subset of eagle.drawing.board.designrules in the XML document
struct ERULES
{
    ERULES() :
        psElongationLong    ( 100 ),
        psElongationOffset  ( 0 ),

        mvStopFrame         ( 1.0 ),
        mvCreamFrame        ( 0.0 ),
        mlMinStopFrame      ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  4.0 ) ),
        mlMaxStopFrame      ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  4.0 ) ),
        mlMinCreamFrame     ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  0.0 ) ),
        mlMaxCreamFrame     ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  0.0 ) ),

        psTop               ( EPAD::UNDEF ),
        psBottom            ( EPAD::UNDEF ),
        psFirst             ( EPAD::UNDEF ),

        srRoundness         ( 0.0 ),
        srMinRoundness      ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  0.0 ) ),
        srMaxRoundness      ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  0.0 ) ),

        rvPadTop            ( 0.25 ),
        // rvPadBottom      ( 0.25 ),
        rlMinPadTop         ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  10 ) ),
        rlMaxPadTop         ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  20 ) ),

        rvViaOuter          ( 0.25 ),
        rlMinViaOuter       ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  10 ) ),
        rlMaxViaOuter       ( EDA_UNIT_UTILS::Mils2IU( pcbIUScale,  20 ) ),
        mdWireWire          ( 0 )
    {}

    void parse( wxXmlNode* aRules, std::function<void()> aCheckpoint );

    ///< percent over 100%.  0-> not elongated, 100->twice as wide as is tall
    ///< Goes into making a scaling factor for "long" pads.
    int    psElongationLong;

    int    psElongationOffset;  ///< the offset of the hole within the "long" pad.

    ///< solder mask, expressed as percentage of the smaller pad/via dimension
    double mvStopFrame;

    ///< solderpaste mask, expressed as percentage of the smaller pad/via dimension
    double mvCreamFrame;
    int    mlMinStopFrame;      ///< solder mask, minimum size (Eagle mils, here nanometers)
    int    mlMaxStopFrame;      ///< solder mask, maximum size (Eagle mils, here nanometers)
    int    mlMinCreamFrame;     ///< solder paste mask, minimum size (Eagle mils, here nanometers)
    int    mlMaxCreamFrame;     ///< solder paste mask, maximum size (Eagle mils, here nanometers)

    int    psTop;               ///< Shape of the top pads
    int    psBottom;            ///< Shape of the bottom pads
    int    psFirst;             ///< Shape of the first pads

    double srRoundness;         ///< corner rounding ratio for SMD pads (percentage)

    ///< corner rounding radius, minimum size (Eagle mils, here nanometers)
    int    srMinRoundness;

    ///< corner rounding radius, maximum size (Eagle mils, here nanometers)
    int    srMaxRoundness;

    double rvPadTop;            ///< top pad size as percent of drill size
    // double   rvPadBottom;    ///< bottom pad size as percent of drill size

    double rlMinPadTop;         ///< minimum copper annulus on through hole pads
    double rlMaxPadTop;         ///< maximum copper annulus on through hole pads

    double rvViaOuter;          ///< copper annulus is this percent of via hole
    double rlMinViaOuter;       ///< minimum copper annulus on via
    double rlMaxViaOuter;       ///< maximum copper annulus on via
    double mdWireWire;          ///< wire to wire spacing I presume.
};


/**
 * Works with Eagle 6.x XML board files and footprints to implement the Pcbnew #PLUGIN API
 * or a portion of it.
 */
class PCB_IO_EAGLE : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Eagle ver. 6.x XML PCB files" ), { "brd" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Eagle ver. 6.x XML library files" ), { "lbr" } );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;
    bool CanReadFootprint( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts, const std::map<std::string, UTF8>* aProperties = nullptr) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool  aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override
    {
        return false;   // until someone writes others like FootprintSave(), etc.
    }

    typedef int BIU;

    PCB_IO_EAGLE();
    ~PCB_IO_EAGLE();

    /**
     * Return the automapped layers.
     *
     * The callback needs to have the context of the current board so it can
     * correctly determine copper layer mapping. Thus, it is not static and is
     * expected to be bind to an instance of PCB_IO_EAGLE.
     *
     * @param aInputLayerDescriptionVector
     * @return Auto-mapped layers
     */
    std::map<wxString, PCB_LAYER_ID> DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

private:
    /// initialize PLUGIN like a constructor would, and futz with fresh BOARD if needed.
    void init( const std::map<std::string, UTF8>* aProperties );

    bool checkHeader( const wxString& aFileName ) const;

    void checkpoint();

    void clear_cu_map();

    /// Convert an Eagle distance to a KiCad distance.
    int kicad_y( const ECOORD& y ) const { return -y.ToPcbUnits(); }
    int kicad_x( const ECOORD& x ) const { return x.ToPcbUnits(); }

    /// create a font size (fontz) from an eagle font size scalar and KiCad font thickness
    VECTOR2I kicad_fontsize( const ECOORD& d, int aTextThickness ) const;

    /**
     * Generate mapping between Eagle and KiCad layers.
     *
     * @warning It is imperative that this gets called correctly because footprint libraries
     *          do not get remapped by the user on load.  Otherwise, Pcbnew will crash when
     *          attempting to load footprint libraries that contain layers that do not exist
     *          in the #EAGLE_LAYER definitions.
     *
     * @param aIsLibraryCache is the flag to indicate when mapping the footprint library cache
     *                        layers rather than the board layers.
     */
    void mapEagleLayersToKicad( bool aIsLibraryCache = false );

    /// Convert an Eagle layer to a KiCad layer.
    PCB_LAYER_ID kicad_layer( int aLayer ) const;

    /**
     * Get the default KiCad layer corresponding to an Eagle layer of the board,
     * a set of sensible layer mapping options and required flag
     *
     * @note The Eagle MILLING, TTEST, BTEST, and HOLES layers are set to #UNDEFINED_LAYER
     *       for historical purposes.  All other Eagle layers that do not directly map to
     *       KiCad layers will be set to #UNDEFINED_LAYER when loading Eagle footprint
     *       libraries.  This should be addressed in the future because in some cases this
     *       will cause data loss.
     *
     * @see #EAGLE_LAYER and defaultKiCadLayer().
     *
     * @param aEagleLayer is the Eagle layer to map.
     * @param aIsLibraryCache is a flag to indicate if the mapping is for board or footprint
     *                        library cache objects.
     * @return a tuple containing the mapped layer.
     */
    std::tuple<PCB_LAYER_ID, LSET, bool> defaultKicadLayer( int aEagleLayer,
                                                            bool aIsLibraryCache = false ) const;

    /// Get Eagle layer name by its number
    const wxString& eagle_layer_name( int aLayer ) const;

    /// Get Eagle layer number by its name
    int eagle_layer_id( const wxString& aLayerName ) const;

    void setKeepoutSettingsToZone( ZONE* aZone, int aLayer ) const;

    /// This PLUGIN only caches one footprint library, this determines which one.
    void cacheLib( const wxString& aLibraryPath );

    /// get a file's  or dir's modification time.
    static wxDateTime getModificationTime( const wxString& aPath );

    /// Determines the minimum copper layer stackup count that includes all mapped layers.
    int getMinimumCopperLayerCount() const;

    // all these loadXXX() throw IO_ERROR or ptree_error exceptions:

    void loadAllSections( wxXmlNode* aDocument );
    void loadDesignRules( wxXmlNode* aDesignRules );
    void loadLayerDefs( wxXmlNode* aLayers );
    void loadPlain( wxXmlNode* aPlain );
    void loadClasses( wxXmlNode* aClasses );
    void loadSignals( wxXmlNode* aSignals );

    /**
     * Load the Eagle "library" XML element, which can occur either under a "libraries"
     * element (if a *.brd file) or under a "drawing" element if a *.lbr file.
     *
     * @param aLib is the portion of the loaded XML document tree that is the "library"
     *             element.
     * @param aLibName is a pointer to the library name or NULL.  If NULL this means
     *                 we are loading a *.lbr not a *.brd file and the key used in m_templates
     *                 is to exclude the library name.
     */
    void loadLibrary( wxXmlNode* aLib, const wxString* aLibName );

    void loadLibraries( wxXmlNode* aLibs );
    void loadElements( wxXmlNode* aElements );

    /**
     * Load a copper or keepout polygon and adds it to the board.
     *
     * @return The loaded zone or nullptr if was not processed.
     */
    ZONE* loadPolygon( wxXmlNode* aPolyNode );

    void orientFootprintAndText( FOOTPRINT* aFootprint, const EELEMENT& e, const EATTR* aNameAttr,
                                 const EATTR* aValueAttr );

    void orientFPText( FOOTPRINT* aFootprint, const EELEMENT& e, PCB_TEXT* aFPText,
                       const EATTR* aAttr );


    /// move the BOARD into the center of the page
    void centerBoard();

    /**
     * Create a FOOTPRINT from an Eagle package.
     */
    FOOTPRINT* makeFootprint( wxXmlNode* aPackage, const wxString& aPkgName );

    void packageWire( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const;
    void packagePad( FOOTPRINT* aFootprint, wxXmlNode* aTree );
    void packageText( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const;
    void packageRectangle( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const;
    void packagePolygon( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const;
    void packageCircle( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const;

    /**
     * @param aFootprint The KiCad footprint to which to assign the hole.
     * @param aTree The Eagle XML node that is of type "hole".
     * @param aCenter If true, center the hole in the footprint and offset the footprint position.
     */
    void packageHole( FOOTPRINT* aFootprint, wxXmlNode* aTree, bool aCenter ) const;
    void packageSMD( FOOTPRINT* aFootprint, wxXmlNode* aTree ) const;

    ///< Handles common pad properties
    void transferPad( const EPAD_COMMON& aEaglePad, PAD* aPad ) const;

    ///< Deletes the footprint templates list
    void deleteTemplates();

    typedef std::vector<ELAYER>     ELAYERS;
    typedef ELAYERS::const_iterator EITER;

    int                              m_cu_map[17];     ///< map eagle to KiCad, cu layers only.
    std::map<int, ELAYER>            m_eagleLayers;    ///< Eagle layer data stored by layer number
    std::map<wxString, int>          m_eagleLayersIds; ///< Eagle layer ids stored by layer name
    std::map<wxString, PCB_LAYER_ID> m_layer_map;      ///< Map of Eagle layers to KiCad layers

    ///< Eagle class number to KiCad netclass
    std::map<wxString, std::shared_ptr<NETCLASS>>  m_classMap;

    wxString                         m_customRules;

    ERULES*       m_rules;          ///< Eagle design rules.
    XPATH*        m_xpath;          ///< keeps track of what we are working on within
                                    ///< XML document during a Load().

    int           m_hole_count;     ///< generates unique footprint names from eagle "hole"s.

    NET_MAP       m_pads_to_nets;   ///< net list

    std::map<wxString, FOOTPRINT*> m_templates;  ///< is part of a FOOTPRINT factory that operates
                                                 ///< using copy construction.
                                                 ///< lookup key is either libname.packagename or
                                                 ///< simply packagename if FootprintLoad() or
                                                 ///< FootprintEnumberate()

    PROGRESS_REPORTER*  m_progressReporter;  ///< optional; may be nullptr
    unsigned            m_doneCount;
    unsigned            m_lastProgressCount;
    unsigned            m_totalCount;         ///< for progress reporting

    int                 m_min_trace;        ///< smallest trace we find on Load(), in BIU.
    int                 m_min_hole;         ///< smallest diameter hole we find on Load(), in BIU.
    int                 m_min_via;          ///< smallest via we find on Load(), in BIU.
    int                 m_min_annulus;      ///< smallest via annulus we find on Load(), in BIU.

    wxString            m_lib_path;
    long long           m_timestamp;
};

#endif  // PCB_IO_EAGLE_H_
