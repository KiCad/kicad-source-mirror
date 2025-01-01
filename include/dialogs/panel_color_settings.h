/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
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

#ifndef PANEL_COLOR_SETTINGS_H
#define PANEL_COLOR_SETTINGS_H

#include <gal/color4d.h>
#include <panel_color_settings_base.h>


class COLOR_SETTINGS;
class COLOR_SWATCH;


class PANEL_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS_BASE
{
public:
    PANEL_COLOR_SETTINGS( wxWindow* aParent );

    ~PANEL_COLOR_SETTINGS() = default;

    enum COLOR_CONTEXT_ID
    {
        ID_COPY = wxID_HIGHEST + 1,
        ID_PASTE,
        ID_REVERT
    };

    virtual void ResetPanel() override;

    virtual wxString GetResetTooltip() const override
    {
        return _( "Reset all colors in this theme to the KiCad defaults" );
    }

    bool Show( bool show ) override;

protected:
    void OnBtnOpenThemeFolderClicked( wxCommandEvent& event ) override;

    void OnLeftDownTheme( wxMouseEvent& event ) override;
    void OnThemeChanged( wxCommandEvent& aEvent ) override;

    void ShowColorContextMenu( wxMouseEvent& aEvent, int aLayer );

    void OnColorChanged( wxCommandEvent& aEvent );

    virtual void createSwatches() = 0;
    void updateSwatches();

    /**
     * Builds the theme listbox and sets the selection to the current theme
     * @param aCurrent is the filename of the current color theme (no extension)
     */
    void createThemeList( const wxString& aCurrent );

    void createSwatch( int aLayer, const wxString& aName );

    void updateColor( int aLayer, const KIGFX::COLOR4D& aColor );

    virtual bool saveCurrentTheme( bool aValidate );

    /**
     * Performs a pre-save validation of the current color theme.
     * @param aQuiet will suppress any warning output (prompt dialogs)
     * @return true if save is allowed
     */
    virtual bool validateSave( bool aQuiet = false )
    {
        return true;
    }

    /**
     * Event fired when a new theme is selected that can be overridden in children
     */
    virtual void onNewThemeSelected() {}

    /**
     * Event fired when the user changes any color
     */
    virtual void onColorChanged() {}

    /**
     * Retrieves the drop down name to be displayed for a color setting
     */
    wxString GetSettingsDropdownName( COLOR_SETTINGS* aSettings );

    COLOR_SETTINGS* m_currentSettings;

    std::map<int, wxStaticText*> m_labels;
    std::map<int, COLOR_SWATCH*> m_swatches;

    KIGFX::COLOR4D m_copied;

    /**
     * A list of layer IDs that are valid for the current color settings dialog.
     *
     * Valid colors will be shown for editing and are the set of colors that actions like resetting
     * to defaults will apply to.
     *
     * This list must be filled in the application-specific color settings panel constructors.
     */
    std::vector<int> m_validLayers;
    int              m_backgroundLayer;

    /**
     * A namespace that will be passed to SETTINGS_MANAGER::SaveColorSettings
     *
     * This should be set to the appropriate namespace in the application-specific constructor
     */
    std::string m_colorNamespace;
};


#endif
