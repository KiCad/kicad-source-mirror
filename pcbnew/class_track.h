/*******************************************************************/
/*	class_track.h: definitions relatives to tracks, vias and zones */
/*******************************************************************/

#ifndef CLASS_TRACK_H
#define CLASS_TRACK_H

#include "base_struct.h"


// Via attributes (m_Shape parmeter)
#define THROUGH_VIA      3              /* Always a through hole via */
#define BURIED_VIA       2              /* this via can be on internal layers */
#define BLIND_VIA        1              /* this via which connect from internal layers to an external layer */
#define NOT_DEFINED_VIA  0              /* reserved (unused) */
#define SQUARE_VIA_SHAPE 0x80000000     /* Flag pour forme carree */

/***/

class TRACK : public BOARD_ITEM
{
public:
    int         m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint     m_Start;            // Line start point
    wxPoint     m_End;              // Line end point

    int         m_Shape;            // vias: shape and type, Track = shape..
    int         m_Drill;            // for vias: via drill (- 1 for default value)

    BOARD_ITEM* start;              // pointers to a connected item (pad or track)
    BOARD_ITEM* end;

    // chain = 0 indique une connexion non encore traitee
    int         m_Param;            // Auxiliary variable ( used in some computations )

protected:
    int         m_NetCode;          // Net number
    int         m_Sous_Netcode;     /* In rastnest routines : for the current net,
                                     *  block number (number common to the current connected items found) */

    TRACK( const TRACK& track );    // protected so Copy() is used instead.

public:
    TRACK( BOARD_ITEM* StructFather, KICAD_T idtype = TYPETRACK );

    /**
     * Function Copy
     * will copy this object whether it is a TRACK or a SEGVIA returning
     * the corresponding type.
     * @return - TRACK*, SEGVIA*, or SEGZONE*, declared as the least common
     *  demoninator: TRACK
     */
    TRACK* Copy() const;

    TRACK* Next() const { return (TRACK*) Pnext; }

    TRACK* Back() const { return (TRACK*) Pback; }


    /* supprime du chainage la structure Struct */
    void    UnLink();

    // Read/write data
    bool    WriteTrackDescr( FILE* File );

    /**
     * Function Insert
     * inserts a TRACK, SEGVIA or SEGZONE into its proper list, either at the
     * list's front or immediately after the InsertPoint.
     * If Insertpoint == NULL, then insert at the beginning of the proper list.
     * If InsertPoint != NULL, then insert immediately after InsertPoint.
     * TRACKs and SEGVIAs are put on the m_Track list, SEGZONE on the m_Zone list.
     * @param InsertPoint See above
     */
    void    Insert( BOARD* Pcb, BOARD_ITEM* InsertPoint );

/*Search the "best" insertion point within the track linked list
 *  the best point is the of the corresponding net code section
 *  @return the item found in the linked list (or NULL if no track)
 */
    TRACK*  GetBestInsertPoint( BOARD* Pcb );

    /* Search (within the track linked list) the first segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK*  GetStartNetCode( int NetCode );

    /* Search (within the track linked list) the last segment matching the netcode
     * ( the linked list is always sorted by net codes )
     */
    TRACK*  GetEndNetCode( int NetCode );

    /**
     * Function GetNet
     * @return int - the net code.
     */
    int GetNet() const { return m_NetCode; }
    void SetNet( int aNetCode ) { m_NetCode = aNetCode; }


    /**
     * Function GetSubNet
     * @return int - the sub net code.
     */
    int GetSubNet() const { return m_Sous_Netcode; }
    void SetSubNet( int aSubNetCode ) { m_Sous_Netcode = aSubNetCode; }


    /**
     * Function GetLength
     * returns the length of the track using the hypotenuse calculation.
     * @return double - the length of the track
     */
    double  GetLength() const;


    /* Display on screen: */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode );

    /* divers */
    int Shape() const { return m_Shape & 0xFF; }

    /**
     * Function ReturnMaskLayer
     * returns a "layer mask", which is a bitmap of all layers on which the
     * TRACK segment or SEGVIA physically resides.
     * @return int - a layer mask, see pcbstruct.h's CUIVRE_LAYER, etc.
     */
    int             ReturnMaskLayer();

    int             IsPointOnEnds( const wxPoint& point, int min_dist = 0 );

    bool            IsNull(); // return TRUE if segment lenght = 0


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void            Display_Infos( WinEDA_DrawFrame* frame );


    /**
     * Function ShowWidth
     * returns the width of the track in displayable user units.
     */
    wxString        ShowWidth();

    /**
     * Function Visit
     * is re-implemented here because TRACKs and SEGVIAs are in the same list
     * within BOARD.  If that were not true, then we could inherit the
     * version from EDA_BaseStruct.  This one does not iterate through scanTypes
     * but only looks at the first item in the list.
     * @param inspector An INSPECTOR instance to use in the inspection.
     * @param testData Arbitrary data used by the inspector.
     * @param scanTypes Which KICAD_T types are of interest and the order
     *  is significant too, terminated by EOT.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *  else SCAN_CONTINUE, and determined by the inspector.
     */
    SEARCH_RESULT   Visit( INSPECTOR* inspector, const void* testData,
                           const KICAD_T scanTypes[] );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool            HitTest( const wxPoint& refPos );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "TRACK" );
    }


#if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif
};


class SEGZONE : public TRACK
{
public:
    SEGZONE( BOARD_ITEM* StructFather );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "ZONE" );
    }


    SEGZONE* Next() const { return (SEGZONE*) Pnext; }
};


class SEGVIA : public TRACK
{
public:
    SEGVIA( BOARD_ITEM* StructFather );

    SEGVIA( const SEGVIA& source ) :
        TRACK( source )
    {
    }


    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual
     * from BOARD_ITEM.  Tests the starting and ending range of layers for the
     * via.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    bool    IsOnLayer( int aLayer ) const;

    void    SetLayerPair( int top_layer, int bottom_layer );
    void    ReturnLayerPair( int* top_layer, int* bottom_layer ) const;

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "VIA" );
    }


#if defined (DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    void Show( int nestLevel, std::ostream& os );

#endif
};


#endif /* CLASS_TRACK_H */
