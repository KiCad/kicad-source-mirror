/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/combo.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>

#include <bitmap_types.h>
#include <bitmaps.h>
#include <kiway.h>
#include <kiway_player.h>
#include <dialog_shim.h>
#include <common.h>
#include <env_paths.h>
#include <widgets/wx_grid.h>
#include <widgets/grid_text_button_helpers.h>
#include <eda_doc.h>


//-------- Renderer ---------------------------------------------------------------------
// None required; just render as normal text.



//-------- Editor Base Class ------------------------------------------------------------
//
// Note: this implementation is an adaptation of wxGridCellChoiceEditor


wxString GRID_CELL_TEXT_BUTTON::GetValue() const
{
    return Combo()->GetValue();
}


void GRID_CELL_TEXT_BUTTON::SetSize( const wxRect& aRect )
{
    wxRect rect( aRect );
    rect.Inflate( -1 );

#if defined( __WXMAC__ )
    rect.Inflate( 3 );      // no FOCUS_RING, even on Mac
#endif

    Combo()->SetSize( rect, wxSIZE_ALLOW_MINUS_ONE );
}


void GRID_CELL_TEXT_BUTTON::StartingKey( wxKeyEvent& event )
{
    // Note: this is a copy of wxGridCellTextEditor's StartingKey()

    // Since this is now happening in the EVT_CHAR event EmulateKeyPress is no
    // longer an appropriate way to get the character into the text control.
    // Do it ourselves instead.  We know that if we get this far that we have
    // a valid character, so not a whole lot of testing needs to be done.

    // wxComboCtrl inherits from wxTextEntry, so can staticly cast
    wxTextEntry* textEntry = static_cast<wxTextEntry*>( Combo() );
    int ch;

    bool isPrintable;

#if wxUSE_UNICODE
    ch = event.GetUnicodeKey();

    if( ch != WXK_NONE )
        isPrintable = true;
    else
#endif // wxUSE_UNICODE
    {
        ch = event.GetKeyCode();
        isPrintable = ch >= WXK_SPACE && ch < WXK_START;
    }

    switch( ch )
    {
    case WXK_DELETE:
        // Delete the initial character when starting to edit with DELETE.
        textEntry->Remove( 0, 1 );
        break;

    case WXK_BACK:
        // Delete the last character when starting to edit with BACKSPACE.
    {
        const long pos = textEntry->GetLastPosition();
        textEntry->Remove( pos - 1, pos );
    }
        break;

    default:
        if( isPrintable )
            textEntry->WriteText( static_cast<wxChar>( ch ) );
        break;
    }
}


void GRID_CELL_TEXT_BUTTON::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    auto evtHandler = static_cast< wxGridCellEditorEvtHandler* >( m_control->GetEventHandler() );

    // Don't immediately end if we get a kill focus event within BeginEdit
    evtHandler->SetInSetFocus( true );

    m_value = aGrid->GetTable()->GetValue( aRow, aCol );

    Combo()->SetValue( m_value );
    Combo()->SetFocus();
}


bool GRID_CELL_TEXT_BUTTON::EndEdit( int, int, const wxGrid*, const wxString&, wxString *aNewVal )
{
    const wxString value = Combo()->GetValue();

    if( value == m_value )
        return false;

    m_value = value;

    if( aNewVal )
        *aNewVal = value;

    return true;
}


void GRID_CELL_TEXT_BUTTON::ApplyEdit( int aRow, int aCol, wxGrid* aGrid )
{
    aGrid->GetTable()->SetValue( aRow, aCol, m_value );
}


void GRID_CELL_TEXT_BUTTON::Reset()
{
    Combo()->SetValue( m_value );
}


#if wxUSE_VALIDATORS
void GRID_CELL_TEXT_BUTTON::SetValidator( const wxValidator& validator )
{
    m_validator.reset( static_cast< wxValidator* >( validator.Clone() ) );
}
#endif


class TEXT_BUTTON_SYMBOL_CHOOSER : public wxComboCtrl
{
public:
    TEXT_BUTTON_SYMBOL_CHOOSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg,
                                const wxString& aPreselect ) :
            wxComboCtrl( aParent ),
            m_dlg( aParentDlg ),
            m_preselect( aPreselect )
    {
        SetButtonBitmaps( KiBitmap( small_library_xpm ) );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        // pick a footprint using the footprint picker.
        wxString      compid = GetValue();

        if( compid.IsEmpty() )
            compid = m_preselect;

        KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_SCH_VIEWER_MODAL, true, m_dlg );

        if( frame->ShowModal( &compid, m_dlg ) )
            SetValue( compid );

        frame->Destroy();
    }

    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;
};


void GRID_CELL_SYMBOL_ID_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                         wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_SYMBOL_CHOOSER( aParent, m_dlg, m_preselect );

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


class TEXT_BUTTON_FP_CHOOSER : public wxComboCtrl
{
public:
    TEXT_BUTTON_FP_CHOOSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg,
                            const wxString& aPreselect ) :
            wxComboCtrl( aParent ),
            m_dlg( aParentDlg ),
            m_preselect( aPreselect )
    {
        SetButtonBitmaps( KiBitmap( small_library_xpm ) );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        // pick a footprint using the footprint picker.
        wxString      fpid = GetValue();

        if( fpid.IsEmpty() )
            fpid = m_preselect;

        KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true, m_dlg );

        if( frame->ShowModal( &fpid, m_dlg ) )
            SetValue( fpid );

        frame->Destroy();
    }

    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;
};


void GRID_CELL_FOOTPRINT_ID_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                            wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_FP_CHOOSER( aParent, m_dlg, m_preselect );

#if wxUSE_VALIDATORS
    // validate text in textctrl, if validator is set
    if ( m_validator )
    {
        Combo()->SetValidator( *m_validator );
    }
#endif

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


class TEXT_BUTTON_URL : public wxComboCtrl
{
public:
    TEXT_BUTTON_URL( wxWindow* aParent, DIALOG_SHIM* aParentDlg ) :
            wxComboCtrl( aParent ),
            m_dlg( aParentDlg )
    {
        SetButtonBitmaps( KiBitmap( www_xpm ) );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        wxString filename = GetValue();

        if( !filename.IsEmpty() && filename != wxT( "~" ) )
            GetAssociatedDocument( m_dlg, GetValue(), &m_dlg->Prj() );
    }

    DIALOG_SHIM* m_dlg;
};


void GRID_CELL_URL_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                   wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_URL( aParent, m_dlg );

#if wxUSE_VALIDATORS
    // validate text in textctrl, if validator is set
    if ( m_validator )
    {
        Combo()->SetValidator( *m_validator );
    }
#endif

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


class TEXT_BUTTON_FILE_BROWSER : public wxComboCtrl
{
public:
    TEXT_BUTTON_FILE_BROWSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg, WX_GRID* aGrid,
                              wxString* aCurrentDir, wxString* aExt = nullptr,
                              bool aNormalize = false,
                              wxString aNormalizeBasePath = wxEmptyString ) :
            wxComboCtrl( aParent ),
            m_dlg( aParentDlg ),
            m_grid( aGrid ),
            m_currentDir( aCurrentDir ),
            m_ext( aExt ),
            m_normalize( aNormalize ),
            m_normalizeBasePath( aNormalizeBasePath )
    {
        SetButtonBitmaps( KiBitmap( small_folder_xpm ) );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        wxString path = GetValue();

        if( path.IsEmpty() )
            path = *m_currentDir;
        else
            path = ExpandEnvVarSubstitutions( path, &m_dlg->Prj() );

        if( m_ext )
        {
            wxFileName fn( path );
            wxFileDialog dlg( nullptr, _( "Select a File" ), fn.GetPath(), fn.GetFullName(), *m_ext,
                    wxFD_FILE_MUST_EXIST | wxFD_OPEN );

            if( dlg.ShowModal() == wxID_OK )
            {
                wxString filePath = dlg.GetPath();
                wxString lastPath = dlg.GetDirectory();
                wxString relPath = wxEmptyString;

                if( m_normalize )
                {
                    relPath = NormalizePath( filePath, &Pgm().GetLocalEnvVariables(),
                                             m_normalizeBasePath );
                    lastPath = NormalizePath( dlg.GetDirectory(), &Pgm().GetLocalEnvVariables(),
                                              m_normalizeBasePath );
                }

                if( relPath.IsEmpty() )
                    relPath = filePath;

                SetValue( relPath );
                m_grid->CommitPendingChanges();
                *m_currentDir = lastPath;
            }
        }
        else
        {
            wxDirDialog dlg( nullptr, _( "Select Path" ), path,
                             wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST );

            if( dlg.ShowModal() == wxID_OK )
            {
                wxString filePath = dlg.GetPath();
                wxString relPath = wxEmptyString;

                if ( m_normalize )
                {
                    relPath = NormalizePath( filePath, &Pgm().GetLocalEnvVariables(),
                                             m_normalizeBasePath );
                }

                if( relPath.IsEmpty() )
                    relPath = filePath;

                SetValue( relPath );
                m_grid->CommitPendingChanges();
                *m_currentDir = relPath;
            }
        }
    }

    DIALOG_SHIM* m_dlg;
    WX_GRID*     m_grid;
    wxString*    m_currentDir;
    wxString*    m_ext;
    bool         m_normalize;
    wxString     m_normalizeBasePath;
};


void GRID_CELL_PATH_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                    wxEvtHandler* aEventHandler )
{
    if( m_ext.IsEmpty() )
        m_control = new TEXT_BUTTON_FILE_BROWSER( aParent, m_dlg, m_grid, m_currentDir, nullptr,
                                                  m_normalize, m_normalizeBasePath );
    else
        m_control = new TEXT_BUTTON_FILE_BROWSER( aParent, m_dlg, m_grid, m_currentDir, &m_ext,
                                                  m_normalize, m_normalizeBasePath );

#if wxUSE_VALIDATORS
    // validate text in textctrl, if validator is set
    if ( m_validator )
    {
        Combo()->SetValidator( *m_validator );
    }
#endif

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}