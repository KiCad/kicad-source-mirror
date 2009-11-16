/********************************/
/* TEXTE_PCB class definition. */
/********************************/
#ifndef CLASS_PCB_TEXT_H
#define CLASS_PCB_TEXT_H

#include "base_struct.h"
#include "PolyLine.h"

class TEXTE_PCB : public BOARD_ITEM, public EDA_TextStruct
{
public:
    TEXTE_PCB( BOARD_ITEM* parent );
    TEXTE_PCB( TEXTE_PCB* textepcb );
    ~TEXTE_PCB();

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return wxPoint& - The position of this object, non-const so it
     *          can be changed
     */
    wxPoint& GetPosition()
    {
        return m_Pos;   // within EDA_TextStruct
    }

    /**
     * Function Move
     * move this object.
     * @param const wxPoint& aMoveVector - the move vector for this object.
     */
    virtual void Move(const wxPoint& aMoveVector)
    {
        m_Pos += aMoveVector;
    }

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

    /* duplicate structure */
    void Copy( TEXTE_PCB* source );

    void Draw( WinEDA_DrawPanel * panel, wxDC* DC, int aDrawMode,
               const wxPoint& offset = ZeroOffset );

    // File Operations:
    int ReadTextePcbDescr( FILE* File, int* LineNum );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

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
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos )
    {
        return TextHitTest( refPos );
    }


    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_Rect intersect this object.
     * @param refArea the given EDA_Rect to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( EDA_Rect& refArea )
    {
        return TextHitTest( refArea );
    }

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT("PTEXT");
    }

    /** Function TransformShapeWithClearanceToPolygon
     * Convert the track shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approximated by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
     */
    void TransformShapeWithClearanceToPolygon(
        std::vector <CPolyPt>& aCornerBuffer,
        int                    aClearanceValue,
        int                    aCircleToSegmentsCount,
        double                 aCorrectionFactor );

#if defined(DEBUG)
    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *          of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );
#endif

};

#endif  // #define CLASS_PCB_TEXT_H
