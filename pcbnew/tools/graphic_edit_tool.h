/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GRAPHIC_EDIT_TOOL_H
#define GRAPHIC_EDIT_TOOL_H

#include <snap/snap_resolver.h>
#include <tools/graphic_edit.h>
#include <tools/pcb_tool_base.h>

#include <set>

class PCB_SELECTION_TOOL;

class GRAPHIC_EDIT_TOOL : public PCB_TOOL_BASE
{
public:
    GRAPHIC_EDIT_TOOL();

    bool Init() override;
    int  Extend( const TOOL_EVENT& aEvent );
    int  Trim( const TOOL_EVENT& aEvent );

private:
    /**
     * What one edit operation gives the shared interactive loop.
     *
     * The loop owns the pointer.  It picks a source from under the cursor on every motion, so
     * the operation only has to say which shapes it takes and what to do with one.
     */
    struct OPERATION
    {
        wxString m_NoResult;
        wxString m_CommitDescription;

        /// Which shapes may be the source.  Anything else is not worth hovering.
        bool ( *m_Accepts )( const PCB_SHAPE& aShape );

        /// Snap kinds this operation cannot use.  Trim reads which side of a crossing the
        /// pointer is on, so a snap derived from one leaves the question no answer.  Empty
        /// unless the operation says otherwise.
        std::set<SNAP_CANDIDATE_SUBTYPE> m_SuppressedSnaps = {};

        /// The colour role the preview borrows.  Trim marks a removal, extend an addition.
        int m_PreviewLayer;

        /// Null bounds the search by the source's own extent, which only cuts what it crosses.
        BOX2I ( *m_QueryBounds )( const BOARD_ITEM& aSource, const VECTOR2I& aPointer, const BOX2I& aWorldBounds );
        GRAPHIC_EDIT_RESULT ( *m_Plan )( const BOARD_ITEM& aSource, const VECTOR2I& aPointer,
                                         const std::vector<const BOARD_ITEM*>& aBoundaries );
    };

    int  runInteractive( const TOOL_EVENT& aEvent, const OPERATION& aOperation );
    void setTransitions() override;

    PCB_SELECTION_TOOL* m_selectionTool = nullptr;
};

#endif
