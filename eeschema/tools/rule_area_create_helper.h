/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EESCHEMA_TOOLS_RULE_CREATE_HELPER__H_
#define EESCHEMA_TOOLS_RULE_CREATE_HELPER__H_

#include <preview_items/polygon_geom_manager.h>
#include <preview_items/polygon_item.h>
#include "sch_rule_area.h"
#include <view/view.h>

namespace KIGFX
{
class VIEW;
}

class TOOL_MANAGER;

/**
 * An adjunct helper to the DRAWING_TOOL interactive tool, which handles incoming geometry
 * changes from a #POLYGON_GEOM_MANAGER and translates that into a SCH_RULE_AREA based on given
 * parameters.
 */
class RULE_AREA_CREATE_HELPER : public POLYGON_GEOM_MANAGER::CLIENT
{
public:
    RULE_AREA_CREATE_HELPER( KIGFX::VIEW& aView, SCH_EDIT_FRAME* aFrame, TOOL_MANAGER* aMgr );

    virtual ~RULE_AREA_CREATE_HELPER();

    SCH_RULE_AREA* GetRuleArea() const { return m_rule_area.get(); }

    /*
     * Interface for receiving #POLYGON_GEOM_MANAGER update
     */
    void OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr ) override;

    bool OnFirstPoint( POLYGON_GEOM_MANAGER& aMgr ) override;

    void OnComplete( const POLYGON_GEOM_MANAGER& aMgr ) override;

    /**
     * Create a new SCH_RULE_AREA
     *
     * @return the new rule area, can be null if the user aborted
     */
    std::unique_ptr<SCH_RULE_AREA> createNewRuleArea();

    /**
     * Commit the current rule area in progress to the schematic.
     *
     * @param aRuleArea is the drawn rule area outline to commit.
     */
    void commitRuleArea( std::unique_ptr<SCH_RULE_AREA> aRuleArea );

private:
    ///< The preview item to display
    KIGFX::PREVIEW::POLYGON_ITEM m_previewItem;

    ///< view that show the preview item
    KIGFX::VIEW& m_parentView;

    ///< The active schematic edit frame
    SCH_EDIT_FRAME* m_frame;

    ///< The rule area in progress
    std::unique_ptr<SCH_RULE_AREA> m_rule_area;

    ///< The TOOL_MANAGER running the tool
    TOOL_MANAGER* m_toolManager;
};

#endif // EESCHEMA_TOOLS_RULE_CREATE_HELPER__H_
