/****************************************************/
/* MIREPCB class definition.  (targets for photos)  */
/****************************************************/
#ifndef MIRE_H
#define MIRE_H

#include "base_struct.h"
#include "richio.h"


class MIREPCB : public BOARD_ITEM
{
public:
    int     m_Width;
    wxPoint m_Pos;
    int     m_Shape;            // bit 0 : 0 = draw +, 1 = draw X
    int     m_Size;

public:
    MIREPCB( BOARD_ITEM* aParent );
    ~MIREPCB();

    MIREPCB*    Next() const { return (MIREPCB*) Pnext; }
    MIREPCB*    Back() const { return (MIREPCB*) Pnext; }

    wxPoint& GetPosition()
    {
        return m_Pos;
    }


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
    virtual void Rotate(const wxPoint& aRotCentre, int aAngle);

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param aCentre - the rotation point.
     */
    virtual void Flip(const wxPoint& aCentre );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    bool    ReadMirePcbDescr( LINE_READER* aReader );

    void    Copy( MIREPCB* source );

    void    Draw( EDA_DRAW_PANEL* panel, wxDC* DC, int aDrawMode,
                  const wxPoint& offset = ZeroOffset );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos );

    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_RECT intersect this object.
     * For now, the anchor must be inside this rect.
     * @param refArea : the given EDA_RECT
     * @return bool - true if a hit, else false
     */
    bool    HitTest( EDA_RECT& refArea );

    EDA_RECT GetBoundingBox() const;
};


#endif  // #define MIRE_H
