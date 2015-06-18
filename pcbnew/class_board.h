/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file class_board.h
 * @brief Class BOARD to handle a board.
 */

#ifndef CLASS_BOARD_H_
#define CLASS_BOARD_H_


#include <dlist.h>

#include <common.h>                         // PAGE_INFO
#include <layers_id_colors_and_visibility.h>
#include <class_netinfo.h>
#include <class_pad.h>
#include <class_colors_design_settings.h>
#include <class_board_design_settings.h>
#include <class_title_block.h>
#include <class_zone_settings.h>
#include <pcb_plot_params.h>


class PCB_BASE_FRAME;
class PCB_EDIT_FRAME;
class PICKED_ITEMS_LIST;
class BOARD;
class ZONE_CONTAINER;
class SEGZONE;
class TRACK;
class D_PAD;
class MARKER_PCB;
class MSG_PANEL_ITEM;
class NETLIST;
class REPORTER;
class RN_DATA;

namespace KIGFX
{
    class RATSNEST_VIEWITEM;
    class WORKSHEET_VIEWITEM;
}


// non-owning container of item candidates when searching for items on the same track.
typedef std::vector< TRACK* >   TRACK_PTRS;


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


/**
 * Class BOARD
 * holds information pertinent to a Pcbnew printed circuit board.
 */
class BOARD : public BOARD_ITEM
{
    friend class PCB_EDIT_FRAME;

private:
    /// the board filename
    wxString                m_fileName;

    // @todo: switch to boost:ptr_vector, and change ~BOARD()
    typedef std::vector<MARKER_PCB*> MARKERS;

    /// MARKER_PCBs for clearance problems, owned by pointer.
    MARKERS                 m_markers;

    // @todo: switch to boost::ptr_vector, and change ~BOARD()
    typedef std::vector<ZONE_CONTAINER*> ZONE_CONTAINERS;

    /// edge zone descriptors, owned by pointer.
    ZONE_CONTAINERS         m_ZoneDescriptorList;

    LAYER                   m_Layer[LAYER_ID_COUNT];

                                                    // if true m_highLight_NetCode is used
    HIGH_LIGHT_INFO         m_highLight;                // current high light data
    HIGH_LIGHT_INFO         m_highLightPrevious;        // a previously stored high light data

    int                     m_fileFormatVersionAtLoad;  ///< the version loaded from the file

    EDA_RECT                m_BoundingBox;
    NETINFO_LIST            m_NetInfo;              ///< net info list (name, design constraints ..
    RN_DATA*                m_ratsnest;

    BOARD_DESIGN_SETTINGS   m_designSettings;
    ZONE_SETTINGS           m_zoneSettings;
    COLORS_DESIGN_SETTINGS* m_colorsSettings;
    PAGE_INFO               m_paper;
    TITLE_BLOCK             m_titles;               ///< text in lower right of screen and plots
    PCB_PLOT_PARAMS         m_plotOptions;

    /// Number of pads connected to the current net.
    int                     m_nodeCount;

    /// Number of unconnected nets in the current rats nest.
    int                     m_unconnectedNetCount;

    /**
     * Function chainMarkedSegments
     * is used by MarkTrace() to set the BUSY flag of connected segments of the trace
     * segment located at \a aPosition on aLayerMask.
     *  Vias are put in list but their flags BUSY is not set
     * @param aPosition A wxPoint object containing the position of the starting search.
     * @param aLayerMask The allowed layers for segments to search.
     * @param aList The track list to fill with points of flagged segments.
     */
    void chainMarkedSegments( wxPoint aPosition, LSET aLayerMask, TRACK_PTRS* aList );

public:
    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_T == aItem->Type();
    }

    void SetFileName( const wxString& aFileName ) { m_fileName = aFileName; }

    const wxString &GetFileName() const { return m_fileName; }

    /// Flags used in ratsnest calculation and update.
    int m_Status_Pcb;

    DLIST<BOARD_ITEM>           m_Drawings;              // linked list of lines & texts
    DLIST<MODULE>               m_Modules;               // linked list of MODULEs
    DLIST<TRACK>                m_Track;                 // linked list of TRACKs and VIAs
    DLIST<SEGZONE>              m_Zone;                  // linked list of SEGZONEs

    /// Ratsnest list for the BOARD
    std::vector<RATSNEST_ITEM>  m_FullRatsnest;

    /// Ratsnest list relative to a given footprint (used while moving a footprint).
    std::vector<RATSNEST_ITEM>  m_LocalRatsnest;

    /// zone contour currently in progress
    ZONE_CONTAINER*             m_CurrentZoneContour;

    BOARD();
    ~BOARD();

    virtual const wxPoint& GetPosition() const;

    virtual void SetPosition( const wxPoint& aPos );

    bool IsEmpty() const
    {
        return m_Drawings.GetCount() == 0 && m_Modules.GetCount() == 0 &&
               m_Track.GetCount() == 0 && m_Zone.GetCount() == 0;
    }

    void Move( const wxPoint& aMoveVector );        // overload

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad()  const { return m_fileFormatVersionAtLoad; }

    /**
     * Function Add
     * adds the given item to this BOARD and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param aControl An int which can vary how the item is added.
     */
    void Add( BOARD_ITEM* aBoardItem, int aControl = 0 );

#define ADD_APPEND 1        ///< aControl flag for Add( aControl ), appends not inserts

    /**
     * Function Delete
     * removes the given single item from this BOARD and deletes its memory.
     * @param aBoardItem The item to remove from this board and delete
     */
    void Delete( BOARD_ITEM* aBoardItem )
    {
        // developers should run DEBUG versions and fix such calls with NULL
        wxASSERT( aBoardItem );

        if( aBoardItem )
            delete Remove( aBoardItem );
    }


    /**
     * Function Remove
     * removes \a aBoardItem from this BOARD and returns it to caller without deleting it.
     * @param aBoardItem The item to remove from this board.
     * @return BOARD_ITEM* \a aBoardItem which was passed in.
     */
    BOARD_ITEM* Remove( BOARD_ITEM* aBoardItem );

    BOARD_ITEM* DuplicateAndAddItem( const BOARD_ITEM* aItem,
                                     bool aIncrementReferences );

    /**
     * Function GetNextModuleReferenceWithPrefix
     * Get the next available module reference with this prefix
     */
    wxString GetNextModuleReferenceWithPrefix( const wxString& aPrefix,
                                               bool aFillSequenceGaps );

    /**
     * Function GetRatsnest()
     * returns list of missing connections between components/tracks.
     * @return RATSNEST* is an object that contains informations about missing connections.
     */
    RN_DATA* GetRatsnest() const
    {
        return m_ratsnest;
    }

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
     * Function ResetHighLight
     * Reset all high light data to the init state
     */
    void ResetHighLight()
    {
        m_highLight.Clear();
        m_highLightPrevious.Clear();
    }

    /**
     * Function GetHighLightNetCode
     * @return netcode of net to highlight (-1 when no net selected)
     */
    int GetHighLightNetCode() { return m_highLight.m_netCode; }

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
    bool IsHighLightNetON() { return m_highLight.m_highLightOn; }

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
     * Function PushHighLight
     * save current high light info for later use
     */
    void PushHighLight();

    /**
     * Function PopHighLight
     * retrieve a previously saved high light info
     */
    void PopHighLight();

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
    bool IsLayerEnabled( LAYER_ID aLayer ) const
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
    bool IsLayerVisible( LAYER_ID aLayer ) const
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

    // these 2 functions are not tidy at this time, since there are PCB_VISIBLEs that
    // are not stored in the bitmap.

    /**
     * Function GetVisibleElements
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * returns a bit-mask of all the element categories that are visible
     * @return int - the visible element bitmap or-ed from enum PCB_VISIBLE
     * @see enum PCB_VISIBLE
     */
    int GetVisibleElements() const;

    /**
     * Function SetVisibleElements
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * changes the bit-mask of visible element categories
     * @param aMask = The new bit-mask of visible element bitmap or-ed from enum PCB_VISIBLE
     * @see enum PCB_VISIBLE
     */
    void SetVisibleElements( int aMask );

    /**
     * Function SetVisibleAlls
     * changes the bit-mask of visible element categories and layers
     * @see enum PCB_VISIBLE
     */
    void SetVisibleAlls();

    /**
     * Function IsElementVisible
     * tests whether a given element category is visible. Keep this as an
     * inline function.
     * @param aPCB_VISIBLE is from the enum by the same name
     * @return bool - true if the element is visible.
     * @see enum PCB_VISIBLE
     */
    bool IsElementVisible( int aPCB_VISIBLE ) const;

    /**
     * Function SetElementVisibility
     * changes the visibility of an element category
     * @param aPCB_VISIBLE is from the enum by the same name
     * @param aNewState = The new visibility state of the element category
     * @see enum PCB_VISIBLE
     */
    void SetElementVisibility( int aPCB_VISIBLE, bool aNewState );

    /**
     * Function IsModuleLayerVisible
     * expects either of the two layers on which a module can reside, and returns
     * whether that layer is visible.
     * @param layer One of the two allowed layers for modules: F_Cu or B_Cu
     * @return bool - true if the layer is visible, else false.
     */
    bool IsModuleLayerVisible( LAYER_ID layer );

    /**
     * Function GetVisibleElementColor
     * returns the color of a pcb visible element.
     * @see enum PCB_VISIBLE
     */
    EDA_COLOR_T GetVisibleElementColor( int aPCB_VISIBLE );

    void SetVisibleElementColor( int aPCB_VISIBLE, EDA_COLOR_T aColor );

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

    /**
     * Function GetColorSettings
     * @return the current COLORS_DESIGN_SETTINGS in use
     */
    COLORS_DESIGN_SETTINGS* GetColorsSettings() const { return m_colorsSettings; }

    /**
     * Function SetColorsSettings
     * @param aColorsSettings = the new COLORS_DESIGN_SETTINGS to use
     */
    void SetColorsSettings( COLORS_DESIGN_SETTINGS* aColorsSettings )
    {
        m_colorsSettings = aColorsSettings;
    }

    /**
     * Function GetBoardPolygonOutlines
     * Extracts the board outlines and build a closed polygon
     * from lines, arcs and circle items on edge cut layer
     * Any closed outline inside the main outline is a hole
     * All contours should be closed, i.e. have valid vertices to build a closed polygon
     * @param aOutlines The CPOLYGONS_LIST to fill in with main outlines.
     * @param aHoles The empty CPOLYGONS_LIST to fill in with holes, if any.
     * @param aErrorText = a wxString reference to display an error message
     *          with the coordinate of the point which creates the error
     *          (default = NULL , no message returned on error)
     * @return true if success, false if a contour is not valid
     */
    bool GetBoardPolygonOutlines( CPOLYGONS_LIST& aOutlines,
                                  CPOLYGONS_LIST& aHoles,
                                  wxString* aErrorText = NULL );

    /**
     * Function ConvertBrdLayerToPolygonalContours
     * Build a set of polygons which are the outlines of copper items
     * (pads, tracks, vias, texts, zones)
     * Holes in vias or pads are ignored
     * Usefull to export the shape of copper layers to dxf polygons
     * or 3D viewer
     * the polygons are not merged.
     * @param aLayer = A copper layer, like B_Cu, etc.
     * @param aOutlines The CPOLYGONS_LIST to fill in with items outline.
     */
    void ConvertBrdLayerToPolygonalContours( LAYER_ID aLayer, CPOLYGONS_LIST& aOutlines );

    /**
     * Function GetLayerID
     * returns the ID of a layer given by aLayerName.  Copper layers may
     * have custom names.
     *
     * @param aLayerName = A layer name, like wxT("B.Cu"), etc.
     *
     * @return LAYER_ID -   the layer id, which for copper layers may
     *                      be custom, else standard.
     */
    const LAYER_ID GetLayerID( const wxString& aLayerName ) const;

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
    const wxString GetLayerName( LAYER_ID aLayer ) const;

    /**
     * Function SetLayerName
     * changes the name of the layer given by aLayer.
     *
     * @param aLayer A layer, like B_Cu, etc.
     * @param aLayerName The new layer name
     * @return bool - true if aLayerName was legal and unique among other
     *   layer names at other layer indices and aLayer was within range, else false.
     */
    bool SetLayerName( LAYER_ID aLayer, const wxString& aLayerName );

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
    static const wxString GetStandardLayerName( LAYER_ID aLayerId )
    {
        // a BOARD's standard layer name is the LAYER_ID fixed name
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
    bool SetLayerDescr( LAYER_ID aIndex, const LAYER& aLayer );

    /**
     * Function GetLayerType
     * returns the type of the copper layer given by aLayer.
     *
     * @param aLayer A layer index, like B_Cu, etc.
     * @return LAYER_T - the layer type, or LAYER_T(-1) if the
     *  index was out of range.
     */
    LAYER_T GetLayerType( LAYER_ID aLayer ) const;

    /**
     * Function SetLayerType
     * changes the type of the layer given by aLayer.
     *
     * @param aLayer A layer index, like B_Cu, etc.
     * @param aLayerType The new layer type.
     * @return bool - true if aLayerType was legal and aLayer was within range, else false.
     */
    bool SetLayerType( LAYER_ID aLayer, LAYER_T aLayerType );

    /**
     * Function SetLayerColor
     * changes a layer color for any valid layer, including non-copper ones.
     */
    void SetLayerColor( LAYER_ID aLayer, EDA_COLOR_T aColor );

    /**
     * Function GetLayerColor
     * gets a layer color for any valid layer, including non-copper ones.
     */
    EDA_COLOR_T GetLayerColor( LAYER_ID aLayer ) const;

    /** Functions to get some items count */
    int GetNumSegmTrack() const;

    /** Calculate the zone segment count */
    int GetNumSegmZone() const;

    /**
     * Function GetNumRatsnests
     * @return int - The number of rats
     */
    unsigned GetRatsnestsCount() const
    {
        return m_FullRatsnest.size();
    }

    /**
     * Function GetNodesCount
     * @return the number of pads members of nets (i.e. with netcode > 0)
     */
    unsigned GetNodesCount() const;

    /**
     * Function SetNodeCount
     * set the number of nodes of the current net to \a aCount.
     *
     * @param aCount is the number of nodes attached to the current net.
     */
    void SetNodeCount( unsigned aCount )   { m_nodeCount = aCount; }

    /**
     * Function GetUnconnectedNetCount
     * @return the number of unconnected nets in the current rats nest.
     */
    unsigned GetUnconnectedNetCount() const { return m_unconnectedNetCount; }

    /**
     * Function SetUnconnectedNetCount
     * sets the number of unconnected nets in the current rats nest to \a aCount.
     *
     * @param aCount is the number of unconneceted nets in the current rats nest.
     */
    void SetUnconnectedNetCount( unsigned aCount ) { m_unconnectedNetCount = aCount; }

    /**
     * Function GetPadCount
     * @return the number of pads in board
     */
    unsigned GetPadCount() const
    {
        return m_NetInfo.GetPadCount();
    }

    /**
     * Function GetPad
     * @return D_PAD* - at the \a aIndex from m_NetInfo
     */
    D_PAD* GetPad( unsigned aIndex ) const
    {
        return m_NetInfo.GetPad( aIndex );
    }

    /**
     * Function GetPads
     * returns a list of all the pads by value.  The returned list is not
     * sorted and contains pointers to PADS, but those pointers do not convey
     * ownership of the respective PADs.
     * @return std::vector<D_PAD*> - a full list of pads
     */
    std::vector<D_PAD*> GetPads()
    {
        return m_NetInfo.m_PadsFullList;
    }

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

    /**
     * Function AppendNet
     * adds a new net description item to the current board.
     * @param aNewNet is the new description item.
     */
    void AppendNet( NETINFO_ITEM* aNewNet )
    {
        m_NetInfo.AppendNet( aNewNet );
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
     * @see PCB_BASE_FRAME::GetBoardBoundingBox() which calls this and doctors the result
     */
    EDA_RECT ComputeBoundingBox( bool aBoardEdgesOnly = false );

    /**
     * Function GetBoundingBox
     * may be called soon after ComputeBoundingBox() to return the same EDA_RECT,
     * as long as the BOARD has not changed.  Remember, ComputeBoundingBox()'s
     * aBoardEdgesOnly argument is considered in this return value also.
     */
    const EDA_RECT GetBoundingBox() const { return m_BoundingBox; }   // override

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

    void GetMsgPanelInfo( std::vector< MSG_PANEL_ITEM >& aList );

    /**
     * Function Draw.
     * Redraw the BOARD items but not cursors, axis or grid.
     * @param aPanel = the panel relative to the board
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ... (not always used)
     * @param aOffset = an draw offset value (default = 0,0)
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               GR_DRAWMODE aDrawMode, const wxPoint& aOffset = ZeroOffset );

    /**
     * Function DrawHighLight
     * redraws the objects in the board that are associated with the given aNetCode
     * and turns on or off the brilliance associated with that net according to the
     * current value of global g_HighLight_Status
     * @param aDrawPanel is needed for the clipping support.
     * @param aDC = the current device context
     * @param aNetCode is the net number to highlight or to dim.
     */
    void DrawHighLight( EDA_DRAW_PANEL* aDrawPanel, wxDC* aDC, int aNetCode );

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
    SEARCH_RESULT Visit( INSPECTOR* inspector, const void* testData,
                         const KICAD_T scanTypes[] );

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
     * Function ReplaceNetlist
     * updates the #BOARD according to \a aNetlist.
     *
     * The changes are made to the board are as follows they are not disabled in the status
     * settings in the #NETLIST:
     * - If a new component is found in the #NETLIST and not in the #BOARD, it is added
     *   to the #BOARD.
     * - If a the component in the #NETLIST is already on the #BOARD, then one or more of the
     *   following actions can occur:
     *   + If the footprint name in the #NETLIST does not match the footprint name on the
     *     #BOARD, the footprint on the #BOARD is replaced with the footprint specified in
     *     the #NETLIST and the proper parameters are copied from the existing footprint.
     *   + If the reference designator in the #NETLIST does not match the reference designator
     *     on the #BOARD, the reference designator is updated from the #NETLIST.
     *   + If the value field in the #NETLIST does not match the value field on the #BOARD,
     *     the value field is updated from the #NETLIST.
     *   + If the time stamp in the #NETLIST does not match the time stamp  on the #BOARD,
     *     the time stamp is updated from the #NETLIST.
     * - After each footprint is added or update as described above, each footprint pad net
     *   name is compared and updated to the value defined in the #NETLIST.
     * - After all of the footprints have been added, updated, and net names properly set,
     *   any extra unlock footprints are removed from the #BOARD.
     *
     * @param aNetlist is the new netlist to revise the contents of the #BOARD with.
     * @param aDeleteSinglePadNets if true, remove nets counting only one pad
     *                             and set net code to 0 for these pads
     * @param aReporter is a #REPORTER object to report the changes \a aNetlist makes to
     *                  the #BOARD.  If NULL, no change reporting occurs.
     */
    void ReplaceNetlist( NETLIST& aNetlist, bool aDeleteSinglePadNets,
                         REPORTER* aReporter = NULL );

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
     * Must be called after a Design Rules edition, or after reading a netlist (or editing
     * the list of nets)  Also this function removes the non existing nets in netclasses
     * and add net nets in default netclass (this happens after reading a netlist)
     */
    void SynchronizeNetsAndNetClasses();

    /***************************************************************************/

    wxString GetClass() const
    {
        return wxT( "BOARD" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const { ShowDummy( os ); } // override
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
                                             LAYER_ID      aStartLayer,
                                             LAYER_ID      aEndLayer,
                                             int aNetCode );

    /**
     * Function RedrawAreasOutlines
     * Redraw all areas outlines on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawAreasOutlines( EDA_DRAW_PANEL* aPanel,
                              wxDC*           aDC,
                              GR_DRAWMODE     aDrawMode,
                              LAYER_ID       aLayer );

    /**
     * Function RedrawFilledAreas
     * Redraw all filled areas on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawFilledAreas( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                            LAYER_ID aLayer );

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
                             LAYER_ID aLayer, wxPoint aStartPointPosition, int aHatch );

    /**
     * Function InsertArea
     * add empty copper area to net, inserting after m_ZoneDescriptorList[iarea]
     * @return pointer to the new area
     */
    ZONE_CONTAINER* InsertArea( int netcode, int iarea, LAYER_ID layer, int x, int y, int hatch );

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
     * Function Test_Drc_Areas_Outlines_To_Areas_Outlines
     * tests area outlines for DRC:
     *      Tests areas inside other areas.
     *      Tests areas too close.
     *
     * @param aArea_To_Examine: area to compare with other areas, or if NULL then
     *          all areas are compared to all others.
     * @param aCreate_Markers: if true create DRC markers. False: do not creates anything
     * @return errors count
     */
    int Test_Drc_Areas_Outlines_To_Areas_Outlines( ZONE_CONTAINER* aArea_To_Examine,
                                                   bool            aCreate_Markers );

    /****** function relative to ratsnest calculations: */

    /**
     * Function Test_Connection_To_Copper_Areas
     * init .m_ZoneSubnet parameter in tracks and pads according to the connections to areas found
     * @param aNetcode = netcode to analyze. if -1, analyze all nets
     */
    void Test_Connections_To_Copper_Areas( int aNetcode = -1 );

    /**
     * Function GetViaByPosition
     * finds the first via at \a aPosition on \a aLayer.
     * <p>
     * This function does not use the normal hit test to locate a via which which tests
     * if a position is within the via's bounding box.  It tests for the actual locate
     * of the via.
     * </p>
     * @param aPosition The wxPoint to HitTest() against.
     * @param aLayer The layer to search.  Use -1 for a don't care.
     * @return VIA* A point a to the VIA object if found, else NULL.
     */
    VIA* GetViaByPosition( const wxPoint& aPosition, LAYER_ID aLayer = UNDEFINED_LAYER ) const;

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
     * Function GetTrack
     * find the segment of \a aTrace at \a aPosition on \a aLayer if \a Layer is visible.
     * Traces that are flagged as deleted or busy are ignored.
     *
     * @param aTrace A pointer to the TRACK object to search.
     * @param aPosition A wxPoint object containing the position to test.
     * @param aLayerMask A layer or layers to mask the hit test.  Use -1 to ignore
     *                   layer mask.
     * @return A TRACK object pointer if found otherwise NULL.
     */
    TRACK* GetTrack( TRACK* aTrace, const wxPoint& aPosition, LSET aLayerMask ) const;

    /**
     * Function MarkTrace
     * marks a chain of trace segments, connected to \a aTrace.
     * <p>
     * Each segment is marked by setting the BUSY bit into m_Flags.  Electrical
     * continuity is detected by walking each segment, and finally the segments
     * are rearranged into a contiguous chain within the given list.
     * </p>
     *
     * @param aTrace The segment within a list of trace segments to test.
     * @param aCount A pointer to an integer where to return the number of
     *               marked segments.
     * @param aTraceLength A pointer to an double where to return the length of the
     *                     trace.
     * @param aInPackageLength A pointer to an double where to return the extra lengths inside
     *                   integrated circuits from the pads connected to this track to the
     *                   die (if any).
     * @param aReorder true for reorder the interesting segments (useful for
     *                 track edition/deletion) in this case the flag BUSY is
     *                 set (the user is responsible of flag clearing). False
     *                 for no reorder : useful when we want just calculate the
     *                 track length in this case, flags are reset
     * @return TRACK* The first in the chain of interesting segments.
     */
    TRACK* MarkTrace( TRACK* aTrace, int* aCount, double* aTraceLength,
                      double* aInPackageLength, bool aReorder );

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
    MODULE* GetFootprint( const wxPoint& aPosition, LAYER_ID aActiveLayer,
                          bool aVisibleOnly, bool aIgnoreLocked = false );

    /**
     * Function GetLockPoint
     * returns the item at the "attachment" point at the end of a trace at \a aPosition
     * on \a aLayerMask.
     * <p>
     * This may be a PAD or another trace segment.
     * </p>
     *
     * @param aPosition A wxPoint object containing the position to test.
     * @param aLayerMask A layer or layers to mask the hit test.  Use -1 to ignore
     *                   layer mask.
     * @return A pointer to a BOARD_ITEM object if found otherwise NULL.
     */
    BOARD_CONNECTED_ITEM* GetLockPoint( const wxPoint& aPosition, LSET aLayerMask );

    /**
     * Function CreateLockPoint
     * creates an intermediate point on \a aSegment and break it into two segments
     * at \a aPosition.
     * <p>
     * The new segment starts from \a aPosition and ends at the end point of \a
     * aSegment.  The original segment now ends at \a aPosition.
     * </p>
     *
     * @param aPosition A wxPoint object containing the position to test and the new
     *                  segment start position if the return value is not NULL.
     * @param aSegment The trace segment to create the lock point on.
     * @param aList The pick list to add the created items to.
     * @return NULL if no new point was created or a pointer to a TRACK ojbect of the
     *         created segment.  If \a aSegment points to a via the exact value of \a
     *         aPosition and a pointer to the via are returned.
     */
    TRACK* CreateLockPoint( wxPoint& aPosition, TRACK* aSegment, PICKED_ITEMS_LIST* aList );
};

#endif      // CLASS_BOARD_H_
