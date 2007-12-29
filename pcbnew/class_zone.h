/**********************************/
/* classes to handle copper zones */
/**********************************/

#ifndef CLASS_ZONE_H
#define CLASS_ZONE_H

#include "PolyLine.h"

/************************/
/* class ZONE_CONTAINER */
/************************/
/* handle a list of polygons delimiting a copper zone
 * a zone is described by a main polygon, a time stamp, a layer and a net name.
 * others polygons inside this main polygon are holes.
*/

class ZONE_CONTAINER : public BOARD_ITEM, public CPolyLine
{
public:
    wxString m_Netname;             /* Net Name */
	int m_CornerSelection;      // For corner moving, corner index to drag, or -1 if no selection

private:
    int     m_NetCode;              // Net number for fast comparisons

public:
	ZONE_CONTAINER(BOARD * parent);
	~ZONE_CONTAINER();

    bool Save( FILE* aFile ) const;
    int  ReadDescr( FILE* aFile, int* aLineNum = NULL );

	wxPoint & GetPosition( ) { static wxPoint pos ;return pos; }
	void UnLink(void) {};

	void Display_Infos( WinEDA_DrawFrame* frame );

	/** Function Draw
	* Draws the zone outline.
	* @param panel = current Draw Panel
	* @param DC = current Device Context
	* @param offset = Draw offset (usually wxPoint(0,0))
	* @param draw_mode = draw mode: OR, XOR ..
	*/
	void Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                   const wxPoint& offset, int draw_mode );
	
	int GetNet( void ) { return m_NetCode; }
	void SetNet( int anet_code ) { m_NetCode = anet_code; }
	/**
	 * Function HitTest
	 * tests if the given wxPoint is within the bounds of this object.
	 * @param refPos A wxPoint to test
	 * @return bool - true if a hit, else false
	 */
	bool HitTest( const wxPoint& refPos );

	/**
	 * Function HitTestForCorner
	 * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
	 * @return -1 if none, corner index in .corner <vector>
	 * @param refPos : A wxPoint to test
	 */
	int HitTestForCorner( const wxPoint& refPos );
	/**
	 * Function HitTestForEdge
	 * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
	 * @return -1 if none,  or index of the starting corner in .corner <vector>
	 * @param refPos : A wxPoint to test
	 */
	int HitTestForEdge( const wxPoint& refPos );
};

/*******************/
/* class EDGE_ZONE */
/*******************/

class EDGE_ZONE : public DRAWSEGMENT
{
public:
    EDGE_ZONE( BOARD_ITEM* StructFather );
    EDGE_ZONE( const EDGE_ZONE& edgezone );
    ~EDGE_ZONE();

    EDGE_ZONE* Next() { return (EDGE_ZONE*) Pnext; }

    EDGE_ZONE* Back() { return (EDGE_ZONE*) Pback; }
    
    
    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */ 
    bool Save( FILE* aFile ) const;
};

#endif	// #ifndef CLASS_ZONE_H
