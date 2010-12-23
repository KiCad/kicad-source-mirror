/**
 * @file sch_bus_entry.h
 *
 */

#ifndef _SCH_BUS_ENTRY_H_
#define _SCH_BUS_ENTRY_H_

#include "sch_item_struct.h"


/* Flags for BUS ENTRY (bus to bus or wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


/**
 * Class SCH_BUS_ENTRY
 *
 * Defines a bus or wire entry.
 */
class SCH_BUS_ENTRY : public SCH_ITEM
{
public:
    int     m_Width;
    wxPoint m_Pos;
    wxSize  m_Size;

public:
    SCH_BUS_ENTRY( const wxPoint& pos = wxPoint( 0, 0 ), int shape = '\\', int id = WIRE_TO_BUS );

    SCH_BUS_ENTRY( const SCH_BUS_ENTRY& aBusEntry );

    ~SCH_BUS_ENTRY() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_BUS_ENTRY" );
    }

    wxPoint m_End() const;

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic bus entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic bus entry from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic bus entry.
     * @return True if the schematic bus entry loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    /**
     * Function Move
     * moves and item to a new position by \a aMoveVector.
     * @param aMoveVector The displacement vector.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /**
     * Function Mirror_Y
     * mirrors the item relative to \a aYaxis_position.
     * @param aYaxis_position The Y axis coordinate to mirror around.
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Mirror_X( int aXaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool doHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
    virtual EDA_ITEM* doClone() const;
};


#endif    // _SCH_BUS_ENTRY_H_
