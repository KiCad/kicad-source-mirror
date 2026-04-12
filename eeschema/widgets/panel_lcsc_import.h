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

#ifndef PANEL_LCSC_IMPORT_H
#define PANEL_LCSC_IMPORT_H

#include <wx/panel.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/timer.h>
#include <lib_id.h>

class SCH_BASE_FRAME;
class SYMBOL_PREVIEW_WIDGET;
class FOOTPRINT_PREVIEW_WIDGET;
class FOOTPRINT_3D_PREVIEW_WIDGET;


/**
 * Panel for importing symbols/footprints/3D models from LCSC/EasyEDA.
 *
 * Imports land in a per-session staging directory (/tmp/kicad_lcsc_<pid>/)
 * and are NOT written to the permanent library until the user clicks
 * "Add to Library".  A watchdog timer fires 400 ms after every preview
 * request and retries the display in case the first render missed.
 */
class PANEL_LCSC_IMPORT : public wxPanel
{
public:
    PANEL_LCSC_IMPORT( SCH_BASE_FRAME* aFrame, wxWindow* aParent,
                       std::function<void()> aAcceptHandler );
    ~PANEL_LCSC_IMPORT();

    LIB_ID GetSelectedLibId() const { return m_importedLibId; }
    bool   HasImportedSymbol() const { return m_importedLibId.IsValid(); }

    void ShutdownCanvases();

private:
    void onImportClicked( wxCommandEvent& aEvent );
    void onPartNumberEnter( wxCommandEvent& aEvent );
    void onResultSelected( wxListEvent& aEvent );
    void onUnitSelected( wxCommandEvent& aEvent );
    void onAddToLibClicked( wxCommandEvent& aEvent );
    void onWatchdogTimer( wxTimerEvent& aEvent );

    bool doImport( const wxString& aLcscId );
    void showPreview( const LIB_ID& aSymbolId, const wxString& aFootprint, int aUnitCount = 1 );

    /**
     * Merge the session staging directory into the permanent library.
     *
     * Copies new symbol entries, footprint files (fixing absolute 3D-model
     * paths) and 3D model files into ~/Documents/Kicad/LCSC_Parts/, then
     * updates the project library-table entry to point at the permanent dir.
     *
     * @return true on success.
     */
    bool addToLibrary();

    SCH_BASE_FRAME*              m_frame;
    std::function<void()>        m_acceptHandler;

    /// Per-session staging dir, e.g. /tmp/kicad_lcsc_<pid>/
    wxString                     m_tempDir;

    // Search bar
    wxTextCtrl*                  m_partNumberCtrl;
    wxButton*                    m_importBtn;
    wxButton*                    m_addToLibBtn;   ///< disabled until first import
    wxStaticText*                m_statusText;
    wxGauge*                     m_progressBar;

    // Results + preview layout
    wxSplitterWindow*            m_splitter;
    wxListCtrl*                  m_resultsList;

    // Symbol unit selector (shown only for multi-unit symbols)
    wxStaticText*                m_unitLabel;
    wxChoice*                    m_unitSelector;

    // Preview widgets
    SYMBOL_PREVIEW_WIDGET*       m_symbolPreview;
    wxNotebook*                  m_fpPreviewBook;
    FOOTPRINT_PREVIEW_WIDGET*    m_fpPreview;
    FOOTPRINT_3D_PREVIEW_WIDGET* m_fp3dPreview;

    LIB_ID                       m_importedLibId;
    LIB_ID                       m_lastFpLibId;

    /// Watchdog: retries preview once ~400 ms after showPreview()
    wxTimer*                     m_watchdog;
    int                          m_watchdogUnit;  ///< unit number to re-display

    // Track imported parts: LCSC ID -> { symbol name, footprint name, unit count }
    struct ImportedPart
    {
        wxString symbolName;
        wxString footprintName;
        int      unitCount = 1;
    };

    std::vector<ImportedPart>    m_importedParts;
};


#endif /* PANEL_LCSC_IMPORT_H */
