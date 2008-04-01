/***************************************/
/* Markers: used to show a drc problem */
/***************************************/

#ifndef CLASS_MARKER_H
#define CLASS_MARKER_H

#include "base_struct.h"

#include "drc_stuff.h"

class MARKER : public BOARD_ITEM
{
protected:
    char*    m_Bitmap;              ///< Shape (bitmap)
    int      m_Type;
    int      m_Color;               ///< color
    wxSize   m_Size;                ///< Size of the graphic symbol

    DRC_ITEM m_drc;

    void     init();

public:

    MARKER( BOARD_ITEM* StructFather );

    /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    MARKER( int aErrorCode, const wxPoint& aMarkerPos,
           const wxString& aText, const wxPoint& aPos,
           const wxString& bText, const wxPoint& bPos );
     /**
     * Constructor
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the object
     * @param aPos The position of the object
     */
    MARKER( int aErrorCode, const wxPoint& aMarkerPos,
           const wxString& aText, const wxPoint& aPos );


    ~MARKER();

    void    UnLink();
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode, const wxPoint& offset = ZeroOffset );


    /**
     * Function GetPosition
     * returns the position of this MARKER.
     */
    wxPoint& GetPosition()
    {
        return (wxPoint&) m_drc.GetPosition();
    }


    /**
     * Function GetPos
     * returns the position of this MARKER, const.
     */
    const wxPoint& GetPos() const
    {
        return m_drc.GetPosition();
    }


    /**
     * Function SetData
     * fills in all the reportable data associated with a MARKER.
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the first of two objects
     * @param aPos The position of the first of two objects
     * @param bText Text describing the second of the two conflicting objects
     * @param bPos The position of the second of two objects
     */
    void SetData( int aErrorCode, const wxPoint& aMarkerPos,
             const wxString& aText, const wxPoint& aPos,
             const wxString& bText, const wxPoint& bPos );

    /**
     * Function SetData
     * fills in all the reportable data associated with a MARKER.
     * @param aErrorCode The categorizing identifier for an error
     * @param aMarkerPos The position of the MARKER on the BOARD
     * @param aText Text describing the object
     * @param aPos The position of the object
     */
    void SetData( int aErrorCode, const wxPoint& aMarkerPos,
             const wxString& aText, const wxPoint& aPos );


    /**
     * Function GetReporter
     * returns the DRC_ITEM held within this MARKER so that its
     * interface may be used.
     * @return const& DRC_ITEM
     */
    const DRC_ITEM& GetReporter() const
    {
        return m_drc;
    }


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void    Display_Infos( WinEDA_DrawFrame* frame );


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


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );
};


#endif      //  CLASS_MARKER_H
