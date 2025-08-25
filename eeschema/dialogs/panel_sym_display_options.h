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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "panel_sym_display_options_base.h"

class APP_SETTINGS_BASE;
class PANEL_GAL_OPTIONS;
class SYMBOL_EDITOR_SETTINGS;


class PANEL_SYM_DISPLAY_OPTIONS : public PANEL_SYM_DISPLAY_OPTIONS_BASE
{
public:
    PANEL_SYM_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void loadSymEditorSettings( SYMBOL_EDITOR_SETTINGS* aCfg );

private:
    PANEL_GAL_OPTIONS* m_galOptsPanel;
};
