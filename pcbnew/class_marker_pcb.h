/***************************************/
/* Markers: used to show a drc problem */
/***************************************/

#ifndef CLASS_MARKER_PCB_H
#define CLASS_MARKER_PCB_H

#include "base_struct.h"
#include "class_marker_base.h"

class MARKER_PCB : public BOARD_ITEM, public MARKER_BASE
{

public:

    MARKER_PCB( BOARD_ITEM* aParent );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER_PCB on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                const wxString& aText, const wxPoint& aPos,
                const wxString& bText, const wxPoint& bPos );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER_PCB on the BOARD
     * @param aText Text describing the object
     * @param aPos The position of the object
     */
    MARKER_PCB( int aErrorCode, const wxPoint& aMarkerPos,
                const wxString& aText, const wxPoint& aPos );

    ~MARKER_PCB();

    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

    /**
     * Function Rotate
     * Rotate this object.
     * @param aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate( const wxPoint& aRotCentre, int aAngle );

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre );

    /**
     * Function Draw
     */
    void    Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDrawMode,
                  const wxPoint& aOffset = ZeroOffset )
    {
        DrawMarker( aPanel, aDC, aDrawMode, aOffset );
    }

    /**
     * Function GetPosition
     * returns the position of this MARKER_PCB.
     */
    wxPoint& GetPosition()
    {
        return (wxPoint&) m_Pos;
    }


    /**
     * Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef )
    {
        return HitTestMarker( aPosRef );
    }

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void    DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const
    {
        // not implemented, this is here to satisfy BOARD_ITEM::Save()
        // "pure" virtual-ness
        return true;
    }

    virtual wxString GetSelectMenuText() const;

    virtual const char** GetMenuImage() const { return (const char**) drc_xpm; }
};


#endif      //  CLASS_MARKER_PCB_H
