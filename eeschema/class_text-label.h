/********************************************/
/* Definitions for the EESchema program:	*/
/********************************************/

#ifndef CLASS_TEXT_LABEL_H
#define CLASS_TEXT_LABEL_H

#include "macros.h"
#include "base_struct.h"

/* Type des labels sur sheet (Labels sur hierarchie) et forme des Global-Labels*/
typedef enum {
    NET_INPUT,
    NET_OUTPUT,
    NET_BIDI,
    NET_TRISTATE,
    NET_UNSPECIFIED,
    NET_TMAX        /* Derniere valeur: fin de tableau */
} TypeSheetLabel;

/* Messages correspondants aux types ou forme des labels */
extern const char* SheetLabelType[];
extern int* TemplateShape[5][4];

class SCH_TEXT : public SCH_ITEM
    , public EDA_TextStruct
{

public:
    int  m_Layer;
    int  m_Shape;
    bool m_IsDangling;          // TRUE if not connected
    
    
public:
    SCH_TEXT( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString,
                    KICAD_T aType = TYPE_SCH_TEXT );
    ~SCH_TEXT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_TEXT" );
    }


    SCH_TEXT*       GenCopy();
    virtual void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode,
                          int Color = -1 );

    void            SwapData( SCH_TEXT* copyitem );

    void            Place( WinEDA_SchematicFrame* frame, wxDC* DC );

    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef );

    EDA_Rect        GetBoundingBox();

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os );

#endif

    

};


class SCH_LABEL : public SCH_TEXT
{
public:
    SCH_LABEL( const wxPoint& pos = wxPoint( 0, 0 ), const wxString& text = wxEmptyString );
    ~SCH_LABEL() { }
    virtual void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode,
                          int Color = -1 );

    virtual wxString GetClass() const
    {
        return wxT( "SCH_LABEL" );
    }

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;
};


class SCH_GLOBALLABEL : public SCH_TEXT
{
public:
    SCH_GLOBALLABEL( const wxPoint& pos = wxPoint( 0, 0 ),
                           const wxString& text = wxEmptyString );
    ~SCH_GLOBALLABEL() { }
    virtual void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode,
                          int Color = -1 );

    virtual wxString GetClass() const
    {
        return wxT( "SCH_GLOBALLABEL" );
    }


    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;

    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef );

    EDA_Rect        GetBoundingBox();

    /** function CreateGraphicShape
      * Calculates the graphic shape (a polygon) associated to the text
      * @param aCorner_list = coordinates list fill with polygon corners ooordinates
      * @param Pos = Postion of the shape
      * format list is
      * <corner_count>, x0, y0, ... xn, yn
     */
    void CreateGraphicShape( std::vector <wxPoint>& aCorner_list, const wxPoint & Pos );

};



class SCH_HIERLABEL : public SCH_TEXT
{
public:
    SCH_HIERLABEL( const wxPoint& pos = wxPoint( 0, 0 ),
                         const wxString& text = wxEmptyString );
    ~SCH_HIERLABEL() { }
    virtual void    Draw( WinEDA_DrawPanel* panel, wxDC* DC, const wxPoint& offset, int draw_mode,
                          int Color = -1 );

    virtual wxString GetClass() const
    {
        return wxT( "SCH_HIERLABEL" );
    }


    /** function CreateGraphicShape
      * Calculates the graphic shape (a polygon) associated to the text
      * @param aCorner_list = coordinates list fill with polygon corners ooordinates
      * @param Pos = Postion of the shape
      * format list is
      * <corner_count>, x0, y0, ... xn, yn
      */
    void CreateGraphicShape( std::vector <wxPoint>& aCorner_list, const wxPoint & Pos );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool    Save( FILE* aFile ) const;

    /** Function HitTest
     * @return true if the point aPosRef is within item area
     * @param aPosRef = a wxPoint to test
     */
    bool HitTest( const wxPoint& aPosRef );

    EDA_Rect        GetBoundingBox();
};

#endif /* CLASS_TEXT_LABEL_H */
