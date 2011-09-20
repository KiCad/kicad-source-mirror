/**
 * @file class_zone.h
 * @brief Classes to handle copper zones
 */

#ifndef CLASS_ZONE_H
#define CLASS_ZONE_H


#include <vector>
#include "gr_basic.h"
#include "PolyLine.h"
#include "class_zone_setting.h"


class EDA_RECT;
class LINE_READER;
class EDA_DRAW_FRAME;
class EDA_DRAW_PANEL;
class PCB_EDIT_FRAME;
class BOARD;
class BOARD_CONNECTED_ITEM;
class ZONE_CONTAINER;


/* a small class used when filling areas with segments */
class SEGMENT
{
public:
    wxPoint m_Start;        // starting point of a segment
    wxPoint m_End;          // ending point of a segment

public:
    SEGMENT() {}

    SEGMENT( const wxPoint& aStart, const wxPoint& aEnd)
    {
        m_Start = aStart;
        m_End = aEnd;
    }
 };


/**
 * Class ZONE_CONTAINER
 * handles a list of polygons defining a copper zone.
 * A zone is described by a main polygon, a time stamp, a layer, and a net name.
 * Other polygons inside the main polygon are holes in the zone.
 */
class ZONE_CONTAINER : public BOARD_CONNECTED_ITEM
{
public:
    wxString              m_Netname;                        // Net Name
    CPolyLine*            m_Poly;                           // outlines

    // For corner moving, corner index to drag, or -1 if no selection.
    int                   m_CornerSelection;
    int                   m_ZoneClearance;                  // clearance value
    int                   m_ZoneMinThickness;               // Min thickness value in filled areas

    // How to fill areas: 0 = use filled polygons, != 0 fill with segments.
    int                   m_FillMode;

    // number of segments to convert a circle to a polygon (uses
    //ARC_APPROX_SEGMENTS_COUNT_LOW_DEF or ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF)
    int                   m_ArcToSegmentsCount;

    int                   m_PadOption;

    // thickness of the gap in thermal reliefs.
    int                   m_ThermalReliefGapValue;

    // thickness of the copper bridge in thermal reliefs
    int                   m_ThermalReliefCopperBridgeValue;
    int                   utility, utility2;                // flags used in polygon calculations

    // true when a zone was filled, false after deleting the filled areas
    bool                  m_IsFilled;

    /* set of filled polygons used to draw a zone as a filled area.
     * from outlines (m_Poly) but unlike m_Poly these filled polygons have no hole
     * (they are* all in one piece)  In very simple cases m_FilledPolysList is same
     * as m_Poly.  In less simple cases (when m_Poly has holes) m_FilledPolysList is
     * a polygon equivalent to m_Poly, without holes but with extra outline segment
     * connecting "holes" with external main outline.  In complex cases an outline
     * described by m_Poly can have many filled areas
     */
    std::vector <CPolyPt> m_FilledPolysList;

    /* set of segments used to fill area, when fill zone by segment is used.
     *  ( m_FillMode == 1 )
     *  in this case segments have m_ZoneMinThickness width
     */
    std::vector <SEGMENT> m_FillSegmList;

private:
    CPolyLine*            smoothedPoly;         // Corner-smoothed version of m_Poly
    int                   cornerSmoothingType;
    unsigned int          cornerRadius;

public:
    ZONE_CONTAINER( BOARD* parent );

    ~ZONE_CONTAINER();

    bool Save( FILE* aFile ) const;

     /**
     * Function ReadDescr
     * reads the data structures for this object from a LINE_READER in "*.brd" format.
     * @param aReader is a pointer to a LINE_READER to read from.
     * @return int - 1 if success, 0 if not.
     */
    int ReadDescr( LINE_READER* aReader );

    /**
     * Function GetPosition
     * @return a wxPoint, position of the first point of the outline
     */
    wxPoint& GetPosition();

    /**
     * Function copy
     * copy useful data from the source.
     * flags and linked list pointers are NOT copied
     */
    void Copy( ZONE_CONTAINER* src );

    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function Draw
     * Draws the zone outline.
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
     * @param offset = Draw offset (usually wxPoint(0,0))
     */
    void Draw( EDA_DRAW_PANEL* panel,
               wxDC*           DC,
               int             aDrawMode,
               const wxPoint&  offset = ZeroOffset );

    /**
     * Function DrawDrawFilledArea
     * Draws the filled  area for this zone (polygon list .m_FilledPolysList)
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param offset = Draw offset (usually wxPoint(0,0))
     * @param aDrawMode = GR_OR, GR_XOR, GR_COPY ..
     */
    void DrawFilledArea( EDA_DRAW_PANEL* panel,
                         wxDC*           DC,
                         int             aDrawMode,
                         const wxPoint&  offset = ZeroOffset );

    /**
     * Function DrawWhileCreateOutline
     * Draws the zone outline when it is created.
     * The moving edges are in XOR graphic mode, old segment in draw_mode graphic mode
     * (usually GR_OR).  The closing edge has its own shape.
     * @param panel = current Draw Panel
     * @param DC = current Device Context
     * @param draw_mode = draw mode: OR, XOR ..
     */
    void DrawWhileCreateOutline( EDA_DRAW_PANEL* panel, wxDC* DC, int draw_mode = GR_OR );


    /* Function GetBoundingBox
     * @return an EDA_RECT that is the bounding box of the zone outline
     */
    EDA_RECT GetBoundingBox() const;

    int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const;

    /**
     * Function Test_For_Copper_Island_And_Remove__Insulated_Islands
     * Remove insulated copper islands found in m_FilledPolysList.
     * @param aPcb = the board to analyze
     */
    void Test_For_Copper_Island_And_Remove_Insulated_Islands( BOARD* aPcb );

    /**
     * Function CalculateSubAreaBoundaryBox
     * Calculates the bounding box of a a filled area ( list of CPolyPt )
     * use m_FilledPolysList as list of CPolyPt (that are the corners of one or more
     * polygons or filled areas )
     * @return an EDA_RECT as bounding box
     * @param aIndexStart = index of the first corner of a polygon (filled area)
     *                      in m_FilledPolysList
     * @param aIndexEnd = index of the last corner of a polygon in m_FilledPolysList
     */
    EDA_RECT CalculateSubAreaBoundaryBox( int aIndexStart, int aIndexEnd );

    /**
     * Function IsOnCopperLayer
     * @return true if this zone is on a copper layer, false if on a technical layer
     */
    bool IsOnCopperLayer( void ) const
    {
        return ( GetLayer() < FIRST_NO_COPPER_LAYER ) ? true : false;
    }

    virtual void SetNet( int anet_code );

    /**
     * Function SetNetNameFromNetCode
     * Find the net name corresponding to the net code.
     * @return bool - true if net found, else false
     */
    bool SetNetNameFromNetCode( void );

    /**
     * Function GetNetName
     * returns the net name.
     * @return wxString - The net name.
     */
    wxString GetNetName() const { return m_Netname; };

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * For zones, this means near an outline segment
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& refPos );

    /**
     * Function HitTestFilledArea
     * tests if the given wxPoint is within the bounds of a filled area of this zone.
     * @param aRefPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTestFilledArea( const wxPoint& aRefPos );

    /**
     * Function BuildFilledPolysListData
     * Build m_FilledPolysList data from real outlines (m_Poly)
     * in order to have drawable (and plottable) filled polygons
     * drawable filled polygons are polygons without hole
     * @param aPcb: the current board (can be NULL for non copper zones)
     * @return number of polygons
     * This function does not add holes for pads and tracks but calls
     * AddClearanceAreasPolygonsToPolysList() to do that for copper layers
     */
    int BuildFilledPolysListData( BOARD* aPcb );

    /**
     * Function AddClearanceAreasPolygonsToPolysList
     * Add non copper areas polygons (pads and tracks with clearance)
     * to a filled copper area
     * used in BuildFilledPolysListData when calculating filled areas in a zone
     * Non copper areas are pads and track and their clearance area
     * The filled copper area must be computed before
     * BuildFilledPolysListData() call this function just after creating the
     *  filled copper area polygon (without clearance areas
     * @param aPcb: the current board
     */
    void AddClearanceAreasPolygonsToPolysList( BOARD* aPcb );

    /**
     * Function CopyPolygonsFromBoolengineToFilledPolysList
     * Copy (Add) polygons created by kbool (after Do_Operation) to m_FilledPolysList
     * @param aBoolengine = the kbool engine used in Do_Operation
     * @return the corner count
     */
    int CopyPolygonsFromBoolengineToFilledPolysList( Bool_Engine* aBoolengine );

    /**
     * Function CopyPolygonsFromFilledPolysListToBoolengine
     * Copy (Add) polygons created by kbool (after Do_Operation) to m_FilledPolysList
     * @param aBoolengine = kbool engine
     * @param aGroup = group in kbool engine (GROUP_A or GROUP_B only)
     * @return the corner count
     */
    int CopyPolygonsFromFilledPolysListToBoolengine( Bool_Engine* aBoolengine,
                                                     GroupType    aGroup = GROUP_A );

    /**
     * Function HitTestForCorner
     * tests if the given wxPoint near a corner
     * Set m_CornerSelection to -1 if nothing found, or index of corner
     * @return true if found
     * @param refPos : A wxPoint to test
     */
    bool HitTestForCorner( const wxPoint& refPos );

    /**
     * Function HitTestForEdge
     * tests if the given wxPoint is near a segment defined by 2 corners.
     * Set m_CornerSelection to -1 if nothing found, or index of the starting corner of vertice
     * @return true if found
     * @param refPos : A wxPoint to test
     */
    bool HitTestForEdge( const wxPoint& refPos );

    /**
     * Function HitTest (overloaded)
     * tests if the given EDA_RECT contains the bounds of this object.
     * @param refArea : the given EDA_RECT
     * @return bool - true if a hit, else false
     */
    bool HitTest( EDA_RECT& refArea );

    /**
     * Function Fill_Zone
     * Calculate the zone filling
     * The zone outline is a frontier, and can be complex (with holes)
     * The filling starts from starting points like pads, tracks.
     * If exists the old filling is removed
     * @param frame = reference to the main frame
     * @param DC = current Device Context
     * @param verbose = true to show error messages
     * @return error level (0 = no error)
     */
    int Fill_Zone( PCB_EDIT_FRAME* frame, wxDC* DC, bool verbose = true );

    /**
     * Function Fill_Zone_Areas_With_Segments
     *  Fill sub areas in a zone with segments with m_ZoneMinThickness width
     * A scan is made line per line, on the whole filled areas, with a step of m_ZoneMinThickness.
     * all intersecting points with the horizontal infinite line and polygons to fill are calculated
     * a list of SEGZONE items is built, line per line
     * @return number of segments created
     */
    int Fill_Zone_Areas_With_Segments();

    /**
     * Function UnFill
     * Removes the zone filling
     * @return true if a previous filling is removed, false if no change
     * (when no filling found)
     */
    bool UnFill();

    /* Geometric transformations: */

    /**
     * Function Move
     * Move the outlines
     * @param offset = moving vector
     */
    void Move( const wxPoint& offset );

    /**
     * Function MoveEdge
     * Move the outline Edge. m_CornerSelection is the start point of the outline edge
     * @param offset = moving vector
     */
    void MoveEdge( const wxPoint& offset );

    /**
     * Function Rotate
     * Move the outlines
     * @param centre = rot centre
     * @param angle = in 0.1 degree
     */
    void Rotate( const wxPoint& centre, int angle );

    /**
     * Function Flip
     * Flip this object, i.e. change the board side for this object
     * (like Mirror() but changes layer)
     * @param aCentre - the rotation point.
     */
    virtual void Flip( const wxPoint& aCentre );

    /**
     * Function Mirror
     * Mirror the outlines , relative to a given horizontal axis
     * the layer is not changed
     * @param mirror_ref = vertical axis position
     */
    void Mirror( const wxPoint& mirror_ref );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    wxString GetClass() const
    {
        return wxT( "ZONE_CONTAINER" );
    }


    /** Access to m_Poly parameters
     */

    int GetNumCorners( void ) const
    {
        return m_Poly->GetNumCorners();
    }


    void RemoveAllContours( void )
    {
        m_Poly->RemoveAllContours();
    }


    wxPoint GetCornerPosition( int aCornerIndex ) const
    {
        return wxPoint( m_Poly->GetX( aCornerIndex ), m_Poly->GetY( aCornerIndex ) );
    }


    void SetCornerPosition( int aCornerIndex, wxPoint new_pos )
    {
        m_Poly->SetX( aCornerIndex, new_pos.x );
        m_Poly->SetY( aCornerIndex, new_pos.y );
    }


    void AppendCorner( wxPoint position )
    {
        m_Poly->AppendCorner( position.x, position.y );
    }


    int GetHatchStyle() const
    {
        return m_Poly->GetHatchStyle();
    }

    /**
     * Function IsSame
     * test is 2 zones are equivalent:
     * 2 zones are equivalent if they have same parameters and same outlines
     * info relative to filling is not take in account
     * @param aZoneToCompare = zone to compare with "this"
     */
    bool IsSame( const ZONE_CONTAINER &aZoneToCompare);

    /**
     * Function GetSmoothedPoly
     * returns a pointer to the corner-smoothed version of
     * m_Poly if it exists, otherwise it returns m_Poly.
     * @return CPolyLine* - pointer to the polygon.
     */
    CPolyLine* GetSmoothedPoly() const
    {
        if( smoothedPoly )
            return smoothedPoly;
        else
            return m_Poly;
    };

    void SetCornerSmoothingType( int aType ) { cornerSmoothingType = aType; };

    int  GetCornerSmoothingType() const { return cornerSmoothingType; };

    void SetCornerRadius( unsigned int aRadius )
    {
        if( aRadius > MAX_ZONE_CORNER_RADIUS )
            cornerRadius = MAX_ZONE_CORNER_RADIUS;
        else if( aRadius < 0 )
            cornerRadius = 0;
        else
            cornerRadius = aRadius;
    };

    unsigned int GetCornerRadius() const { return cornerRadius; };

    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return  add_zone_xpm; }
};


#endif  // #ifndef CLASS_ZONE_H
