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

class ZONE_CONTAINER : public BOARD_ITEM
{
public:
    enum m_PadInZone {              // How pads are covered by copper in zone
        PAD_NOT_IN_ZONE,            // Pads are not covered
        THERMAL_PAD,                // Use thermal relief for pads
        PAD_IN_ZONE                 // pads are covered by copper
    };
    
    wxString    m_Netname;          // Net Name
    CPolyLine*  m_Poly;             // outlines
    int         m_CornerSelection;  // For corner moving, corner index to drag, or -1 if no selection
    int         m_ZoneClearance;    // clearance value
    int         m_GridFillValue;    // Grid used for filling
    m_PadInZone m_PadOption;        // see m_PadInZone
    int         utility, utility2;  // flags used in polygon calculations

private:
    int         m_NetCode;          // Net number for fast comparisons

public:
    ZONE_CONTAINER( BOARD * parent );
    ~ZONE_CONTAINER();

    bool    Save( FILE* aFile ) const;
    int     ReadDescr( FILE* aFile, int* aLineNum = NULL );

    wxPoint& GetPosition()
    {
        static wxPoint pos; 
        return pos;
    }

    void UnLink( void )
    {
    };

    /** 
     * Function copy
     * copy usefull data from the source.
     * flags and linked list pointers are NOT copied
     */
    void    Copy( ZONE_CONTAINER* src );

    void    Display_Infos( WinEDA_DrawFrame* frame );

    /**
     * Function Draw
     * Draws the zone outline.
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param offset = Draw offset (usually wxPoint(0,0))
     * @param draw_mode = draw mode: OR, XOR ..
     */
    void    Draw( WinEDA_DrawPanel* panel, wxDC* DC,
                  const wxPoint& offset, int draw_mode );


    /**
     * Function DrawWhileCreateOutline
     * Draws the zone outline when ir is created.
	 * The moving edges are in XOR graphic mode, old segment in draw_mode graphic mode (usually GR_OR)
	 * The closing edge has its owm shape
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param draw_mode = draw mode: OR, XOR ..
     */
    void    DrawWhileCreateOutline( WinEDA_DrawPanel* panel, wxDC* DC, int draw_mode = GR_OR );


    int GetNet( void ) const
    {
        return m_NetCode;
    }
    void    SetNet( int anet_code );

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool    HitTest( const wxPoint& refPos );

    /**
     * Function HitTestForCorner
     * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
     * @return -1 if none, corner index in .corner <vector>
     * @param refPos : A wxPoint to test
     */
    int     HitTestForCorner( const wxPoint& refPos );

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint near a corner, or near the segment define by 2 corners.
     * @return -1 if none,  or index of the starting corner in .corner <vector>
     * @param refPos : A wxPoint to test
     */
    int     HitTestForEdge( const wxPoint& refPos );

    /**
     * Function HitTest (overlayed)
     * tests if the given EDA_Rect contains the bounds of this object.
     * @param refArea : the given EDA_Rect
     * @return bool - true if a hit, else false
     */
    bool    HitTest( EDA_Rect& refArea );

    /**
     * Function Fill_Zone()
     * Calculate the zone filling
     * The zone outline is a frontier, and can be complex (with holes)
     * The filling starts from starting points like pads, tracks.
     * If exists the old filling is removed
     * @param frame = reference to the main frame
     * @param DC = current Device Context
     * @param verbose = true to show error messages
     * @return error level (0 = no error)
     */
    int     Fill_Zone( WinEDA_PcbFrame* frame, wxDC* DC, bool verbose = TRUE );

    /* Geometric transformations: */

    /**
     * Function Move
     * Move the outlines
     * @param offset = moving vector
     */
    void    Move( const wxPoint& offset );

    /**
     * Function Move
     * Move the outlines
     * @param centre = rot centre
     * @param angle = in 0.1 degree
     */
    void    Rotate( const wxPoint& centre, int angle );

    /**
     * Function Mirror
     * Mirror the outlines , relative to a given horizontal axis
     * the layer is not changed
     * @param mirror_ref = vertical axis position
     */
    void    Mirror( const wxPoint& mirror_ref );
    
    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "ZONE_CONTAINER" );
    }
	
	/** Acces to m_Poly parameters
	*/
	
	int GetNumCorners(void)
	{
		return m_Poly->GetNumCorners();
	}
	
	void RemoveAllContours(void)
	{
		m_Poly->RemoveAllContours();
	}
	
	wxPoint GetCornerPosition(int aCornerIndex)
	{
		return wxPoint(m_Poly->GetX(aCornerIndex), m_Poly->GetY(aCornerIndex));
	}
	
	void SetCornerPosition(int aCornerIndex, wxPoint new_pos)
	{
		m_Poly->SetX(aCornerIndex, new_pos.x);
		m_Poly->SetY(aCornerIndex, new_pos.y);
	}
	
	void AppendCorner( wxPoint position )
	{
		m_Poly->AppendCorner( position.x, position.y );
	}
};


#endif  // #ifndef CLASS_ZONE_H
