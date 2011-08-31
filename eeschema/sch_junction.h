/**
 * @file sch_junction.h
 *
 */

#ifndef _SCH_JUNCTION_H_
#define _SCH_JUNCTION_H_


#include "sch_item_struct.h"


class SCH_JUNCTION : public SCH_ITEM
{
public:
    wxPoint m_Pos;                  /* XY coordinates of connection. */
    wxSize  m_Size;

public:
    SCH_JUNCTION( const wxPoint& pos = wxPoint( 0, 0 ) );

    SCH_JUNCTION( const SCH_JUNCTION& aJunction );

    ~SCH_JUNCTION() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_JUNCTION" );
    }

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_RECT GetBoundingBox() const;

    virtual void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
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
     * Load schematic junction entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic junction from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic junction.
     * @return True if the schematic junction loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function Move
     * moves then item to a new position by \a aMoveVector.
     * @param aMoveVector The displacement vector.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /**
     * Function Mirror_Y
     * mirrors the item relative to \a aYaxis_position.
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Mirror_X( int aXaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual bool IsConnectable() const { return true; }

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

    virtual wxString GetSelectMenuText() const { return wxString( _( "Junction" ) ); }

    virtual BITMAP_DEF GetMenuImage() const { return  add_junction_xpm; }

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os );
#endif

private:
    virtual bool doHitTest( const wxPoint& aPoint, int aAccuracy ) const;
    virtual bool doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const;
    virtual bool doIsConnected( const wxPoint& aPosition ) const;
    virtual EDA_ITEM* doClone() const;
    virtual void doPlot( PLOTTER* aPlotter );
};


#endif    // _SCH_JUNCTION_H_
