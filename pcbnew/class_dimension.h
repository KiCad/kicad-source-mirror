/**
 * @file class_dimension.h
 * @brief DIMENSION class definition.
 */

#ifndef DIMENSION_H
#define DIMENSION_H


#include "class_board_item.h"


class LINE_READER;
class EDA_DRAW_PANEL;
class TEXTE_PCB;


class DIMENSION : public BOARD_ITEM
{
public:
    int        m_Width;
    wxPoint    m_Pos;
    int        m_Shape;
    int        m_Unit;  /* 0 = inches, 1 = mm */
    int        m_Value; /* value of  PCB dimensions. */

    TEXTE_PCB* m_Text;
    int        m_crossBarOx, m_crossBarOy, m_crossBarFx, m_crossBarFy;
    int        m_featureLineGOx, m_featureLineGOy, m_featureLineGFx, m_featureLineGFy;
    int        m_featureLineDOx, m_featureLineDOy, m_featureLineDFx, m_featureLineDFy;
    int        m_arrowD1Ox, m_arrowD1Oy, m_arrowD1Fx, m_arrowD1Fy;
    int        m_arrowD2Ox, m_arrowD2Oy, m_arrowD2Fx, m_arrowD2Fy;
    int        m_arrowG1Ox, m_arrowG1Oy, m_arrowG1Fx, m_arrowG1Fy;
    int        m_arrowG2Ox, m_arrowG2Oy, m_arrowG2Fx, m_arrowG2Fy;

public:
    DIMENSION( BOARD_ITEM* aParent );
    ~DIMENSION();

    wxPoint GetPosition() const
    {
        return m_Pos;
    }

    /**
     * Function SetLayer
     * sets the layer this item is on.
     * @param aLayer The layer number.
     */
    void SetLayer( int aLayer );

    /**
     * Function AdjustDimensionDetails
     * Calculate coordinates of segments used to draw the dimension.
     * @param aDoNotChangeText (bool) if false, the dimension text is initialized
     */
    void AdjustDimensionDetails( bool aDoNotChangeText = false );

    bool ReadDimensionDescr( LINE_READER* aReader );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    void SetText( const wxString& NewText );
    wxString GetText( void ) const;

    void Copy( DIMENSION* source );

    void Draw( EDA_DRAW_PANEL* panel, wxDC* DC,
               int aColorMode, const wxPoint& offset = ZeroOffset );

    /**
     * Function Move
     * @param offset : moving vector
     */
    void Move(const wxPoint& offset);

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
     * Function Mirror
     * Mirror the Dimension , relative to a given horizontal axis
     * the text is not mirrored. only its position (and angle) is mirrored
     * the layer is not changed
     * @param axis_pos : vertical axis position
     */
    void Mirror( const wxPoint& axis_pos );

    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& ref_pos );

    /**
     * Function HitTest (overlaid)
     * tests if the given EDA_RECT intersect this object.
     * For now, the anchor must be inside this rect.
     * @param refArea : the given EDA_RECT
     * @return bool - true if a hit, else false
     */
    bool HitTest( EDA_RECT& refArea );


    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "DIMENSION" );
    }

    EDA_RECT GetBoundingBox() const;

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_dimension_xpm; }
};

#endif  // #define DIMENSION_H
