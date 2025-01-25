/**
 * @file dialog_symbol_remap.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <macros.h>
#include <pgm_base.h>
#include <kiface_base.h>
#include <project.h>
#include <confirm.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <wx_filename.h>
#include "widgets/wx_html_report_panel.h"

#include <libraries/legacy_symbol_library.h>
#include <core/kicad_algo.h>
#include <symbol_viewer_frame.h>
#include <project_rescue.h>
#include <sch_io/sch_io_mgr.h>
#include <sch_symbol.h>
#include <sch_screen.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <settings/settings_manager.h>
#include <symbol_lib_table.h>
#include <env_paths.h>
#include <project_sch.h>
#include <wx/msgdlg.h>

#include <dialog_symbol_remap.h>


DIALOG_SYMBOL_REMAP::DIALOG_SYMBOL_REMAP( SCH_EDIT_FRAME* aParent ) :
    DIALOG_SYMBOL_REMAP_BASE( aParent ),
    m_frame( aParent )
{
    m_remapped = false;

    wxString projectPath = Prj().GetProjectPath();

    if( !wxFileName::IsDirWritable( projectPath ) )
    {
        wxString msg = wxString::Format( _( "Remapping is not possible because you have "
                                            "insufficient privileges to the project folder '%s'." ),
                                         projectPath );

        DisplayInfoMessage( this, msg );

        // Disable the remap button.
        m_remapped = true;
    }

    wxString text;

    text = _( "This schematic currently uses the project symbol library list look up method "
              "for loading library symbols.  KiCad will attempt to map the existing symbols "
              "to use the new symbol library table.  Remapping will change some project files "
              "and schematics may not be compatible with older versions of KiCad.  All files "
              "that are changed will be backed up to the \"rescue-backup\" folder in the project "
              "folder should you need to revert any changes.  If you choose to skip this step, "
              "you will be responsible for manually remapping the symbols." );

    m_htmlCtrl->AppendToPage( text );

    m_messagePanel->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );
}


void DIALOG_SYMBOL_REMAP::OnRemapSymbols( wxCommandEvent& aEvent )
{
    SCH_EDIT_FRAME* parent = dynamic_cast< SCH_EDIT_FRAME* >( GetParent() );

    wxCHECK_RET( parent != nullptr, "Parent window is not type SCH_EDIT_FRAME." );

    if( !backupProject( m_messagePanel->Reporter() ) )
        return;

    // Ignore the never show rescue setting for one last rescue of legacy symbol
    // libraries before remapping to the symbol library table.  This ensures the
    // best remapping results.

    LEGACY_RESCUER rescuer( Prj(), &parent->Schematic(), &parent->GetCurrentSheet(),
                            parent->GetCanvas()->GetBackend() );

    if( RESCUER::RescueProject( this, rescuer, false ) )
    {
        wxBusyCursor busy;

        auto viewer = (SYMBOL_VIEWER_FRAME*) parent->Kiway().Player( FRAME_SCH_VIEWER, false );

        if( viewer )
            viewer->ReCreateLibList();

        parent->ClearUndoORRedoList( EDA_BASE_FRAME::UNDO_LIST, 1 );
        parent->SyncView();
        parent->GetCanvas()->Refresh();
        parent->OnModify();
    }

    // The schematic is fully loaded, any legacy library symbols have been rescued.  Now
    // check to see if the schematic has not been converted to the symbol library table
    // method for looking up symbols.

    wxFileName prjSymLibTableFileName( Prj().GetProjectPath(),
                                       SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

    // Delete the existing project symbol library table.
    if( prjSymLibTableFileName.FileExists() )
        wxRemoveFile( prjSymLibTableFileName.GetFullPath() );

    createProjectSymbolLibTable( m_messagePanel->Reporter() );
    Prj().SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, nullptr );
    PROJECT_SCH::SchSymbolLibTable( &Prj() );

    remapSymbolsToLibTable( m_messagePanel->Reporter() );

    // Remove all of the libraries from the legacy library list.
    wxString paths;
    wxArrayString libNames;

    LEGACY_SYMBOL_LIBS::SetLibNamesAndPaths( &Prj(), paths, libNames );

    // Reload the cache symbol library.
    Prj().SetElem( PROJECT::ELEM::SCH_SYMBOL_LIBS, nullptr );
    PROJECT_SCH::LegacySchLibs( &Prj() );

    Raise();
    m_remapped = true;
}


size_t DIALOG_SYMBOL_REMAP::getLibsNotInGlobalSymbolLibTable( std::vector< LEGACY_SYMBOL_LIB* >& aLibs )
{
    for( LEGACY_SYMBOL_LIB& lib : *PROJECT_SCH::LegacySchLibs( &Prj() ) )
    {
        // Ignore the cache library.
        if( lib.IsCache() )
            continue;

        // Check for the obvious library name.
        wxString libFileName = lib.GetFullFileName();

        if( !SYMBOL_LIB_TABLE::GetGlobalLibTable().FindRowByURI( libFileName ) )
            aLibs.push_back( &lib );
    }

    return aLibs.size();
}


void DIALOG_SYMBOL_REMAP::createProjectSymbolLibTable( REPORTER& aReporter )
{
    std::vector<LEGACY_SYMBOL_LIB*> libs;

    if( getLibsNotInGlobalSymbolLibTable( libs ) )
    {
        wxBusyCursor          busy;
        SYMBOL_LIB_TABLE      libTable;
        std::vector<wxString> libNames = SYMBOL_LIB_TABLE::GetGlobalLibTable().GetLogicalLibs();

        for( LEGACY_SYMBOL_LIB* lib : libs )
        {
            wxString libName = lib->GetName();
            int libNameInc = 1;
            int libNameLen = libName.Length();

            // Spaces in the file name will break the symbol name because they are not
            // quoted in the symbol library file format.
            libName.Replace( wxS( " " ), wxS( "-" ) );

            // Don't create duplicate table entries.
            while( alg::contains( libNames, libName ) )
            {
                libName = libName.Left( libNameLen );
                libName << libNameInc;
                libNameInc++;
            }

            wxString   type = SCH_IO_MGR::ShowType( SCH_IO_MGR::SCH_LEGACY );
            wxFileName fn( lib->GetFullFileName() );

            // Use environment variable substitution where possible.  This is based solely
            // on the internal user environment variable list.  Checking against all of the
            // system wide environment variables is probably not a good idea.
            wxString   normalizedPath = NormalizePath( fn, &Pgm().GetLocalEnvVariables(), &Prj() );
            wxFileName normalizedFn( normalizedPath );

            // Don't add symbol libraries that do not exist.
            if( normalizedFn.Normalize(FN_NORMALIZE_FLAGS | wxPATH_NORM_ENV_VARS )
                    && normalizedFn.FileExists() )
            {
                aReporter.Report( wxString::Format( _( "Adding library '%s', file '%s' to project "
                                                       "symbol library table." ),
                                                    libName,
                                                    normalizedPath ),
                                  RPT_SEVERITY_INFO );

                libTable.InsertRow( new SYMBOL_LIB_TABLE_ROW( libName, normalizedPath, type ) );
            }
            else
            {
                aReporter.Report( wxString::Format( _( "Library '%s' not found." ),
                                                    normalizedPath ),
                                  RPT_SEVERITY_WARNING );
            }
        }

        // Don't save empty project symbol library table.
        if( !libTable.IsEmpty() )
        {
            wxFileName fn( Prj().GetProjectPath(), SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );

            try
            {
                FILE_OUTPUTFORMATTER formatter( fn.GetFullPath() );
                libTable.Format( &formatter, 0 );
            }
            catch( const IO_ERROR& ioe )
            {
                aReporter.ReportTail( wxString::Format( _( "Error writing project symbol library "
                                                           "table.\n  %s" ),
                                                        ioe.What() ),
                                      RPT_SEVERITY_ERROR );
            }

            aReporter.ReportTail( _( "Created project symbol library table.\n" ),
                                  RPT_SEVERITY_INFO );
        }
    }
}


void DIALOG_SYMBOL_REMAP::remapSymbolsToLibTable( REPORTER& aReporter )
{
    wxBusyCursor busy;
    SCH_SCREENS  schematic( m_frame->Schematic().Root() );
    SCH_SYMBOL*  symbol;
    SCH_SCREEN*  screen;

    for( screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
    {
        for( EDA_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            symbol = dynamic_cast<SCH_SYMBOL*>( item );

            wxCHECK2( symbol, continue );

            if( !remapSymbolToLibTable( symbol ) )
            {
                aReporter.Report( wxString::Format( _( "No symbol %s found in symbol library "
                                                       "table." ),
                                                    symbol->GetLibId().GetLibItemName().wx_str() ),
                                  RPT_SEVERITY_WARNING );
            }
            else
            {
                aReporter.Report( wxString::Format( _( "Symbol %s mapped to symbol library '%s'." ),
                                            symbol->GetLibId().GetLibItemName().wx_str(),
                                            symbol->GetLibId().GetLibNickname().wx_str() ),
                                  RPT_SEVERITY_ACTION );

                screen->SetContentModified();
            }
        }
    }

    aReporter.Report( _( "Symbol library table mapping complete!" ), RPT_SEVERITY_INFO );
    schematic.UpdateSymbolLinks();
}


bool DIALOG_SYMBOL_REMAP::remapSymbolToLibTable( SCH_SYMBOL* aSymbol )
{
    wxCHECK_MSG( aSymbol != nullptr, false, "Null pointer passed to remapSymbolToLibTable." );
    wxCHECK_MSG( aSymbol->GetLibId().GetLibNickname().empty(), false,
                 "Cannot remap symbol that is already mapped." );
    wxCHECK_MSG( !aSymbol->GetLibId().GetLibItemName().empty(), false,
                 "The symbol LIB_ID name is empty." );

    for( LEGACY_SYMBOL_LIB& lib : *PROJECT_SCH::LegacySchLibs( &Prj() ) )
    {
        // Ignore the cache library.
        if( lib.IsCache() )
            continue;

        LIB_SYMBOL* alias = lib.FindSymbol( aSymbol->GetLibId().GetLibItemName().wx_str() );

        // Found in the same library as the old look up method assuming the user didn't
        // change the libraries or library ordering since the last time the schematic was
        // loaded.
        if( alias )
        {
            // Find the same library in the symbol library table using the full path and file name.
            wxString libFileName = lib.GetFullFileName();

            const LIB_TABLE_ROW* row =
                    PROJECT_SCH::SchSymbolLibTable( &Prj() )->FindRowByURI( libFileName );

            if( row )
            {
                LIB_ID id = aSymbol->GetLibId();

                id.SetLibNickname( row->GetNickName() );

                // Don't resolve symbol library links now.
                aSymbol->SetLibId( id );
                return true;
            }
        }
    }

    return false;
}


bool DIALOG_SYMBOL_REMAP::backupProject( REPORTER& aReporter )
{
    static wxString backupFolder = "rescue-backup";

    wxString    errorMsg;
    wxFileName  srcFileName;
    wxFileName  destFileName;
    wxFileName  backupPath;
    SCH_SCREENS schematic( m_frame->Schematic().Root() );

    // Copy backup files to different folder so as not to pollute the project folder.
    destFileName.SetPath( Prj().GetProjectPath() );
    destFileName.AppendDir( backupFolder );
    backupPath = destFileName;

    if( !destFileName.DirExists() )
    {
        if( !destFileName.Mkdir() )
        {
            errorMsg.Printf( _( "Cannot create project remap back up folder '%s'." ),
                             destFileName.GetPath() );

            wxMessageDialog dlg( this, errorMsg, _( "Backup Error" ),
                                 wxYES_NO | wxCENTRE | wxRESIZE_BORDER | wxICON_QUESTION );
            dlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Continue with Rescue" ) ),
                                wxMessageDialog::ButtonLabel( _( "Abort Rescue" ) ) );

            if( dlg.ShowModal() == wxID_NO )
                return false;
        }
    }

    {
        wxBusyCursor busy;

        // Time stamp to append to file name in case multiple remappings are performed.
        wxString timeStamp = wxDateTime::Now().Format( "-%Y-%m-%d-%H-%M-%S" );

        // Back up symbol library table.
        srcFileName.SetPath( Prj().GetProjectPath() );
        srcFileName.SetName( SYMBOL_LIB_TABLE::GetSymbolLibTableFileName() );
        destFileName = srcFileName;
        destFileName.AppendDir( backupFolder );
        destFileName.SetName( destFileName.GetName() + timeStamp );

        aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                            srcFileName.GetFullPath(),
                                            destFileName.GetFullPath() ),
                          RPT_SEVERITY_INFO );

        if( wxFileName::Exists( srcFileName.GetFullPath() )
                && !wxCopyFile( srcFileName.GetFullPath(), destFileName.GetFullPath() ) )
        {
            errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                          srcFileName.GetFullPath() );
        }

        // Back up the schematic files.
        for( SCH_SCREEN* screen = schematic.GetFirst(); screen; screen = schematic.GetNext() )
        {
            destFileName = screen->GetFileName();
            destFileName.SetName( destFileName.GetName() + timeStamp );

            // Check for nest hierarchical schematic paths.
            if( destFileName.GetPath() != backupPath.GetPath() )
            {
                destFileName.SetPath( backupPath.GetPath() );

                wxArrayString srcDirs = wxFileName( screen->GetFileName() ).GetDirs();
                wxArrayString destDirs = wxFileName( Prj().GetProjectPath() ).GetDirs();

                for( size_t i = destDirs.GetCount(); i < srcDirs.GetCount(); i++ )
                    destFileName.AppendDir( srcDirs[i] );
            }
            else
            {
                destFileName.AppendDir( backupFolder );
            }

            aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                                screen->GetFileName(),
                                                destFileName.GetFullPath() ),
                              RPT_SEVERITY_INFO );

            if( !destFileName.DirExists()
             && !destFileName.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
            {
                errorMsg += wxString::Format( _( "Failed to create backup folder '%s'.\n" ),
                                              destFileName.GetPath() );
                continue;
            }

            if( wxFileName::Exists( screen->GetFileName() )
                    && !wxCopyFile( screen->GetFileName(), destFileName.GetFullPath() ) )
            {
                errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                              screen->GetFileName() );
            }
        }

        // Back up the project file.
        destFileName = Prj().GetProjectFullName();
        destFileName.SetName( destFileName.GetName() + timeStamp );
        destFileName.AppendDir( backupFolder );

        aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                            Prj().GetProjectFullName(),
                                            destFileName.GetFullPath() ),
                          RPT_SEVERITY_INFO );

        if( wxFileName::Exists( Prj().GetProjectFullName() )
                && !wxCopyFile( Prj().GetProjectFullName(), destFileName.GetFullPath() ) )
        {
            errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                          Prj().GetProjectFullName() );
        }

        // Back up the cache library.
        srcFileName.SetPath( Prj().GetProjectPath() );
        srcFileName.SetName( Prj().GetProjectName() + wxS( "-cache" ) );
        srcFileName.SetExt( FILEEXT::LegacySymbolLibFileExtension );

        destFileName = srcFileName;
        destFileName.SetName( destFileName.GetName() + timeStamp );
        destFileName.AppendDir( backupFolder );

        aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                            srcFileName.GetFullPath(),
                                            destFileName.GetFullPath() ),
                          RPT_SEVERITY_INFO );

        if( srcFileName.Exists()
                && !wxCopyFile( srcFileName.GetFullPath(), destFileName.GetFullPath() ) )
        {
            errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                          srcFileName.GetFullPath() );
        }

        // Back up the old (2007) cache library.
        srcFileName.SetPath( Prj().GetProjectPath() );
        srcFileName.SetName( Prj().GetProjectName() + wxS( ".cache" ) );
        srcFileName.SetExt( FILEEXT::LegacySymbolLibFileExtension );

        destFileName = srcFileName;
        destFileName.SetName( destFileName.GetName() + timeStamp );
        destFileName.AppendDir( backupFolder );

        aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                            srcFileName.GetFullPath(),
                                            destFileName.GetFullPath() ),
                          RPT_SEVERITY_INFO );

        if( srcFileName.Exists()
                && !wxCopyFile( srcFileName.GetFullPath(), destFileName.GetFullPath() ) )
        {
            errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                          srcFileName.GetFullPath() );
        }

        // Back up the rescue symbol library if it exists.
        srcFileName.SetName( Prj().GetProjectName() + wxS( "-rescue" ) );
        destFileName.SetName( srcFileName.GetName() + timeStamp );

        aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                            srcFileName.GetFullPath(),
                                            destFileName.GetFullPath() ),
                          RPT_SEVERITY_INFO );

        if( srcFileName.Exists()
                && !wxCopyFile( srcFileName.GetFullPath(), destFileName.GetFullPath() ) )
        {
            errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                          srcFileName.GetFullPath() );
        }

        // Back up the rescue symbol library document file if it exists.
        srcFileName.SetExt( FILEEXT::LegacySymbolDocumentFileExtension );
        destFileName.SetExt( srcFileName.GetExt() );

        aReporter.Report( wxString::Format( _( "Backing up file '%s' to '%s'." ),
                                            srcFileName.GetFullPath(),
                                            destFileName.GetFullPath() ),
                          RPT_SEVERITY_INFO );

        if( srcFileName.Exists()
                && !wxCopyFile( srcFileName.GetFullPath(), destFileName.GetFullPath() ) )
        {
            errorMsg += wxString::Format( _( "Failed to back up file '%s'.\n" ),
                                          srcFileName.GetFullPath() );
        }
    }

    if( !errorMsg.IsEmpty() )
    {
        wxMessageDialog dlg( this, _( "Some of the project files could not be backed up." ),
                             _( "Backup Error" ),
                             wxYES_NO | wxCENTRE | wxRESIZE_BORDER | wxICON_QUESTION );
        errorMsg.Trim();
        dlg.SetExtendedMessage( errorMsg );
        dlg.SetYesNoLabels( wxMessageDialog::ButtonLabel( _( "Continue with Rescue" ) ),
                            wxMessageDialog::ButtonLabel( _( "Abort Rescue" ) ) );

        if( dlg.ShowModal() == wxID_NO )
            return false;
    }

    return true;
}


void DIALOG_SYMBOL_REMAP::OnUpdateUIRemapButton( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( !m_remapped );
}
