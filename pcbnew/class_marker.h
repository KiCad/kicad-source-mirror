/***************************************/
/* Markers: used to show a drc problem */
/***************************************/

#ifndef CLASS_MARKER_H
#define CLASS_MARKER_H 

#include "base_struct.h"


class MARKER : public BOARD_ITEM
{
private:
    wxString m_Diag;                /* Associated text (comment) */
    
public:
    wxPoint  m_Pos;
    char*    m_Bitmap;              /* Shape (bitmap) */
    int      m_Type;
    int      m_Color;               /* color */
	wxSize m_Size;					/* Size of the graphic symbol */

public:
    MARKER( BOARD_ITEM* StructFather );
    ~MARKER();
    
    void    UnLink();
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode );

    
    /**
     * Function GetMessage
     * @return const wxString& - the diagnostic message
     */
    const wxString& GetMessage()
    {
        return m_Diag;
    }
    void SetMessage( const wxString& aMsg )
    {
        m_Diag = aMsg;
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


#endif		//  end #ifndef CLASS_MARKER_H
