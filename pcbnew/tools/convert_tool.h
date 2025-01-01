/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/pcb_tool_base.h>
#include <pcbnew_settings.h>

class CONDITIONAL_MENU;
class PCB_SELECTION_TOOL;
class PCB_BASE_FRAME;


class CONVERT_TOOL : public PCB_TOOL_BASE
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
    int CreatePolys( const TOOL_EVENT& aEvent );

    /**
     * Convert selected polygon-like object to graphic lines, if possible.
     */
    int CreateLines( const TOOL_EVENT& aEvent );

    /**
     * Convert selected segment (graphic or track) to an arc of the same type
     */
    int SegmentToArc( const TOOL_EVENT& aEvent );

    /**
     * Convert selected items to outset versions of themselves.
     */
    int OutsetItems( const TOOL_EVENT& aEvent );

    ///< @copydoc TOOL_INTERACTIVE::setTransitions()
    void setTransitions() override;

private:
    /**
     * Retrieve the start and end points for a generic item.
     *
     * @param aItem is an item that has a start and end point.
     * @return a segment from start to end, or std::nullopt if invalid.
     */
    static std::optional<SEG> getStartEndPoints( EDA_ITEM* aItem );

    /**
     * Try to make polygons from chained segments in the selected items.
     *
     * Polygons are formed from chains of lines/arcs.  Each set containing two or more lines/arcs
     * that are connected will be added to the return SHAPE_POLY_SET as an outline.  No attempt
     * is made to guess at holes.
     *
     * @param aItems is a list of items to process.
     * @return a #SHAPE_POLY_SET containing any polygons that were created.
     */
    SHAPE_POLY_SET makePolysFromChainedSegs( const std::deque<EDA_ITEM*>& aItems,
                                             CONVERT_STRATEGY aStrategy );

    /**
     * Make polygons from graphic shapes and zones.
     *
     * @param aItems is a list of items to process.
     * @return a #SHAPE_POLY_SET containing any polygons that were created.
     */
    SHAPE_POLY_SET makePolysFromOpenGraphics( const std::deque<EDA_ITEM*>& aItems, int aGap );
    SHAPE_POLY_SET makePolysFromClosedGraphics( const std::deque<EDA_ITEM*>& aItems,
                                                CONVERT_STRATEGY aStrategy );

    /**
     * Initialize the user settings for the tool.
     */
    void initUserSettings();

private:
    PCB_SELECTION_TOOL* m_selectionTool;
    CONDITIONAL_MENU*   m_menu;
    PCB_BASE_FRAME*     m_frame;
    CONVERT_SETTINGS    m_userSettings;
};

#endif
