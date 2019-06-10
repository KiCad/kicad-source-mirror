/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef EDA_3D_ACTIONS_H
#define EDA_3D_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * Class EDA_3D_ACTIONS
 *
 * Note: these aren't "real" actions; we just use them to see the hotkeys display.
 */
class EDA_3D_ACTIONS : public ACTIONS
{
public:
    static TOOL_ACTION pivotCenter;
    static TOOL_ACTION moveLeft;
    static TOOL_ACTION moveRight;
    static TOOL_ACTION moveUp;
    static TOOL_ACTION moveDown;
    static TOOL_ACTION homeView;
    static TOOL_ACTION resetView;

    static TOOL_ACTION viewFront;
    static TOOL_ACTION viewBack;
    static TOOL_ACTION viewLeft;
    static TOOL_ACTION viewRight;
    static TOOL_ACTION viewTop;
    static TOOL_ACTION viewBottom;

    static TOOL_ACTION rotate45axisZ;
    static TOOL_ACTION zoomIn;
    static TOOL_ACTION zoomOut;
    static TOOL_ACTION attributesTHT;
    static TOOL_ACTION attributesSMD;
    static TOOL_ACTION attributesVirtual;

    ///> @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override { return OPT<TOOL_EVENT>(); }
};

#endif
