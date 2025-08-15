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

#pragma once

#include <dialogs/panel_maintenance_base.h>


class COMMON_SETTINGS;
class EDA_BASE_FRAME;


class PANEL_MAINTENANCE : public PANEL_MAINTENANCE_BASE
{
public:
    PANEL_MAINTENANCE( wxWindow* aParent, EDA_BASE_FRAME* aFrame );
    ~PANEL_MAINTENANCE() = default;

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void applySettingsToPanel( COMMON_SETTINGS& aSettings );

    void doClearDontShowAgain();
    void doClearDialogState();

    void onClearFileHistory( wxCommandEvent& event ) override;
    void onClearDontShowAgain( wxCommandEvent& event ) override;
    void onClearDialogState( wxCommandEvent& event ) override;
    void onResetAll( wxCommandEvent& event ) override;

protected:
    EDA_BASE_FRAME* m_frame;
};
