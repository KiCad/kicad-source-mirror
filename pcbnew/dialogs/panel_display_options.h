/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2010-2014 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#include "panel_display_options_base.h"

class PCBNEW_SETTINGS;
class FOOTPRINT_EDITOR_SETTINGS;
class APP_SETTINGS_BASE;
class PANEL_GAL_OPTIONS;


class PANEL_DISPLAY_OPTIONS : public PANEL_DISPLAY_OPTIONS_BASE
{
public:
    PANEL_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings );
    ~PANEL_DISPLAY_OPTIONS() override;

    bool Show( bool aShow ) override;

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void ResetPanel() override;

private:
    void OnAddLayerItem( wxCommandEvent& event ) override;
    void OnDeleteLayerItem( wxCommandEvent& event ) override;
    void onLayerChange( wxGridEvent& event ) override;

    void loadPCBSettings( PCBNEW_SETTINGS* aCfg );
    void loadFPSettings( const FOOTPRINT_EDITOR_SETTINGS* aCfg );

    int getNextAvailableLayer() const;

private:
    bool               m_isPCBEdit;
    PANEL_GAL_OPTIONS* m_galOptsPanel;
};

