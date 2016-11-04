/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2016 KiCad Developers, see change_log.txt for contributors.
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
class LIB_ALIAS;
class LIB_PART;


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

    void OnSize( wxSizeEvent& event ) override;

    /**
     * Function ReCreateListLib
     *
     * Creates or recreates the list of current loaded libraries.
     * This list is sorted, with the library cache always at end of the list
     */
    void ReCreateListLib();

    void ReCreateListCmp();
    void DisplayLibInfos();
    void RedrawActiveWindow( wxDC* DC, bool EraseBg ) override;
    void OnCloseWindow( wxCloseEvent& Event );
    void CloseLibraryViewer( wxCommandEvent& event );
    void ReCreateHToolbar() override;
    void ReCreateVToolbar() override;
    void ReCreateMenuBar() override;

    void OnLeftClick( wxDC* DC, const wxPoint& MousePos ) override;
    double BestZoom() override;
    void ClickOnLibList( wxCommandEvent& event );
    void ClickOnCmpList( wxCommandEvent& event );
    void OnSetRelativeOffset( wxCommandEvent& event );
    void OnSelectSymbol( wxCommandEvent& aEvent );
    void OnShowElectricalType( wxCommandEvent& event );

    bool GeneralControl( wxDC* aDC, const wxPoint& aPosition, EDA_KEY aHotKey = 0 ) override;

    ///> @copydoc EDA_DRAW_FRAME::GetHotKeyDescription()
    EDA_HOTKEY* GetHotKeyDescription( int aCommand ) const override;

    /**
     * Function OnHotKey
     * handle hot key events.
     * <p?
     * Some commands are relative to the item under the mouse cursor.  Commands are
     * case insensitive
     * </p>
     */
    bool OnHotKey( wxDC* aDC, int aHotKey, const wxPoint& aPosition,
                   EDA_ITEM* aItem = NULL ) override;

    void LoadSettings( wxConfigBase* aCfg ) override;
    void SaveSettings( wxConfigBase* aCfg ) override;

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

    bool GetShowElectricalType() { return m_showPinElectricalTypeName; }
    void SetShowElectricalType( bool aShow ) { m_showPinElectricalTypeName = aShow; }

private:
    /**
     * Function OnActivate
     * is called when the frame frame is activate to reload the libraries and component lists
     * that can be changed by the schematic editor or the library editor.
     */
    virtual void OnActivate( wxActivateEvent& event ) override;

    /**
     * Function ExportToSchematicLibraryPart
     * exports the current component to schematic and close the library browser.
     */
    void ExportToSchematicLibraryPart( wxCommandEvent& event );
    bool OnRightClick( const wxPoint& MousePos, wxMenu* PopMenu ) override;
    void DClickOnCmpList( wxCommandEvent& event );

    void onUpdateAlternateBodyStyleButton( wxUpdateUIEvent& aEvent );
    void onUpdateNormalBodyStyleButton( wxUpdateUIEvent& aEvent );
    void onUpdateViewDoc( wxUpdateUIEvent& aEvent );
    void OnUpdateElectricalType( wxUpdateUIEvent& aEvent );
    void onSelectNextSymbol( wxCommandEvent& aEvent );
    void onSelectPreviousSymbol( wxCommandEvent& aEvent );
    void onViewSymbolDocument( wxCommandEvent& aEvent );
    void onSelectSymbolBodyStyle( wxCommandEvent& aEvent );
    void onSelectSymbolUnit( wxCommandEvent& aEvent );

    LIB_ALIAS* getSelectedAlias();
    LIB_PART* getSelectedSymbol();

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

    // TODO(hzeller): looks like these members were chosen to be static to survive different
    // instances of this browser and communicate it to the next instance. This looks like an
    // ugly hack, and should be solved differently.
    static wxString m_libraryName;

    static wxString m_entryName;

    static int      m_unit;
    static int      m_convert;

    /**
     * the option to show the pin electrical name in the component editor
     */
    bool m_showPinElectricalTypeName;

    DECLARE_EVENT_TABLE()
};

#endif  // LIBVIEWFRM_H_
