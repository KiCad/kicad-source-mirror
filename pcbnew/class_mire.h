/**
 * @file class_mire.h
 * @brief PCB_TARGET class definition.  (targets for photo plots).
 */

#ifndef MIRE_H
#define MIRE_H


#include "class_board_item.h"


class EDA_RECT;
class LINE_READER;
class EDA_DRAW_PANEL;


class PCB_TARGET : public BOARD_ITEM
{
public:
    int     m_Width;
    wxPoint m_Pos;
    int     m_Shape;            // bit 0 : 0 = draw +, 1 = draw X
    int     m_Size;

public:
    PCB_TARGET( BOARD_ITEM* aParent );
    ~PCB_TARGET();

    PCB_TARGET* Next() const { return (PCB_TARGET*) Pnext; }
    PCB_TARGET* Back() const { return (PCB_TARGET*) Pnext; }

    const wxPoint GetPosition() const
    {
        return m_Pos;
    }

    void SetPosition( const wxPoint& aPos ) { m_Pos = aPos; }

    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move( const wxPoint& aMoveVector )
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
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    bool ReadMirePcbDescr( LINE_READER* aReader );

    void Copy( PCB_TARGET* source );

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int aDrawMode,
               const wxPoint& offset = ZeroOffset );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& refPos );

    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_RECT intersect this object.
     * For now, the anchor must be inside this rect.
     * @param refArea : the given EDA_RECT
     * @return bool - true if a hit, else false
     */
    bool HitTest( EDA_RECT& refArea );

    EDA_RECT GetBoundingBox() const;

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_mires_xpm; }
};


#endif  // #define MIRE_H
