/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

class wxListBox;
class SCHLIB_FILTER;
class LIB_ALIAS;
class LIB_PART;
class SYMBOL_LIB_TABLE_ROW;


/**
 * Symbol library viewer main window.
 */
class LIB_VIEW_FRAME : public SCH_BASE_FRAME
{
public:

    /**
     * Constructor
     * @param aKiway
     * @param aParent = the parent frame
     * @param aFrameType must be either FRAME_SCH_LIB_VIEWER or FRAME_SCH_LIB_VIEWER_MODAL
     * @param aLibrary = the library to open when starting (default = NULL)
     */
    LIB_VIEW_FRAME( KIWAY* aKiway, wxWindow* aParent,
                    FRAME_T aFrameType, const wxString& aLibraryName = wxEmptyString );

    ~LIB_VIEW_FRAME();

    /**
     * Function ShowModal
     *
     * Runs the Symbol Viewer as a modal dialog.
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
     * Creates or recreates a sorted list of currently loaded libraries.
     *
     * @return whether the selection of either library or component was changed (i.e. because the
     * selected library no longer exists)
     */
    bool ReCreateListLib();

    /**
     * Create or recreate the list of components in the currently selected library.
     *
     * @return whether the selection was changed (i.e. because the selected component no longer
     * exists)
     */
    bool ReCreateListCmp();

    void DisplayLibInfos();
    void OnCloseWindow( wxCloseEvent& Event );
    void CloseLibraryViewer( wxCommandEvent& event );
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateOptToolbar() override {}
    void ReCreateMenuBar() override;

    double BestZoom() override;
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnCmpList( wxCommandEvent& event );
    void OnSelectSymbol( wxCommandEvent& aEvent );

    void LoadSettings( wxConfigBase* aCfg ) override;
    void SaveSettings( wxConfigBase* aCfg ) override;
    void CommonSettingsChanged( bool aEnvVarsChanged ) override;

    /**
     * Set a filter to display only libraries and/or components which match the filter.
     *
     * @param aFilter is a filter to pass the allowed library name list and/or some other filter
     *                see SCH_BASE_FRAME::SelectComponentFromLibrary() for details.
     *                if aFilter == NULL, remove all filtering
     */
    void SetFilter( const SCHLIB_FILTER* aFilter );

    /**
     * Set the selected library in the library window.
     *
     * @param aLibName name of the library to be selected.
     */
    void SetSelectedLibrary( const wxString& aLibName );

    /**
     * Set the selected component.
     *
     * @param aComponentName : the name of the component to be selected.
     */
    void SetSelectedComponent( const wxString& aComponentName );

    // Accessors:
    /**
     * Set unit and convert, and set flag preventing them from automatically resetting to 1
     *
     * @param aUnit - unit; if invalid will be set to 1
     * @param aConvert - convert; if invalid will be set to 1
     */
    void SetUnitAndConvert( int aUnit, int aConvert );
    int GetUnit() const { return m_unit; }
    int GetConvert() const { return m_convert; }

    LIB_PART* GetSelectedSymbol() const;
    LIB_ALIAS* GetSelectedAlias() const;

    const BOX2I GetDocumentExtents() const override;

    void SyncToolbars() override;

private:
    // Sets up the tool framework
    void setupTools();

    /**
     * Called when the frame is activated to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    void OnActivate( wxActivateEvent& event );

    void DClickOnCmpList( wxCommandEvent& event );

    void onUpdateUnitChoice( wxUpdateUIEvent& aEvent );

    void onSelectNextSymbol( wxCommandEvent& aEvent );
    void onSelectPreviousSymbol( wxCommandEvent& aEvent );
    void onSelectSymbolUnit( wxCommandEvent& aEvent );

    void updatePreviewSymbol();

// Private members:
    wxChoice*           m_unitChoice;

    // List of libraries (for selection )
    wxListBox*          m_libList;          // The list of libs
    int                 m_libListWidth;     // Last width of the window

    // List of components in the selected library
    wxListBox*          m_cmpList;          // The list of components
    int                 m_cmpListWidth;     // Last width of the window

    // Filters to build list of libs/list of parts
    bool                m_listPowerCmpOnly;
    wxArrayString       m_allowedLibs;

    static wxString     m_libraryName;
    static wxString     m_entryName;

    static int          m_unit;
    static int          m_convert;

    /**
     * Updated to `true` if a list rewrite on GUI activation resulted in the component
     * selection changing, or if the user has changed the selection manually.
     */
    bool m_selection_changed;

    LIB_ALIAS*      m_previewItem;

    DECLARE_EVENT_TABLE()
};

#endif  // LIB_VIEW_FRAME_H__

