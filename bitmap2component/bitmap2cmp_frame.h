/*
 * This program source code file is part of KICAD, a free EDA CAD application.
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
#ifndef BITMOP2CMP_GUI_H_
#define BITMOP2CMP_GUI_H_

#include <kiway_player.h>
#include <bitmap2cmp_frame.h>

#include <eda_units.h>
#include <potracelib.h>

class BITMAP2CMP_PANEL;

class BITMAP2CMP_FRAME : public KIWAY_PLAYER
{
public:
    BITMAP2CMP_FRAME( KIWAY* aKiway, wxWindow* aParent );
    ~BITMAP2CMP_FRAME();

    // overload KIWAY_PLAYER virtual
    bool OpenProjectFiles( const std::vector<wxString>& aFilenames, int aCtl = 0 ) override;

    void OnLoadFile();

    void OnFileHistory( wxCommandEvent& event );
    void OnClearFileHistory( wxCommandEvent& event );

    /**
     * Generate a schematic library which contains one component:
     * the logo
     */
    void ExportEeschemaFormat();

    /**
     * Generate a footprint in S expr format
     */
    void ExportPcbnewFormat();

    /**
     * Generate a postscript file
     */
    void ExportPostScriptFormat();

    /**
     * Generate a file suitable to be copied into a drawing sheet (.kicad_wks) file
     */
    void ExportDrawingSheetFormat();

    void UpdateTitle();
    void ShowChangedLanguage() override;

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    wxWindow* GetToolCanvas() const override;

    /**
     * Event handler for the wxID_EXIT and wxID_CLOSE events.
     */
    void OnExit( wxCommandEvent& aEvent );

protected:
    void doReCreateMenuBar() override;

    DECLARE_EVENT_TABLE();

private:
    BITMAP2CMP_PANEL* m_panel;
    wxStatusBar*      m_statusBar;

    wxString          m_srcFileName;
    wxString          m_outFileName;
};

#endif// BITMOP2CMP_GUI_H_
