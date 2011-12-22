/**
 * @file class_board.h
 * @brief Class BOARD to handle a board.
 */

#ifndef CLASS_BOARD_H
#define CLASS_BOARD_H


#include "dlist.h"

#include "layers_id_colors_and_visibility.h"
#include "class_netinfo.h"
#include "class_pad.h"
#include "class_colors_design_settings.h"
#include "class_board_design_settings.h"
#include "common.h"                         // PAGE_INFO

class PCB_BASE_FRAME;
class PCB_EDIT_FRAME;
class PICKED_ITEMS_LIST;
class BOARD;
class ZONE_CONTAINER;
class SEGZONE;
class TRACK;
class D_PAD;
class MARKER_PCB;


// non-owning container of item candidates when searching for items on the same track.
typedef std::vector< TRACK* > TRACK_PTRS;


#define HISTORY_MAX_COUNT 8


/**
 * Enum LAYER_T
 * gives the allowed types of layers, same as Specctra DSN spec.
 */
enum LAYER_T
{
    LT_SIGNAL,
    LT_POWER,
    LT_MIXED,
    LT_JUMPER
};


/**
 * Struct LAYER
 * holds information pertinent to a layer of a BOARD.
 */
struct LAYER
{
    /** The name of the layer, there should be no spaces in this name. */
    wxString m_Name;

    /** The type of the layer */
    LAYER_T m_Type;

//    int         m_Color;
//    bool        m_Visible;      // ? use flags in m_Color instead ?

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


/**
 * Struct VIA_DIMENSION
 * is a small helper container to handle a stock of specific vias each with
 * unique diameter and drill sizes in the BOARD class.
 */
struct VIA_DIMENSION
{
    int m_Diameter;     // <= 0 means use Netclass via diameter
    int m_Drill;        // <= 0 means use Netclass via drill

    VIA_DIMENSION()
    {
        m_Diameter = 0;
        m_Drill    = 0;
    }

    VIA_DIMENSION( int aDiameter, int aDrill )
    {
        m_Diameter = aDiameter;
        m_Drill    = aDrill;
    }

    bool operator == ( const VIA_DIMENSION& other ) const
    {
        return (m_Diameter == other.m_Diameter) && (m_Drill == other.m_Drill);
    }

    bool operator < ( const VIA_DIMENSION& other ) const
    {
        if( m_Diameter != other.m_Diameter )
            return m_Diameter < other.m_Diameter;

        return m_Drill < other.m_Drill;
    }
};


// Helper class to handle high light nets
class HIGH_LIGHT_INFO
{
    friend class BOARD;
protected:
    int m_netCode;           // net selected for highlight (-1 when no net selected )
    bool m_highLightOn;     // highlight active

protected:
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
    // @todo: switch to boost:ptr_vector, and change ~BOARD()
    typedef std::vector<MARKER_PCB*> MARKERS;

    /// MARKER_PCBs for clearance problems, owned by pointer.
    MARKERS             m_markers;

    // @todo: switch to boost::ptr_vector, and change ~BOARD()
    typedef std::vector<ZONE_CONTAINER*> ZONE_CONTAINERS;

    /// edge zone descriptors, owned by pointer.
    ZONE_CONTAINERS     m_ZoneDescriptorList;

    LAYER               m_Layer[NB_COPPER_LAYERS];
                                                        // if true m_hightLight_NetCode is used
    HIGH_LIGHT_INFO     m_hightLight;                   // current high light data
    HIGH_LIGHT_INFO     m_hightLightPrevious;           // a previously stored high light data

    int                 m_fileFormatVersionAtLoad;      ///< the version in the *.brd header on first line

    EDA_RECT            m_BoundingBox;

    NETINFO_LIST        m_NetInfo;                      ///< net info list (name, design constraints ..

    BOARD_DESIGN_SETTINGS   m_designSettings;
    COLORS_DESIGN_SETTINGS* m_colorsSettings;       // Link to current colors settings
    PAGE_INFO               m_paper;

    /**
     * Function chainMarkedSegments
     * is used by MarkTrace() to set the BUSY flag of connected segments of the trace
     * segment located at \a aPosition on aLayerMask.
     *  Vias are put in list but their flags BUSY is not set
     * @param aPosition A wxPoint object containing the position of the starting search.
     * @param aLayerMask The allowed layers for segments to search.
     * @param aList The track list to fill with points of flagged segments.
     */
    void chainMarkedSegments( wxPoint aPosition, int aLayerMask, TRACK_PTRS* aList );


public:

    /// Flags used in ratsnest calculation and update.
    int m_Status_Pcb;

    /// Active pads (pads attached to a net ) count.
    int m_NbNodes;

    /// Active ratsnest count (ratsnests not already connected by tracks)
    int m_NbNoconnect;

    DLIST<BOARD_ITEM>           m_Drawings;              // linked list of lines & texts
    DLIST<MODULE>               m_Modules;               // linked list of MODULEs
    DLIST<TRACK>                m_Track;                 // linked list of TRACKs and SEGVIAs
    DLIST<SEGZONE>              m_Zone;                  // linked list of SEGZONEs

    /// Ratsnest list for the BOARD
    std::vector<RATSNEST_ITEM>  m_FullRatsnest;

    /// Ratsnest list relative to a given footprint (used while moving a footprint).
    std::vector<RATSNEST_ITEM>  m_LocalRatsnest;

    /// zone contour currently in progress
    ZONE_CONTAINER*             m_CurrentZoneContour;

    /// List of current netclasses. There is always the default netclass.
    NETCLASSES                  m_NetClasses;

    /// Current net class name used to display netclass info.
    /// This is also the last used netclass after starting a track.
    wxString                    m_CurrentNetClassName;

    // handling of vias and tracks size:
    // the first value is always the value of the current NetClass
    // The others values are extra values

    /// Vias size and drill list(max count = HISTORY_MAX_COUNT)
    std::vector <VIA_DIMENSION> m_ViasDimensionsList;

    // The first value is the current netclass via size
    // tracks widths (max count = HISTORY_MAX_COUNT)
    // The first value is the current netclass track width
    std::vector <int>           m_TrackWidthList;

    /// Index for m_ViaSizeList to select the value.
    /// 0 is the index selection of the default value Netclass
    unsigned m_ViaSizeSelector;

    // Index for m_TrackWidthList to select the value.
    unsigned m_TrackWidthSelector;


    BOARD();
    ~BOARD();

    void SetFileFormatVersionAtLoad( int aVersion ) { m_fileFormatVersionAtLoad = aVersion; }
    int GetFileFormatVersionAtLoad()  const { return m_fileFormatVersionAtLoad; }


    /**
     * Function GetDefaultLayerName
     * returns a default name of a PCB layer when given \a aLayerNumber.  This
     * function is static so it can be called without a BOARD instance.  Use
     * GetLayerName() if want the layer names of a specific BOARD, which could
     * be different than the default if the user has renamed any copper layers.
     *
     * @param  aLayerNumber is the layer number to fetch
     * @return wxString - containing the layer name or "BAD INDEX" if aLayerNumber
     *                      is not legal
     */
    static wxString GetDefaultLayerName( int aLayerNumber );

    const wxPoint GetPosition() const       // overload
    {
        return wxPoint( 0, 0 );     // dummy for pure virtual
    }
    void SetPosition( const wxPoint& aPos ) {}  // overload

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
     * Function ResetHighLight
     * Reset all high light data to the init state
     */
    void ResetHighLight()
    {
        m_hightLight.Clear();
        m_hightLightPrevious.Clear();
    }

    /**
     * Function GetHighLightNetCode
     * @return netcode of net to highlight (-1 when no net selected)
     */
    int GetHighLightNetCode() { return m_hightLight.m_netCode; }

    /**
     * Function SetHighLightNet
     * @param aNetCode = netcode of net to highlight
     */
    void SetHighLightNet( int aNetCode)
    {
        m_hightLight.m_netCode = aNetCode;
    }

    /**
     * Function IsHighLightNetON
     * @return true if a net is currently highlighted
     */
    bool IsHighLightNetON() { return m_hightLight.m_highLightOn; }

    /**
     * Function HighLightOFF
     * Disable highlight.
     */
    void HighLightOFF() { m_hightLight.m_highLightOn = false; }

    /**
     * Function HighLightON
     * Enable highlight.
     * if m_hightLight_NetCode >= 0, this net will be highlighted
     */
    void HighLightON() { m_hightLight.m_highLightOn = true; }

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
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * Returns a bit-mask of all the layers that are enabled
     * @return int - the enabled layers in bit-mapped form.
     */
    int  GetEnabledLayers() const;

    /**
     * Function SetEnabledLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * Changes the bit-mask of enabled layers
     * @param aLayerMask = The new bit-mask of enabled layers
     */
    void SetEnabledLayers( int aLayerMask );

    /**
     * Function IsLayerEnabled
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * tests whether a given layer is enabled
     * @param aLayer = The layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool IsLayerEnabled( int aLayer ) const
    {
        return m_designSettings.IsLayerEnabled( aLayer );
    }

    /**
     * Function IsLayerVisible
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * tests whether a given layer is visible
     * @param aLayerIndex = The index of the layer to be tested
     * @return bool - true if the layer is visible.
     */
    bool IsLayerVisible( int aLayerIndex ) const
    {
        return m_designSettings.IsLayerVisible( aLayerIndex );
    }

    /**
     * Function GetVisibleLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * Returns a bit-mask of all the layers that are visible
     * @return int - the visible layers in bit-mapped form.
     */
    int  GetVisibleLayers() const;

    /**
     * Function SetVisibleLayers
     * is a proxy function that calls the correspondent function in m_BoardSettings
     * changes the bit-mask of visible layers
     * @param aLayerMask = The new bit-mask of visible layers
     */
    void SetVisibleLayers( int aLayerMask );

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
     * @param layer One of the two allowed layers for modules: LAYER_N_FRONT or LAYER_N_BACK
     * @return bool - true if the layer is visible, else false.
     */
    bool IsModuleLayerVisible( int layer );

    /**
     * Function GetVisibleElementColor
     * returns the color of a pcb visible element.
     * @see enum PCB_VISIBLE
     */
    int GetVisibleElementColor( int aPCB_VISIBLE );

    void SetVisibleElementColor( int aPCB_VISIBLE, int aColor );

    /**
     * Function GetDesignSettings
     * @return the BOARD_DESIGN_SETTINGS for this BOARD
     */
    // const BOARD_DESIGN_SETTINGS& GetDesignSettings() const  want to use this one
    BOARD_DESIGN_SETTINGS& GetDesignSettings()
    {
        return m_designSettings;
    }

    /**
     * Function SetDesignSettings
     * @param aDesignSettings the new BOARD_DESIGN_SETTINGS to use
     */
    void SetDesignSettings( const BOARD_DESIGN_SETTINGS& aDesignSettings );

    const PAGE_INFO& GetPageSettings() const                { return m_paper; }
    void SetPageSettings( const PAGE_INFO& aPageSettings )  { m_paper = aPageSettings; }

    /**
     * Function SetBoardSettings
     * @return the current COLORS_DESIGN_SETTINGS in use
     */
    COLORS_DESIGN_SETTINGS* GetColorsSettings() const { return m_colorsSettings; }

    /**
     * Function SetColorsSettings
     * @param aColorsSettings = the new COLORS_DESIGN_SETTINGS to use
     */
    void SetColorsSettings(COLORS_DESIGN_SETTINGS* aColorsSettings)
    {
        m_colorsSettings = aColorsSettings;
    }

    /**
     * Function GetLayerName
     * returns the name of the layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like LAYER_N_BACK, etc.
     * @return wxString - the layer name.
     */
    wxString GetLayerName( int aLayerIndex ) const;

    /**
     * Function SetLayerName
     * changes the name of the layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like LAYER_N_BACK, etc.
     * @param aLayerName The new layer name
     * @return bool - true if aLayerName was legal and unique among other
     *   layer names at other layer indices and aLayerIndex was within range, else false.
     */
    bool SetLayerName( int aLayerIndex, const wxString& aLayerName );

    /**
     * Function GetLayerType
     * returns the type of the copper layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like LAYER_N_BACK, etc.
     * @return LAYER_T - the layer type, or LAYER_T(-1) if the
     *  index was out of range.
     */
    LAYER_T GetLayerType( int aLayerIndex ) const;

    /**
     * Function SetLayerType
     * changes the type of the layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like LAYER_N_BACK, etc.
     * @param aLayerType The new layer type.
     * @return bool - true if aLayerType was legal and aLayerIndex was within range, else false.
     */
    bool SetLayerType( int aLayerIndex, LAYER_T aLayerType );

    /**
     * Function SetLayerColor
     * changes a layer color for any valid layer, including non-copper ones.
     */
    void SetLayerColor( int aLayer, int aColor );

    /**
     * Function GetLayerColor
     * gets a layer color for any valid layer, including non-copper ones.
     */
    int GetLayerColor( int aLayer );

    /** Functions to get some items count */
    int GetNumSegmTrack() const;

    /** Calculate the zone segment count */
    int GetNumSegmZone() const;

    unsigned GetNoconnectCount() const;    // Return the number of missing links.

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

    void AppendNet( NETINFO_ITEM* aNewNet )
    {
        m_NetInfo.AppendNet( aNewNet );
    }

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
    EDA_RECT GetBoundingBox() const { return m_BoundingBox; }   // override

    void SetBoundingBox( const EDA_RECT& aBox ) { m_BoundingBox = aBox; }

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function Draw.
     * Redraw the BOARD items but not cursors, axis or grid.
     * @param aPanel = the panel relative to the board
     * @param aDC = the current device context
     * @param aDrawMode = GR_COPY, GR_OR ... (not always used)
     * @param aOffset = an draw offset value (default = 0,0)
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               int aDrawMode, const wxPoint& aOffset = ZeroOffset );

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
     * Function ReturnSortedNetnamesList
     * @param aNames An array string to fill with net names.
     * @param aSortbyPadsCount  true = sort by active pads count, false = no sort (i.e.
     *                          leave the sort by net names)
     * @return int - net names count.
     */
    int ReturnSortedNetnamesList( wxArrayString& aNames, bool aSortbyPadsCount );

    /**************************************/
    /**
    * Function relative to NetClasses: **/
    /**************************************/

    /**
     * Function SynchronizeNetsAndNetClasses
     * copies NETCLASS info to each NET, based on NET membership in a NETCLASS.
     * Must be called after a Design Rules edition, or after reading a netlist (or editing
     * the list of nets)  Also this function removes the non existing nets in netclasses
     * and add net nets in default netclass (this happens after reading a netlist)
     */
    void SynchronizeNetsAndNetClasses();

    /**
     * Function SetCurrentNetClass
     * Must be called after a netclass selection (or after a netclass parameter change
     * Initialize vias and tracks values displayed in comb boxes of the auxiliary toolbar
     * and some others parameters (netclass name ....)
     * @param aNetClassName = the new netclass name
     * @return true if lists of tracks and vias sizes are modified
     */
    bool SetCurrentNetClass( const wxString& aNetClassName );

    /**
     * Function GetBiggestClearanceValue
     * @return the biggest clearance value found in NetClasses list
     */
    int GetBiggestClearanceValue();

    /**
     * Function GetCurrentTrackWidth
     * @return the current track width, according to the selected options
     * ( using the default netclass value or a preset value )
     * the default netclass is always in m_TrackWidthList[0]
     */
    int GetCurrentTrackWidth()
    {
        return m_TrackWidthList[m_TrackWidthSelector];
    }

    /**
     * Function GetCurrentViaSize
     * @return the current via size, according to the selected options
     * ( using the default netclass value or a preset value )
     * the default netclass is always in m_TrackWidthList[0]
     */
    int GetCurrentViaSize()
    {
        return m_ViasDimensionsList[m_ViaSizeSelector].m_Diameter;
    }

    /**
     * Function GetCurrentViaDrill
     * @return the current via size, according to the selected options
     * ( using the default netclass value or a preset value )
     * the default netclass is always in m_TrackWidthList[0]
     */
    int GetCurrentViaDrill()
    {
        return m_ViasDimensionsList[m_ViaSizeSelector].m_Drill > 0 ?
               m_ViasDimensionsList[m_ViaSizeSelector].m_Drill : -1;
    }

    /**
     * Function GetCurrentMicroViaSize
     * @return the current micro via size,
     * that is the current netclass value
     */
    int  GetCurrentMicroViaSize();

    /**
     * Function GetCurrentMicroViaDrill
     * @return the current micro via drill,
     * that is the current netclass value
     */
    int  GetCurrentMicroViaDrill();

    /***************************************************************************/

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "BOARD" );
    }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;     // overload
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
     * @param aEndLayer the last layer (-1 to ignore it) to test
     * @return ZONE_CONTAINER* return a pointer to the ZONE_CONTAINER found, else NULL
     */
    ZONE_CONTAINER* HitTestForAnyFilledArea( const wxPoint& aRefPos,
                                             int            aStartLayer,
                                             int            aEndLayer = -1 );

    /**
     * Function RedrawAreasOutlines
     * Redraw all areas outlines on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawAreasOutlines( EDA_DRAW_PANEL* aPanel,
                              wxDC*           aDC,
                              int             aDrawMode,
                              int             aLayer );

    /**
     * Function RedrawFilledAreas
     * Redraw all filled areas on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawFilledAreas( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDrawMode, int aLayer );

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
     * @return pointer to the new area
     */
    ZONE_CONTAINER* AddArea( PICKED_ITEMS_LIST* aNewZonesList, int aNetcode,
                             int aLayer, wxPoint aStartPointPosition, int aHatch );

    /**
     * Function InsertArea
     * add empty copper area to net, inserting after m_ZoneDescriptorList[iarea]
     * @return pointer to the new area
     */
    ZONE_CONTAINER* InsertArea( int netcode, int iarea, int layer, int x, int y, int hatch );

    /**
     *  Function CompleteArea
     * complete copper area contour by adding a line from last to first corner
     * if there is only 1 or 2 corners, remove (delete) the area
     * @param area_to_complete = area to complete or remove
     * @param style = style of last corner
     * @return 1 if Ok, 0 if area removed
     */
    int CompleteArea( ZONE_CONTAINER* area_to_complete, int style );

    /**
     * Function TestAreaPolygon
     * Test an area for self-intersection.
     *
     * @param CurrArea = copper area to test
     * @return :
     * -1 if arcs intersect other sides
     *  0 if no intersecting sides
     *  1 if intersecting sides, but no intersecting arcs
     * Also sets utility2 flag of area with return value
     */
    int TestAreaPolygon( ZONE_CONTAINER* CurrArea );

    /**
     * Function ClipAreaPolygon
     * Process an area that has been modified, by clipping its polygon against itself.
     * This may change the number and order of copper areas in the net.
     * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new areas pickers (useful
     *                        in undo commands) can be NULL
     * @param aCurrArea = the zone to process
     * @param bMessageBoxInt == true, shows message when clipping occurs.
     * @param  bMessageBoxArc == true, shows message when clipping can't be done due to arcs.
     * @param bRetainArcs = true to handle arcs (not really used in KiCad)
     * @return :
     *  -1 if arcs intersect other sides, so polygon can't be clipped
     *   0 if no intersecting sides
     *   1 if intersecting sides
     * Also sets areas->utility1 flags if areas are modified
     */
    int ClipAreaPolygon( PICKED_ITEMS_LIST* aNewZonesList,
                         ZONE_CONTAINER*    aCurrArea,
                         bool               bMessageBoxArc,
                         bool               bMessageBoxInt,
                         bool               bRetainArcs = true );

    /**
     * Process an area that has been modified, by clipping its polygon against
     * itself and the polygons for any other areas on the same net.
     * This may change the number and order of copper areas in the net.
     * @param aModifiedZonesList = a PICKED_ITEMS_LIST * where to store deleted or added areas
     *                      (useful in undo commands. Can be NULL
     * @param modified_area = area to test
     * @param bMessageBoxInt : if true, shows message boxes when clipping occurs.
     * @param bMessageBoxArc if true, shows message when clipping can't be done due to arcs.
     * @return :
     * -1 if arcs intersect other sides, so polygon can't be clipped
     *  0 if no intersecting sides
     *  1 if intersecting sides, polygon clipped
     */
    int AreaPolygonModified( PICKED_ITEMS_LIST* aModifiedZonesList,
                             ZONE_CONTAINER*    modified_area,
                             bool               bMessageBoxArc,
                             bool               bMessageBoxInt );

    /**
     * Function CombineAllAreasInNet
     * Checks all copper areas in net for intersections, combining them if found
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful
     *                       in undo commands can be NULL
     * @param aNetCode = net to consider
     * @param bMessageBox : if true display warning message box
     * @param bUseUtility : if true, don't check areas if both utility flags are 0
     * Sets utility flag = 1 for any areas modified
     * If an area has self-intersecting arcs, doesn't try to combine it
     */
    int CombineAllAreasInNet( PICKED_ITEMS_LIST* aDeletedList,
                              int                aNetCode,
                              bool               bMessageBox,
                              bool               bUseUtility );

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
     * @return : 0 if no intersection
     *         1 if intersection
     *         2 if arcs intersect
     */
    int TestAreaIntersection( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_test );

    /**
     * Function CombineAreas
     * If possible, combine 2 copper areas
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful
     *                       in undo commands can be NULL
     * @param area_ref = the main area (zone)
     * @param area_to_combine = the zone that can be merged with area_ref
     * area_ref must be BEFORE area_to_combine
     * area_to_combine will be deleted, if areas are combined
     * @return : 0 if no intersection
     *         1 if intersection
     *         2 if arcs intersect
     */
    int CombineAreas( PICKED_ITEMS_LIST* aDeletedList,
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
     * @param aLayerMask The layers to search.  Use -1 for a don't care.
     * @return TRACK* A point a to the SEGVIA object if found, else NULL.
     */
    TRACK* GetViaByPosition( const wxPoint& aPosition, int aLayerMask = -1 );

    /**
     * Function GetPad
     * finds a pad \a aPosition on \a aLayer.
     *
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayerMask A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found or NULL if not found.
     */
    D_PAD* GetPad( const wxPoint& aPosition, int aLayerMask = ALL_LAYERS );

    /**
     * Function GetPad
     * finds a pad connected to \a aEndPoint of \a aTrace.
     *
     * @param aTrace A pointer to a TRACK object to hit test against.
     * @param aEndPoint The end point of \a aTrace the hit test against.
     * @return A pointer to a D_PAD object if found or NULL if not found.
     */
    D_PAD* GetPad( TRACK* aTrace, int aEndPoint );

    /**
     * Function GetPadFast
     * return pad found at \a aPosition on \a aLayer uning the fast search method.
     * <p>
     * The fast search method only works if the pad list has already been built.
     * </p>
     * @param aPosition A wxPoint object containing the position to hit test.
     * @param aLayer A layer or layers to mask the hit test.
     * @return A pointer to a D_PAD object if found or NULL if not found.
     */
    D_PAD* GetPadFast( const wxPoint& aPosition, int aLayer );

    /**
     * Function GetPad
     * locates the pad connected at \a aPosition on \a aLayer starting at list postion
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
    D_PAD* GetPad( std::vector<D_PAD*>& aPadList, const wxPoint& aPosition, int aLayerMask );

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
     * Function GetTrace
     * find the segment of \a aTrace at \a aPosition on \a aLayer if \a Layer is visible.
     * Traces that are flagged as deleted or busy are ignored.
     *
     * @param aTrace A pointer to the TRACK object to search.
     * @param aPosition A wxPoint object containing the position to test.
     * @param aLayerMask A layer or layers to mask the hit test.  Use -1 to ignore
     *                   layer mask.
     * @return A TRACK object pointer if found otherwise NULL.
     */
    TRACK* GetTrace( TRACK* aTrace, const wxPoint& aPosition, int aLayerMask );

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
     * @param aTraceLength A pointer to an integer where to return the length of the
     *                     trace.
     * @param aDieLength A pointer to an integer where to return the extra lengths inside
     *                   integrated circuits from the pads connected to this track to the
     *                   die (if any).
     * @param aReorder true for reorder the interesting segments (useful for
     *                 track edition/deletion) in this case the flag BUSY is
     *                 set (the user is responsible of flag clearing). False
     *                 for no reorder : useful when we want just calculate the
     *                 track length in this case, flags are reset
     * @return TRACK* The first in the chain of interesting segments.
     */
    TRACK* MarkTrace( TRACK* aTrace, int* aCount, int* aTraceLength,
                      int* aDieLength, bool aReorder );

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
    MODULE* GetFootprint( const wxPoint& aPosition, int aActiveLayer,
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
    BOARD_CONNECTED_ITEM* GetLockPoint( const wxPoint& aPosition, int aLayerMask );

    /**
     * Function CreateLockPoint
     * creates an intermediate point on \a aSegment and break it into two segments
     * at \a aPoition.
     * <p>
     * The new segment starts from \a aPosition and ends at the end point of \a
     * aSegement.  The original segment now ends at \a aPosition.
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

#endif      // #ifndef CLASS_BOARD_H
