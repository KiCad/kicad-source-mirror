/*************************************************/
/* class_board.h - Class BOARD to handle a board */
/*************************************************/

#ifndef CLASS_BOARD_H
#define CLASS_BOARD_H

#include "dlist.h"
#include "class_netinfo.h"


class ZONE_CONTAINER;
class EDA_BoardDesignSettings;


/**
 * Enum LAYER_T
 * gives the allowed types of layers, same as Specctra DSN spec.
 */
enum LAYER_T {
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
 * Class BOARD
 * holds information pertinent to a PCBNEW printed circuit board.
 */
#define HISTORY_MAX_COUNT 8
class BOARD : public BOARD_ITEM
{
    friend class WinEDA_PcbFrame;

private:
    typedef std::vector<MARKER_PCB*> MARKERS;               // @todo: switch to boost:ptr_vector, and change ~BOARD()
    MARKERS                  m_markers;                     ///< MARKER_PCBs for clearance problems, owned by pointer

    typedef std::vector<ZONE_CONTAINER*> ZONE_CONTAINERS;   // @todo: switch to boost::ptr_vector, and change ~BOARD()
    ZONE_CONTAINERS          m_ZoneDescriptorList;          ///< edge zone descriptors, owned by pointer

    LAYER                    m_Layer[NB_COPPER_LAYERS];

public:
    WinEDA_BasePcbFrame*     m_PcbFrame;                    // Window de visualisation
    EDA_Rect                 m_BoundaryBox;                 // Board size and position
    int m_Status_Pcb;                                       // Flags used in ratsnet calculation and update
    EDA_BoardDesignSettings* m_BoardSettings;               // Link to current design settings
    int m_NbNodes;                                          // Active pads (pads attached to a net ) count
    int m_NbNoconnect;                                      // Active ratsnet count (rastnests not alraedy connected by tracks)

    DLIST<BOARD_ITEM>          m_Drawings;                  // linked list of lines & texts
    DLIST<MODULE>              m_Modules;                   // linked list of MODULEs
    DLIST<TRACK>               m_Track;                     // linked list of TRACKs and SEGVIAs
    DLIST<SEGZONE>             m_Zone;                      // linked list of SEGZONEs

    NETINFO_LIST*              m_NetInfo;                   // nets info list (name, design constraints ..

    std::vector<RATSNEST_ITEM> m_FullRatsnest;              // Rastnest list for the BOARD
    std::vector<RATSNEST_ITEM> m_LocalRatsnest;             /* Rastnest list relative to a given footprint
                                                             *  (used while moving a footprint) */

    ZONE_CONTAINER* m_CurrentZoneContour;                   // zone contour currently in progress
 
    NETCLASSES m_NetClasses;                                ///< List of current netclasses. There is always the default netclass
    wxString   m_CurrentNetClassName;                       /* Current net class name used to display netclass info.
                                                             *  this is also the last used netclass after starting a track
                                                             */

    
    // handling of vias and tracks size:
    // the first value is always the value of the current NetClass
    // The others values are extra values
    std::vector <int>    m_ViaSizeHistory;          // Last used via sizes (max count = HISTORY_MAX_COUNT)
    std::vector <int>    m_TrackWidthHistory;       // Last used track widths (max count = HISTORY_MAX_COUNT)
    int m_ViaSizeSelector;                          // index for m_ViaSizeHistory to select the value
                                                    // O is the selection of the default value Netclass
    int m_TrackWidthSelector;                       // index for m_TrackWidthHistory to select the value

    /**********************************/
public:
    BOARD( EDA_BaseStruct* aParent, WinEDA_BasePcbFrame* frame );
    ~BOARD();

    /**
     * Function GetPosition
     * is here to satisfy BOARD_ITEM's requirements, but this implementation
     * is a dummy.
     * @return const wxPoint& of (0,0)
     */
    wxPoint& GetPosition();


    /**
     * Function Add
     * adds the given item to this BOARD and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param aControl An int which can vary how the item is added.
     */
    void     Add( BOARD_ITEM* aBoardItem, int aControl = 0 );

#define ADD_APPEND 1        ///< aControl flag for Add( aControl ), appends not inserts


    /**
     * Function Delete
     * removes the given single item from this BOARD and deletes its memory.
     * @param aBoardItem The item to remove from this board and delete
     */
    void    Delete( BOARD_ITEM* aBoardItem )
    {
        wxASSERT( aBoardItem );  // developers should run DEBUG versions and fix such calls with NULL
        if( aBoardItem )
            delete Remove( aBoardItem );
    }


    /**
     * Function Remove
     * removes \a aBoardItem from this BOARD and returns it to caller without
     * deleting it.
     * @param aBoardItem The item to remove from this board.
     * @return BOARD_ITEM* \a aBoardItem which was passed in.
     */
    BOARD_ITEM* Remove( BOARD_ITEM* aBoardItem );


    /**
     * Function DeleteMARKERs
     * deletes ALL MARKERS from the board.
     */
    void        DeleteMARKERs();

    /**
     * Function DeleteZONEOutlines
     * deletes ALL zone outlines from the board.
     */
    void        DeleteZONEOutlines();


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
     * Function GetCopperLayerCount
     * @return int - The number of copper layers in the BOARD.
     */
    int      GetCopperLayerCount() const;

    /**
     * Function GetLayerName
     * returns the name of the copper layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like COPPER_LAYER_N, etc.
     * @return wxString - the layer name.
     */
    wxString GetLayerName( int aLayerIndex ) const;

    /**
     * Function SetLayerName
     * changes the name of the layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like COPPER_LAYER_N, etc.
     * @param aLayerName The new layer name
     * @return bool - true if aLayerName was legal and unique amoung other
     *   layer names at other layer indices and aLayerIndex was within range, else false.
     */
    bool     SetLayerName( int aLayerIndex, const wxString& aLayerName );

    /**
     * Function GetLayerType
     * returns the type of the copper layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like COPPER_LAYER_N, etc.
     * @return LAYER_T - the layer type, or LAYER_T(-1) if the
     *  index was out of range.
     */
    LAYER_T  GetLayerType( int aLayerIndex ) const;

    /**
     * Function SetLayerName
     * changes the name of the layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like COPPER_LAYER_N, etc.
     * @param aLayerType The new layer type.
     * @return bool - true if aLayerType was legal and aLayerIndex was within range, else false.
     */
    bool     SetLayerType( int aLayerIndex, LAYER_T aLayerType );


    /* Routines de calcul des nombres de segments pistes et zones */
    int      GetNumSegmTrack();
    int      GetNumSegmZone();
    unsigned GetNoconnectCount();        // retourne le nombre de connexions manquantes

    /**
     * Function GetNumRatsnests
     * @return int - The number of rats
     */
    unsigned     GetRatsnestsCount()
    {
        return m_FullRatsnest.size();
    }


    /** Function GetNodesCount
     * @return the number of pads members of nets (i.e. with netcode > 0)
     */
    unsigned GetNodesCount();

    /** Function GetPadsCount
     * @return the number of pads in board
     */
    unsigned     GetPadsCount()
    {
        return m_NetInfo->GetPadsCount();
    }


    // Calcul du rectangle d'encadrement:
    bool          ComputeBoundaryBox();


    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void          DisplayInfo( WinEDA_DrawFrame* frame );

    void          Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                        int aDrawMode, const wxPoint& offset = ZeroOffset );

    /**
     * Function DrawHighLight
     * redraws the objects in the board that are associated with the given aNetCode
     * and turns on or off the brilliance associated with that net according to the
     * current value of global g_HightLigt_Status
     * @param aDrawPanel is needed for the clipping support.
     * @param aNetCode is the net number to highlight or to dim.
     */
    void          DrawHighLight( WinEDA_DrawPanel* aDrawPanel, wxDC* DC, int aNetCode );

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
     * Function FindNet
     * searches for a net with the given netcode.
     * @param aNetcode A netcode to search for.
     * @return NETINFO_ITEM_ITEM* - the net or NULL if not found.
     */
    NETINFO_ITEM* FindNet( int aNetcode ) const;

    /**
     * Function FindNet overlayed
     * searches for a net with the given name.
     * @param aNetname A Netname to search for.
     * @return NETINFO_ITEM* - the net or NULL if not found.
     */
    NETINFO_ITEM* FindNet( const wxString& aNetname ) const;

    /**
     * Function FindModuleByReference
     * searches for a MODULE within this board with the given
     * reference designator.  Finds only the first one, if there
     * is more than one such MODULE.
     * @param aReference The reference designator of the MODULE to find.
     * @return MODULE* - If found, the MODULE having the given reference
     *  designator, else NULL.
     */
    MODULE*       FindModuleByReference( const wxString& aReference ) const;

    /**
     * Function ReturnSortedNetnamesList
     * @param aNames An array string to fill with net names.
     * @param aSortbyPadsCount : true = sort by active pads count, false = no sort (i.e. leave the sort by net names)
     * @return int - net names count.
     */
    int           ReturnSortedNetnamesList( wxArrayString& aNames, bool aSortbyPadsCount );

    /**
     * Function SynchronizeNetsAndNetClasses
     * copies NETCLASS info to each NET, based on NET membership in a NETCLASS.
     * Must be called after a Design Rules edition, or after reading a netlist (or editing the list of nets)
     * Also this function removes the non existing nets in netclasses and add net nets in default netclass
     * (this happens after reading a netlist)
     * @param none
     * @return none
     */
    void          SynchronizeNetsAndNetClasses();

    /**
     * Function SetCurrentNetClass
     * Must be called after a netclass selection (or after a netclass parameter change
     * Initialise vias and tracks values displayed in comb boxs of the auxiliary toolbar
     * and some others parametres (netclass name ....)
     */
     void SetCurrentNetClass( const wxString & aNetClassName);

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool          Save( FILE* aFile ) const;


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

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif


    /*************************/
    /* Copper Areas handling */
    /*************************/

    /**
     * Function HitTestForAnyFilledArea
     * tests if the given wxPoint is within the bounds of a filled area of this zone.
     * the test is made on zones on layer from aStartLayer to aEndLayer
     * Note: if a zone has its flag BUSY (in .m_State) is set, it is ignored.
     * @param refPos A wxPoint to test
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
    void            RedrawAreasOutlines( WinEDA_DrawPanel* panel,
                                         wxDC*             aDC,
                                         int               aDrawMode,
                                         int               aLayer );

    /**
     * Function RedrawFilledAreas
     * Redraw all filled areas on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawFilledAreas( WinEDA_DrawPanel* panel,
                            wxDC*             aDC,
                            int               aDrawMode,
                            int               aLayer );

    /**
     * Function SetAreasNetCodesFromNetNames
     * Set the .m_NetCode member of all copper areas, according to the area Net Name
     * The SetNetCodesFromNetNames is an equivalent to net name, for fas comparisons.
     * However the Netcode is an arbitrary equyivalence, it must be set after each netlist read
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

    /** Function AddArea
     * Add an empty copper area to board areas list
     * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new areas  pickers (useful in undo commands)
     *                      can be NULL
     * @param aNetcode = the necode of the copper area (0 = no net)
     * @param aLayer = the layer of area
     * @param aStartPointPosition = position of the first point of the polygon outline of this area
     * @param aHatch = hacth option
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
    int             CompleteArea( ZONE_CONTAINER* area_to_complete, int style );

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
    int             TestAreaPolygon( ZONE_CONTAINER* CurrArea );

    /**
     * Function ClipAreaPolygon
     * Process an area that has been modified, by clipping its polygon against itself.
     * This may change the number and order of copper areas in the net.
     * @param aNewZonesList = a PICKED_ITEMS_LIST * where to store new areas pickers (useful in undo commands)
     *                      can be NULL
     * @param aCurrArea = the zone to process
     * @param bMessageBoxInt == true, shows message when clipping occurs.
     * @param  bMessageBoxArc == true, shows message when clipping can't be done due to arcs.
     * @param bRetainArcs = true to handle arcs (not really used in kicad)
     * @return:
     *	-1 if arcs intersect other sides, so polygon can't be clipped
     *	 0 if no intersecting sides
     *	 1 if intersecting sides
     * Also sets areas->utility1 flags if areas are modified
     */
    int             ClipAreaPolygon( PICKED_ITEMS_LIST* aNewZonesList,
                                     ZONE_CONTAINER*    aCurrArea,
                                     bool               bMessageBoxArc,
                                     bool               bMessageBoxInt,
                                     bool               bRetainArcs = TRUE );

    /**
     * Process an area that has been modified, by clipping its polygon against
     * itself and the polygons for any other areas on the same net.
     * This may change the number and order of copper areas in the net.
     * @param aModifiedZonesList = a PICKED_ITEMS_LIST * where to store deleted or added areas
     *                      (useful in undo commands. Can be NULL
     * @param modified_area = area to test
     * @param bMessageBox : if TRUE, shows message boxes when clipping occurs.
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
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful in undo commands
     *                      can be NULL
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

    /** Function RemoveArea
     * remove copper area from net, and put it in a deleted list (if exists)
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful in undo commands
     *                      can be NULL
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
    int  TestAreaIntersection( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_test );

    /**
     * Function CombineAreas
     * If possible, combine 2 copper areas
     * @param aDeletedList = a PICKED_ITEMS_LIST * where to store deleted areas (useful in undo commands
     *                      can be NULL
     * @param area_ref = tje main area (zone)
     * @param area_to_combine = the zone that can be merged with area_ref
     * area_ref must be BEFORE area_to_combine
     * area_to_combine will be deleted, if areas are combined
     * @return : 0 if no intersection
     *         1 if intersection
     *         2 if arcs intersect
     */
    int  CombineAreas( PICKED_ITEMS_LIST* aDeletedList,
                       ZONE_CONTAINER*    area_ref,
                       ZONE_CONTAINER*    area_to_combine );

    /**
     * Function Test_Drc_Areas_Outlines_To_Areas_Outlines
     * Test Areas outlines for DRC:
     *      Test areas inside other areas
     *      Test areas too close
     * @param aArea_To_Examine: area to compare with other areas. if NULL: all areas are compared tp all others
     * @param aCreate_Markers: if true create DRC markers. False: do not creates anything
     * @return errors count
     */
    int  Test_Drc_Areas_Outlines_To_Areas_Outlines( ZONE_CONTAINER* aArea_To_Examine,
                                                    bool            aCreate_Markers );

    /****** function relative to ratsnest calculations: */

    /**
     * Function Test_Connection_To_Copper_Areas
     * init .m_ZoneSubnet parameter in tracks and pads according to the connections to areas found
     * @param aNetcode = netcode to analyse. if -1, analyse all nets
     */
    void Test_Connections_To_Copper_Areas( int aNetcode = -1 );
};

#endif      // #ifndef CLASS_BOARD_H
