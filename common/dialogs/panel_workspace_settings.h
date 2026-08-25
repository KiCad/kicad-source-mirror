/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_WORKSPACE_SETTINGS_H
#define PANEL_WORKSPACE_SETTINGS_H

#include <widgets/resettable_panel.h>

class wxCheckBox;
class wxChoice;

/**
 * The Preferences > Design Workspace page. Until its enable box is ticked no backend loads and no workspace UI
 * appears.
 */
class PANEL_WORKSPACE_SETTINGS : public RESETTABLE_PANEL
{
public:
    PANEL_WORKSPACE_SETTINGS( wxWindow* aParent );

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

    void ResetPanel() override;

private:
    void loadBackendChoices();

    wxCheckBox* m_enableIntegration;
    wxChoice*   m_defaultBackend;
};

#endif // PANEL_WORKSPACE_SETTINGS_H
