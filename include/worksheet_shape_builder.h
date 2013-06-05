/**
 * @file worksheet_shape_builder.h
 * @brief classes and function to generate graphics to plt or draw titles blocks
 * and frame references
 */

#ifndef  WORKSHEET_SHAPE_BUILDER_H
#define  WORKSHEET_SHAPE_BUILDER_H

#include <vector2d.h>
#include <eda_text.h>

class WS_DRAW_ITEM_TEXT;        // Forward declaration

#define TB_DEFAULT_TEXTSIZE             1.5  // default worksheet text size in mm

// Text attributes set in m_Flags (ORed bits)
 #define USE_BOLD 1             // has meaning for texts
 #define USE_THICK_LINE 1       // equivalent to bold for lines
 #define USE_ITALIC 2           // has meaning for texts
 #define USE_TEXT_COLOR 4
 #define SET_UPPER_LIMIT 8      // Flag used to calculate variable position items

// A coordinate is relative to a page corner.
// Any of the 4 corners can be a reference.
// The default is the right bottom corner
enum corner_anchor
{
    RB_CORNER,      // right bottom corner
    RT_CORNER,      // right top corner
    LB_CORNER,      // left bottom corner
    LT_CORNER,      // left top corner
};

// a coordinate point
// The position is always relative to the corner anchor
// Note the coordinate is from the anchor point
// to the opposite corner.
class POINT_COORD
{
public:
    DPOINT            m_Pos;
    int               m_Anchor;
public:
    POINT_COORD() { m_Anchor = RB_CORNER; }
    POINT_COORD( DPOINT aPos, enum corner_anchor aAnchor = RB_CORNER )
    {
        m_Pos = aPos;
        m_Anchor = aAnchor;
    }
};


// Work sheet structure type definitions.
class WORKSHEET_DATAITEM
{
public:
    enum WS_ItemType {
        WS_TEXT,
        WS_SEGMENT,
        WS_RECT
    };
    WS_ItemType       m_Type;
    POINT_COORD       m_Pos;
    POINT_COORD       m_End;
    double            m_LineWidth;
    int               m_Flags;
    int               m_RepeatCount;        // repeat count for duplicate items
    DPOINT            m_IncrementVector;    // For duplicate items: move vector
                                            // for position increment
    int               m_IncrementLabel;

    static double     m_WSunits2Iu;         // conversion factor between
                                            // ws units (mils) and draw/plot units
    static DPOINT     m_RB_Corner;          // cordinates of the right bottom corner
                                            // (ws units)
    static DPOINT     m_LT_Corner;          // cordinates of the left top corner
                                            // (ws units)

public:
    WORKSHEET_DATAITEM( WS_ItemType aType )
    {
        m_Type = aType;
        m_Flags = 0;
        m_RepeatCount = 1;
        m_IncrementLabel = 0;
        m_LineWidth = 0.0;
    }

    void SetStart( double aPosx, double aPosy, enum corner_anchor aAnchor = RB_CORNER )
    {
        m_Pos.m_Pos.x = aPosx;
        m_Pos.m_Pos.y = aPosy;
        m_Pos.m_Anchor = aAnchor;
    }

    void SetEnd( double aPosx, double aPosy, enum corner_anchor aAnchor = RB_CORNER )
    {
        m_End.m_Pos.x = aPosx;
        m_End.m_Pos.y = aPosy;
        m_End.m_Anchor = aAnchor;
    }

    const wxPoint GetStartPosUi( int ii = 0 ) const;
    const wxPoint GetEndPosUi( int ii = 0 ) const;
    const DPOINT GetStartPos( int ii = 0 ) const;
    const DPOINT GetEndPos( int ii = 0 ) const;
    bool IsInsidePage( int ii ) const;
    int GetPenSizeUi() {return KiROUND( m_LineWidth * m_WSunits2Iu ); }
};

class WORKSHEET_DATAITEM_TEXT : public WORKSHEET_DATAITEM
{
public:
    wxString          m_TextBase;           // The basic text, with format symbols
    wxString          m_FullText;           // The expanded text, shown on screen
    int               m_IncrementLabel;
    double            m_Orient;             //  Orientation in degrees
    enum EDA_TEXT_HJUSTIFY_T m_Hjustify;
    enum EDA_TEXT_VJUSTIFY_T m_Vjustify;
    DSIZE             m_TextSize;
    DSIZE             m_BoundingBoxSize;    // When not null, this is the max
                                            // size of the full text.
                                            // the text size will be modified
                                            // to keep the full text insite this
                                            // bound.
    DSIZE             m_ConstrainedTextSize;// Actual text size, if constrained by
                                            // the m_BoundingBoxSize constraint

public:
    WORKSHEET_DATAITEM_TEXT( const wxChar* aTextBase );

    /**
     * transfert the text justification and orientation
     * to aGText
     */
    void TransfertSetupToGraphicText(  WS_DRAW_ITEM_TEXT* aGText );

    /**
     * Try to build text wihich is an increment of m_TextBase
     * has meaning only if m_TextBase is a basic text (one char)
     * If the basic char is a digit, build a number
     * If the basic char is a letter, use the letter with ascii code
     * aIncr + (basic char ascc code)
     * @param aIncr = the increment value
     * return the incremented label in m_FullText
     */
    void IncrementLabel( int aIncr );

    /**
     * Calculates m_ConstrainedTextSize from m_TextSize
     * to keep the X size and the full Y size of the text
     * smaller than m_BoundingBoxSize
     * if m_BoundingBoxSize.x or m_BoundingBoxSize.y > 0
     * if m_BoundingBoxSize.x or m_BoundingBoxSize.y == 0
     * the corresponding text size is not constrained
     */
    void SetConstrainedTextSize();
};

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
 * this class stores the list of graphic items:
 * rect, lines, polygons and texts to draw/plot
 * the title block and frame references, and parameters to
 * draw/plot them
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
    int      m_penSize;         // The default line width for drawings.
                                // used when an item has a pen size = 0
    int      m_sheetNumber;     // the value of the sheet number, for basic inscriptions
    int      m_sheetCount;      // the value of the number of sheets, in schematic
                                // for basic inscriptions, in schematic
    const TITLE_BLOCK* m_titleBlock;    // for basic inscriptions
    const wxString* m_paperFormat;      // for basic inscriptions
    const wxString* m_fileName;         // for basic inscriptions
    const wxString* m_sheetFullName;    // for basic inscriptions


public:
    WS_DRAW_ITEM_LIST()
    {
        m_idx = 0;
        m_milsToIu = 1.0;
        m_penSize = 1;
        m_sheetNumber = 1;
        m_sheetCount = 1;
        m_titleBlock = NULL;
        m_paperFormat = NULL;
        m_fileName = NULL;
        m_sheetFullName = NULL;
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

    /**
     * Function SetSheetNumber
     * Set the value of the sheet number, for basic inscriptions
     * @param aSheetNumber the number to display.
     */
    void SetSheetNumber( int aSheetNumber )
    {
        m_sheetNumber = aSheetNumber;
    }

    /**
     * Function SetSheetCount
     * Set the value of the count of sheets, for basic inscriptions
     * @param aSheetCount the number of esheets to display.
     */
    void SetSheetCount( int aSheetCount )
    {
        m_sheetCount = aSheetCount;
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
     * @param aSheetPathHumanReadable The human readable sheet path.
     * @param aTitleBlock The sheet title block, for basic inscriptions.
     * @param aLineColor The color for drawing and fixed text.
     * @param aTextColor The color for user inscriptions.
     */
    void BuildWorkSheetGraphicList( const wxString& aPaperFormat,
                                    const wxString& aFileName,
                                    const wxString& aSheetPathHumanReadable,
                                    const TITLE_BLOCK& aTitleBlock,
                                    EDA_COLOR_T aLineColor, EDA_COLOR_T aTextColor );
    /**
     * Function BuildFullText
     * returns the full text corresponding to the aTextbase,
     * after replacing format symbols by the corresponding value
     *
     * Basic texts in Ki_WorkSheetData struct use format notation
     * like "Title %T" to identify at run time the full text
     * to display.
     * Currently format identifier is % followed by a letter or 2 letters
     *
     * %% = replaced by %
     * %K = Kicad version
     * %Z = paper format name (A4, USLetter)
     * %Y = company name
     * %D = date
     * %R = revision
     * %S = sheet number
     * %N = number of sheets
     * %Cx = comment (x = 0 to 9 to identify the comment)
     * %F = filename
     * %P = sheet path or sheet full name
     * %T = title
     * Other fields like Developer, Verifier, Approver could use %Cx
     * and are seen as comments for format
     *
     * @param aTextbase = the text with format symbols
     * @return the text, after replacing the format symbols by the actual value
     */
    wxString BuildFullText( const wxString& aTextbase );
};


/**
 * WORKSHEET_LAYOUT handles the grpahic items list to draw/plot
 * the title block and other items (page references ...
 */
class WORKSHEET_LAYOUT
{
    std::vector <WORKSHEET_DATAITEM*> m_list;

public:
    WORKSHEET_LAYOUT() {};
    ~WORKSHEET_LAYOUT() {ClearList(); }

    void ClearList()
    {
        for( unsigned ii = 0; ii < m_list.size(); ii++ )
            delete m_list[ii];
    }

    /**
     * Add an item to the list of items
     */
    void Append( WORKSHEET_DATAITEM* aItem )
    {
        m_list.push_back( aItem );
    }

    /**
     * @return the item from its index aIdx, or NULL if does not exist
     */
    WORKSHEET_DATAITEM* GetItem( unsigned aIdx ) const
    {
        if( aIdx < m_list.size() )
            return m_list[aIdx];
        else
            return NULL;
    }

    /**
     * @return the item count
     */
    unsigned GetCount() const { return m_list.size(); }

    /**
     * Fills the list with the default layout shape
     */
    void SetDefaultLayout();
};

#endif      // WORKSHEET_SHAPE_BUILDER_H
