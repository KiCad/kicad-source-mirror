/**
 * @file worksheet_shape_builder.h
 * @brief classes and function to generate graphics to plt or draw titles blocks
 * and frame references
 */

#ifndef  WORKSHEET_SHAPE_BUILDER_H
#define  WORKSHEET_SHAPE_BUILDER_H


/*
 * Helper classes to handle basic graphic items used to raw/plot
 * title blocks and frame references
 * segments
 * rect
 * polygons (for logos)
 * graphic texts
 */
class WS_DRAW_ITEM_BASE     // This basic class, not directly usable.
{
public:
    enum WS_DRAW_TYPE {
        wsg_line, wsg_rect, wsg_poly, wsg_text
    };
protected:
    WS_DRAW_TYPE    m_type; // wsg_line, wsg_rect, wsg_poly, wsg_text
    EDA_COLOR_T     m_color;
protected:
    WS_DRAW_ITEM_BASE( WS_DRAW_TYPE aType, EDA_COLOR_T aColor )
    {
        m_type  = aType;
        m_color = aColor;
    }

public:
    virtual ~WS_DRAW_ITEM_BASE() {}

    // Accessors:
    EDA_COLOR_T GetColor() { return m_color; }
    WS_DRAW_TYPE GetType() { return m_type; };
};

// This class draws a thick segment
class WS_DRAW_ITEM_LINE : public WS_DRAW_ITEM_BASE
{
    wxPoint m_start;    // start point of line/rect
    wxPoint m_end;      // end point
    int     m_penWidth;
public:
    WS_DRAW_ITEM_LINE( wxPoint aStart, wxPoint aEnd,
                       int aPenWidth, EDA_COLOR_T aColor ) :
        WS_DRAW_ITEM_BASE( wsg_line, aColor )
    {
        m_start     = aStart;
        m_end       = aEnd;
        m_penWidth  = aPenWidth;
    }

    // Accessors:
    int GetPenWidth() { return m_penWidth; }
    const wxPoint&  GetStart() { return m_start; }
    const wxPoint&  GetEnd() { return m_end; }
};

// This class draws a polygon
class WS_DRAW_ITEM_POLYGON : public WS_DRAW_ITEM_BASE
{
    int m_penWidth;
    bool m_fill;

public:
    std::vector <wxPoint> m_Corners;
public:
    WS_DRAW_ITEM_POLYGON( bool aFill, int aPenWidth, EDA_COLOR_T aColor ) :
        WS_DRAW_ITEM_BASE( wsg_poly, aColor )
    {
        m_penWidth = aPenWidth;
        m_fill = aFill;
    }

    // Accessors:
    int GetPenWidth() { return m_penWidth; }
    bool IsFilled() { return m_fill; }
};

// This class draws a not filled rectangle with thick segment
class WS_DRAW_ITEM_RECT : public WS_DRAW_ITEM_LINE
{
public:
    WS_DRAW_ITEM_RECT( wxPoint aStart, wxPoint aEnd,
                       int aPenWidth, EDA_COLOR_T aColor ) :
        WS_DRAW_ITEM_LINE( aStart, aEnd, aPenWidth, aColor )
    {
        m_type = wsg_rect;
    }
};

// This class draws a graphic text.
// it is derived from an EDA_TEXT, so it handle all caracteristics
// of this graphic text (justification, rotation ... )
class WS_DRAW_ITEM_TEXT : public WS_DRAW_ITEM_BASE, public EDA_TEXT
{
public:
    WS_DRAW_ITEM_TEXT( wxString& aText, wxPoint aPos, wxSize aSize,
                       int aPenWidth, EDA_COLOR_T aColor,
                       bool aItalic = false, bool aBold = false ) :
        WS_DRAW_ITEM_BASE( wsg_text, aColor ), EDA_TEXT( aText )
    {
        SetTextPosition( aPos );
        SetSize( aSize );
        SetThickness( aPenWidth );
        SetItalic( aItalic );
        SetBold( aBold );
    }

    // Accessors:
    int GetPenWidth() { return GetThickness(); }
};

/*
 * this class stores the list of graphic items to draw/plot
 * the title block and frame references
 */
class WS_DRAW_ITEM_LIST
{
    std::vector <WS_DRAW_ITEM_BASE*> m_graphicList;     // Items to draw/plot
    unsigned m_idx;             // for GetFirst, GetNext functions
    wxPoint  m_LTmargin;        // The left top margin in mils of the page layout.
    wxPoint  m_RBmargin;        // The right bottom margin in mils of the page layout.
    wxSize   m_pageSize;        // the page size in mils
    double   m_milsToIu;        // the scalar to convert pages units ( mils)
                                // to draw/plot units.
    int      m_penSize;         // The line width for drawings.

public:
    WS_DRAW_ITEM_LIST()
    {
        m_idx = 0;
        m_milsToIu = 1.0;
        m_penSize = 1;
    }

    ~WS_DRAW_ITEM_LIST()
    {
        for( unsigned ii = 0; ii < m_graphicList.size(); ii++ )
            delete m_graphicList[ii];
    }

    /* Function SetPenSize
     * Set the defualt pen size to draw/plot lines and texts
     * @param aPenSize the thickness of lines
     */
    void SetPenSize( int aPenSize )
    {
        m_penSize = aPenSize;
    }

    /* Function SetMilsToIUfactor
     * Set the scalar to convert pages units ( mils) to draw/plot units
     * @param aScale the conversion factor
     */
    void SetMilsToIUfactor( double aScale )
    {
        m_milsToIu = aScale;
    }

    /* Function SetPageSize
     * Set the size of the page layout
     * @param aPageSize size (in mils) of the page layout.
     */
    void SetPageSize( const wxSize& aPageSize )
    {
        m_pageSize = aPageSize;
    }

    /* Function SetMargins
     * Set the left top margin and the right bottom margin
     * of the page layout
     * @param aLTmargin The left top margin of the page layout.
     * @param aRBmargin The right bottom margin of the page layout.
     */
    void SetMargins( const wxPoint& aLTmargin, const wxPoint& aRBmargin )
    {
        m_LTmargin = aLTmargin;
        m_RBmargin = aRBmargin;
    }

    void Append( WS_DRAW_ITEM_BASE* aItem )
    {
        m_graphicList.push_back( aItem );
    }

    WS_DRAW_ITEM_BASE* GetFirst()
    {
        m_idx = 0;

        if( m_graphicList.size() )
            return m_graphicList[0];
        else
            return NULL;
    }

    WS_DRAW_ITEM_BASE* GetNext()
    {
        m_idx++;

        if( m_graphicList.size() > m_idx )
            return m_graphicList[m_idx];
        else
            return NULL;
    }

    /**
     * Function BuildWorkSheetGraphicList is a core function for
     * drawing or plotting the page layout with
     * the frame and the basic inscriptions.
     * It fills the list of basic graphic items to draw or plot.
     * currently lines, rect, polygons and texts
     *
     * @param aPaperFormat The paper size type, for basic inscriptions.
     * @param aFileName The file name, for basic inscriptions.
     * @param aTitleBlock The sheet title block, for basic inscriptions.
     * @param aSheetCount The number of sheets (for basic inscriptions).
     * @param aSheetNumber The sheet number (for basic inscriptions).
     * @param aLineColor The color for drawing and fixed text.
     * @param aTextColor The color for user inscriptions.
     */
    void BuildWorkSheetGraphicList( const wxString& aPaperFormat,
                                    const wxString& aFileName,
                                    const wxString& aSheetPathHumanReadable,
                                    const TITLE_BLOCK& aTitleBlock,
                                    int aSheetCount, int aSheetNumber,
                                    EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor );
};


#endif      // WORKSHEET_SHAPE_BUILDER_H
