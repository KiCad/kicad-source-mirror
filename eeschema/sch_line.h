/**
 * @file sch_line.h
 *
 */

#ifndef _SCH_LINE_H_
#define _SCH_LINE_H_


#include "sch_item_struct.h"


/**
 * Class SCH_LINE
 * is a segment description base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
    bool    m_StartIsDangling;
    bool    m_EndIsDangling;    // TRUE if not connected  (wires, tracks...)

public:
    int     m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

public:
    SCH_LINE( const wxPoint& pos = wxPoint( 0, 0 ), int layer = LAYER_NOTES );
    SCH_LINE( const SCH_LINE& aLine );
    ~SCH_LINE() { }

    SCH_LINE* Next() const { return (SCH_LINE*) Pnext; }
    SCH_LINE* Back() const { return (SCH_LINE*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_LINE" );
    }

    bool IsEndPoint( const wxPoint& aPoint ) const
    {
        return aPoint == m_Start || aPoint == m_End;
    }

    bool IsNull() const { return m_Start == m_End; }

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic line from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic line from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic line.
     * @return True if the schematic line loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    /**
     * Function Move
     * moves the item by \a aMoveVector.
     * @param aMoveVector The displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector );

    virtual void Mirror_X( int aXaxis_position );

    /**
     * Function Mirror_Y
     * mirrors the item relative to \a aYaxis_position.
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    /**
     * Check line against \a aLine to see if it overlaps and merge if it does.
     *
     * This method will change the line to be equivalent of the line and \a aLine if the
     * two lines overlap.  This method is used to merge multple line segments into a single
     * line.
     *
     * @param aLine - Line to compare.
     * @return True if lines overlap and the line was merged with \a aLine.
     */
    bool MergeOverlap( SCH_LINE* aLine );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDangling() const { return m_StartIsDangling || m_EndIsDangling; }

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    /**
     * Function IsConnectable
     * returns true if the schematic item can connect to another schematic item.
     */
    virtual bool IsConnectable() const;

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;
#endif

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool doHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
    virtual bool doIsConnected( const wxPoint& aPosition ) const;
    virtual EDA_ITEM* doClone() const;
};


#endif    // _SCH_LINE_H_
