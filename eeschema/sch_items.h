/************************************************************************/
/* classes to handle items used in schematic: wires, bus, junctions ... */
/************************************************************************/

#ifndef CLASS_SCHEMATIC_ITEMS_H
#define CLASS_SCHEMATIC_ITEMS_H


#include "sch_item_struct.h"

#include "general.h"


/* Flags for BUS ENTRY (bus to bus or wire to bus */
#define WIRE_TO_BUS 0
#define BUS_TO_BUS  1


/**
 * Class SCH_LINE
 * is a segment description base class to describe items which have 2 end
 * points (track, wire, draw line ...)
 */
class SCH_LINE : public SCH_ITEM
{
public:
    int     m_Width;            // 0 = line, > 0 = tracks, bus ...
    wxPoint m_Start;            // Line start point
    wxPoint m_End;              // Line end point

    bool    m_StartIsDangling;
    bool    m_EndIsDangling;    // TRUE if not connected  (wires, tracks...)

public:
    SCH_LINE( const wxPoint& pos = wxPoint( 0, 0 ), int layer = LAYER_NOTES );
    ~SCH_LINE() { }

    SCH_LINE* Next() const { return (SCH_LINE*) Pnext; }
    SCH_LINE* Back() const { return (SCH_LINE*) Pback; }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_LINE" );
    }

    bool IsEndPoint( const wxPoint& aPoint ) const
    {
        return aPoint == m_Start || aPoint == m_End;
    }

    SCH_LINE* GenCopy();

    bool IsNull() const { return m_Start == m_End; }

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display purposes.
     * This box should be an enclosing perimeter for visible components of this
     * object, and the units should be in the pcb or schematic coordinate system.
     * It is OK to overestimate the size by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic line from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic line from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic line.
     * @return True if the schematic line loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector );

    virtual void Mirror_X( int aXaxis_position );

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );

    virtual void Rotate( wxPoint rotationPoint );

    /**
     * Check line against \a aLine to see if it overlaps and merge if it does.
     *
     * This method will change the line to be equivalent of the line and \a aLine if the
     * two lines overlap.  This method is used to merge multple line segments into a single
     * line.
     *
     * @param aLine - Line to compare.
     * @return True if lines overlap and the line was merged with \a aLine.
     */
    bool MergeOverlap( SCH_LINE* aLine );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList );

    virtual bool IsDangling() const { return m_StartIsDangling || m_EndIsDangling; }

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

#if defined(DEBUG)
    void Show( int nestLevel, std::ostream& os ) const;
#endif

private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
    virtual bool DoIsConnected( const wxPoint& aPosition ) const;
};


class SCH_NO_CONNECT : public SCH_ITEM
{
public:
    wxPoint m_Pos;                      /* XY coordinates of NoConnect. */
    wxSize  m_Size;                     // size of this symbol

public:
    SCH_NO_CONNECT( const wxPoint& pos = wxPoint( 0, 0 ) );
    ~SCH_NO_CONNECT() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_NO_CONNECT" );
    }

    SCH_NO_CONNECT* GenCopy();

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& affset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic no connect entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic no connect from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic no connect.
     * @return True if the schematic no connect loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }

    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
};


/**
 * Class SCH_BUS_ENTRY
 *
 * Defines a bus or wire entry.
 */
class SCH_BUS_ENTRY : public SCH_ITEM
{
public:
    int     m_Width;
    wxPoint m_Pos;
    wxSize  m_Size;

public:
    SCH_BUS_ENTRY( const wxPoint& pos = wxPoint( 0, 0 ), int shape = '\\', int id = WIRE_TO_BUS );
    ~SCH_BUS_ENTRY() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_BUS_ENTRY" );
    }


    SCH_BUS_ENTRY* GenCopy();

    wxPoint m_End() const;

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic bus entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic bus entry from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic bus entry.
     * @return True if the schematic bus entry loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
};

class SCH_POLYLINE : public SCH_ITEM
{
public:
    int m_Width;                            /* Thickness */
    std::vector<wxPoint> m_PolyPoints;      // list of points (>= 2)

public:
    SCH_POLYLINE( int layer = LAYER_NOTES );
    ~SCH_POLYLINE();

    virtual wxString GetClass() const
    {
        return wxT( "SCH_POLYLINE" );
    }


    SCH_POLYLINE* GenCopy();

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic poly line entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic poly line from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic poly line.
     * @return True if the schematic poly line loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    /**
     * Function AddPoint
     * add a corner to m_PolyPoints
     */
    void AddPoint( const wxPoint& point )
    {
        m_PolyPoints.push_back( point );
    }


    /**
     * Function GetCornerCount
     * @return the number of corners
     */

    unsigned GetCornerCount() const { return m_PolyPoints.size(); }

    /**
     * Function GetPenSize
     * @return the size of the "pen" that be used to draw or plot this item
     */
    virtual int GetPenSize() const;

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        for( unsigned ii = 0; ii < GetCornerCount(); ii++ )
            m_PolyPoints[ii] += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
};


class SCH_JUNCTION : public SCH_ITEM
{
public:
    wxPoint m_Pos;                  /* XY coordinates of connection. */
    wxSize  m_Size;

public:
    SCH_JUNCTION( const wxPoint& pos = wxPoint( 0, 0 ) );
    ~SCH_JUNCTION() { }

    virtual wxString GetClass() const
    {
        return wxT( "SCH_JUNCTION" );
    }

    /**
     * Function GetBoundingBox
     * returns the orthogonal, bounding box of this object for display
     * purposes.  This box should be an enclosing perimeter for visible
     * components of this object, and the units should be in the pcb or
     * schematic coordinate system.  It is OK to overestimate the size
     * by a few counts.
     */
    EDA_Rect GetBoundingBox() const;

    SCH_JUNCTION* GenCopy();

    virtual void Draw( WinEDA_DrawPanel* aPanel, wxDC* aDC, const wxPoint& aOffset,
                       int aDrawMode, int aColor = -1 );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.sch"
     * format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;

    /**
     * Load schematic junction entry from \a aLine in a .sch file.
     *
     * @param aLine - Essentially this is file to read schematic junction from.
     * @param aErrorMsg - Description of the error if an error occurs while loading the
     *                    schematic junction.
     * @return True if the schematic junction loaded successfully.
     */
    virtual bool Load( LINE_READER& aLine, wxString& aErrorMsg );

    // Geometric transforms (used in block operations):

    /** virtual function Move
     * move item to a new position.
     * @param aMoveVector = the displacement vector
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    /** virtual function Mirror_Y
     * mirror item relative to an Y axis
     * @param aYaxis_position = the y axis position
     */
    virtual void Mirror_Y( int aYaxis_position );
    virtual void Mirror_X( int aXaxis_position );
    virtual void Rotate( wxPoint rotationPoint );

    virtual void GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList );

    virtual bool IsSelectStateChanged( const wxRect& aRect );

    virtual void GetConnectionPoints( vector< wxPoint >& aPoints ) const;

#if defined(DEBUG)
    void         Show( int nestLevel, std::ostream& os );

#endif

private:
    virtual bool DoHitTest( const wxPoint& aPoint, int aAccuracy, SCH_FILTER_T aFilter ) const;
    virtual bool DoHitTest( const EDA_Rect& aRect, bool aContained, int aAccuracy ) const;
    virtual bool DoIsConnected( const wxPoint& aPosition ) const;
};


#endif /* CLASS_SCHEMATIC_ITEMS_H */
