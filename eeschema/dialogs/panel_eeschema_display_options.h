/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD_PANEL_EESCHEMA_DISPLAY_OPTIONS_H
#define KICAD_PANEL_EESCHEMA_DISPLAY_OPTIONS_H

#include <widgets/unit_binder.h>
#include "panel_eeschema_display_options_base.h"

class SCH_EDIT_FRAME;
class GAL_OPTIONS_PANEL;


class PANEL_EESCHEMA_DISPLAY_OPTIONS : public PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE
{
    SCH_EDIT_FRAME*    m_frame;

    GAL_OPTIONS_PANEL* m_galOptsPanel;

    UNIT_BINDER        m_busWidth;
    UNIT_BINDER        m_wireWidth;
    UNIT_BINDER        m_junctionSize;

public:
    PANEL_EESCHEMA_DISPLAY_OPTIONS( SCH_EDIT_FRAME* aFrame, wxWindow* aWindow );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


#endif //KICAD_PANEL_EESCHEMA_DISPLAY_OPTIONS_H
