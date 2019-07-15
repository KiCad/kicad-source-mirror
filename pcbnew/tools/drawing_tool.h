/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
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

#include <core/optional.h>
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
class DRAWSEGMENT;
class POLYGON_GEOM_MANAGER;

/**
 * Class DRAWING_TOOL
 *
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

    ///> The possible drawing modes of DRAWING_TOOL
    enum class MODE
    {
        NONE,
        LINE,
        CIRCLE,
        ARC,
        TEXT,
        ANCHOR,
        DXF,
        DIMENSION,
        KEEPOUT,
        ZONE,
        GRAPHIC_POLYGON,
        VIA
    };

    /**
     * Function GetDrawingMode
     *
     * Returns the current drawing mode of the DRAWING_TOOL, or
     * MODE::NONE if not currently in any drawing mode
     */
    MODE GetDrawingMode() const;

    /**
     * Function DrawLine()
     * Starts interactively drawing a line. After invoking the function it expects the user
     * to click at least two times to determine the origin and the end for a line. If there are
     * more clicks, the line is drawn as a continous polyline.
     */
    int DrawLine( const TOOL_EVENT& aEvent );

    /**
     * Function DrawCircle()
     * Starts interactively drawing a circle. After invoking the function it expects the user
     * to first click on a point that is going to be used as the center of the circle. The second
     * click determines the circle radius.
     */
    int DrawCircle( const TOOL_EVENT& aEvent );

    /**
     * Function DrawArc()
     * Starts interactively drawing an arc. After invoking the function it expects the user
     * to first click on a point that is going to be used as the center of the arc. The second
     * click determines the origin and radius, the third one - the angle.
     */
    int DrawArc( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceText()
     * Displays a dialog that allows one to input text and its settings and then
     * lets the user decide where to place the text in editor.
     */
    int PlaceText( const TOOL_EVENT& aEvent );

    /**
     * Function DrawDimension()
     * Starts interactively drawing a dimension. After invoking the function it expects the user
     * to first click on a point that is going to be used as the origin of the dimension.
     * The second click determines the end and the third click modifies its height.
     */
    int DrawDimension( const TOOL_EVENT& aEvent );

    /**
     * Function DrawZone()
     * Starts interactively drawing a zone. After invoking the function a zone settings dialog
     * is displayed. After confirmation it allows the user to set points that are going to be used
     * as a boundary polygon of the zone. Double click or clicking on the origin of the boundary
     * polyline finishes the drawing.
     *
     * The event parameter indicates which type of zone to draw:
     *  ADD      add a new zone/keepout with fresh settings
     *  CUTOUT   add a cutout to an existing zone
     *  SIMILAR  add a new zone with the same settings as an existing one
     */
    int DrawZone(  const TOOL_EVENT& aEvent );

    int DrawVia( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceImportedGraphics()
     * Places a drawing imported from a DXF or SVG file in module editor.
     */
    int PlaceImportedGraphics( const TOOL_EVENT& aEvent );

    /**
     * Function SetAnchor()
     * Places the footprint anchor (only in module editor).
     */
    int SetAnchor( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void setTransitions() override;

private:

    ///> Starts drawing a selected shape (i.e. DRAWSEGMENT).
    ///> @param aShape is the type of created shape (@see STROKE_T).
    ///> @param aGraphic is an object that is going to be used by the tool for drawing. It has to
    ///> be already created. The tool deletes the object if it is not added to a BOARD.
    ///> @param aStartingPoint is a starting point for this new DRAWSEGMENT. If exists
    ///> the new item has its start point set to aStartingPoint,
    ///> and its settings (width, layer) set to the current default values.
    ///> @return False if the tool was cancelled before the origin was set or origin and end are
    ///> the same point.
    bool drawSegment( const std::string& aTool, int aShape, DRAWSEGMENT*& aGraphic,
                      OPT<VECTOR2D> aStartingPoint );

    ///> Starts drawing an arc.
    ///> @param aGraphic is an object that is going to be used by the tool for drawing. It has to
    ///> be already created. The tool deletes the object if it is not added to a BOARD.
    ///> @return False if the tool was cancelled before the origin was set or origin and end are
    ///> the same point.
    bool drawArc( const std::string& aTool, DRAWSEGMENT*& aGraphic, bool aImmediateMode );

    /**
     * Draws a polygon, that is added as a zone or a keepout area.
     *
     * @param aKeepout dictates if the drawn polygon is a zone or a
     * keepout area.
     * @param aMode dictates the mode of the zone tool:
     *  ADD      add a new zone/keepout with fresh settings
     *  CUTOUT   add a cutout to an existing zone
     *  SIMILAR  add a new zone with the same settings as an existing one
     */

    /**
     * Function getSourceZoneForAction()
     *
     * Gets a source zone item for an action that takes an existing zone
     * into account (for example a cutout of an existing zone). The source
     * zone is taken from the current selection
     *
     * @param aMode mode of the zone tool
     * @param aZone updated pointer to a suitable source zone,
     * or nullptr if none found, or the action doesn't need a source
     * @return true if a suitable zone was found, or the action doesn't
     * need a zone. False if the action needs a zone but none was found.
     */
    bool getSourceZoneForAction( ZONE_MODE aMode, ZONE_CONTAINER*& aZone );

    /**
     * Run the event loop for polygon creation, sending user input
     * on to the given POLYGON_GEOM_MANAGER for processing into a
     * complete polygon.
     */
    void runPolygonEventLoop( POLYGON_GEOM_MANAGER& aPolyGeomMgr );

    /**
     * Function constrainDimension()
     * Forces the dimension lime to be drawn on multiple of 45 degrees
     * @param aDimension is the dimension element currently being drawn
     */
    void constrainDimension( DIMENSION* dimension );

    ///> Returns the appropriate width for a segment depending on the settings.
    int getSegmentWidth( PCB_LAYER_ID aLayer ) const;

    ///> Selects a non-copper layer for drawing
    PCB_LAYER_ID getDrawingLayer() const;

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    BOARD* m_board;
    PCB_BASE_EDIT_FRAME* m_frame;
    MODE m_mode;

    /// Stores the current line width for multisegment drawing.
    unsigned int m_lineWidth;

    // How does line width change after one -/+ key press.
    static const unsigned int WIDTH_STEP;


    // give internal access to drawing helper classes
    friend class ZONE_CREATE_HELPER;
};

#endif /* __DRAWING_TOOL_H */
