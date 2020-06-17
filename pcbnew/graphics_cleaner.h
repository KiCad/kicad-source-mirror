/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <class_board.h>

class MODULE;
class BOARD_COMMIT;
class CLEANUP_ITEM;


// Helper class used to clean tracks and vias
class GRAPHICS_CLEANER
{
public:
    GRAPHICS_CLEANER( DRAWINGS& aDrawings, MODULE* aParentModule, BOARD_COMMIT& aCommit );

    /**
     * the cleanup function.
     * @param aMergeRects = merge for segments forming a rectangle into a rect
     * @param aDeleteRedundant = true to delete null graphics and duplicated graphics
     */
    void CleanupBoard( bool aDryRun, std::vector<CLEANUP_ITEM*>* aItemsList, bool aMergeRects,
                       bool aDeleteRedundant );

private:
    bool isNullSegment( DRAWSEGMENT* aSegment );
    bool areEquivalent( DRAWSEGMENT* aSegment1, DRAWSEGMENT* aSegment2 );

    void cleanupSegments();
    void mergeRects();
    void removeItems( std::set<BOARD_ITEM*>& aItems );

private:
    DRAWINGS&                   m_drawings;
    MODULE*                     m_parentModule;  // nullptr if not in ModEdit
    BOARD_COMMIT&               m_commit;
    bool                        m_dryRun;
    std::vector<CLEANUP_ITEM*>* m_itemsList;
};


#endif //KICAD_GRAPHICS_CLEANER_H
