/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __DRAWING_TOOL_H
#define __DRAWING_TOOL_H

#include <stack>
#include <optional>
#include <tool/tool_menu.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_actions.h>

namespace KIGFX
{
    class VIEW;
    class VIEW_CONTROLS;
}

class BOARD;
class PCB_BASE_EDIT_FRAME;
class PCB_SHAPE;
class POLYGON_GEOM_MANAGER;
class PCB_TUNING_PATTERN;
class STATUS_MIN_MAX_POPUP;


/**
 * Tool responsible for drawing graphical elements like lines, arcs, circles, etc.
 */

class DRAWING_TOOL : public PCB_TOOL_BASE
{
public:
    DRAWING_TOOL();
    ~DRAWING_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///< The possible drawing modes of DRAWING_TOOL
    enum class MODE
    {
        NONE,
        LINE,
        RECTANGLE,
        CIRCLE,
        ARC,
        BEZIER,
        IMAGE,
        TEXT,
        ANCHOR,
        POINT,
        DXF,
        DIMENSION,
        KEEPOUT,
        ZONE,
        GRAPHIC_POLYGON,
        VIA,
        TUNING
    };

    /**
     * Return the current drawing mode of the DRAWING_TOOL or #MODE::NONE if not currently in
     * any drawing mode.
     */
    MODE GetDrawingMode() const;

    /**
     */
    std::vector<BOARD_ITEM*> DrawSpecificationStackup( const VECTOR2I& origin, PCB_LAYER_ID aLayer,
                                                       bool aDrawNow, VECTOR2I* tablesize );

    /**
     */
    int PlaceStackup( const TOOL_EVENT& aEvent );

    /**
     */
    int PlaceTuningPattern( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing a line.
     *
     * After invoking the function it expects the user to click at least two times to determine
     * the origin and the end for a line. If there are more clicks, the line is drawn as a
     * continuous polyline.
     */
    int DrawLine( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing a rectangle.
     *
     * After invoking the function it expects the user to first click on a point that is going
     * to be used as the top-left of the rectangle. The second click determines the bottom-right.
     */
    int DrawRectangle( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing a circle.
     *
     * After invoking the function it expects the user to first click on a point that is going
     * to be used as the center of the circle. The second click determines the circle radius.
     */
    int DrawCircle( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing an arc.
     *
     * After invoking the function it expects the user to first click on a point that is going
     * to be used as the center of the arc. The second click determines the origin and radius,
     * the third one - the angle.
     */
    int DrawArc( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing a bezier curve.
     *
     * An interactive geometry manager will handle adding/editing the control points.
     */
    int DrawBezier( const TOOL_EVENT& aEvent );

    /**
     * Display a dialog that allows one to select a reference image and then decide where to
     * place the image in the editor.
     */
    int PlaceReferenceImage( const TOOL_EVENT& aEvent );

    /**
     * Place a reference 0D point
     */
    int PlacePoint( const TOOL_EVENT& aEvent );

    /**
     * Display a dialog that allows one to input text and its settings and then lets the user
     * decide where to place the text in editor.
     */
    int PlaceText( const TOOL_EVENT& aEvent );

    /*
     * Start interactively drawing a table (rows & columns of TEXTBOXes).
     */
    int DrawTable( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing a dimension.
     *
     * After invoking the function it expects the user to first click on a point that is going
     * to be used as the origin of the dimension.  The second click determines the end and the
     * third click modifies its height.
     */
    int DrawDimension( const TOOL_EVENT& aEvent );

    /**
     * Start interactively drawing a zone.
     *
     * After invoking the function a zone settings dialog is displayed. After confirmation it
     * allows the user to set points that are going to be used as a boundary polygon of the
     * zone.  Double click or clicking on the origin of the boundary polyline finishes the
     * drawing.
     *
     * The event parameter indicates which type of zone to draw:
     *  - ADD      add a new zone/keepout with fresh settings.
     *  - CUTOUT   add a cutout to an existing zone.
     *  - SIMILAR  add a new zone with the same settings as an existing one.
     */
    int DrawZone(  const TOOL_EVENT& aEvent );

    int DrawVia( const TOOL_EVENT& aEvent );

    /**
     * Place a drawing imported from a DXF or SVG file in footprint editor.
     */
    int PlaceImportedGraphics( const TOOL_EVENT& aEvent );

    /**
     * Interactively place a set of @ref BOARD_ITEM.
     * As a list of BOARD_ITEMs can be resource intesive to move around,
     * we can use a reduced set of BOARD_ITEMs for preview purpose only.
     *
     * @param aEvent
     * @param aItems BOARD_ITEMs to add to the board.
     * @param aPreview BOARD_ITEMs only used during placement / preview.
     * @param aLayers   Set of allowed destination when asking the user.
     *                  If set to NULL, the user is not asked and all BOARD_ITEMs remain on
     *                  their layers.
     */
    int InteractivePlaceWithPreview( const TOOL_EVENT& aEvent,
                                     std::vector<BOARD_ITEM*>& aItems,
                                     std::vector<BOARD_ITEM*>& aPreview, LSET* aLayers );


    /**
     * Place the footprint anchor (only in footprint editor).
     */
    int SetAnchor( const TOOL_EVENT& aEvent );

    /**
     * Toggle the horizontal/vertical/45-degree constraint for drawing tools.
     */
    int ToggleHV45Mode( const TOOL_EVENT& toolEvent );

    ///< Set up handlers for various events.
    void setTransitions() override;

    void SetStroke( const STROKE_PARAMS& aStroke, PCB_LAYER_ID aLayer )
    {
        m_layer = aLayer;
        m_stroke = aStroke;
    }

    void UpdateStatusBar() const;

private:
    enum class DRAW_ONE_RESULT
    {
        // The drawing was accepted "normally"
        // E.g. for a poly-line, you might then begin a chained next segment
        ACCEPTED,
        // The drawing was cancelled with no shape accepted.
        CANCELLED,
        // The drawing was reset - no shape was accepted this time,
        // but the tool remains active.
        RESET,
        // A shape was accepted, but the tool should reset for the
        // next one (e.g. no chaining)
        ACCEPTED_AND_RESET,
    };

    /**
     * Start drawing a selected shape (i.e. PCB_SHAPE).
     *
     * @param aGraphic is an object that is going to be used by the tool for drawing. Must be
     *                 already created. The tool deletes the object if it is not added to a BOARD.
     * @param aStartingPoint is a starting point for this new PCB_SHAPE. If it exists the new
     *                       item has its start point set to aStartingPoint, and its settings
     *                       (width, layer) set to the current default values.
     * @return False if the tool was canceled before the origin was set or origin and end are
     *         the same point.
     */
    bool drawShape( const TOOL_EVENT& aTool, PCB_SHAPE** aGraphic,
                    std::optional<VECTOR2D> aStartingPoint,
                    std::stack<PCB_SHAPE*>* aCommittedGraphics );

    /**
     * Start drawing an arc.
     *
     * @param aGraphic is an object that is going to be used by the tool for drawing. Must be
     *                 already created. The tool deletes the object if it is not added to a BOARD.
     * @return False if the tool was canceled before the origin was set or origin and end are
     *         the same point.
     */
    bool drawArc( const TOOL_EVENT& aTool, PCB_SHAPE** aGraphic,
                  std::optional<VECTOR2D> aStartingPoint );

    /**
     * Draw a bezier curve.
     *
     * @param aTool is the event that triggered the drawing.
     * @param aStartingPoint is the starting point of the curve (e.g. the end point of the
     *                      previous curve).
     * @param aStartingControl1Point is the previous control point of the curve (which can
     *                               be used to create a smooth transition between two curves).
     * @param aCancelled is set to true if the tool was canceled before the curve was finished.
     *
     * @return A new PCB_SHAPE object representing the bezier curve, or nullptr if
     *         the tool was cancelled or reset.
     */
    std::unique_ptr<PCB_SHAPE> drawOneBezier( const TOOL_EVENT&   aTool,
                                              const OPT_VECTOR2I& aStartingPoint,
                                              const OPT_VECTOR2I& aStartingControl1Point,
                                              DRAW_ONE_RESULT&    aResult );


    /**
     * Draw a polygon, that is added as a zone or a keepout area.
     *
     * @param aKeepout dictates if the drawn polygon is a zone or a keepout area.
     * @param aMode dictates the mode of the zone tool:
     *  - ADD      add a new zone/keepout with fresh settings
     *  - CUTOUT   add a cutout to an existing zone
     *  - SIMILAR  add a new zone with the same settings as an existing one
     */

    /**
     * Get a source zone item for an action that takes an existing zone into account (for
     * example a cutout of an existing zone).
     *
     * The source zone is taken from the current selection.
     *
     * @param aMode mode of the zone tool
     * @param aZone updated pointer to a suitable source zone, or nullptr if none found, or
     *              the action doesn't need a source
     * @return true if a suitable zone was found, or the action doesn't need a zone. False if
     *         the action needs a zone but none was found.
     */
    bool getSourceZoneForAction( ZONE_MODE aMode, ZONE** aZone );

    /**
     * Force the dimension lime to be drawn on multiple of 45 degrees.
     *
     * @param aDimension is the dimension element currently being drawn.
     */
    void constrainDimension( PCB_DIMENSION_BASE* aDim );

    /**
     * Clamps the end vector to respect numeric limits of difference representation
     *
     * @param aOrigin - the origin vector.
     * @param aEnd - the end vector.
     * @return clamped end vector.
     */
    VECTOR2I getClampedDifferenceEnd( const VECTOR2I& aOrigin, const VECTOR2I& aEnd )
    {
        typedef std::numeric_limits<int> coord_limits;
        const int                        guardValue = 1;

        VECTOR2I::extended_type maxDiff = coord_limits::max() - guardValue;

        VECTOR2I::extended_type xDiff = VECTOR2I::extended_type( aEnd.x ) - aOrigin.x;
        VECTOR2I::extended_type yDiff = VECTOR2I::extended_type( aEnd.y ) - aOrigin.y;

        if( xDiff > maxDiff )
            xDiff = maxDiff;
        if( yDiff > maxDiff )
            yDiff = maxDiff;

        if( xDiff < -maxDiff )
            xDiff = -maxDiff;
        if( yDiff < -maxDiff )
            yDiff = -maxDiff;

        return aOrigin + VECTOR2I( int( xDiff ), int( yDiff ) );
    }

    /**
     * Clamps the end vector to respect numeric limits of radius representation
     *
     * @param aOrigin - the origin vector.
     * @param aEnd - the end vector.
     * @return clamped end vector.
     */
    VECTOR2I getClampedRadiusEnd( const VECTOR2I& aOrigin, const VECTOR2I& aEnd )
    {
        typedef std::numeric_limits<int> coord_limits;
        const int                        guardValue = 10;

        VECTOR2I::extended_type xDiff = VECTOR2I::extended_type( aEnd.x ) - aOrigin.x;
        VECTOR2I::extended_type yDiff = VECTOR2I::extended_type( aEnd.y ) - aOrigin.y;

        double maxRadius = coord_limits::max() / 2 - guardValue;
        double radius = std::hypot( xDiff, yDiff );

        if( radius > maxRadius )
        {
            double scaleFactor = maxRadius / radius;

            xDiff = KiROUND<double, int>( xDiff * scaleFactor );
            yDiff = KiROUND<double, int>( yDiff * scaleFactor );
        }

        return aOrigin + VECTOR2I( int( xDiff ), int( yDiff ) );
    }

    KIGFX::VIEW*              m_view;
    KIGFX::VIEW_CONTROLS*     m_controls;
    BOARD*                    m_board;
    PCB_BASE_EDIT_FRAME*      m_frame;
    MODE                      m_mode;
    bool                      m_inDrawingTool;     // Re-entrancy guard

    PCB_LAYER_ID              m_layer;             // The layer we last drew on
    STROKE_PARAMS             m_stroke;            // Current stroke for multi-segment drawing
    TEXT_ATTRIBUTES           m_textAttrs;

    PCB_SELECTION             m_preview;
    BOARD_CONNECTED_ITEM*     m_pickerItem;
    PCB_TUNING_PATTERN*       m_tuningPattern;

    static const unsigned int WIDTH_STEP;          // Amount of width change for one -/+ key press
    static const unsigned int COORDS_PADDING;      // Padding from coordinates limits for this tool


    friend class              ZONE_CREATE_HELPER;  // give internal access to helper classes
};

#endif /* __DRAWING_TOOL_H */
