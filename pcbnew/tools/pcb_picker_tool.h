/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef PCB_PICKER_TOOL_H
#define PCB_PICKER_TOOL_H

#include <layers_id_colors_and_visibility.h>
#include <tool/picker_tool.h>
#include <tools/pcb_tool_base.h>

/**
 *Generic tool for picking an item.
 */
class PCB_PICKER_TOOL : public PCB_TOOL_BASE, public PICKER_TOOL_BASE
{
public:
    PCB_PICKER_TOOL();
    virtual ~PCB_PICKER_TOOL() = default;

    ///< Main event loop.
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Set the tool's snap layer set.
     */
    inline void SetLayerSet( LSET aLayerSet ) { m_layerMask = aLayerSet; }

protected:
    ///< @copydoc TOOL_INTERACTIVE::setTransitions();
    void setTransitions() override;

    ///< Applies the requested VIEW_CONTROLS settings.
    void setControls();

    ///< Reinitialize tool to its initial state.
    void reset() override;

private:
    ///< The layer set to use for optional snapping.
    LSET                  m_layerMask;
};

#endif /* PCB_PICKER_TOOL_H */
