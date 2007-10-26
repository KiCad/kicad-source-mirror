/***************************************/
/* Markers: used to show a drc problem */
/***************************************/

#ifndef CLASS_MARKER_H
#define CLASS_MARKER_H 

#include "base_struct.h"


class MARQUEUR : public BOARD_ITEM
{
public:
    wxPoint  m_Pos;
    char*    m_Bitmap;              /* Shape (bitmap) */
    int      m_Type;
    int      m_Color;               /* color */
    wxString m_Diag;                /* Associated text (comment) */
	wxSize m_Size;					/* Size of the graphic symbol */

public:
    MARQUEUR( BOARD_ITEM* StructFather );
    ~MARQUEUR();
    void    UnLink();
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, int DrawMode );
    
    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */ 
    void    Display_Infos( WinEDA_DrawFrame* frame );

    
    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );
};


#endif		//  end #ifndef CLASS_MARKER_H
