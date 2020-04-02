/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PANEL_EESCHEMA_COLOR_SETTINGS_H_
#define PANEL_EESCHEMA_COLOR_SETTINGS_H_

#include <gal/color4d.h>
#include <layers_id_colors_and_visibility.h>
#include <panel_color_settings.h>

class COLOR_SETTINGS;
class SCH_BASE_FRAME;
class PAGE_INFO;
class SCH_ITEM;
class SCH_PREVIEW_PANEL;
class TITLE_BLOCK;

namespace KIGFX
{
    class WS_PROXY_VIEW_ITEM;
}

class PANEL_EESCHEMA_COLOR_SETTINGS : public PANEL_COLOR_SETTINGS
{
public:
    PANEL_EESCHEMA_COLOR_SETTINGS( SCH_BASE_FRAME* aFrame, wxWindow* aParent );

    ~PANEL_EESCHEMA_COLOR_SETTINGS() override;

protected:
    bool TransferDataFromWindow() override;

    bool TransferDataToWindow() override;

    void SetColor( wxCommandEvent& aEvent );

    void OnBtnResetClicked( wxCommandEvent& aEvent ) override;

    void OnBtnRenameClicked( wxCommandEvent& aEvent ) override;

    void OnThemeChanged( wxCommandEvent& aEvent ) override;

    void OnBtnSaveClicked( wxCommandEvent& aEvent ) override;

    void OnBtnNewClicked( wxCommandEvent& aEvent ) override;

    void OnThemeNameChanged( wxCommandEvent& aEvent ) override;

    void OnFilenameChanged( wxCommandEvent& aEvent ) override;

    void OnFilenameChar( wxKeyEvent& aEvent ) override;

    void OnSize( wxSizeEvent& aEvent ) override;

    void ShowColorContextMenu( wxMouseEvent& aEvent, SCH_LAYER_ID aLayer );

    enum COLOR_CONTEXT_ID
    {
        ID_COPY = wxID_HIGHEST + 1,
        ID_PASTE,
        ID_REVERT
    };

private:
    SCH_BASE_FRAME* m_frame;

    SCH_PREVIEW_PANEL* m_preview;

    COLOR_SETTINGS* m_currentSettings;

    wxSize m_buttonSizePx;

    PAGE_INFO* m_page;

    TITLE_BLOCK* m_titleBlock;

    KIGFX::WS_PROXY_VIEW_ITEM* m_ws;

    std::vector<EDA_ITEM*> m_previewItems;

    std::map<SCH_LAYER_ID, wxBitmapButton*> m_buttons;

    bool m_filenameEdited;

    bool m_isNewTheme;

    bool m_dirty;

    KIGFX::COLOR4D m_copied;

    bool saveCurrentTheme();

    void createPreviewItems();

    void createButtons();

    void updateColor( SCH_LAYER_ID aLayer, const KIGFX::COLOR4D& aColor );

    void drawButton( wxBitmapButton* aButton, const KIGFX::COLOR4D& aColor );

    void updatePreview();

    void zoomFitPreview();

    bool validateFilename();

    void showThemeNamePanel( bool aShow = true );

    static wxString suggestFilename( const wxString& aThemeName );
};


#endif
