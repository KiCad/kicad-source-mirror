/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
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

/**
 * @file display_footprints_frame.h
 */
#ifndef DISPLAY_FOOTPRINTS_FRAME_H
#define DISPLAY_FOOTPRINTS_FRAME_H

#include <pcb_base_frame.h>
#include <pcbnew_settings.h>

class REPORTER;
class COMPONENT;

// The name (for wxWidgets) of the footprint viewer frame
#define FOOTPRINTVIEWER_FRAME_NAME wxT( "FootprintViewerFrame" )


/**
 * Display footprints.
 */
class DISPLAY_FOOTPRINTS_FRAME : public PCB_BASE_FRAME
{
public:
    DISPLAY_FOOTPRINTS_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~DISPLAY_FOOTPRINTS_FRAME() override;

    /**
     * Refresh the full display for this frame.
     *
     * Set the title, the status line and redraw the canvas.
     * Must be called after the footprint to display is modified.
     */
    void InitDisplay();

    ///< @copydoc PCB_BASE_FRAME::GetModel()
    BOARD_ITEM_CONTAINER* GetModel() const override;

    /**
     * Update the gal canvas (view, colors ...).
     */
    void updateView();

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    PCB_VIEWERS_SETTINGS_BASE* GetViewerSettingsBase() const override;

    MAGNETIC_SETTINGS* GetMagneticItemsSettings() override;

    /// @copydoc EDA_DRAW_FRAME::UpdateMsgPanel()
    void UpdateMsgPanel() override;

    COLOR_SETTINGS* GetColorSettings( bool aForceRefresh = false ) const override;

    /**
     * @return the color of the grid.
     */
    COLOR4D GetGridColor() override;

    FOOTPRINT* GetFootprint( const wxString& aFootprintName, REPORTER& aReporter );

    SELECTION& GetCurrentSelection() override;

    void ReloadFootprint( FOOTPRINT* aFootprint ) override;
    DECLARE_EVENT_TABLE()

protected:
    void setupUIConditions() override;

private:
    wxString   m_currentFootprint;
    COMPONENT* m_currentComp;
};

#endif   // DISPLAY_FOOTPRINTS_FRAME_H
