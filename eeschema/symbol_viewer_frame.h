/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
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


#ifndef LIB_VIEW_FRAME_H__
#define LIB_VIEW_FRAME_H__

#include <sch_base_frame.h>
#include <sch_screen.h>
#include <tool/selection.h>

class WX_LISTBOX;
class wxSearchCtrl;
class SYMBOL_LIBRARY_FILTER;
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
    SYMBOL_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent );

    ~SYMBOL_VIEWER_FRAME();

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

    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnSymbolList( wxCommandEvent& event );

    void LoadSettings( APP_SETTINGS_BASE* aCfg ) override;
    void SaveSettings( APP_SETTINGS_BASE* aCfg ) override;

    WINDOW_SETTINGS* GetWindowSettings( APP_SETTINGS_BASE* aCfg ) override;

    void CommonSettingsChanged( int aFlags ) override;

    /**
     * Set the selected library in the library window.
     */
    void SetSelectedLibrary( const wxString& aLibName,
                             const wxString& aSubLibName = wxEmptyString );

    /**
     * Set the selected symbol.
     */
    void SetSelectedSymbol( const wxString& aSymbolName );

    // Accessors:
    /**
     * Set unit and convert, and set flag preventing them from automatically resetting to 1.
     *
     * @param aUnit is the unit, if invalid will be set to 1.
     * @param aBodyStyle is the alternate body style, if invalid will be set to 1.
     */
    void SetUnitAndBodyStyle( int aUnit, int aBodyStyle );
    int GetUnit() const { return m_unit; }
    int GetBodyStyle() const { return m_bodyStyle; }

    LIB_SYMBOL* GetSelectedSymbol() const;

    const BOX2I GetDocumentExtents( bool aIncludeAllVisible = true ) const override;

    SELECTION& GetCurrentSelection() override;

    void KiwayMailIn( KIWAY_EXPRESS& mail ) override;

protected:
    void configureToolbars() override;

    void setupUIConditions() override;

    void doReCreateMenuBar() override;

private:
    // Set up the tool framework.
    void setupTools();

    /**
     * Called when the frame is activated to reload the libraries and symbol lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );

    void DClickOnSymbolList( wxCommandEvent& event );

    void onUpdateUnitChoice( wxUpdateUIEvent& aEvent );
    void onUpdateBodyStyleChoice( wxUpdateUIEvent& aEvent );

    void OnLibFilter( wxCommandEvent& aEvent );
    void OnSymFilter( wxCommandEvent& aEvent );
    void OnCharHook( wxKeyEvent& aEvent ) override;

    void onSelectNextSymbol( wxCommandEvent& aEvent );
    void onSelectPreviousSymbol( wxCommandEvent& aEvent );
    void onSelectSymbolUnit( wxCommandEvent& aEvent );
    void onSelectSymbolBodyStyle( wxCommandEvent& aEvent );

    void updatePreviewSymbol();

private:
    wxChoice*           m_unitChoice;
    wxChoice*           m_bodyStyleChoice;

    wxSearchCtrl*       m_libFilter;
    WX_LISTBOX*         m_libList;             // The list of libraries.
    int                 m_libListWidth;        // Last width of the window.

    wxSearchCtrl*       m_symbolFilter;
    WX_LISTBOX*         m_symbolList;          // The list of symbols.
    int                 m_symbolListWidth;     // Last width of the window.

    // Filters to build list of libs/list of symbols.
    bool                m_listPowerOnly;
    wxArrayString       m_allowedLibs;

    static LIB_ID       m_currentSymbol;

    static int          m_unit;
    static int          m_bodyStyle;
    static bool         m_show_progress;

    /**
     * Updated to `true` if a list rewrite on GUI activation resulted in the symbol
     * selection changing, or if the user has changed the selection manually.
     */
    bool                m_selection_changed;

    std::unique_ptr<LIB_SYMBOL> m_previewItem;

    DECLARE_EVENT_TABLE()
};

#endif  // LIB_VIEW_FRAME_H__

