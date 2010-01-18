/*****************************/
/* COTATION class definition */
/*****************************/
#ifndef DIMENSION_H
#define DIMENSION_H

#include "base_struct.h"

class COTATION : public BOARD_ITEM
{
public:
    int        m_Width;
    wxPoint    m_Pos;
    int        m_Shape;
    int        m_Unit;  /* 0 = inches, 1 = mm */
    int        m_Value; /* value of  PCB dimensions. */

    TEXTE_PCB* m_Text;
    int        Barre_ox, Barre_oy, Barre_fx, Barre_fy;
    int        TraitG_ox, TraitG_oy, TraitG_fx, TraitG_fy;
    int        TraitD_ox, TraitD_oy, TraitD_fx, TraitD_fy;
    int        FlecheD1_ox, FlecheD1_oy, FlecheD1_fx, FlecheD1_fy;
    int        FlecheD2_ox, FlecheD2_oy, FlecheD2_fx, FlecheD2_fy;
    int        FlecheG1_ox, FlecheG1_oy, FlecheG1_fx, FlecheG1_fy;
    int        FlecheG2_ox, FlecheG2_oy, FlecheG2_fx, FlecheG2_fy;

public:
    COTATION( BOARD_ITEM* aParent );
    ~COTATION();

    COTATION* Next() const { return (COTATION*) Pnext; }
    COTATION* Back() const { return (COTATION*) Pback; }

    wxPoint& GetPosition()
    {
        return m_Pos;
    }


    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     */
    void  SetLayer( int aLayer );

    bool    ReadCotationDescr( FILE* File, int* LineNum );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    void    SetText( const wxString& NewText );
    wxString GetText( void );

    void    Copy( COTATION* source );

    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                  int aColorMode, const wxPoint& offset = ZeroOffset );

    /**
     * Function Move
     * @param offset : moving vector
     */
    void    Move(const wxPoint& offset);

    /**
     * Function Rotate
     * Rotate this object.
     * @param const wxPoint& aRotCentre - the rotation point.
     * @param aAngle - the rotation angle in 0.1 degree.
     */
    virtual void Rotate(const wxPoint& aRotCentre, int aAngle);

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * @param const wxPoint& aCentre - the rotation point.
     */
    virtual void Flip(const wxPoint& aCentre );

    /**
     * Function Mirror
     * Mirror the Dimension , relative to a given horizontal axis
     * the text is not mirrored. only its position (and angle) is mirrored
     * the layer is not changed
     * @param axis_pos : vertical axis position
     */
    void    Mirror(const wxPoint& axis_pos);

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_DrawFrame in which to print status information.
     */
    void    DisplayInfo( WinEDA_DrawFrame* frame );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );

    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_Rect intersect this object.
     * For now, the anchor must be inside this rect.
     * @param refArea : the given EDA_Rect
     * @return bool - true if a hit, else false
     */
    bool    HitTest( EDA_Rect& refArea );


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "DIMENSION" );
    }
};

#endif  // #define DIMENSION_H
