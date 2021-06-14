/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef LIB_VIEW_FRAME_H__
#define LIB_VIEW_FRAME_H__

#include <sch_base_frame.h>
#include <sch_screen.h>
#include <tool/selection.h>

class wxListBox;
class SCHLIB_FILTER;
class LIB_SYMBOL;
class SYMBOL_LIB_TABLE_ROW;


/**
 * Symbol library viewer main window.
 */
class SYMBOL_VIEWER_FRAME : public SCH_BASE_FRAME
{
public:

    /**
     * @param aKiway
     * @param aParent is the parent frame of the viewer.
     * @param aFrameType must be either #FRAME_SCH_LIB_VIEWER or #FRAME_SCH_LIB_VIEWER_MODAL.
     * @param aLibrary is the library to open when starting (default = NULL).
     */
    SYMBOL_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent, FRAME_T aFrameType,
                         const wxString& aLibraryName = wxEmptyString );

    ~SYMBOL_VIEWER_FRAME();

    /**
     * Runs the symbol viewer as a modal dialog.
     *
     * @param aSymbol an optional FPID string to initialize the viewer with and to
     *                return a selected footprint through.
     */
    bool ShowModal( wxString* aSymbol, wxWindow* aParent ) override;

    /**
     * Send the selected symbol back to the caller.
     */
    void FinishModal();

    void OnSize( wxSizeEvent& event ) override;

    /**
     * Create o recreates a sorted list of currently loaded libraries.
     *
     * @return whether the selection of either library or symbol was changed (i.e. because the
     *         selected library no longer exists).
     */
    bool ReCreateLibList();

    /**
     * Create or recreate the list of symbols in the currently selected library.
     *
     * @return whether the selection was changed (i.e. because the selected symbol no longer
     *         exists).
     */
    bool ReCreateSymbolList();

    void DisplayLibInfos();
    void doCloseWindow() override;
    void CloseLibraryViewer( wxCommandEvent& event );
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override {}
    void ReCreateMenuBar() override;

    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnCmpList( wxCommandEvent& event );
    void OnSelectSymbol( wxCommandEvent& aEvent );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    void CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged ) override;

    /**
     * Set a filter to display only libraries and/or symbols which match the filter.
     *
     * @param aFilter is a filter to pass the allowed library name list and/or some other filter
     *                see SCH_BASE_FRAME::SelectSymbolFromLibrary() for details.
     *                if aFilter == NULL, remove all filtering.
     */
    void SetFilter( const SCHLIB_FILTER* aFilter );

    /**
     * Set the selected library in the library window.
     */
    void SetSelectedLibrary( const wxString& aLibName );

    /**
     * Set the selected symbol.
     */
    void SetSelectedSymbol( const wxString& aSymbolName );

    // Accessors:
    /**
     * Set unit and convert, and set flag preventing them from automatically resetting to 1.
     *
     * @param aUnit is the unit, if invalid will be set to 1.
     * @param aConvert is the alternate body style, if invalid will be set to 1.
     */
    void SetUnitAndConvert( int aUnit, int aConvert );
    int GetUnit() const { return m_unit; }
    int GetConvert() const { return m_convert; }

    LIB_SYMBOL* GetSelectedSymbol() const;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    SELECTION& GetCurrentSelection() override;

protected:
    void setupUIConditions() override;

private:
    // Set up the tool framework.
    void setupTools();

    /**
     * Called when the frame is activated to reload the libraries and symbol lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );

    void DClickOnCmpList( wxCommandEvent& event );

    void onUpdateUnitChoice( wxUpdateUIEvent& aEvent );

    void onSelectNextSymbol( wxCommandEvent& aEvent );
    void onSelectPreviousSymbol( wxCommandEvent& aEvent );
    void onSelectSymbolUnit( wxCommandEvent& aEvent );

    void updatePreviewSymbol();

private:
    wxChoice*           m_unitChoice;

    wxListBox*          m_libList;             // The list of libraries.
    int                 m_libListWidth;        // Last width of the window.

    wxListBox*          m_symbolList;          // The list of symbols.
    int                 m_symbolListWidth;     // Last width of the window.

    // Filters to build list of libs/list of symbols.
    bool                m_listPowerOnly;
    wxArrayString       m_allowedLibs;

    static wxString     m_libraryName;
    static wxString     m_entryName;

    static int          m_unit;
    static int          m_convert;

    /**
     * Updated to `true` if a list rewrite on GUI activation resulted in the symbol
     * selection changing, or if the user has changed the selection manually.
     */
    bool m_selection_changed;

    LIB_SYMBOL* m_previewItem;

    DECLARE_EVENT_TABLE()
};

#endif  // LIB_VIEW_FRAME_H__

