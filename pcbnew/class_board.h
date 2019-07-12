/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CLASS_BOARD_H_
#define CLASS_BOARD_H_

#include <tuple>
#include <core/iterators.h>
#include <board_design_settings.h>
#include <board_item_container.h>
#include <class_module.h>
#include <class_pad.h>
#include <pcb_general_settings.h>
#include <common.h> // PAGE_INFO
#include <eda_rect.h>
#include <layers_id_colors_and_visibility.h>
#include <netinfo.h>
#include <pcb_plot_params.h>
#include <title_block.h>
#include <zone_settings.h>

#include <memory>

using std::unique_ptr;

class PCB_BASE_FRAME;
class PCB_EDIT_FRAME;
class PICKED_ITEMS_LIST;
class BOARD;
class ZONE_CONTAINER;
class TRACK;
class D_PAD;
class MARKER_PCB;
class MSG_PANEL_ITEM;
class NETLIST;
class REPORTER;
class SHAPE_POLY_SET;
class CONNECTIVITY_DATA;
class COMPONENT;

/**
 * Enum LAYER_T
 * gives the allowed types of layers, same as Specctra DSN spec.
 */
enum LAYER_T
{
    LT_UNDEFINED = -1,
    LT_SIGNAL,
    LT_POWER,
    LT_MIXED,
    LT_JUMPER
};


/**
 * Class LAYER
 * holds information pertinent to a layer of a BOARD.
 */
struct LAYER
{
    LAYER()
    {
        clear();
    }

    void clear()
    {
        m_type    = LT_SIGNAL;
        m_visible = true;
        m_number  = 0;
        m_name.clear();
    }

    /*
    LAYER( const wxString& aName = wxEmptyString,
            LAYER_T aType = LT_SIGNAL, bool aVisible = true, int aNumber = -1 ) :
        m_name( aName ),
        m_type( aType ),
        m_visible( aVisible ),
        m_number( aNumber )
    {
    }
    */

    wxString    m_name;         ///< The name of the layer, there should be no spaces in this name.
    LAYER_T     m_type;         ///< The type of the layer
    bool        m_visible;
    int         m_number;

    /**
     * Function ShowType
     * converts a LAYER_T enum to a const char*
     * @param aType The LAYER_T to convert
     * @return const char* - The string representation of the layer type.
     */
    static const char* ShowType( LAYER_T aType );

    /**
     * Function ParseType
     * converts a string to a LAYER_T
     * @param aType The const char* to convert
     * @return LAYER_T - The binary representation of the layer type, or
     *   LAYER_T(-1) if the string is invalid
     */
    static LAYER_T     ParseType( const char* aType );
};


// Helper class to handle high light nets
class HIGH_LIGHT_INFO
{
    friend class BOARD;

protected:
    int m_netCode;           // net selected for highlight (-1 when no net selected )
    bool m_highLightOn;      // highlight active

    void Clear()
    {
        m_netCode = -1;
        m_highLightOn = false;
    }

    HIGH_LIGHT_INFO()
    {
        Clear();
    }
};


DECL_VEC_FOR_SWIG( MARKERS, MARKER_PCB* )
DECL_VEC_FOR_SWIG( ZONE_CONTAINERS, ZONE_CONTAINER* )
DECL_DEQ_FOR_SWIG( TRACKS, TRACK* )


/**
 * Class BOARD
 * holds information pertinent to a Pcbnew printed circuit board.
 */
class BOARD : public BOARD_ITEM_CONTAINER
{
    friend class PCB_EDIT_FRAME;

private:
    /// the board filename
    wxString                m_fileName;

    /// MARKER_PCBs for clearance problems, owned by pointer.
    MARKERS                 m_markers;

    /// BOARD_ITEMs for drawings on the board, owned by pointer.
    DRAWINGS                m_drawings;

    /// MODULES for components on the board, owned by pointer.
    MODULES                 m_modules;

    /// TRACKS for traces on the board, owned by pointer.
    TRACKS                  m_tracks;

    /// edge zone descriptors, owned by pointer.
    ZONE_CONTAINERS         m_ZoneDescriptorList;

    LAYER                   m_Layer[PCB_LAYER_ID_COUNT];

                                                    // if true m_highLight_NetCode is used
    HIGH_LIGHT_INFO         m_highLight;                // current high light data
    HIGH_LIGHT_INFO         m_highLightPrevious;        // a previously stored high light data

    int                     m_fileFormatVersionAtLoad;  ///< the version loaded from the file

    std::shared_ptr<CONNECTIVITY_DATA>      m_connectivity;

    BOARD_DESIGN_SETTINGS   m_designSettings;
    ZONE_SETTINGS           m_zoneSettings;
    PCB_GENERAL_SETTINGS*   m_generalSettings;      ///< reference only; I have no ownership
    PAGE_INFO               m_paper;
    TITLE_BLOCK             m_titles;               ///< text in lower right of screen and plots
    PCB_PLOT_PARAMS         m_plotOptions;
    NETINFO_LIST            m_NetInfo;              ///< net info list (name, design constraints ..


    // The default copy constructor & operator= are inadequate,
    // either write one or do not use it at all
    BOARD( const BOARD& aOther ) :
        BOARD_ITEM_CONTAINER( aOther ), m_NetInfo( this )
    {
        assert( false );
    }

    BOARD& operator=( const BOARD& aOther )
    {
        assert( false );
        return *this;       // just to mute warning
    }

public:
    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_T == aItem->Type();
    }

    void SetFileName( const wxString& aFileName ) { m_fileName = aFileName; }

    const wxString &GetFileName() const { return m_fileName; }

    TRACKS& Tracks()
    {
        return m_tracks;
    }
    const TRACKS& Tracks() const
    {
        return m_tracks;
    }

    MODULES& Modules()
    {
        return m_modules;
    }
    const MODULES& Modules() const
    {
        return m_modules;
    }

    DRAWINGS& Drawings()
    {
        return m_drawings;
    }

    ZONE_CONTAINERS& Zones()
    {
        return m_ZoneDescriptorList;
    }

    const std::vector<BOARD_CONNECTED_ITEM*> AllConnectedItems();

    /// zone contour currently in progress
    ZONE_CONTAINER*             m_CurrentZoneContour;

    BOARD();
    ~BOARD();

    const wxPoint GetPosition() const override;
    void SetPosition( const wxPoint& aPos ) override;

    bool IsEmpty() const
    {
        return m_drawings.empty() && m_modules.empty() && m_tracks.empty() &&
               m_ZoneDescriptorList.empty();
    }

    void Move( const wxPoint& aMoveVector ) override;

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad()  const { return m_fileFormatVersionAtLoad; }

    void Add( BOARD_ITEM* aItem, ADD_MODE aMode = ADD_INSERT ) override;

    void Remove( BOARD_ITEM* aBoardItem ) override;

    /**
     * Gets the first module in the list (used in footprint viewer/editor) or NULL if none
     * @return first module or null pointer
     */
    MODULE* GetFirstModule() const
    {
        return m_modules.empty() ? nullptr : m_modules.front();
    }

    /**
     * Removes all modules from the deque and frees the memory associated with them
     */
    void DeleteAllModules()
    {
        for( MODULE* mod : m_modules )
            delete mod;

        m_modules.clear();
    }

    BOARD_ITEM* GetItem( void* aWeakReference );

    BOARD_ITEM* Duplicate( const BOARD_ITEM* aItem, bool aAddToBoard = false );

    /**
     * Function GetConnectivity()
     * returns list of missing connections between components/tracks.
     * @return an object that contains informations about missing connections.
     */
    std::shared_ptr<CONNECTIVITY_DATA> GetConnectivity() const
    {
        return m_connectivity;
    }

    /**
     * Builds or rebuilds the board connectivity database for the board,
     * especially the list of connected items, list of nets and rastnest data
     * Needed after loading a board to have the connectivity database updated.
     */
    void BuildConnectivity();


    /**
     * Function DeleteMARKERs
     * deletes ALL MARKERS from the board.
     */
    void DeleteMARKERs();

    /**
     * Function DeleteZONEOutlines
     * deletes ALL zone outlines from the board.
     */
    void DeleteZONEOutlines();

    /**
     * Function GetMARKER
     * returns the MARKER at a given index.
     * @param index The array type index into a collection of MARKER_PCBS.
     * @return MARKER_PCB* - a pointer to the MARKER_PCB or NULL if index out of range.
     */
    MARKER_PCB* GetMARKER( int index ) const
    {
        if( (unsigned) index < m_markers.size() )
            return m_markers[index];

        return NULL;
    }

    /**
     * Function GetMARKERCount
     * @return int - The number of MARKER_PCBS.
     */
    int GetMARKERCount() const
    {
        return (int) m_markers.size();
    }

    /**
     * Function SetAuxOrigin
     * sets the origin point used for plotting.
     */
    void SetAuxOrigin( const wxPoint& aPoint )      { m_designSettings.m_AuxOrigin = aPoint; }
    const wxPoint& GetAuxOrigin() const             { return m_designSettings.m_AuxOrigin; }

    /**
     * Function SetGridOrigin
     * sets the origin point of the grid.
     */
    void SetGridOrigin( const wxPoint& aPoint )     { m_designSettings.m_GridOrigin = aPoint; }
    const wxPoint& GetGridOrigin() const            { return m_designSettings.m_GridOrigin; }

    /**
     * Function ResetNetHighLight
     * Reset all high light data to the init state
     */
    void ResetNetHighLight()
    {
        m_highLight.Clear();
        m_highLightPrevious.Clear();
    }

    /**
     * Function GetHighLightNetCode
     * @return netcode of net to highlight (-1 when no net selected)
     */
    int GetHighLightNetCode() const { return m_highLight.m_netCode; }

    /**
     * Function SetHighLightNet
     * @param aNetCode = netcode of net to highlight
     */
    void SetHighLightNet( int aNetCode)
    {
        m_highLight.m_netCode = aNetCode;
    }

    /**
     * Function IsHighLightNetON
     * @return true if a net is currently highlighted
     */
    bool IsHighLightNetON() const { return m_highLight.m_highLightOn; }

    /**
     * Function HighLightOFF
     * Disable highlight.
     */
    void HighLightOFF() { m_highLight.m_highLightOn = false; }

    /**
     * Function HighLightON
     * Enable highlight.
     * if m_highLight_NetCode >= 0, this net will be highlighted
     */
    void HighLightON() { m_highLight.m_highLightOn = true; }

    /**
     * Function GetCopperLayerCount
     * @return int - The number of copper layers in the BOARD.
     */
    int  GetCopperLayerCount() const;

    void SetCopperLayerCount( int aCount );

    /**
     * Function GetEnabledLayers
     * is a proxy function that calls the corresponding function in m_BoardSettings
     * Returns a bit-mask of all the layers that are enabled
     * @return int - the enabled layers in bit-mapped form.
     */
    LSET GetEnabledLayers() const;

    /**
     * Function SetEnabledLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * Changes the bit-mask of enabled layers
     * @param aLayerMask = The new bit-mask of enabled layers
     */
    void SetEnabledLayers( LSET aLayerMask );

    /**
     * Function IsLayerEnabled
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * tests whether a given layer is enabled
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool IsLayerEnabled( PCB_LAYER_ID aLayer ) const
    {
        return m_designSettings.IsLayerEnabled( aLayer );
    }

    /**
     * Function IsLayerVisible
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * tests whether a given layer is visible
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool IsLayerVisible( PCB_LAYER_ID aLayer ) const
    {
        return m_designSettings.IsLayerVisible( aLayer );
    }

    /**
     * Function GetVisibleLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * Returns a bit-mask of all the layers that are visible
     * @return int - the visible layers in bit-mapped form.
     */
    LSET  GetVisibleLayers() const;

    /**
     * Function SetVisibleLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * changes the bit-mask of visible layers
     * @param aLayerMask = The new bit-mask of visible layers
     */
    void SetVisibleLayers( LSET aLayerMask );

    // these 2 functions are not tidy at this time, since there are PCB_LAYER_IDs that
    // are not stored in the bitmap.

    /**
     * Function GetVisibleElements
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * returns a bit-mask of all the element categories that are visible
     * @return int - the visible element bitmap or-ed from enum GAL_LAYER_ID
     * @see enum GAL_LAYER_ID
     */
    int GetVisibleElements() const;

    /**
     * Function SetVisibleElements
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * changes the bit-mask of visible element categories
     * @param aMask = The new bit-mask of visible element bitmap or-ed from enum GAL_LAYER_ID
     * @see enum GAL_LAYER_ID
     */
    void SetVisibleElements( int aMask );

    /**
     * Function SetVisibleAlls
     * changes the bit-mask of visible element categories and layers
     * @see enum GAL_LAYER_ID
     */
    void SetVisibleAlls();

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aLayer is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum GAL_LAYER_ID
     */
    bool IsElementVisible( GAL_LAYER_ID aLayer ) const;

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aLayer is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum GAL_LAYER_ID
     */
    void SetElementVisibility( GAL_LAYER_ID aLayer, bool aNewState );

    /**
     * Function IsModuleLayerVisible
     * expects either of the two layers on which a module can reside, and returns
     * whether that layer is visible.
     * @param aLayer One of the two allowed layers for modules: F_Cu or B_Cu
     * @return bool - true if the layer is visible, else false.
     */
    bool IsModuleLayerVisible( PCB_LAYER_ID aLayer );

    /**
     * Function GetDesignSettings
     * @return the BOARD_DESIGN_SETTINGS for this BOARD
     */
    BOARD_DESIGN_SETTINGS& GetDesignSettings() const
    {
        // remove const-ness with cast.
        return (BOARD_DESIGN_SETTINGS&) m_designSettings;
    }

    /**
     * Function SetDesignSettings
     * @param aDesignSettings the new BOARD_DESIGN_SETTINGS to use
     */
    void SetDesignSettings( const BOARD_DESIGN_SETTINGS& aDesignSettings )
    {
        m_designSettings = aDesignSettings;
    }

    const PAGE_INFO& GetPageSettings() const                { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    const PCB_PLOT_PARAMS& GetPlotOptions() const           { return m_plotOptions; }
    void SetPlotOptions( const PCB_PLOT_PARAMS& aOptions )  { m_plotOptions = aOptions; }

    TITLE_BLOCK& GetTitleBlock()                            { return m_titles; }
    void SetTitleBlock( const TITLE_BLOCK& aTitleBlock )    { m_titles = aTitleBlock; }

    const ZONE_SETTINGS& GetZoneSettings() const            { return m_zoneSettings; }
    void SetZoneSettings( const ZONE_SETTINGS& aSettings )  { m_zoneSettings = aSettings; }

    wxString    GetSelectMenuText( EDA_UNITS_T aUnits ) const override;

    /**
     * Function GetColorSettings
     * @return the current COLORS_DESIGN_SETTINGS in use
     */
    const COLORS_DESIGN_SETTINGS& Colors() const { return m_generalSettings->Colors(); }

    const PCB_GENERAL_SETTINGS& GeneralSettings() const { return *m_generalSettings; }

    void SetGeneralSettings( PCB_GENERAL_SETTINGS* aGeneralSettings )
    {
        m_generalSettings = aGeneralSettings;
    }

    /**
     * Function GetBoardPolygonOutlines
     * Extracts the board outlines and build a closed polygon
     * from lines, arcs and circle items on edge cut layer
     * Any closed outline inside the main outline is a hole
     * All contours should be closed, i.e. have valid vertices to build a closed polygon
     * @param aOutlines The SHAPE_POLY_SET to fill in with outlines/holes.
     * @param aErrorText = a wxString reference to display an error message
     *          with the coordinate of the point which creates the error
     *          (default = nullptr , no message returned on error)
     * @param aErrorLocation = a wxPoint giving the location of the Error message on the board
     *          if left null (default), no location is returned
     *
     * @return true if success, false if a contour is not valid
     */
    bool GetBoardPolygonOutlines( SHAPE_POLY_SET& aOutlines,
                                  wxString* aErrorText = nullptr, wxPoint* aErrorLocation = nullptr );

    /**
     * Function ConvertBrdLayerToPolygonalContours
     * Build a set of polygons which are the outlines of copper items
     * (pads, tracks, vias, texts, zones)
     * Holes in vias or pads are ignored
     * Usefull to export the shape of copper layers to dxf polygons
     * or 3D viewer
     * the polygons are not merged.
     * @param aLayer = A copper layer, like B_Cu, etc.
     * @param aOutlines The SHAPE_POLY_SET to fill in with items outline.
     */
    void ConvertBrdLayerToPolygonalContours( PCB_LAYER_ID aLayer, SHAPE_POLY_SET& aOutlines );

    /**
     * Function GetLayerID
     * returns the ID of a layer given by aLayerName.  Copper layers may
     * have custom names.
     *
     * @param aLayerName = A layer name, like wxT("B.Cu"), etc.
     *
     * @return PCB_LAYER_ID -   the layer id, which for copper layers may
     *                      be custom, else standard.
     */
    const PCB_LAYER_ID GetLayerID( const wxString& aLayerName ) const;

    /**
     * Function GetLayerName
     * returns the name of a layer given by aLayer.  Copper layers may
     * have custom names.
     *
     * @param aLayer = A layer, like B_Cu, etc.
     *
     * @return wxString -   the layer name, which for copper layers may
     *                      be custom, else standard.
     */
    const wxString GetLayerName( PCB_LAYER_ID aLayer ) const;

    /**
     * Function SetLayerName
     * changes the name of the layer given by aLayer.
     *
     * @param aLayer A layer, like B_Cu, etc.
     * @param aLayerName The new layer name
     * @return bool - true if aLayerName was legal and unique among other
     *   layer names at other layer indices and aLayer was within range, else false.
     */
    bool SetLayerName( PCB_LAYER_ID aLayer, const wxString& aLayerName );

    /**
     * Function GetStandardLayerName
     * returns an "English Standard" name of a PCB layer when given \a aLayerNumber.
     * This function is static so it can be called without a BOARD instance.  Use
     * GetLayerName() if want the layer names of a specific BOARD, which could
     * be different than the default if the user has renamed any copper layers.
     *
     * @param  aLayerId is the layer identifier (index) to fetch
     * @return const wxString - containing the layer name or "BAD INDEX" if aLayerId
     *                      is not legal
     */
    static wxString GetStandardLayerName( PCB_LAYER_ID aLayerId )
    {
        // a BOARD's standard layer name is the PCB_LAYER_ID fixed name
        return LSET::Name( aLayerId );
    }

    /**
     * Function SetLayerDescr
     * returns the type of the copper layer given by aLayer.
     *
     * @param aIndex A layer index in m_Layer
     * @param aLayer A reference to a LAYER description.
     * @return false if the index was out of range.
     */
    bool SetLayerDescr( PCB_LAYER_ID aIndex, const LAYER& aLayer );

    /**
     * Function GetLayerType
     * returns the type of the copper layer given by aLayer.
     *
     * @param aLayer A layer index, like B_Cu, etc.
     * @return LAYER_T - the layer type, or LAYER_T(-1) if the
     *  index was out of range.
     */
    LAYER_T GetLayerType( PCB_LAYER_ID aLayer ) const;

    /**
     * Function SetLayerType
     * changes the type of the layer given by aLayer.
     *
     * @param aLayer A layer index, like B_Cu, etc.
     * @param aLayerType The new layer type.
     * @return bool - true if aLayerType was legal and aLayer was within range, else false.
     */
    bool SetLayerType( PCB_LAYER_ID aLayer, LAYER_T aLayerType );

    /**
     * Function GetNodesCount
     * @param aNet Only count nodes belonging to this net
     * @return the number of pads members of nets (i.e. with netcode > 0)
     */
    unsigned GetNodesCount( int aNet = -1 );

    /**
     * Function GetUnconnectedNetCount
     * @return the number of unconnected nets in the current ratsnest.
     */
    unsigned GetUnconnectedNetCount() const;

    /**
     * Function GetPadCount
     * @return the number of pads in board
     */
    unsigned GetPadCount();

    /**
     * Function GetPad
     * @return D_PAD* - at the \a aIndex
     */
    D_PAD* GetPad( unsigned aIndex ) const;

    /**
     * Function GetPads
     * returns a reference to a list of all the pads.  The returned list is not
     * sorted and contains pointers to PADS, but those pointers do not convey
     * ownership of the respective PADs.
     * @return D_PADS - a full list of pads
     */
    const std::vector<D_PAD*> GetPads();

    void BuildListOfNets()
    {
        m_NetInfo.buildListOfNets();
    }

    /**
     * Function FindNet
     * searches for a net with the given netcode.
     * @param aNetcode A netcode to search for.
     * @return NETINFO_ITEM_ITEM* - the net or NULL if not found.
     */
    NETINFO_ITEM* FindNet( int aNetcode ) const;

    /**
     * Function FindNet overloaded
     * searches for a net with the given name.
     * @param aNetname A Netname to search for.
     * @return NETINFO_ITEM* - the net or NULL if not found.
     */
    NETINFO_ITEM* FindNet( const wxString& aNetname ) const;

    NETINFO_LIST& GetNetInfo()
    {
        return m_NetInfo;
    }

#ifndef SWIG
    /**
     * Function BeginNets
     * @return iterator to the first element of the NETINFO_ITEMs list
     */
    NETINFO_LIST::iterator BeginNets() const
    {
        return m_NetInfo.begin();
    }

    /**
     * Function EndNets
     * @return iterator to the last element of the NETINFO_ITEMs list
     */
    NETINFO_LIST::iterator EndNets() const
    {
        return m_NetInfo.end();
    }
#endif

    /**
     * Function GetNetCount
     * @return the number of nets (NETINFO_ITEM)
     */
    unsigned GetNetCount() const
    {
        return m_NetInfo.GetNetCount();
    }

    /**
     * Function ComputeBoundingBox
     * calculates the bounding box containing all board items (or board edge segments).
     * @param aBoardEdgesOnly is true if we are interested in board edge segments only.
     * @return EDA_RECT - the board's bounding box
     */
    EDA_RECT ComputeBoundingBox( bool aBoardEdgesOnly = false ) const;

    const EDA_RECT GetBoundingBox() const override
    {
        return ComputeBoundingBox( false );
    }

    /**
     * Function GetBoardEdgesBoundingBox
     * Returns the board bounding box calculated using exclusively the board edges (graphics
     * on Edge.Cuts layer). If there are items outside of the area limited by Edge.Cuts graphics,
     * the items will not be taken into account.
     * @return bounding box calculated using exclusively the board edges.
     */
    const EDA_RECT GetBoardEdgesBoundingBox() const
    {
        return ComputeBoundingBox( true );
    }

    void GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector< MSG_PANEL_ITEM >& aList ) override;

    /**
     * Function Print.
     * Print the BOARD items.
     * @param aFrame = the current Frame
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ... (not always used)
     * @param aOffset = an draw offset value (default = 0,0)
     */
    void Print( PCB_BASE_FRAME* aFrame, wxDC* aDC, const wxPoint& aOffset = ZeroOffset ) override;

    /**
     * Function Visit
     * may be re-implemented for each derived class in order to handle
     * all the types given by its member data.  Implementations should call
     * inspector->Inspect() on types in scanTypes[], and may use IterateForward()
     * to do so on lists of such data.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *  else SCAN_CONTINUE, and determined by the inspector.
     */
    SEARCH_RESULT Visit( INSPECTOR inspector, void* testData, const KICAD_T scanTypes[] ) override;

    /**
     * Function FindModuleByReference
     * searches for a MODULE within this board with the given
     * reference designator.  Finds only the first one, if there
     * is more than one such MODULE.
     * @param aReference The reference designator of the MODULE to find.
     * @return MODULE* - If found, the MODULE having the given reference
     *  designator, else NULL.
     */
    MODULE* FindModuleByReference( const wxString& aReference ) const;

    /**
     * Function FindModule
     * searches for a module matching \a aRefOrTimeStamp depending on the state of
     * \a aSearchByTimeStamp.
     * @param aRefOrTimeStamp is the search string.
     * @param aSearchByTimeStamp searches by the module time stamp value if true.  Otherwise
     *                           search by reference designator.
     * @return MODULE* - If found, the module meeting the search criteria, else NULL.
     */
    MODULE* FindModule( const wxString& aRefOrTimeStamp, bool aSearchByTimeStamp = false ) const;

    /**
     * Function SortedNetnamesList
     * @param aNames An array string to fill with net names.
     * @param aSortbyPadsCount  true = sort by active pads count, false = no sort (i.e.
     *                          leave the sort by net names)
     * @return int - net names count.
     */
    int SortedNetnamesList( wxArrayString& aNames, bool aSortbyPadsCount );

    /**
     * Function SynchronizeNetsAndNetClasses
     * copies NETCLASS info to each NET, based on NET membership in a NETCLASS.
     * Must be called after a Design Rules edit, or after reading a netlist (or editing
     * the list of nets)  Also this function removes the non existing nets in netclasses
     * and add net nets in default netclass (this happens after reading a netlist)
     */
    void SynchronizeNetsAndNetClasses();

    /***************************************************************************/

    wxString GetClass() const override
    {
        return wxT( "BOARD" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const override { ShowDummy( os ); }
#endif


    /*************************/
    /* Copper Areas handling */
    /*************************/

    /**
     * Function HitTestForAnyFilledArea
     * tests if the given wxPoint is within the bounds of a filled area of this zone.
     * the test is made on zones on layer from aStartLayer to aEndLayer
     * Note: if a zone has its flag BUSY (in .m_State) is set, it is ignored.
     * @param aRefPos A wxPoint to test
     * @param aStartLayer the first layer to test
     * @param aEndLayer the last layer to test
     * @param aNetCode = the netcode used to filter zones (-1 to to test all zones)
     * @return ZONE_CONTAINER* return a pointer to the ZONE_CONTAINER found, else NULL
     */
    ZONE_CONTAINER* HitTestForAnyFilledArea( const wxPoint& aRefPos,
                                             PCB_LAYER_ID      aStartLayer,
                                             PCB_LAYER_ID      aEndLayer,
                                             int aNetCode );

    /**
     * Function SetAreasNetCodesFromNetNames
     * Set the .m_NetCode member of all copper areas, according to the area Net Name
     * The SetNetCodesFromNetNames is an equivalent to net name, for fast comparisons.
     * However the Netcode is an arbitrary equivalence, it must be set after each netlist read
     * or net change
     * Must be called after pad netcodes are calculated
     * @return : error count
     * For non copper areas, netcode is set to 0
     */
    int SetAreasNetCodesFromNetNames( void );

    /**
     * Function GetArea
     * returns the Area (Zone Container) at a given index.
     * @param index The array type index into a collection of ZONE_CONTAINER *.
     * @return ZONE_CONTAINER* - a pointer to the Area or NULL if index out of range.
     */
    ZONE_CONTAINER* GetArea( int index ) const
    {
        if( (unsigned) index < m_ZoneDescriptorList.size() )
            return m_ZoneDescriptorList[index];

        return NULL;
    }

    /**
     * Function GetAreaIndex
     * returns the Area Index  for the given Zone Container.
     * @param  aArea :The ZONE_CONTAINER to find.
     * @return an Area Index in m_ZoneDescriptorList or -1 if non found.
     */
    int GetAreaIndex( const ZONE_CONTAINER* aArea ) const
    {
        for( int ii = 0; ii < GetAreaCount(); ii++ )    // Search for aArea in list
        {
            if( aArea == GetArea( ii ) )                // Found !
                return ii;
        }

        return -1;
    }

    /**
     * Function GetAreaCount
     * @return int - The number of Areas or ZONE_CONTAINER.
     */
    int GetAreaCount() const
    {
        return (int) m_ZoneDescriptorList.size();
    }

    /* Functions used in test, merge and cut outlines */

    /**
     * Function AddArea
     * Add an empty copper area to board areas list
     * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new areas  pickers (useful
     *                        in undo commands) can be NULL
     * @param aNetcode = the netcode of the copper area (0 = no net)
     * @param aLayer = the layer of area
     * @param aStartPointPosition = position of the first point of the polygon outline of this area
     * @param aHatch = hatch option
     * @return a reference to the new area
     */
    ZONE_CONTAINER* AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode,
                             PCB_LAYER_ID aLayer, wxPoint aStartPointPosition, int aHatch );

    /**
     * Add a copper area to net, inserting after m_ZoneDescriptorList[aAreaIdx]
     * @param aNetcode is the netcode of the new copper zone
     * @param aAreaIdx is the netcode of the new copper zone
     * @param aLayer is the copper layer id of the new copper zone
     * @param aCornerX,aCornerY is the coordinate of the first corner
     * (a zone cannot have a empty outline)
     * @param aHatch is the hatch option
     * @return pointer to the new area
     */
    ZONE_CONTAINER* InsertArea( int aNetcode, int aAreaIdx, PCB_LAYER_ID aLayer,
                                int aCornerX, int aCornerY, int aHatch );

    /**
     * Function NormalizeAreaPolygon
     * Process an area that has been modified, by normalizing its polygon against itself.
     * i.e. convert a self-intersecting polygon to one (or more) non self-intersecting polygon(s)
     * This may change the number and order of copper areas in the net.
     * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new created areas pickers
     * @param aCurrArea = the zone to process
     * @return true if changes are made
     */
    bool NormalizeAreaPolygon( PICKED_ITEMS_LIST* aNewZonesList, ZONE_CONTAINER* aCurrArea );

    /**
     * Function OnAreaPolygonModified
     * Process an area that has been modified, by normalizing its polygon
     * and merging the intersecting polygons for any other areas on the same net.
     * This may change the number and order of copper areas in the net.
     * @param aModifiedZonesList = a PICKED_ITEMS_LIST * where to store deleted or added areas
     *                             (useful in undo commands can be NULL
     * @param modified_area = area to test
     * @return true if some areas modified
    */
    bool OnAreaPolygonModified( PICKED_ITEMS_LIST* aModifiedZonesList,
                                ZONE_CONTAINER*    modified_area );

    /**
     * Function CombineAllAreasInNet
     * Checks all copper areas in net for intersections, combining them if found
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful
     *                       in undo commands can be NULL
     * @param aNetCode = net to consider
     * @param aUseLocalFlags : if true, don't check areas if both local flags are 0
     * Sets local flag = 1 for any areas modified
     * @return true if some areas modified
     */
    bool CombineAllAreasInNet( PICKED_ITEMS_LIST* aDeletedList,
                               int                aNetCode,
                               bool               aUseLocalFlags );

    /**
     * Function RemoveArea
     * remove copper area from net, and put it in a deleted list (if exists)
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful
     * in undo commands can be NULL
     * @param  area_to_remove = area to delete or put in deleted list
     */
    void RemoveArea( PICKED_ITEMS_LIST* aDeletedList, ZONE_CONTAINER* area_to_remove );

    /**
     * Function TestAreaIntersections
     * Check for intersection of a given copper area with other areas in same net
     * @param area_to_test = area to compare to all other areas in the same net
     */
    bool TestAreaIntersections( ZONE_CONTAINER* area_to_test );

    /**
     * Function TestAreaIntersection
     * Test for intersection of 2 copper areas
     * area_to_test must be after area_ref in m_ZoneDescriptorList
     * @param area_ref = area reference
     * @param area_to_test = area to compare for intersection calculations
     * @return : false if no intersection, true if intersection
     */
    bool TestAreaIntersection( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_test );

    /**
     * Function CombineAreas
     * If possible, combine 2 copper areas
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas
     *                      (useful for undo).
     * @param area_ref = the main area (zone)
     * @param area_to_combine = the zone that can be merged with area_ref
     * area_ref must be BEFORE area_to_combine
     * area_to_combine will be deleted, if areas are combined
     * @return : true if area_to_combine is combined with area_ref (and therefore be deleted)
     */
    bool CombineAreas( PICKED_ITEMS_LIST* aDeletedList,
                       ZONE_CONTAINER*    area_ref,
                       ZONE_CONTAINER*    area_to_combine );

    /**
     * Function GetPad
     * finds a pad \a aPosition on \a aLayer.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found or NULL if not found.
     */
    D_PAD* GetPad( const wxPoint& aPosition, LSET aLayerMask );
    D_PAD* GetPad( const wxPoint& aPosition )
    {
        return GetPad( aPosition, LSET().set() );
    }

    /**
     * Function GetPad
     * finds a pad connected to \a aEndPoint of \a aTrace.
     *
     * @param aTrace A pointer to a TRACK object to hit test against.
     * @param aEndPoint The end point of \a aTrace the hit test against.
     * @return A pointer to a D_PAD object if found or NULL if not found.
     */
    D_PAD* GetPad( TRACK* aTrace, ENDPOINT_T aEndPoint );

    /**
     * Function GetPadFast
     * return pad found at \a aPosition on \a aLayerMask using the fast search method.
     * <p>
     * The fast search method only works if the pad list has already been built.
     * </p>
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found or NULL if not found.
     */
    D_PAD* GetPadFast( const wxPoint& aPosition, LSET aLayerMask );

    /**
     * Function GetPad
     * locates the pad connected at \a aPosition on \a aLayer starting at list position
     * \a aPad
     * <p>
     * This function uses a fast search in this sorted pad list and it is faster than
     * GetPadFast().  This list is a sorted pad list must be built before calling this
     * function.
     * </p>
     * @note The normal pad list is sorted by increasing netcodes.
     * @param aPadList = the list of pads candidates (a std::vector<D_PAD*>)
     * @param aPosition A wxPoint object containing the position to test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return a D_PAD object pointer to the connected pad.
     */
    D_PAD* GetPad( std::vector<D_PAD*>& aPadList, const wxPoint& aPosition, LSET aLayerMask );

    /**
     * Function PadDelete
     * deletes a given bad from the BOARD by removing it from its module and
     * from the m_NetInfo.  Makes no UI calls.
     * @param aPad is the pad to delete.
     */
    void PadDelete( D_PAD* aPad );

    /**
     * Function GetSortedPadListByXthenYCoord
     * first empties then fills the vector with all pads and sorts them by
     * increasing x coordinate, and for increasing y coordinate for same values of x coordinates.
     * The vector only holds pointers to the pads and
     * those pointers are only references to pads which are owned by the BOARD
     * through other links.
     * @param aVector Where to put the pad pointers.
     * @param aNetCode = the netcode filter:
     *                  = -1 to build the full pad list.
     *                  = a given netcode to build the pad list relative to the given net
     */
    void GetSortedPadListByXthenYCoord( std::vector<D_PAD*>& aVector, int aNetCode = -1 );

    /**
     * Returns data on the length and number of track segments connected to a given track.
     * This uses the connectivity data for the board to calculate connections
     *
     * @param aTrack Starting track (can also be a via) to check against for connection.
     * @return a tuple containing <number, length, package length>
     */
    std::tuple<int, double, double> GetTrackLength( const TRACK& aTrack ) const;

    /**
     * Function TrackInNet
     * collects all the TRACKs and VIAs that are members of a net given by aNetCode.
     * Used from python.
     * @param aNetCode gives the id of the net.
     * @return TRACKS - which are in the net identified by @a aNetCode.
     */
    TRACKS TracksInNet( int aNetCode );

    /**
     * Function GetFootprint
     * get a footprint by its bounding rectangle at \a aPosition on \a aLayer.
     * <p>
     * If more than one footprint is at \a aPosition, then the closest footprint on the
     * active layer is returned.  The distance is calculated via manhattan distance from
     * the center of the bounding rectangle to \a aPosition.
     *
     * @param aPosition A wxPoint object containing the position to test.
     * @param aActiveLayer Layer to test.
     * @param aVisibleOnly Search only the visible layers if true.
     * @param aIgnoreLocked Ignore locked modules when true.
     * @return MODULE* The best module or NULL if none.
     */
    MODULE* GetFootprint( const wxPoint& aPosition, PCB_LAYER_ID aActiveLayer,
                          bool aVisibleOnly, bool aIgnoreLocked = false );

    /**
     * Function ClearAllNetCodes()
     * Resets all items' netcodes to 0 (no net).
     */
    void ClearAllNetCodes();

    void SanitizeNetcodes();
};

#endif      // CLASS_BOARD_H_
