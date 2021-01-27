/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef CONVERT_TOOL_H_
#define CONVERT_TOOL_H_

#include <geometry/shape_poly_set.h>
#include <tool/tool_interactive.h>

class CONDITIONAL_MENU;
class PCB_SELECTION_TOOL;
class PCB_BASE_FRAME;


class CONVERT_TOOL : public TOOL_INTERACTIVE
{
public:
    CONVERT_TOOL();
    virtual ~CONVERT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override {}

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Convert selected lines to a polygon, if possible.
     */
    int LinesToPoly( const TOOL_EVENT& aEvent );

    /**
     * Convert selected polygon-like object to graphic lines, if possible.
     */
    int PolyToLines( const TOOL_EVENT& aEvent );

    /**
     * Convert selected segment (graphic or track) to an arc of the same type
     */
    int SegmentToArc( const TOOL_EVENT& aEvent );

    ///< @copydoc TOOL_INTERACTIVE::setTransitions()
    void setTransitions() override;

private:
    /**
     * Retrieve the start and end points for a generic item.
     *
     * @param aItem is an item that has a start and end point.
     * @return a segment from start to end, or NULLOPT if invalid.
     */
    static OPT<SEG> getStartEndPoints( EDA_ITEM* aItem );

    /**
     * Try to make polygons from segments in the selected items.
     *
     * Polygons are formed from chains of lines/arcs.  Each set containing two or more lines/arcs
     * that are connected will be added to the return SHAPE_POLY_SET as an outline.  No attempt
     * is made to guess at holes.
     *
     * @param aItems is a list of items to process.
     * @return a #SHAPE_POLY_SET containing any polygons that were created.
     */
    static SHAPE_POLY_SET makePolysFromSegs( const std::deque<EDA_ITEM*>& aItems );

    /**
     * Try to make polygons from rectangles.
     *
     * @param aItems is a list of rect shapes to process.
     * @return a #SHAPE_POLY_SET containing any polygons that were created.
     */
    static SHAPE_POLY_SET makePolysFromRects( const std::deque<EDA_ITEM*>& aItems );

    /**
     * Try to make polygons from circles.
     *
     * @param aItems is a list of circle shapes to process.
     * @return a #SHAPE_POLY_SET containing any polygons that were created.
     */
    static SHAPE_POLY_SET makePolysFromCircles( const std::deque<EDA_ITEM*>& aItems );

    PCB_SELECTION_TOOL* m_selectionTool;
    CONDITIONAL_MENU*   m_menu;
    PCB_BASE_FRAME*     m_frame;
};

#endif
