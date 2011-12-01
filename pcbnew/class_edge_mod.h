/**
 * @file class_edge_mod.h
 * @brief EDGE_MODULE class definition.
 */

#ifndef _CLASS_EDGE_MOD_H_
#define _CLASS_EDGE_MOD_H_


#include "class_drawsegment.h"


class LINE_READER;
class EDA_3D_CANVAS;
class EDA_DRAW_FRAME;


class EDGE_MODULE : public DRAWSEGMENT
{
public:
    wxPoint m_Start0;       // Start point or center, relative to module origin, orient 0.
    wxPoint m_End0;         // End point, relative to module origin, orient 0.

public:
    EDGE_MODULE( MODULE* parent, STROKE_T aShape = S_SEGMENT );
    EDGE_MODULE( EDGE_MODULE* edge );
    ~EDGE_MODULE();

    EDGE_MODULE* Next() const { return (EDGE_MODULE*) Pnext; }
    EDGE_MODULE* Back() const { return (EDGE_MODULE*) Pback; }

    void Copy( EDGE_MODULE* source ); // copy structure

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    int ReadDescr( LINE_READER* aReader );

    void SetDrawCoord();

    /* drawing functions */
    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
               int aDrawMode, const wxPoint& offset = ZeroOffset );

    void Draw3D( EDA_3D_CANVAS* glcanvas );

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "MGRAPHIC" );

        // return wxT( "EDGE" );  ?
    }

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  show_mod_edge_xpm; }

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *                  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

#endif
};

#endif    // _CLASS_EDGE_MOD_H_
