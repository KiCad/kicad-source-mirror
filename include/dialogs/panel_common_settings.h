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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <dialogs/panel_common_settings_base.h>
#include <widgets/zoom_correction_ctrl.h>


class COMMON_SETTINGS;


class PANEL_COMMON_SETTINGS : public PANEL_COMMON_SETTINGS_BASE
{
public:
    PANEL_COMMON_SETTINGS( wxWindow* aParent );
    ~PANEL_COMMON_SETTINGS() override;

    void ResetPanel() override;

protected:
    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void applySettingsToPanel( COMMON_SETTINGS& aSettings );

    void OnTextEditorClick( wxCommandEvent& event ) override;
    void OnPDFViewerClick( wxCommandEvent& event ) override;
    void OnRadioButtonPdfViewer( wxCommandEvent& event ) override;

    /**
     * Event fired when the canvas scale field is modified
     */
    void OnCanvasScaleChange( wxCommandEvent& aEvent );

    /**
     * Event fired when the canvas auto-scale option is changed
     */
    void OnCanvasScaleAuto( wxCommandEvent& aEvent ) override;

private:
    void ShowFileManagerWidgets( bool aBool );
    void setPdfViewerPathState();

    ZOOM_CORRECTION_CTRL* m_zoomCorrectionCtrl;
};
