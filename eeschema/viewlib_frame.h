/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2014 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file viewlib_frame.h
 */

#ifndef LIBVIEWFRM_H_
#define LIBVIEWFRM_H_


#include <wx/gdicmn.h>

#include <sch_base_frame.h>
#include <class_sch_screen.h>

class wxListBox;
class PART_LIB;
class SCHLIB_FILTER;


/**
 * Component library viewer main window.
 */
class LIB_VIEW_FRAME : public SCH_BASE_FRAME
{
public:

    /**
     * Constructor
     * @param aKiway
     * @param aParent = the parent frame
     * @param aFrameType must be given either FRAME_SCH_LIB_VIEWER or
     *  FRAME_SCH_LIB_VIEWER_MODAL
     * @param aLibrary = the library to open when starting (default = NULL)
     */
    LIB_VIEW_FRAME( KIWAY* aKiway, wxWindow* aParent,
            FRAME_T aFrameType, PART_LIB* aLibrary = NULL );

    ~LIB_VIEW_FRAME();

    /**
     * Function GetLibViewerFrameName (static)
     * @return the frame name used when creating the frame
     * used to get a reference to this frame, if exists
     */
    static const wxChar* GetLibViewerFrameName();

    void OnSize( wxSizeEvent& event );

    /**
     * Function ReCreateListLib
     *
     * Creates or recreates the list of current loaded libraries.
     * This list is sorted, with the library cache always at end of the list
     */
    void ReCreateListLib();

    void ReCreateListCmp();
    void Process_Special_Functions( wxCommandEvent& event );
    void DisplayLibInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg );
    void OnCloseWindow( wxCloseEvent& Event );
    void CloseLibraryViewer( wxCommandEvent& event );
    void ReCreateHToolbar();
    void ReCreateVToolbar();
    void ReCreateMenuBar();

    void OnLeftClick( wxDC* DC, const wxPoint& MousePos );
    double BestZoom();
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnCmpList( wxCommandEvent& event );
    void OnSetRelativeOffset( wxCommandEvent& event );

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, int aHotKey = 0 );

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const;

    /**
     * Function OnHotKey
     * handle hot key events.
     * <p?
     * Some commands are relative to the item under the mouse cursor.  Commands are
     * case insensitive
     * </p>
     */
    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition, EDA_ITEM* aItem = NULL );

    void LoadSettings( wxConfigBase* aCfg );
    void SaveSettings( wxConfigBase* aCfg );

    /**
     * set a filter to display only libraries and/or components
     * which match the filter
     *
     * @param aFilter is a filter to pass the allowed library name list
     *          and/or some other filter
     *  see SCH_BASE_FRAME::SelectComponentFromLibrary() for details.
     * if aFilter == NULL, remove all filtering
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
    void SetUnit( int aUnit ) { m_unit = aUnit; }
    int GetUnit( void ) { return m_unit; }

    void SetConvert( int aConvert ) { m_convert = aConvert; }
    int GetConvert( void ) { return m_convert; }

private:
    /**
     * Function OnActivate
     * is called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    virtual void OnActivate( wxActivateEvent& event );

    void SelectCurrentLibrary();
    void SelectAndViewLibraryPart( int option );

    /**
     * Function ExportToSchematicLibraryPart
     * exports the current component to schematic and close the library browser.
     */
    void ExportToSchematicLibraryPart( wxCommandEvent& event );
    void ViewOneLibraryContent( PART_LIB* Lib, int Flag );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu );
    void DClickOnCmpList( wxCommandEvent& event );

// Private members:
    wxComboBox*         m_selpartBox;

    // List of libraries (for selection )
    wxListBox*          m_libList;          // The list of libs
    int                 m_libListWidth;     // Last width of the window

    // List of components in the selected library
    wxListBox*          m_cmpList;          // The list of components
    int                 m_cmpListWidth;     // Last width of the window

    // Filters to build list of libs/list of parts
    bool                m_listPowerCmpOnly;
    wxArrayString       m_allowedLibs;

    wxString            m_configPath;       // subpath for configuration

    // TODO(hzeller): looks like these members were chosen to be static to survive different
    // instances of this browser and communicate it to the next instance. This looks like an
    // ugly hack, and should be solved differently.
    static wxString m_libraryName;

    static wxString m_entryName;

    static int      m_unit;
    static int      m_convert;

    DECLARE_EVENT_TABLE()
};

#endif  // LIBVIEWFRM_H_
