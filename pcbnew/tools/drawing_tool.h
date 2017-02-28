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

#include <tools/pcb_tool.h>
#include <boost/optional.hpp>

#include <tool/tool_menu.h>

namespace KIGFX
{
    class VIEW;
    class VIEW_CONTROLS;
}
class BOARD;
class PCB_BASE_EDIT_FRAME;
class DRAWSEGMENT;

/**
 * Class DRAWING_TOOL
 *
 * Tool responsible for drawing graphical elements like lines, arcs, circles, etc.
 */

class DRAWING_TOOL : public PCB_TOOL
{
public:
    DRAWING_TOOL();
    ~DRAWING_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    ///> Get the DRAWING_TOOL top-level context menu
    inline TOOL_MENU& GetToolMenu()
    {
        return m_menu;
    }

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
     * Displays a dialog that allows to input text and its settings and then lets the user decide
     * where to place the text in editor.
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
     */
    int DrawZone( const TOOL_EVENT& aEvent );

    /**
     * Function DrawKeepout()
     * Starts interactively drawing a keepout area. After invoking the function an area settings
     * dialog is displayed. After confirmation it allows the user to set points that are going to
     * be used as a boundary polygon of the area. Double click or clicking on the origin of the
     * boundary polyline finishes the drawing.
     */
    int DrawKeepout( const TOOL_EVENT& aEvent );

    /**
     * Function DrawZoneCutout()
     * Starts interactively drawing a zone cutout area of an existing zone.
     * The normal zone interactive tool is used, but the zone settings
     * dialog is not shown (since the cutout affects only shape of an
     * existing zone).
     */
    int DrawZoneCutout( const TOOL_EVENT& aEvent );

    /**
     * Function DrawSimilarZone()
     * Starts interactively drawing a zone with same settings as
     * an existing zone.
     * The normal zone interactive tool is used, but the zone settings
     * dialog is not shown at the start.
     */
    int DrawSimilarZone( const TOOL_EVENT& aEvent );

    /**
     * Function PlaceDXF()
     * Places a drawing imported from a DXF file in module editor.
     */
    int PlaceDXF( const TOOL_EVENT& aEvent );

    /**
     * Function SetAnchor()
     * Places the footprint anchor (only in module editor).
     */
    int SetAnchor( const TOOL_EVENT& aEvent );

    ///> Sets up handlers for various events.
    void SetTransitions() override;

private:

    enum class ZONE_MODE
    {
        ADD,            ///< Add a new zone/keepout with fresh settings
        CUTOUT,         ///< Make a cutout to an existing zone
        SIMILAR         ///< Add a new zone with the same settings as an existing one
    };

    ///> Shows the context menu for the drawing tool
    ///> This menu consists of normal UI functions (zoom, grid, etc)
    ///> And any suitable global functions for the active drawing type.
    void showContextMenu();

    ///> Starts drawing a selected shape (i.e. DRAWSEGMENT).
    ///> @param aShape is the type of created shape (@see STROKE_T).
    ///> @param aGraphic is an object that is going to be used by the tool for drawing. It has to
    ///> be already created. The tool deletes the object if it is not added to a BOARD.
    ///> @return False if the tool was cancelled before the origin was set or origin and end are
    ///> the same point.
    bool drawSegment( int aShape, DRAWSEGMENT*& aGraphic,
                      boost::optional<VECTOR2D> aStartingPoint = boost::none );

    ///> Starts drawing an arc.
    ///> @param aGraphic is an object that is going to be used by the tool for drawing. It has to
    ///> be already created. The tool deletes the object if it is not added to a BOARD.
    ///> @return False if the tool was cancelled before the origin was set or origin and end are
    ///> the same point.
    bool drawArc( DRAWSEGMENT*& aGraphic );

    /**
     * Draws a polygon, that is added as a zone or a keepout area.
     *
     * @param aKeepout dictates if the drawn polygon is a zone or a
     * keepout area.
     * @param aMode dictates the mode of the zone tool
     */
    int drawZone( bool aKeepout, ZONE_MODE aMode );

    /**
     * Function createNewZone()
     *
     * Prompt the user for new zone settings, and create a new zone with
     * those settings
     *
     * @param aKeepout should the zone be a keepout
     * @return the new zone, can be null if the user aborted
     */
    std::unique_ptr<ZONE_CONTAINER> createNewZone( bool aKeepout );

    /**
     * Function createZoneFromExisting
     *
     * Create a new zone with the settings from an existing zone
     *
     * @param aSrcZone the zone to copy settings from
     * @return the new zone
     */
    std::unique_ptr<ZONE_CONTAINER> createZoneFromExisting( const ZONE_CONTAINER& aSrcZone );

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
     * Function performZoneCutout()
     *
     * Cut one zone out of another one (i.e. subtraction) and
     * update the zone.
     *
     * @param aExistingZone the zone to removed area from
     * @param aCutout the area to remove
     */
    void performZoneCutout( ZONE_CONTAINER& aExistingZone, ZONE_CONTAINER& aCutout );

    /**
     * Function make45DegLine()
     * Forces a DRAWSEGMENT to be drawn at multiple of 45 degrees. The origin stays the same,
     * the end of the aSegment is modified according to the current cursor position.
     * @param aSegment is the segment that is currently drawn.
     * @param aHelper is a helper line that shows the next possible segment.
     */
    void make45DegLine( DRAWSEGMENT* aSegment, DRAWSEGMENT* aHelper ) const;

    ///> Returns the appropriate width for a segment depending on the settings.
    int getSegmentWidth( unsigned int aLayer ) const;

    ///> Selects a non-copper layer for drawing
    LAYER_ID getDrawingLayer() const;

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    BOARD* m_board;
    PCB_BASE_EDIT_FRAME* m_frame;
    MODE m_mode;

    /// Stores the current line width for multisegment drawing.
    unsigned int m_lineWidth;

    /// Menu model displayed by the tool.
    TOOL_MENU m_menu;

    // How does line width change after one -/+ key press.
    static const unsigned int WIDTH_STEP;
};

#endif /* __DRAWING_TOOL_H */
