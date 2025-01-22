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

#include <design_block.h>
#include <design_block_lib_table.h>
#include <paths.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <common.h>
#include <kidialog.h>
#include <widgets/design_block_pane.h>
#include <dialog_design_block_properties.h>
#include <widgets/panel_design_block_chooser.h>
#include <kiface_base.h>
#include <core/kicad_algo.h>
#include <template_fieldnames.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/choicdlg.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <confirm.h>
#include <wildcards_and_files_ext.h>
#include <tool/tool_manager.h>

DESIGN_BLOCK_PANE::DESIGN_BLOCK_PANE( EDA_DRAW_FRAME* aParent, const LIB_ID* aPreselect,
                                       std::vector<LIB_ID>& aHistoryList ) :
    WX_PANEL( aParent ),
    m_frame( aParent )
{
    m_frame->Bind( EDA_LANG_CHANGED, &DESIGN_BLOCK_PANE::OnLanguageChanged, this );
}


DESIGN_BLOCK_PANE::~DESIGN_BLOCK_PANE()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &DESIGN_BLOCK_PANE::OnLanguageChanged, this );
}


void DESIGN_BLOCK_PANE::OnLanguageChanged( wxCommandEvent& aEvent )
{
    if( m_chooserPanel )
        m_chooserPanel->ShowChangedLanguage();

    setLabelsAndTooltips();

    aEvent.Skip();
}


void DESIGN_BLOCK_PANE::SaveSettings()
{
    m_chooserPanel->SaveSettings();
}


LIB_ID DESIGN_BLOCK_PANE::GetSelectedLibId( int* aUnit ) const
{
    return m_chooserPanel->GetSelectedLibId( aUnit );
}


void DESIGN_BLOCK_PANE::SelectLibId( const LIB_ID& aLibId )
{
    m_chooserPanel->SelectLibId( aLibId );
}


void DESIGN_BLOCK_PANE::RefreshLibs()
{
    m_chooserPanel->RefreshLibs();
}


DESIGN_BLOCK* DESIGN_BLOCK_PANE::GetDesignBlock( const LIB_ID& aLibId, bool aUseCacheLib,
                                                 bool aShowErrorMsg )
{
    DESIGN_BLOCK_LIB_TABLE* prjLibs = m_frame->Prj().DesignBlockLibs();

    wxCHECK_MSG( prjLibs, nullptr, wxS( "Invalid design block library table." ) );

    DESIGN_BLOCK* designBlock = nullptr;

    try
    {
        designBlock = prjLibs->DesignBlockLoadWithOptionalNickname( aLibId, true );
    }
    catch( const IO_ERROR& ioe )
    {
        if( aShowErrorMsg )
        {
            wxString msg = wxString::Format( _( "Error loading design block %s from library '%s'." ),
                                             aLibId.GetLibItemName().wx_str(),
                                             aLibId.GetLibNickname().wx_str() );
            DisplayErrorMessage( m_frame, msg, ioe.What() );
        }
    }

    return designBlock;
}


DESIGN_BLOCK* DESIGN_BLOCK_PANE::GetSelectedDesignBlock( bool aUseCacheLib, bool aShowErrorMsg )
{
    if( !GetSelectedLibId().IsValid() )
        return nullptr;

    return GetDesignBlock( GetSelectedLibId(), aUseCacheLib, aShowErrorMsg );
}


wxString DESIGN_BLOCK_PANE::CreateNewDesignBlockLibrary( const wxString& aLibName,
                                                         const wxString& aProposedName )
{
    return createNewDesignBlockLibrary( aLibName, aProposedName, selectDesignBlockLibTable() );
}


wxString DESIGN_BLOCK_PANE::createNewDesignBlockLibrary( const wxString&         aLibName,
                                                         const wxString&         aProposedName,
                                                         DESIGN_BLOCK_LIB_TABLE* aTable )
{
    if( aTable == nullptr )
        return wxEmptyString;

    wxFileName fn;
    bool       doAdd = false;
    bool       isGlobal = ( aTable == &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() );
    wxString   initialPath = aProposedName;

    if( initialPath.IsEmpty() )
        initialPath = isGlobal ? PATHS::GetDefaultUserDesignBlocksPath()
                               : m_frame->Prj().GetProjectPath();

    if( aLibName.IsEmpty() )
    {
        fn = initialPath;

        if( !m_frame->LibraryFileBrowser( false, fn, FILEEXT::KiCadDesignBlockLibPathWildcard(),
                                          FILEEXT::KiCadDesignBlockLibPathExtension, false,
                                          isGlobal, initialPath ) )
        {
            return wxEmptyString;
        }

        doAdd = true;
    }
    else
    {
        fn = EnsureFileExtension( aLibName, FILEEXT::KiCadDesignBlockLibPathExtension );

        if( !fn.IsAbsolute() )
        {
            fn.SetName( aLibName );
            fn.MakeAbsolute( initialPath );
        }
    }

    // We can save libs only using DESIGN_BLOCK_IO_MGR::KICAD_SEXP format (.pretty libraries)
    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T piType = DESIGN_BLOCK_IO_MGR::KICAD_SEXP;
    wxString                                 libPath = fn.GetFullPath();

    try
    {
        IO_RELEASER<DESIGN_BLOCK_IO> pi( DESIGN_BLOCK_IO_MGR::FindPlugin( piType ) );

        bool writable = false;
        bool exists   = false;

        try
        {
            writable = pi->IsLibraryWritable( libPath );
            exists   = fn.Exists();
        }
        catch( const IO_ERROR& )
        {
            // best efforts....
        }

        if( exists )
        {
            if( !writable )
            {
                wxString msg = wxString::Format( _( "Library %s is read only." ), libPath );
                m_frame->ShowInfoBarError( msg );
                return wxEmptyString;
            }
            else
            {
                wxString msg = wxString::Format( _( "Library %s already exists." ), libPath );
                KIDIALOG dlg( m_frame, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
                dlg.SetOKLabel( _( "Overwrite" ) );
                dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

                if( dlg.ShowModal() == wxID_CANCEL )
                    return wxEmptyString;

                pi->DeleteLibrary( libPath );
            }
        }

        pi->CreateLibrary( libPath );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( m_frame, ioe.What() );
        return wxEmptyString;
    }

    if( doAdd )
        AddDesignBlockLibrary( libPath, aTable );

    return libPath;
}


bool DESIGN_BLOCK_PANE::AddDesignBlockLibrary( const wxString&         aFilename,
                                               DESIGN_BLOCK_LIB_TABLE* aTable )
{
    if( aTable == nullptr )
        aTable = selectDesignBlockLibTable();

    if( aTable == nullptr )
        return wxEmptyString;

    bool isGlobal = ( aTable == &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable() );

    wxFileName fn( aFilename );

    if( aFilename.IsEmpty() )
    {
        if( !m_frame->LibraryFileBrowser( true, fn, FILEEXT::KiCadDesignBlockLibPathWildcard(),
                                          FILEEXT::KiCadDesignBlockLibPathExtension, true, isGlobal,
                                          PATHS::GetDefaultUserDesignBlocksPath() ) )
        {
            return false;
        }
    }

    wxString libPath = fn.GetFullPath();
    wxString libName = fn.GetName();

    if( libName.IsEmpty() )
        return false;

    // Open a dialog to ask for a description
    wxString description = wxGetTextFromUser( _( "Enter a description for the library:" ),
                                              _( "Library Description" ), wxEmptyString, m_frame );

    DESIGN_BLOCK_IO_MGR::DESIGN_BLOCK_FILE_T lib_type =
            DESIGN_BLOCK_IO_MGR::GuessPluginTypeFromLibPath( libPath );

    if( lib_type == DESIGN_BLOCK_IO_MGR::FILE_TYPE_NONE )
        lib_type = DESIGN_BLOCK_IO_MGR::KICAD_SEXP;

    wxString type = DESIGN_BLOCK_IO_MGR::ShowType( lib_type );

    // KiCad lib is our default guess.  So it might not have the .kicad_blocks extension
    // In this case, the extension is part of the library name
    if( lib_type == DESIGN_BLOCK_IO_MGR::KICAD_SEXP
        && fn.GetExt() != FILEEXT::KiCadDesignBlockLibPathExtension )
        libName = fn.GetFullName();

    // try to use path normalized to an environmental variable or project path
    wxString normalizedPath = NormalizePath( libPath, &Pgm().GetLocalEnvVariables(), &m_frame->Prj() );

    try
    {
        DESIGN_BLOCK_LIB_TABLE_ROW* row = new DESIGN_BLOCK_LIB_TABLE_ROW(
                libName, normalizedPath, type, wxEmptyString, description );
        aTable->InsertRow( row );

        if( isGlobal )
            DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable().Save(
                    DESIGN_BLOCK_LIB_TABLE::GetGlobalTableFileName() );
        else
            m_frame->Prj().DesignBlockLibs()->Save( m_frame->Prj().DesignBlockLibTblName() );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( m_frame, ioe.What() );
        return false;
    }

    LIB_ID libID( libName, wxEmptyString );
    RefreshLibs();
    SelectLibId( libID );

    return true;
}


bool DESIGN_BLOCK_PANE::DeleteDesignBlockLibrary( const wxString& aLibName, bool aConfirm )
{
    if( aLibName.IsEmpty() )
    {
        DisplayErrorMessage( m_frame, _( "Please select a library to delete." ) );
        return false;
    }

    if( !m_frame->Prj().DesignBlockLibs()->IsDesignBlockLibWritable( aLibName ) )
    {
        wxString msg = wxString::Format( _( "Library '%s' is read only." ), aLibName );
        m_frame->ShowInfoBarError( msg );
        return false;
    }

    // Confirmation
    wxString msg = wxString::Format( _( "Delete design block library '%s' from disk? This will "
                                        "delete all design blocks within the library." ),
                                     aLibName.GetData() );

    if( aConfirm && !IsOK( m_frame, msg ) )
        return false;

    try
    {
        m_frame->Prj().DesignBlockLibs()->DesignBlockLibDelete( aLibName );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( m_frame, ioe.What() );
        return false;
    }

    msg.Printf( _( "Design block library '%s' deleted" ), aLibName.GetData() );
    m_frame->SetStatusText( msg );

    RefreshLibs();

    return true;
}


bool DESIGN_BLOCK_PANE::DeleteDesignBlockFromLibrary( const LIB_ID& aLibId, bool aConfirm )
{
    if( !aLibId.IsValid() )
        return false;

    wxString libname = aLibId.GetLibNickname();
    wxString dbname = aLibId.GetLibItemName();

    if( !m_frame->Prj().DesignBlockLibs()->IsDesignBlockLibWritable( libname ) )
    {
        wxString msg = wxString::Format( _( "Library '%s' is read only." ), libname );
        m_frame->ShowInfoBarError( msg );
        return false;
    }

    // Confirmation
    wxString msg = wxString::Format( _( "Delete design block '%s' in library '%s' from disk?" ),
                                     dbname.GetData(), libname.GetData() );

    if( aConfirm && !IsOK( m_frame, msg ) )
        return false;

    try
    {
        m_frame->Prj().DesignBlockLibs()->DesignBlockDelete( libname, dbname );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( m_frame, ioe.What() );
        return false;
    }

    msg.Printf( _( "Design block '%s' deleted from library '%s'" ), dbname.GetData(),
                libname.GetData() );

    m_frame->SetStatusText( msg );

    RefreshLibs();

    return true;
}


bool DESIGN_BLOCK_PANE::EditDesignBlockProperties( const LIB_ID& aLibId )
{
    if( !aLibId.IsValid() )
        return false;

    wxString libname = aLibId.GetLibNickname();
    wxString dbname = aLibId.GetLibItemName();

    if( !m_frame->Prj().DesignBlockLibs()->IsDesignBlockLibWritable( libname ) )
    {
        wxString msg = wxString::Format( _( "Library '%s' is read only." ), libname );
        m_frame->ShowInfoBarError( msg );
        return false;
    }

    DESIGN_BLOCK* designBlock = GetDesignBlock( aLibId, true, true );

    if( !designBlock )
        return false;

    wxString                       originalName = designBlock->GetLibId().GetLibItemName();
    DIALOG_DESIGN_BLOCK_PROPERTIES dlg( m_frame, designBlock );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    wxString newName = designBlock->GetLibId().GetLibItemName();

    try
    {
        if( originalName != newName )
        {
            if( m_frame->Prj().DesignBlockLibs()->DesignBlockExists( libname, newName ) )
                if( !checkOverwrite( m_frame, libname, newName ) )
                    return false;

            m_frame->Prj().DesignBlockLibs()->DesignBlockSave( libname, designBlock );
            m_frame->Prj().DesignBlockLibs()->DesignBlockDelete( libname, originalName );
        }
        else
            m_frame->Prj().DesignBlockLibs()->DesignBlockSave( libname, designBlock );
    }
    catch( const IO_ERROR& ioe )
    {
        DisplayError( m_frame, ioe.What() );
        return false;
    }

    RefreshLibs();
    SelectLibId( designBlock->GetLibId() );

    return true;
}


bool DESIGN_BLOCK_PANE::checkOverwrite( wxWindow* aFrame, wxString& libname, wxString& newName )
{
    wxString msg = wxString::Format( _( "Design block '%s' already exists in library '%s'." ),
                                     newName.GetData(), libname.GetData() );

    if( OKOrCancelDialog( aFrame, _( "Confirmation" ), msg, _( "Overwrite existing design block?" ),
                          _( "Overwrite" ) )
        != wxID_OK )
    {
        return false;
    }

    return true;
}


DESIGN_BLOCK_LIB_TABLE* DESIGN_BLOCK_PANE::selectDesignBlockLibTable( bool aOptional )
{
    // If no project is loaded, always work with the global table
    if( m_frame->Prj().IsNullProject() )
    {
        DESIGN_BLOCK_LIB_TABLE* ret = &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable();

        if( aOptional )
        {
            wxMessageDialog dlg( m_frame, _( "Add the library to the global library table?" ),
                                 _( "Add To Global Library Table" ), wxYES_NO );

            if( dlg.ShowModal() != wxID_OK )
                ret = nullptr;
        }

        return ret;
    }

    wxArrayString libTableNames;
    libTableNames.Add( _( "Global" ) );
    libTableNames.Add( _( "Project" ) );

    wxSingleChoiceDialog dlg( m_frame, _( "Choose the Library Table to add the library to:" ),
                              _( "Add To Library Table" ), libTableNames );

    if( aOptional )
    {
        dlg.FindWindow( wxID_CANCEL )->SetLabel( _( "Skip" ) );
        dlg.FindWindow( wxID_OK )->SetLabel( _( "Add" ) );
    }

    if( dlg.ShowModal() != wxID_OK )
        return nullptr;

    switch( dlg.GetSelection() )
    {
    case 0: return &DESIGN_BLOCK_LIB_TABLE::GetGlobalLibTable();
    case 1: return m_frame->Prj().DesignBlockLibs();
    default: return nullptr;
    }
}
