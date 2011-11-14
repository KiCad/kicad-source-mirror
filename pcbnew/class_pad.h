/**
 * @file class_pad.h
 * @brief Pad object description
 */

#ifndef _PAD_H_
#define _PAD_H_


#include "class_board_item.h"
#include "pad_shapes.h"
#include "PolyLine.h"

#include "lengthpcb.h"


class LINE_READER;
class EDA_3D_CANVAS;
class EDA_DRAW_PANEL;
class MODULE;


/* Default layers used for pads, according to the pad type.
 * this is default values only, they can be changed for a given pad
 */

// PAD_STANDARD:
#define PAD_STANDARD_DEFAULT_LAYERS ALL_CU_LAYERS | SILKSCREEN_LAYER_FRONT | \
    SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT

// PAD_CONN:
#define PAD_CONN_DEFAULT_LAYERS LAYER_FRONT | SOLDERPASTE_LAYER_FRONT | SOLDERMASK_LAYER_FRONT

// PAD_SMD:
#define PAD_SMD_DEFAULT_LAYERS LAYER_FRONT | SOLDERMASK_LAYER_FRONT

//PAD_HOLE_NOT_PLATED:
#define PAD_HOLE_NOT_PLATED_DEFAULT_LAYERS ALL_CU_LAYERS | SILKSCREEN_LAYER_FRONT | \
    SOLDERMASK_LAYER_BACK | SOLDERMASK_LAYER_FRONT


// Helper class to store parameters used to draw a pad
class PAD_DRAWINFO
{
public:
    EDA_DRAW_PANEL* m_DrawPanel;  // the EDA_DRAW_PANEL used to draw a PAD ; can be null
    int m_DrawMode;               // the draw mode
    int m_Color;                  // color used to draw the pad shape , from pad layers and
                                  // visible layers
    int m_HoleColor;              // color used to draw the pad hole
    int m_NPHoleColor;            // color used to draw a pad Not Plated hole
    int m_PadClearance;           // clearance value, used to draw the pad area outlines
    wxSize m_Mask_margin;         // margin, used to draw solder paste when only one layer is shown
    bool m_Display_padnum;        // true to show pad number
    bool m_Display_netname;       // true to show net name
    bool m_ShowPadFilled;         // true to show pad as solid area, false to show pas in
                                  // sketch mode
    bool m_ShowNCMark;            // true to show pad not connected mark
    bool m_ShowNotPlatedHole;     // true when the pad hole in not plated, to draw a specific
                                  // pad shape
    bool m_IsPrinting;            // true to print, false to display on screen.
    wxPoint m_Offset;             // general draw offset

    PAD_DRAWINFO();
};


class D_PAD : public BOARD_CONNECTED_ITEM
{
private:
    wxString m_Netname;         // Full net name like /mysheet/mysubsheet/vout used by Eeschema
    wxString m_ShortNetname;    // short net name, like vout from /mysheet/mysubsheet/vout


public:
    wxPoint m_Pos;                  // pad Position on board

    union
    {
        unsigned long m_NumPadName;
        char          m_Padname[4]; /* Pad name (4 char) or a long identifier (used in pad name
                                     * comparisons because  this is faster than string comparison)
                                     */
    };

    int m_layerMask;                // Bitwise layer :1= copper layer, 15= cmp,
                                    // 2..14 = internal layers
                                    // 16 .. 31 = technical layers

    int    m_PadShape;              ///< Shape: PAD_CIRCLE, PAD_RECT, PAD_OVAL, PAD_TRAPEZOID
    int    m_DrillShape;            ///< Shape PAD_CIRCLE, PAD_OVAL
                                    /// @TODO: as m_DrillShape is eqv. to m_Drill.x==.y
                                    /// it would be relatively easy to remove
                                    /// this redundant flag.

    VECTOR_PCB m_Drill;             ///< Drill diam (drill shape = PAD_CIRCLE) or drill size
                                    /// (shape = OVAL) for drill shape = PAD_CIRCLE, drill
                                    /// diam = m_Drill.x

    VECTOR_PCB m_Offset;    /* This parameter is useful only for oblong pads (it can be used for other
                         * shapes, but without any interest).
                         * this is the offset between the pad hole and the pad shape (you must
                         * understand here pad shape = copper area around the hole)
                         * Most of cases, the hole is the center of the shape (m_Offset = 0).
                         * But some board designers use oblong pads with a hole moved to one of the
                         * oblong pad shape ends.
                         * In all cases the pad position is the pad hole.
                         * The physical shape position (used to draw it for instance) is pad
                         * position (m_Pos) + m_Offset.
                         * D_PAD::ReturnShapePos() returns the physical shape position according to
                         * the offset and the pad rotation.*/

    VECTOR_PCB m_Size;                 // X and Y size ( relative to orient 0)

    VECTOR_PCB m_DeltaSize;            // delta on rectangular shapes

    VECTOR_PCB m_Pos0;                 // Initial Pad position (i.e. pas position relative to the
                                    // module anchor, orientation 0

    int     m_ShapeMaxRadius;       // radius of the circle containing the pad shape
    int     m_Attribut;             // NORMAL, PAD_SMD, PAD_CONN, PAD_HOLE_NOT_PLATED
    int     m_Orient;               // in 1/10 degrees
    static int m_PadSketchModePenSize;      // Pen size used to draw pads in sketch mode
                                    // (mode used to print pads on silkscreen layer)

    LENGTH_PCB m_LengthDie;            // Length net from pad to die on chip

    // Local clearance. When null, the module default value is used.
    // when the module default value is null, the netclass value is used
    // Usually the local clearance is null
    LENGTH_PCB m_LocalClearance;

    // Local mask margins: when NULL, the parent footprint design values are used
    LENGTH_PCB m_LocalSolderMaskMargin;            // Local solder mask margin
    LENGTH_PCB m_LocalSolderPasteMargin;           // Local solder paste margin absolute value
    double m_LocalSolderPasteMarginRatio;      // Local solder mask margin ratio value of pad size
                                               // The final margin is the sum of these 2 values

private:
    int m_SubRatsnest;                  // variable used in rats nest computations
                                        // handle subnet (block) number in ratsnest connection

public:
    D_PAD( MODULE* parent );
    D_PAD( D_PAD* pad );
    ~D_PAD();

    void Copy( D_PAD* source );

    D_PAD* Next() { return (D_PAD*) Pnext; }

    /**
     * Function GetNetname
     * @return const wxString * , a pointer to the full netname
     */
    wxString GetNetname() const { return m_Netname; }

    /**
     * Function GetShortNetname
     * @return const wxString * , a pointer to the short netname
     */
    wxString GetShortNetname() const { return m_ShortNetname; }

    /**
     * Function SetNetname
     * @param aNetname: the new netname
     */
    void SetNetname( const wxString& aNetname );

    /**
     * Function GetShape
     * @return the shape of this pad.
     */
    int GetShape() const { return m_PadShape & 0xFF;  }

    /**
     * Function GetPosition
     * returns the position of this object.
     * @return const wxPoint& - The position of this object.
     */
    wxPoint& GetPosition()
    {
        return m_Pos;
    }


    void SetPosition( const wxPoint& aPos )
    {
        m_Pos = aPos;
    }

    /**
     * Function TransformShapeWithClearanceToPolygon
     * Convert the pad shape to a closed polygon
     * Used in filling zones calculations
     * Circles and arcs are approximated by segments
     * @param aCornerBuffer = a buffer to store the polygon
     * @param aClearanceValue = the clearance around the pad
     * @param aCircleToSegmentsCount = the number of segments to approximate a circle
     * @param aCorrectionFactor = the correction to apply to circles radius to keep
     * clearance when the circle is approximated by segment bigger or equal
     * to the real clearance value (usually near from 1.0)
    */
    void TransformShapeWithClearanceToPolygon( std::vector <CPolyPt>& aCornerBuffer,
                                               int aClearanceValue,
                                               int aCircleToSegmentsCount,
                                               double aCorrectionFactor );

     /**
     * Function GetClearance
     * returns the clearance in internal units.  If \a aItem is not NULL then the
     * returned clearance is the greater of this object's clearance and
     * aItem's clearance.  If \a aItem is NULL, then this objects clearance
     * is returned.
     * @param aItem is another BOARD_CONNECTED_ITEM or NULL
     * @return int - the clearance in internal units.
     */
    virtual int GetClearance( BOARD_CONNECTED_ITEM* aItem = NULL ) const;

   // Mask margins handling:

    /**
     * Function GetSolderMaskMargin
     * @return the margin for the solder mask layer
     * usually > 0 (mask shape bigger than pad
     * value is
     * 1 - the local value
     * 2 - if null, the parent footprint value
     * 1 - if null, the global value
     */
    int GetSolderMaskMargin();

    /**
     * Function GetSolderPasteMargin
     * @return the margin for the solder mask layer
     * usually < 0 (mask shape smaller than pad
     * because the margin can be dependent on the pad size, the margin has a x and a y value
     * value is
     * 1 - the local value
     * 2 - if null, the parent footprint value
     * 1 - if null, the global value
     */
    wxSize GetSolderPasteMargin();

    /* Reading and writing data on files */
    int ReadDescr( LINE_READER* aReader );

    /**
     * Function Save
     * writes the data structures for this object out to a FILE in "*.brd" format.
     * @param aFile The FILE to write to.
     * @return bool - true if success writing else false.
     */
    bool Save( FILE* aFile ) const;


    /* drawing functions */
    /**
     * Draw a pad:
     * @param aPanel = the EDA_DRAW_PANEL panel
     * @param aDC = the current device context
     * @param aDrawMode = mode: GR_OR, GR_XOR, GR_AND...
     * @param aOffset = draw offset
     */
    void Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
               int aDrawMode, const wxPoint& aOffset = ZeroOffset );

    void Draw3D( EDA_3D_CANVAS* glcanvas );

    /**
     * Function DrawShape
     * basic function to draw a pad.
     * <p>
     * This function is used by Draw after calculation of parameters (color, ) final
     * orientation transforms are set. It can also be called to draw a pad on any panel
     * even if this panel is not a EDA_DRAW_PANEL for instance on a wxPanel inside the
     * pad editor.
     * </p>
     */
    void DrawShape( EDA_RECT* aClipBox, wxDC* aDC, PAD_DRAWINFO& aDrawInfo );

    /**
     * Function BuildPadPolygon
     * Has meaning only for polygonal pads (trapezoid and rectangular)
     * Build the Corner list of the polygonal shape,
     * depending on shape, extra size (clearance ...) and orientation
     * @param aCoord = a buffer to fill (4 corners).
     * @param aInflateValue = wxSize: the clearance or margin value. value > 0:
     *                        inflate, < 0 deflate
     * @param aRotation = full rotation of the polygon
     */
    void BuildPadPolygon( wxPoint aCoord[4], wxSize aInflateValue, int aRotation ) const;

    /**
     * Function BuildSegmentFromOvalShape
     * Has meaning only for OVAL (and ROUND) pads
     * Build an equivalent segment having the same shape as the OVAL shape,
     * Useful in draw function and in DRC and HitTest functions,
     *  because segments are already well handled by track tests
     * @param aSegStart = the starting point of the equivalent segment relative to the shape
     *                    position.
     * @param aSegEnd = the ending point of the equivalent segment, relative to the shape position
     * @param aRotation = full rotation of the segment
     * @return the width of the segment
     */
    int BuildSegmentFromOvalShape( wxPoint& aSegStart, wxPoint& aSegEnd, int aRotation ) const;

    // others
    void SetPadName( const wxString& name );    // Change pad name

    wxString ReturnStringPadName() const;           // Return pad name as string in a wxString

    void ReturnStringPadName( wxString& text ) const; // Return pad name as string in a buffer

    void ComputeShapeMaxRadius();                           // compute radius

    int GetMaxRadius() const;

    const wxPoint ReturnShapePos();

    /**
     * Function GetNet
     * @return int - the netcode
     */
    int GetSubRatsnest() const { return m_SubRatsnest; }

    void SetSubRatsnest( int aSubRatsnest ) { m_SubRatsnest = aSubRatsnest; }


    /**
     * Function DisplayInfo
     * has knowledge about the frame and how and where to put status information
     * about this object into the frame's message panel.
     * Is virtual from EDA_ITEM.
     * @param frame A EDA_DRAW_FRAME in which to print status information.
     */
    void DisplayInfo( EDA_DRAW_FRAME* frame );

    /**
     * Function IsOnLayer
     * tests to see if this object is on the given layer.  Is virtual so
     * objects like D_PAD, which reside on multiple layers can do their own
     * form of testing.
     * @param aLayer The layer to test for.
     * @return bool - true if on given layer, else false.
     */
    bool IsOnLayer( int aLayer ) const;

    /**
     * Function HitTest
     * tests if the given wxPoint is within the bounds of this object.
     * @param refPos A wxPoint to test
     * @return bool - true if a hit, else false
     */
    bool HitTest( const wxPoint& refPos );

    /**
     * Function GetClass
     * returns the class name.
     * @return wxString
     */
    virtual wxString GetClass() const
    {
        return wxT( "PAD" );
    }

    /**
     * Function GetBoundingBox
     * returns the bounding box of this pad
     * Mainly used to redraw the screen area occupied by the pad
     */
    EDA_RECT GetBoundingBox() const;

    /**
     * Function Compare
     * compares two pads and return 0 if they are equal.
     * @return int - <0 if left less than right, 0 if equal, >0 if left greater than right.
     */
    static int Compare( const D_PAD* padref, const D_PAD* padcmp );

    /**
     * Function Move
     * move this object.
     * @param aMoveVector - the move vector for this object.
     */
    virtual void Move( const wxPoint& aMoveVector )
    {
        m_Pos += aMoveVector;
    }


    virtual wxString GetSelectMenuText() const;

    virtual BITMAP_DEF GetMenuImage() const { return pad_xpm; }

    /**
     * Function ShowPadShape
     * @return the name of the shape
     */
    wxString ShowPadShape() const;

    /**
     * Function ShowPadAttr
     * @return the name of the pad type (attribute) : STD, SMD ...
     */
    wxString ShowPadAttr() const;

#if defined(DEBUG)

    /**
     * Function Show
     * is used to output the object tree, currently for debugging only.
     * @param nestLevel An aid to prettier tree indenting, and is the level
     *                  of nesting of this object within the overall tree.
     * @param os The ostream& to output to.
     */
    virtual void Show( int nestLevel, std::ostream& os );

#endif
};


#endif     // _PAD_H_
