/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_EESCHEMA_DISPLAY_OPTIONS_H
#define PANEL_EESCHEMA_DISPLAY_OPTIONS_H

#include "panel_eeschema_display_options_base.h"

class APP_SETTINGS_BASE;
class GAL_OPTIONS_PANEL;
class EESCHEMA_SETTINGS;


class PANEL_EESCHEMA_DISPLAY_OPTIONS : public PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE
{
public:
    PANEL_EESCHEMA_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void loadEEschemaSettings( EESCHEMA_SETTINGS* aCfg );

private:
    GAL_OPTIONS_PANEL* m_galOptsPanel;
};


#endif // PANEL_EESCHEMA_DISPLAY_OPTIONS_H
