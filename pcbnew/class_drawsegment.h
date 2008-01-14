/*************************************/
/* class to handle a graphic segment */
/**************************************/

#ifndef CLASS_DRAWSEGMENT_H
#define CLASS_DRAWSEGMENT_H

class DRAWSEGMENT : public BOARD_ITEM
{
public:
    int     m_Width;            // 0 = line. if > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

    int     m_Shape;            // Shape: line, Circle, Arc
    int     m_Type;             // Used in complex associations ( Dimensions.. )
    int     m_Angle;            // Used only for Arcs: Arc angle in 1/10 deg

public:
    DRAWSEGMENT( BOARD_ITEM* StructFather, KICAD_T idtype = TYPEDRAWSEGMENT );
    ~DRAWSEGMENT();


    /**
     * Function GetPosition
     * returns the position of this object.
     * Required by pure virtual BOARD_ITEM::GetPosition()
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Start;
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;

    bool    ReadDrawSegmentDescr( FILE* File, int* LineNum );

    /* remove this from the linked list */
    void    UnLink();

    void    Copy( DRAWSEGMENT* source );


    /**
     * Function Display_Infos
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_BaseStruct.
     * @param frame A WinEDA_BasePcbFrame in which to print status information.
     */
    void    Display_Infos( WinEDA_DrawFrame* frame );


    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param ref_pos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& ref_pos );

    /**
     * Function HitTest (overlayed)
     * tests if the given EDA_Rect intersect this object.
     * For now, an ending point must be inside this rect.
     * @param refPos the given EDA_Rect to test
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
        return wxT( "DRAWSEGMENT" );
    }
};


#endif      // #ifndef CLASS_DRAWSEGMENT_H
