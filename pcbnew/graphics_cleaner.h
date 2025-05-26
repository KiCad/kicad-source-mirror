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

#ifndef KICAD_GRAPHICS_CLEANER_H
#define KICAD_GRAPHICS_CLEANER_H

#include <board.h>

class FOOTPRINT;
class BOARD_COMMIT;
class CLEANUP_ITEM;
class TOOL_MANAGER;


// Helper class used to clean tracks and vias
class GRAPHICS_CLEANER
{
public:
    GRAPHICS_CLEANER( const DRAWINGS& aDrawings, FOOTPRINT* aParentFootprint, BOARD_COMMIT& aCommit,
                      TOOL_MANAGER* aToolManager );

    /**
     * the cleanup function.
     * @param aMergeRects = merge for segments forming a rectangle into a rect
     * @param aDeleteRedundant = true to delete null graphics and duplicated graphics
     * @param aMergePads = true to apply Pad Editor's merge algorithm to all pads in footprint
     *                     (it is assumed this will only be run on FPEditor boards)
     */
    void CleanupBoard( bool aDryRun, std::vector<std::shared_ptr<CLEANUP_ITEM>>* aItemsList,
                       bool aMergeRects, bool aDeleteRedundant, bool aMergePads,
                       bool aFixBoardOutlines, int aTolerance );

private:
    bool isNullShape( PCB_SHAPE* aShape );
    bool areEquivalent( PCB_SHAPE* aShape1, PCB_SHAPE* aShape2 );

    void cleanupShapes();
    void fixBoardOutlines();
    void mergeRects();
    void mergePads();

private:
    const DRAWINGS& m_drawings;
    FOOTPRINT*      m_parentFootprint;  // nullptr if not in Footprint Editor
    BOARD_COMMIT&   m_commit;
    TOOL_MANAGER*   m_toolMgr;
    bool            m_dryRun;
    int             m_epsilon;
    int             m_maxError;
    int             m_outlinesTolerance;

    std::vector<std::shared_ptr<CLEANUP_ITEM>>* m_itemsList;
};


#endif //KICAD_GRAPHICS_CLEANER_H
