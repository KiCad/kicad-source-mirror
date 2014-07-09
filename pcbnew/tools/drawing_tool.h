/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#include <tool/tool_interactive.h>

namespace KIGFX
{
    class VIEW;
    class VIEW_CONTROLS;
}
class BOARD;
class PCB_EDIT_FRAME;
class DRAWSEGMENT;

/**
 * Class DRAWING_TOOL
 *
 * Tool responsible for drawing graphical elements like lines, arcs, circles, etc.
 */

class DRAWING_TOOL : public TOOL_INTERACTIVE
{
public:
    DRAWING_TOOL();
    ~DRAWING_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason );

    /**
     * Function DrawLine()
     * Starts interactively drawing a line. After invoking the function it expects the user
     * to click at least two times to determine the origin and the end for a line. If there are
     * more clicks, the line is drawn as a continous polyline.
     */
    int DrawLine( TOOL_EVENT& aEvent );

    /**
     * Function DrawCircle()
     * Starts interactively drawing a circle. After invoking the function it expects the user
     * to first click on a point that is going to be used as the center of the circle. The second
     * click determines the circle radius.
     */
    int DrawCircle( TOOL_EVENT& aEvent );

    /**
     * Function DrawArc()
     * Starts interactively drawing an arc. After invoking the function it expects the user
     * to first click on a point that is going to be used as the center of the arc. The second
     * click determines the origin and radius, the third one - the angle.
     */
    int DrawArc( TOOL_EVENT& aEvent );

    /**
     * Function PlaceText()
     * Displays a dialog that allows to input text and its settings and then lets the user decide
     * where to place the text in editor.
     */
    int PlaceText( TOOL_EVENT& aEvent );

    /**
     * Function DrawDimension()
     * Starts interactively drawing a dimension. After invoking the function it expects the user
     * to first click on a point that is going to be used as the origin of the dimension.
     * The second click determines the end and the third click modifies its height.
     */
    int DrawDimension( TOOL_EVENT& aEvent );

    /**
     * Function DrawZone()
     * Starts interactively drawing a zone. After invoking the function a zone settings dialog
     * is displayed. After confirmation it allows the user to set points that are going to be used
     * as a boundary polygon of the zone. Double click or clicking on the origin of the boundary
     * polyline finishes the drawing.
     */
    int DrawZone( TOOL_EVENT& aEvent );

    /**
     * Function DrawKeepout()
     * Starts interactively drawing a keepout area. After invoking the function an area settings
     * dialog is displayed. After confirmation it allows the user to set points that are going to
     * be used as a boundary polygon of the area. Double click or clicking on the origin of the
     * boundary polyline finishes the drawing.
     */
    int DrawKeepout( TOOL_EVENT& aEvent );

    /**
     * Function PlaceTarget()
     * Allows user to place a layer alignment target.
     */
    int PlaceTarget( TOOL_EVENT& aEvent );

    /**
     * Function PlaceModule()
     * Displays a dialog to select a module to be added and allows the user to set its position.
     */
    int PlaceModule( TOOL_EVENT& aEvent );

    /**
     * Function PlaceDXF()
     * Places a drawing imported from a DXF file in module editor.
     */
    int PlaceDXF( TOOL_EVENT& aEvent );

    /**
     * Function SetAnchor()
     * Places the footprint anchor (only in module editor).
     */
    int SetAnchor( TOOL_EVENT& aEvent );

    /**
     * Function EditModules()
     * Toggles edit module mode. When enabled, one may select parts of modules individually
     * (graphics, pads, etc.), so they can be modified.
     * @param aEnabled decides if the mode should be enabled.
     */
    void EditModules( bool aEnabled )
    {
        m_editModules = aEnabled;
    }

private:
    ///> Starts drawing a selected shape (i.e. DRAWSEGMENT).
    ///> @param aShape is the type of created shape (@see STROKE_T).
    ///> @param aGraphic is an object that is going to be used by the tool for drawing. It has to
    ///> be already created. The tool deletes the object if it is not added to a BOARD.
    ///> @return False if the tool was cancelled before the origin was set or origin and end are
    ///> the same point.
    bool drawSegment( int aShape, DRAWSEGMENT*& aGraphic );

    ///> Starts drawing an arc.
    ///> @param aGraphic is an object that is going to be used by the tool for drawing. It has to
    ///> be already created. The tool deletes the object if it is not added to a BOARD.
    ///> @return False if the tool was cancelled before the origin was set or origin and end are
    ///> the same point.
    bool drawArc( DRAWSEGMENT*& aGraphic );

    ///> Draws a polygon, that is added as a zone or a keepout area.
    ///> @param aKeepout decides if the drawn polygon is a zone or a keepout area.
    int drawZone( bool aKeepout );

    /**
     * Function placeTextModule()
     * Displays a dialog that allows to input text and its settings and then lets the user decide
     * where to place the text in module .
     */
    int placeTextModule();

    /**
     * Function placeTextPcb()
     * Displays a dialog that allows to input text and its settings and then lets the user decide
     * where to place the text in board editor.
     */
    int placeTextPcb();

    /**
     * Function make45DegLine()
     * Forces a DRAWSEGMENT to be drawn at multiple of 45 degrees. The origin stays the same,
     * the end of the aSegment is modified according to the current cursor position.
     * @param aSegment is the segment that is currently drawn.
     * @param aHelper is a helper line that shows the next possible segment.
     */
    void make45DegLine( DRAWSEGMENT* aSegment, DRAWSEGMENT* aHelper ) const;

    ///> Sets up handlers for various events.
    void setTransitions();

    KIGFX::VIEW* m_view;
    KIGFX::VIEW_CONTROLS* m_controls;
    BOARD* m_board;
    PCB_EDIT_FRAME* m_frame;

    /// Edit module mode flag
    bool m_editModules;

    // How does line width change after one -/+ key press.
    static const int WIDTH_STEP = 100000;
};

#endif /* __DRAWING_TOOL_H */
