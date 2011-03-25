/***************************************************/
/* classes to handle markers used in schematic ... */
/***************************************************/

#ifndef _TYPE_SCH_MARKER_H_
#define _TYPE_SCH_MARKER_H_

#include "sch_item_struct.h"
#include "class_marker_base.h"

/* Marker are mainly used to show an ERC error
 */

enum TypeMarker {
    /* Markers type */
    MARK_UNSPEC,
    MARK_ERC,
    MARK_PCB,
    MARK_SIMUL,
    MARK_NMAX        /* Lats value: end of list */
};


/* Names for corresponding types of markers: */
extern const wxChar* NameMarqueurType[];


class SCH_MARKER : public SCH_ITEM, public MARKER_BASE
{
public:
    SCH_MARKER();
    SCH_MARKER( const wxPoint& aPos, const wxString& aText );
    SCH_MARKER( const SCH_MARKER& aMarker );
    ~SCH_MARKER();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_MARKER" );
    }

    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDraw_mode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    virtual EDA_Rect GetBoundingBox() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Rotate( wxPoint rotationPoint );
    virtual void Mirror_X( int aXaxis_position );

    /**
     * Compare DRC marker main and auxiliary text against search string.
     *
     * @param aSearchData - Criteria to search against.
     * @param aFindLocation - a wxPoint where to put the location of matched item. can be NULL.
     * @return True if the DRC main or auxiliary text matches the search criteria.
     */
    virtual bool Matches( wxFindReplaceData& aSearchData, wxPoint * aFindLocation );

    /**
     * Show the marker electronics rule check error on the message panel.
     *
     * @param aFrame - Top window that owns the message panel.
     */
    void DisplayInfo( EDA_DRAW_FRAME* aFrame );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual wxString GetSelectMenuText() const { return wxString( _( "ERC Marker" ) ); }

    virtual const char** GetMenuImage() const { return (const char**) erc_xpm; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os );

#endif

    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual EDA_ITEM* doClone() const;
};

#endif /* _TYPE_SCH_MARKER_H_ */
