/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tool/tool_manager.h>
#include <gerbview_id.h>
#include "gerbview_actions.h"


OPT<TOOL_EVENT> GERBVIEW_ACTIONS::TranslateLegacyId( int aId )
{
    switch( aId )
    {
    case ID_HIGHLIGHT_REMOVE_ALL:
        return GERBVIEW_ACTIONS::highlightClear.MakeEvent();

    case ID_HIGHLIGHT_CMP_ITEMS:
        return GERBVIEW_ACTIONS::highlightComponent.MakeEvent();

    case ID_HIGHLIGHT_NET_ITEMS:
        return GERBVIEW_ACTIONS::highlightNet.MakeEvent();

    case ID_HIGHLIGHT_APER_ATTRIBUTE_ITEMS:
        return GERBVIEW_ACTIONS::highlightAttribute.MakeEvent();
    }

    return OPT<TOOL_EVENT>();
}
