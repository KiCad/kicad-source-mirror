/**************************************************************/
/* class_board.h - Class BOARD to handle a board */
/**************************************************************/

#ifndef CLASS_BOARD_H
#define CLASS_BOARD_H


#include "dlist.h"


class ZONE_CONTAINER;
class EDA_BoardDesignSettings;


/**
 * Enum LAYER_T
 * gives the allowed types of layers, same as Specctra DSN spec.
 */
enum LAYER_T
{
    LT_SIGNAL,
    LT_POWER,
    LT_MIXED,
    LT_JUMPER,
};


/**
 * Struct LAYER
 * holds information pertinent to a layer of a BOARD.
 */
struct LAYER
{
    /** The name of the layer, there should be no spaces in this name. */
    wxString    m_Name;

    /** The type of the layer */
    LAYER_T     m_Type;

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
    static LAYER_T ParseType( const char* aType );
};


/**
 * Class BOARD
 * holds information pertinent to a PCBNEW printed circuit board.
 */
class BOARD : public BOARD_ITEM
{
    friend class WinEDA_PcbFrame;

private:
    typedef std::vector<MARKER*>  MARKERS;      // @todo: switch to boost:ptr_vector, and change ~BOARD()
    MARKERS         m_markers;                  ///< MARKERs for clearance problems, owned by pointer

    typedef std::vector<ZONE_CONTAINER*>  ZONE_CONTAINERS;  // @todo: switch to boost::ptr_vector, and change ~BOARD()
    ZONE_CONTAINERS m_ZoneDescriptorList; 	   ///< edge zone descriptors, owned by pointer

    LAYER           m_Layer[NB_COPPER_LAYERS];

public:
    WinEDA_BasePcbFrame*    m_PcbFrame;         // Window de visualisation
    EDA_Rect                m_BoundaryBox;      // Board size and position
    int                     m_Unused;
    int                     m_Status_Pcb;       // Flags used in ratsnet calculation and update
    EDA_BoardDesignSettings* m_BoardSettings;   // Link to current design settings
    int             m_NbNodes;                  // Active pads (pads attached to a net ) count
    int             m_NbLinks;                  // Ratsnet count
    int             m_NbLoclinks;               // Rastests to shew while creating a track
    int             m_NbNoconnect;              // Active ratsnet count (rastnest not alraedy connected by tracks

    DLIST<BOARD_ITEM> m_Drawings;               // linked list of lines & texts
    DLIST<MODULE>   m_Modules;                  // linked list of MODULEs
    DLIST<EQUIPOT>  m_Equipots;                 // linked list of nets

    DLIST<TRACK>    m_Track;                    // linked list of TRACKs and SEGVIAs

    DLIST<SEGZONE>  m_Zone;                     // linked list of SEGZONEs

    std::vector<D_PAD*> m_Pads;                 // Entry for a sorted pad list (used in ratsnest calculations)

    CHEVELU*        m_Ratsnest;                 // Rastnest list
    CHEVELU*        m_LocalRatsnest;            // Rastnest list used while moving a footprint

    ZONE_CONTAINER* m_CurrentZoneContour;     	// zone contour currently in progress

    BOARD( EDA_BaseStruct* aParent, WinEDA_BasePcbFrame* frame );
    ~BOARD();

    /**
     * Function GetPosition
     * is here to satisfy BOARD_ITEM's requirements, but this implementation
     * is a dummy.
     * @return const wxPoint& of (0,0)
     */
    wxPoint& GetPosition();

    /* supprime du chainage la structure Struct */
    void    UnLink();

    /**
     * Function Add
     * adds the given item to this BOARD and takes ownership of its memory.
     * @param aBoardItem The item to add to this board.
     * @param aControl An int which can vary how the item is added.
     */
    void    Add( BOARD_ITEM* aBoardItem, int aControl = 0 );
#define ADD_APPEND      1   ///< aControl flag for Add( aControl ), appends not inserts


    /**
     * Function Delete
     * deletes the given single item from this BOARD and deletes its memory.  If you
     * need the object after deletion, first copy it.
     * @param aBoardItem The item to remove from this board and delete
     */
    void    Delete( BOARD_ITEM* aBoardItem );

    /**
     * Function DeleteMARKERs
     * deletes ALL MARKERS from the board.
     */
    void    DeleteMARKERs();

    /**
     * Function DeleteZONEOutlines
     * deletes ALL zone outlines from the board.
     */
    void    DeleteZONEOutlines();


    /**
     * Function DeleteMARKER
     * deletes one MARKER from the board.
     * @param aIndex The index of the marker to delete.
     */
    void    DeleteMARKER( int aIndex );


    /**
     * Function GetMARKER
     * returns the MARKER at a given index.
     * @param index The array type index into a collection of MARKERS.
     * @return MARKER* - a pointer to the MARKER or NULL if index out of range.
     */
    MARKER* GetMARKER( int index ) const
    {
        if( (unsigned) index < m_markers.size() )
            return m_markers[index];
        return NULL;
    }


    /**
     * Function GetMARKERCount
     * @return int - The number of MARKERS.
     */
    int GetMARKERCount() const
    {
        return (int) m_markers.size();
    }

    /**
     * Function GetCopperLayerCount
     * @return int - The number of copper layers in the BOARD.
     */
    int GetCopperLayerCount() const;

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
    bool SetLayerName( int aLayerIndex, const wxString& aLayerName );

    /**
     * Function GetLayerType
     * returns the type of the copper layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like COPPER_LAYER_N, etc.
     * @return LAYER_T - the layer type, or LAYER_T(-1) if the
     *  index was out of range.
     */
    LAYER_T GetLayerType( int aLayerIndex ) const;

    /**
     * Function SetLayerName
     * changes the name of the layer given by aLayerIndex.
     *
     * @param aLayerIndex A layer index, like COPPER_LAYER_N, etc.
     * @param aLayerType The new layer type.
     * @return bool - true if aLayerType was legal and aLayerIndex was within range, else false.
     */
    bool SetLayerType( int aLayerIndex, LAYER_T aLayerType );


    /* Routines de calcul des nombres de segments pistes et zones */
    int     GetNumSegmTrack();
    int     GetNumSegmZone();
    int     GetNumNoconnect();    // retourne le nombre de connexions manquantes

    /**
     * Function GetNumRatsnests
     * @return int - The number of rats
     */
    int     GetNumRatsnests()
    {
        return m_NbLinks;
    }

    int     GetNumNodes();        // retourne le nombre de pads a netcode > 0

    // Calcul du rectangle d'encadrement:
    bool    ComputeBoundaryBox();


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void    Display_Infos( WinEDA_DrawFrame* frame );

    void Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                      int aDrawMode, const wxPoint& offset = ZeroOffset );


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
     * @return EQUIPOT* - the net or NULL if not found.
     */
    EQUIPOT* FindNet( int aNetcode ) const;

    /**
     * Function FindNet overlayed
     * searches for a net with the given name.
     * @param aNetname A Netname to search for.
     * @return EQUIPOT* - the net or NULL if not found.
     */
    EQUIPOT* FindNet( const wxString & aNetname ) const;

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
     * searches for a net with the given netcode.
     * @param aNames An array string to fill with net names.
     * @param aSort_Type : NO_SORT = no sort, ALPHA_SORT = sort by alphabetic order, PAD_CNT_SORT = sort by active pads count.
     * @return int - net names count.
     */
    enum netname_sort_type {
        NO_SORT,
        ALPHA_SORT,
        PAD_CNT_SORT
    };
    int ReturnSortedNetnamesList( wxArrayString & aNames, const int aSort_Type);


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
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif

    /**************************/
    /* footprint operations : */
    /**************************/
    void Change_Side_Module( MODULE* Module, wxDC* DC );

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
    ZONE_CONTAINER*  HitTestForAnyFilledArea( const wxPoint& aRefPos, int aStartLayer, int aEndLayer = -1 );

    /**
     * Function RedrawAreasOutlines
     * Redraw all areas outlines on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawAreasOutlines(WinEDA_DrawPanel* panel, wxDC * aDC, int aDrawMode, int aLayer);

    /**
     * Function RedrawFilledAreas
     * Redraw all filled areas on layer aLayer ( redraw all if aLayer < 0 )
     */
    void RedrawFilledAreas(WinEDA_DrawPanel* panel, wxDC * aDC, int aDrawMode, int aLayer);

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
    int SetAreasNetCodesFromNetNames(void);

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
    int GetAreaIndex( const ZONE_CONTAINER* aArea) const
    {
        for( int ii = 0; ii < GetAreaCount(); ii++ )	// Search for aArea in list
        {
            if ( aArea == GetArea( ii ) )	// Found !
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
      * add empty copper area to net
      * @return pointer to the new area
     */
    ZONE_CONTAINER* AddArea( int netcode, int layer, int x, int y, int hatch );

    /**
     * remove copper area from net
     * @param  area = area to remove
     * @return 0
     */
    int RemoveArea( ZONE_CONTAINER* area_to_remove );

    /**
     * Function InsertArea
      * add empty copper area to net, inserting after m_ZoneDescriptorList[iarea]
     * @return pointer to the new area
     */
    ZONE_CONTAINER* InsertArea( int netcode, int iarea, int layer, int x, int y, int hatch );

    /**
     Function CompleteArea
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
     * @param bMessageBoxInt == TRUE, shows message when clipping occurs.
     * @param  bMessageBoxArc == TRUE, shows message when clipping can't be done due to arcs.
     * @return:
     *	-1 if arcs intersect other sides, so polygon can't be clipped
     *	 0 if no intersecting sides
     *	 1 if intersecting sides
     * Also sets areas->utility1 flags if areas are modified
    */
    int ClipAreaPolygon( ZONE_CONTAINER* CurrArea,
                                bool bMessageBoxArc, bool bMessageBoxInt, bool bRetainArcs = TRUE );

    /**
     * Process an area that has been modified, by clipping its polygon against
     * itself and the polygons for any other areas on the same net.
     * This may change the number and order of copper areas in the net.
     * @param modified_area = area to test
     * @param bMessageBox : if TRUE, shows message boxes when clipping occurs.
     * @return :
     * -1 if arcs intersect other sides, so polygon can't be clipped
     *  0 if no intersecting sides
     *  1 if intersecting sides, polygon clipped
     */
    int AreaPolygonModified( ZONE_CONTAINER* modified_area,
                                    bool            bMessageBoxArc,
                                    bool            bMessageBoxInt );

    /**
     * Function CombineAllAreasInNet
      * Checks all copper areas in net for intersections, combining them if found
      * @param aNetCode = net to consider
      * @param bMessageBox : if true display warning message box
      * @param bUseUtility : if true, don't check areas if both utility flags are 0
      * Sets utility flag = 1 for any areas modified
      * If an area has self-intersecting arcs, doesn't try to combine it
     */
    int CombineAllAreasInNet( int aNetCode, bool bMessageBox, bool bUseUtility );

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
      * area_ref must be BEFORE area_to_combine
      * area_to_combine will be deleted, if areas are combined
      * @return : 0 if no intersection
      *         1 if intersection
      *         2 if arcs intersect
     */
    int CombineAreas( ZONE_CONTAINER* area_ref, ZONE_CONTAINER* area_to_combine );

    /**
     * Function Test_Drc_Areas_Outlines_To_Areas_Outlines
     * Test Areas outlines for DRC:
     *      Test areas inside other areas
     *      Test areas too close
     * @param aArea_To_Examine: area to compare with other areas. if NULL: all areas are compared tp all others
     * @param aCreate_Markers: if true create DRC markers. False: do not creates anything
     * @return errors count
    */
    int Test_Drc_Areas_Outlines_To_Areas_Outlines( ZONE_CONTAINER* aArea_To_Examine,bool aCreate_Markers );

    /****** function relative to ratsnest calculations: */

    /**
     * Function Test_Connection_To_Copper_Areas
     * init .m_ZoneSubnet parameter in tracks and pads according to the connections to areas found
     * @param aNetcode = netcode to analyse. if -1, analyse all nets
     */
    void Test_Connections_To_Copper_Areas( int aNetcode = -1 );

};

#endif		// #ifndef CLASS_BOARD_H
