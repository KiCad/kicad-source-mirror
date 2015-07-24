/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file dialog_freeroute_exchange.cpp
 * Dialog to access to FreeRoute, the web bases free router, export/import files
 * to/from FreeRoute
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <confirm.h>
#include <gestfich.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <macros.h>
#include <class_board.h>

#include <../common/dialogs/dialog_display_info_HTML_base.h>

#include <dialog_freeroute_exchange.h>

#ifdef __WINDOWS__
#include <wx/msw/registry.h>
#endif


void PCB_EDIT_FRAME::Access_to_External_Tool( wxCommandEvent& event )
{
    DIALOG_FREEROUTE dialog( this );
    dialog.ShowModal();
}



DIALOG_FREEROUTE::DIALOG_FREEROUTE( PCB_EDIT_FRAME* parent ):
    DIALOG_FREEROUTE_BASE( parent )
{
    m_Parent = parent;
    MyInit();

    m_sdbSizerOK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}



// Specific data initialization
void DIALOG_FREEROUTE::MyInit()
{
    SetFocus();
    m_freeRouterFound = false;

    wxFileName fileName( FindKicadFile( wxT( "freeroute.jar" ) ), wxPATH_UNIX );

    if( fileName.FileExists() )
        m_freeRouterFound = true;

    m_buttonLaunchFreeroute->Enable( m_freeRouterFound );

}

const char * s_FreeRouteHelpInfo =
#include <dialog_freeroute_exchange_help_html.h>
;

void DIALOG_FREEROUTE::OnHelpButtonClick( wxCommandEvent& event )
{
    DIALOG_DISPLAY_HTML_TEXT_BASE help_Dlg( this, wxID_ANY,
        _("Freeroute Help"),wxDefaultPosition, wxSize( 650,550 ) );

    wxString msg = FROM_UTF8(s_FreeRouteHelpInfo);
    help_Dlg.m_htmlWindow->AppendToPage( msg );
    help_Dlg.ShowModal();
}


void DIALOG_FREEROUTE::OnExportButtonClick( wxCommandEvent& event )
{
    m_Parent->ExportToSpecctra( event );
}


void DIALOG_FREEROUTE::OnImportButtonClick( wxCommandEvent& event )
{
    m_Parent->ImportSpecctraSession(  event );

    /* Connectivity must be rebuild.
     * because for large board it can take some time, this is made only on demand
     */
    if( IsOK( this, _("Do you want to rebuild connectivity data ?" ) ) )
        m_Parent->Compile_Ratsnest( NULL, true );
}


// Run freeroute, if it is available (freeroute.jar found in kicad binaries)
void DIALOG_FREEROUTE::OnLaunchButtonClick( wxCommandEvent& event )
{
    wxString dsnFile;

    if( m_freeRouterFound )
    {
        dsnFile = createDSN_File();

        if( dsnFile.IsEmpty() )     // Something is wrong or command cancelled
            return;
    }

    wxFileName jarfileName( FindKicadFile( wxT( "freeroute.jar" ) ), wxPATH_UNIX );
    wxString command;

    // Find the Java application on Windows.
    // Colud be no more needed since we now have to run only java, not java web start
#ifdef __WINDOWS__

    // If you thought the registry was brain dead before, now you have to deal with
    // accessing it in either 64 or 32 bit mode depending on the build version of
    // Windows and the build version of KiCad.

    // This key works for 32 bit Java on 32 bit Windows and 64 bit Java on 64 bit Windows.
    wxString keyName = wxT( "SOFTWARE\\JavaSoft\\Java Runtime Environment" );
    wxRegKey key( wxRegKey::HKLM, keyName,
                  wxIsPlatform64Bit() ? wxRegKey::WOW64ViewMode_64 :
                  wxRegKey::WOW64ViewMode_Default );

    // It's possible that 32 bit Java is installed on 64 bit Windows.
    if( !key.Exists() && wxIsPlatform64Bit() )
    {
        keyName = wxT( "SOFTWARE\\Wow6432Node\\JavaSoft\\Java Runtime Environment" );
        key.SetName( wxRegKey::HKLM, keyName );
    }

    if( !key.Exists() )
    {
        ::wxMessageBox( _( "It appears that the Java run time environment is not "
                           "installed on this computer.  Java is required to use "
                           "FreeRoute." ),
                        _( "Pcbnew Error" ), wxOK | wxICON_ERROR );
        return;
    }

    key.Open( wxRegKey::Read );

    // Get the current version of java installed to determine the executable path.
    wxString value;
    key.QueryValue( wxT( "CurrentVersion" ), value );
    key.SetName( key.GetName() + wxT( "\\" ) + value );

    key.QueryValue( wxT( "JavaHome" ), value );
    command = value + wxFileName::GetPathSeparator();
    command << wxT("bin\\java");
#else   //  __WINDOWS__
        command = wxT( "java" );
#endif

    command << wxT(" -jar ");
    // add "freeroute.jar" to command line:
    command << wxChar( '"' ) << jarfileName.GetFullPath() << wxChar( '"' );
    // add option to load the .dsn file
    command << wxT( " -de " );
    // add *.dsn full filename (quoted):
    command << wxChar( '"' ) << dsnFile << wxChar( '"' );

    ProcessExecute( command );
}

const wxString DIALOG_FREEROUTE::createDSN_File()
{
    wxFileName fn( m_Parent->GetBoard()->GetFileName() );
    wxString dsn_ext = wxT( "dsn" );
    fn.SetExt( dsn_ext );
    wxString mask    = wxT( "*." ) + dsn_ext;

    wxString fullFileName = EDA_FileSelector( _( "Specctra DSN file:" ),
                                     fn.GetPath(), fn.GetFullName(),
                                     dsn_ext, mask,
                                     this, wxFD_SAVE, false );

    if( !fullFileName.IsEmpty() )
    {
        if( ! m_Parent->ExportSpecctraFile( fullFileName ) ) // the file was not created
            return wxEmptyString;
    }

    return fullFileName;
}


