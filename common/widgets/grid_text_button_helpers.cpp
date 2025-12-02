/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 CERN
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

#include <wx/checkbox.h>
#include <wx/combo.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/textctrl.h>

#include <bitmaps.h>
#include <embedded_files.h>
#include <kiway.h>
#include <kiway_player.h>
#include <kiway_express.h>
#include <string_utils.h>
#include <dialog_shim.h>
#include <common.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <widgets/wx_grid.h>
#include <widgets/filedlg_hook_embed_file.h>
#include <widgets/grid_text_button_helpers.h>
#include <eda_doc.h>


//-------- Renderer ---------------------------------------------------------------------
// None required; just render as normal text.



class TEXT_BUTTON_SYMBOL_CHOOSER : public wxComboCtrl
{
public:
    TEXT_BUTTON_SYMBOL_CHOOSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg,
                                const wxString& aPreselect ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 0, 0 ) ),
            m_dlg( aParentDlg ),
            m_preselect( aPreselect )
    {
        SetButtonBitmaps( KiBitmapBundle( BITMAPS::small_library ) );

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    wxString escapeLibId( const wxString& aRawValue )
    {
        wxString itemName;
        wxString libName = aRawValue.BeforeFirst( ':', &itemName );
        return EscapeString( libName, CTX_LIBID ) + ':' + EscapeString( itemName, CTX_LIBID );
    }

    void OnButtonClick() override
    {
        // pick a symbol using the symbol picker.
        wxString rawValue = GetValue();

        if( rawValue.IsEmpty() )
            rawValue = m_preselect;

        wxString symbolId = escapeLibId( rawValue );

        if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_SYMBOL_CHOOSER, true, m_dlg ) )
        {
            if( frame->ShowModal( &symbolId, m_dlg ) )
                SetValue( UnescapeString( symbolId ) );

            frame->Destroy();
        }
    }

    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;
};


void GRID_CELL_SYMBOL_ID_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                         wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_SYMBOL_CHOOSER( aParent, m_dlg, m_preselect );
    WX_GRID::CellEditorSetMargins( Combo() );

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


class TEXT_BUTTON_FP_CHOOSER : public wxComboCtrl
{
public:
    TEXT_BUTTON_FP_CHOOSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg,
                            const wxString& aSymbolNetlist, const wxString& aPreselect ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 0, 0 ),
                         wxTE_PROCESS_ENTER | wxBORDER_NONE ),
            m_dlg( aParentDlg ),
            m_preselect( aPreselect ),
            m_symbolNetlist( aSymbolNetlist.ToStdString() )
    {
        m_buttonFpChooserLock = false;
        SetButtonBitmaps( KiBitmapBundle( BITMAPS::small_library ) );

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        if( m_buttonFpChooserLock ) // The button to show the FP chooser is clicked, but
                                    // a previous click is currently in progress.
            return;

        // Disable the button until we have finished processing it.  Normally this is not an issue
        // but if the footprint chooser is loading for the first time, it can be slow enough that
        // multiple clicks will cause multiple instances of the footprint loader process to start
        m_buttonFpChooserLock = true;

        // pick a footprint using the footprint picker.
        wxString fpid = GetValue();

        if( fpid.IsEmpty() )
            fpid = m_preselect;

        if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
        {
            if( !m_symbolNetlist.empty() )
            {
                KIWAY_EXPRESS event( FRAME_FOOTPRINT_CHOOSER, MAIL_SYMBOL_NETLIST,
                                     m_symbolNetlist );
                frame->KiwayMailIn( event );
            }

            if( frame->ShowModal( &fpid, m_dlg ) )
                SetValue( fpid );

            frame->Destroy();
        }

        m_buttonFpChooserLock = false;
    }

protected:
    DIALOG_SHIM* m_dlg;
    wxString     m_preselect;

    // Lock flag to lock the button to show the FP chooser
    // true when the button is busy, waiting all footprints loaded to
    // avoid running more than once the FP chooser
    bool         m_buttonFpChooserLock;

    /*
     * Symbol netlist format:
     *   pinNumber pinName <tab> pinNumber pinName...
     *   fpFilter fpFilter...
     */
    std::string  m_symbolNetlist;
};


void GRID_CELL_FPID_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                    wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_FP_CHOOSER( aParent, m_dlg, m_symbolNetlist, m_preselect );
    WX_GRID::CellEditorSetMargins( Combo() );

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
    TEXT_BUTTON_URL( wxWindow* aParent, DIALOG_SHIM* aParentDlg, SEARCH_STACK* aSearchStack,
                     std::vector<EMBEDDED_FILES*> aFilesStack ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 0, 0 ),
                         wxTE_PROCESS_ENTER | wxBORDER_NONE ),
            m_dlg( aParentDlg ),
            m_searchStack( aSearchStack ),
            m_filesStack( aFilesStack )
    {
        UpdateButtonBitmaps();

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );

        // Bind event to handle text changes
        Bind( wxEVT_TEXT, &TEXT_BUTTON_URL::OnTextChange, this );
    }

    ~TEXT_BUTTON_URL()
    {
        Unbind( wxEVT_TEXT, &TEXT_BUTTON_URL::OnTextChange, this );

        m_filesStack.clear();   // we don't own pointers
    }

    // We don't own any of our raw pointers, so compiler's copy c'tor an operator= are OK.

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        m_dlg->PrepareForModalSubDialog();

        wxString filename = GetValue();

        if( filename.IsEmpty() || filename == wxT( "~" ) )
        {
            FILEDLG_HOOK_EMBED_FILE customize;

            wxFileDialog openFileDialog( this, _( "Open file" ), "", "", _( "All Files" ) + wxT( " (*.*)|*.*" ),
                                         wxFD_OPEN | wxFD_FILE_MUST_EXIST );

            openFileDialog.SetCustomizeHook( customize );

            if( openFileDialog.ShowModal() == wxID_OK )
            {
                filename = openFileDialog.GetPath();
                wxFileName fn( filename );

                if( customize.GetEmbed() )
                {
                    EMBEDDED_FILES::EMBEDDED_FILE* result = m_filesStack[0]->AddFile( fn, false );
                    SetValue( result->GetLink() );
                }
                else
                {
                    SetValue( "file://" + filename );
                }
            }
        }
        else
        {
            GetAssociatedDocument( m_dlg, GetValue(), &m_dlg->Prj(), m_searchStack, m_filesStack );
        }

        m_dlg->CleanupAfterModalSubDialog();
    }

    void OnTextChange(wxCommandEvent& event)
    {
        UpdateButtonBitmaps();
        event.Skip(); // Ensure that other handlers can process this event too
    }

    void UpdateButtonBitmaps()
    {
        if( GetValue().IsEmpty() )
            SetButtonBitmaps( KiBitmapBundle( BITMAPS::small_folder ) );
        else
            SetButtonBitmaps( KiBitmapBundle( BITMAPS::www ) );
    }

protected:
    DIALOG_SHIM*                 m_dlg;
    SEARCH_STACK*                m_searchStack;     // No ownership of pointer
    std::vector<EMBEDDED_FILES*> m_filesStack;      // No ownership of pointers
};


void GRID_CELL_URL_EDITOR::Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_URL( aParent, m_dlg, m_searchStack, m_filesStack );
    WX_GRID::CellEditorSetMargins( Combo() );

#if wxUSE_VALIDATORS
    // validate text in textctrl, if validator is set
    if ( m_validator )
        Combo()->SetValidator( *m_validator );
#endif

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


class TEXT_BUTTON_FILE_BROWSER : public wxComboCtrl
{
public:
    TEXT_BUTTON_FILE_BROWSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg, WX_GRID* aGrid,
                              wxString* aCurrentDir, const wxString& aFileFilter = wxEmptyString,
                              bool aNormalize = false,
                              const wxString& aNormalizeBasePath = wxEmptyString,
                              std::function<wxString( const wxString& )> aEmbedCallback = nullptr ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 0, 0 ),
                         wxTE_PROCESS_ENTER | wxBORDER_NONE ),
            m_dlg( aParentDlg ),
            m_grid( aGrid ),
            m_currentDir( aCurrentDir ),
            m_normalize( aNormalize ),
            m_normalizeBasePath( aNormalizeBasePath ),
            m_fileFilter( aFileFilter ),
            m_embedCallback( std::move( aEmbedCallback ) )
    {
        SetButtonBitmaps( KiBitmapBundle( BITMAPS::small_folder ) );

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );
    }

    TEXT_BUTTON_FILE_BROWSER( wxWindow* aParent, DIALOG_SHIM* aParentDlg, WX_GRID* aGrid,
                              wxString* aCurrentDir,
                              std::function<wxString( WX_GRID* grid, int row )> aFileFilterFn,
                              bool aNormalize = false,
                              const wxString& aNormalizeBasePath = wxEmptyString,
                              std::function<wxString( const wxString& )> aEmbedCallback = nullptr ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 0, 0 ),
                         wxTE_PROCESS_ENTER | wxBORDER_NONE ),
            m_dlg( aParentDlg ),
            m_grid( aGrid ),
            m_currentDir( aCurrentDir ),
            m_normalize( aNormalize ),
            m_normalizeBasePath( aNormalizeBasePath ),
            m_fileFilterFn( std::move( aFileFilterFn ) ),
            m_embedCallback( std::move( aEmbedCallback ) )
    {
        SetButtonBitmaps( KiBitmapBundle( BITMAPS::small_folder ) );

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );
    }


protected:
    void DoSetPopupControl( wxComboPopup* popup ) override
    {
        m_popup = nullptr;
    }

    void OnButtonClick() override
    {
        m_dlg->PrepareForModalSubDialog();

        if( m_fileFilterFn )
            m_fileFilter = m_fileFilterFn( m_grid, m_grid->GetGridCursorRow() );

        wxFileName fn = GetValue();

        if( fn.GetPath().IsEmpty() && m_currentDir )
            fn.SetPath( *m_currentDir );
        else
            fn.SetPath( ExpandEnvVarSubstitutions( fn.GetPath(), &m_dlg->Prj() ) );

        if( !m_fileFilter.IsEmpty() )
        {
            FILEDLG_HOOK_EMBED_FILE customize( false );
            wxFileDialog dlg( m_dlg, _( "Select a File" ), fn.GetPath(), fn.GetFullName(),
                              m_fileFilter, wxFD_FILE_MUST_EXIST | wxFD_OPEN );

            if( m_embedCallback )
                dlg.SetCustomizeHook( customize );

            if( dlg.ShowModal() == wxID_OK )
            {
                wxString filePath = dlg.GetPath();
                wxString lastPath = dlg.GetDirectory();
                wxString relPath = wxEmptyString;

                if( m_embedCallback && customize.GetEmbed() )
                {
                    relPath = m_embedCallback( filePath );

                    if( relPath.IsEmpty() )
                    {
                        m_dlg->CleanupAfterModalSubDialog();
                        return;
                    }
                }
                else if( m_normalize )
                {
                    relPath = NormalizePath( filePath, &Pgm().GetLocalEnvVariables(),
                                             m_normalizeBasePath );
                    lastPath = NormalizePath( dlg.GetDirectory(), &Pgm().GetLocalEnvVariables(),
                                              m_normalizeBasePath );
                }
                else
                {
                    relPath = filePath;
                }

                SetValue( relPath );

                if( !m_grid->CommitPendingChanges() )
                {;} // shouldn't happen, but Coverity doesn't know that

                if( m_currentDir )
                    *m_currentDir = lastPath;
            }
        }
        else
        {
            wxDirDialog dlg( m_dlg, _( "Select Path" ), fn.GetPath(),
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
                else
                {
                    relPath = filePath;
                }

                SetValue( relPath );

                if( !m_grid->CommitPendingChanges() )
                {;} // shouldn't happen, but Coverity doesn't know that

                if( m_currentDir )
                    *m_currentDir = relPath;
            }
        }

        m_dlg->CleanupAfterModalSubDialog();
    }

protected:
    DIALOG_SHIM* m_dlg;
    WX_GRID*     m_grid;
    wxString*    m_currentDir;
    bool         m_normalize;
    wxString     m_normalizeBasePath;

    wxString                                            m_fileFilter;
    std::function<wxString( WX_GRID* aGrid, int aRow )> m_fileFilterFn;
    std::function<wxString( const wxString& )>          m_embedCallback;
};


void GRID_CELL_PATH_EDITOR::Create( wxWindow* aParent, wxWindowID aId,
                                    wxEvtHandler* aEventHandler )
{
    if( m_fileFilterFn )
    {
        m_control = new TEXT_BUTTON_FILE_BROWSER( aParent, m_dlg, m_grid, m_currentDir,
                                                  m_fileFilterFn, m_normalize,
                                                  m_normalizeBasePath, m_embedCallback );
    }
    else
    {
        m_control = new TEXT_BUTTON_FILE_BROWSER( aParent, m_dlg, m_grid, m_currentDir,
                                                  m_fileFilter, m_normalize, m_normalizeBasePath,
                                                  m_embedCallback );
    }

    WX_GRID::CellEditorSetMargins( Combo() );

#if wxUSE_VALIDATORS
    // validate text in textctrl, if validator is set
    if ( m_validator )
        Combo()->SetValidator( *m_validator );
#endif

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}

class TEXT_BUTTON_RUN_FUNCTION final : public wxComboCtrl
{
public:
    TEXT_BUTTON_RUN_FUNCTION( wxWindow* aParent, DIALOG_SHIM* aParentDlg, std::function<void( int, int )>& aFunction,
                              int& aRow, int& aCol ) :
            wxComboCtrl( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 0, 0 ),
                         wxTE_PROCESS_ENTER | wxBORDER_NONE ),
            m_dlg( aParentDlg ),
            m_function( aFunction ),
            m_row( aRow ),
            m_col( aCol )
    {
        SetButtonBitmaps( KiBitmapBundle( BITMAPS::small_refresh ) );

        // win32 fix, avoids drawing the "native dropdown caret"
        Customize( wxCC_IFLAG_HAS_NONSTANDARD_BUTTON );
    }

protected:
    void DoSetPopupControl( wxComboPopup* popup ) override { m_popup = nullptr; }

    void OnButtonClick() override { m_function( m_row, m_col ); }

    DIALOG_SHIM* m_dlg;
    std::function<void( int, int )>& m_function;
    int&                             m_row;
    int&                             m_col;
};


void GRID_CELL_RUN_FUNCTION_EDITOR::Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler )
{
    m_control = new TEXT_BUTTON_RUN_FUNCTION( aParent, m_dlg, m_function, m_row, m_col );
    WX_GRID::CellEditorSetMargins( Combo() );

#if wxUSE_VALIDATORS
    // validate text in textctrl, if validator is set
    if( m_validator )
    {
        Combo()->SetValidator( *m_validator );
    }
#endif

    wxGridCellEditor::Create( aParent, aId, aEventHandler );
}


void GRID_CELL_RUN_FUNCTION_EDITOR::BeginEdit( int aRow, int aCol, wxGrid* aGrid )
{
    m_row = aRow;
    m_col = aCol;

    GRID_CELL_TEXT_BUTTON::BeginEdit( aRow, aCol, aGrid );
}
