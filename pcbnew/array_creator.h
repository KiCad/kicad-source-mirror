/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Created on: 11 Mar 2016, author John Beard
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ARRAY_CREATOR_H_
#define ARRAY_CREATOR_H_

#include <dialogs/dialog_create_array.h>

#include <board.h>
#include <footprint.h>
#include <board_item.h>
#include <tools/pcb_selection.h>

class TOOL_MANAGER;

/*!
 * Class that performs array creation by producing a dialog to gather parameters and then
 * creating and laying out the items.
 */
class ARRAY_CREATOR
{
public:
    ARRAY_CREATOR( PCB_BASE_FRAME& aParent, bool aIsFootprintEditor,
                   const PCB_SELECTION& aSelection, TOOL_MANAGER* aToolManager ) :
            m_parent( aParent ),
            m_isFootprintEditor( aIsFootprintEditor ),
            m_selection( aSelection ),
            m_toolMgr( aToolManager )
    {}

    virtual ~ARRAY_CREATOR() {}

    /*!
     * Open the dialog, gather parameters and create the array
     */
    void Invoke();

private:
    PCB_BASE_FRAME&      m_parent;
    bool                 m_isFootprintEditor;
    const PCB_SELECTION  m_selection;
    TOOL_MANAGER*        m_toolMgr;
};

#endif /* ARRAY_CREATOR_H_ */
