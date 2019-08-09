/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2018-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef CVPCB_ACTIONS_H
#define CVPCB_ACTIONS_H

#include <core/optional.h>
#include <tool/actions.h>
#include <tool/tool_event.h>
class TOOL_ACTION;

/**
 * Class CVPCB_ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of CVPCB_ACTIONS is created
 * inside of ACTION_MANAGER object that registers the actions.
 */
class CVPCB_ACTIONS : public ACTIONS
{
public:
    /// Activation actions
    static TOOL_ACTION selectionActivate;
    static TOOL_ACTION controlActivate;

    /// Window control actions
    static TOOL_ACTION changeFocusRight;
    static TOOL_ACTION changeFocusLeft;

    /// Open the footprint viewer
    static TOOL_ACTION showFootprintViewer;

    /// Navigate the component tree
    static TOOL_ACTION gotoPreviousNA;
    static TOOL_ACTION gotoNextNA;

    /// Management actions
    static TOOL_ACTION saveAssociations;
    static TOOL_ACTION showEquFileTable;

    /// Footprint Association actions
    static TOOL_ACTION autoAssociate;
    static TOOL_ACTION associate;
    static TOOL_ACTION deleteAll;
    static TOOL_ACTION deleteAssoc;

    /// Footprint Filtering actions
    static TOOL_ACTION filterFPbyKeywords;
    static TOOL_ACTION filterFPbyPin;
    static TOOL_ACTION filterFPbyLibrary;
    static TOOL_ACTION filterFPbyDisplayName;

    ///> @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override { return OPT<TOOL_EVENT>(); }
};

#endif
