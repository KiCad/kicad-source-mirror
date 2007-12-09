/**************************************************************/
/* class_board.h - Class BOARD to handle a board */
/**************************************************************/

#ifndef CLASS_BOARD_H
#define CLASS_BOARD_H

class ZONE_CONTAINER;
class EDA_BoardDesignSettings;

/***********************************************/
/* class BOARD : handle datas to build a board */
/***********************************************/


class BOARD : public BOARD_ITEM
{
    friend class WinEDA_PcbFrame;
    
private:

    std::vector<MARKER*> m_markers;             ///< MARKERs for clearance problems, owned by pointer                                                 
//    std::vector<MARKER*> m_markersUnconnected;  ///< MARKERs for unconnected problems, owned by pointer                                                 
    std::vector<ZONE_CONTAINER*> m_ZoneDescriptorList; 	///< edge zone descriptors, owned by pointer                                                 

    
public:
    WinEDA_BasePcbFrame*    m_PcbFrame;         // Window de visualisation
    EDA_Rect                m_BoundaryBox;      // Board size and position
    int                     m_Unused;
    int                     m_Status_Pcb;       // Flags used in ratsnet calculation and update
    EDA_BoardDesignSettings* m_BoardSettings;   // Link to current design settings
    int             m_NbNets;                   // Nets (equipotentielles) count
    int             m_NbNodes;                  // Active pads (pads attached to a net ) count
    int             m_NbLinks;                  // Ratsnet count
    int             m_NbLoclinks;               // Rastests to shew while creating a track
    int             m_NbNoconnect;              // Active ratsnet count (rastnest not alraedy connected by tracks
    int             m_NbSegmTrack;              // Track items count
    int             m_NbSegmZone;               // Zone items count

    BOARD_ITEM*     m_Drawings;                 // linked list of lines & texts
    MODULE*         m_Modules;                  // linked list of MODULEs
    EQUIPOT*        m_Equipots;                 // linked list of nets
    TRACK*          m_Track;                    // linked list of TRACKs and SEGVIAs
    SEGZONE*        m_Zone;                     // linked list of SEGZONEs
    D_PAD**         m_Pads;                     // Entry for a sorted pad list (used in ratsnest calculations)
    int             m_NbPads;                   // Pad count
    CHEVELU*        m_Ratsnest;                 // Rastnest list
    CHEVELU*        m_LocalRatsnest;            // Rastnest list used while moving a footprint

    EDGE_ZONE*      m_CurrentLimitZone;         /* pointeur sur la liste des segments
                                                 * de delimitation de la zone en cours de trace */

    BOARD( EDA_BaseStruct* StructFather, WinEDA_BasePcbFrame* frame );
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
};

#endif		// #ifndef CLASS_BOARD_H
