/**
 * @file lib_export.cpp
 * @brief Eeschema library maintenance routines to backup modified libraries and
 *        create, edit, and delete components.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "eeschema_id.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"

#include <wx/filename.h>


extern int ExportPartId;


void LIB_EDIT_FRAME::OnImportPart( wxCommandEvent& event )
{
    wxString     errMsg;
    wxFileName   fn;
    CMP_LIBRARY* LibTmp;
    LIB_ALIAS*   LibEntry;

    m_lastDrawItem = NULL;

    wxFileDialog dlg( this, _( "Import Component" ), m_LastLibImportPath,
                      wxEmptyString, CompLibFileWildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    LibTmp = CMP_LIBRARY::LoadLibrary( fn, errMsg );

    if( LibTmp == NULL )
        return;

    LibEntry = LibTmp->GetFirstEntry();

    if( LibEntry == NULL )
    {
        wxString msg;

        msg.Printf( _( "Component library file <%s> is empty." ), GetChars( fn.GetFullPath() ) );
        DisplayError( this,  msg );
        return;
    }

    if( LoadOneLibraryPartAux( LibEntry, LibTmp ) )
    {
        fn = dlg.GetPath();
        m_LastLibImportPath = fn.GetPath();
        DisplayLibInfos();
        GetScreen()->ClearUndoRedoList();
        DrawPanel->Refresh();
    }

    delete LibTmp;
}


void LIB_EDIT_FRAME::OnExportPart( wxCommandEvent& event )
{
    wxFileName   fn;
    wxString     msg, title;
    CMP_LIBRARY* CurLibTmp;
    bool         createLib = ( event.GetId() == ExportPartId ) ? false : true;

    if( m_component == NULL )
    {
        DisplayError( this, _( "There is no component selected to save." ) );
        return;
    }

    fn = m_component->GetName().Lower();
    fn.SetExt( CompLibFileExtension );

    title = createLib ? _( "New Library" ) : _( "Export Component" );

    wxFileDialog dlg( this, title, wxGetCwd(), fn.GetFullName(),
                      CompLibFileWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    CurLibTmp = m_library;

    m_library = new CMP_LIBRARY( LIBRARY_TYPE_EESCHEMA, fn );

    SaveOnePartInMemory();

    bool success = m_library->Save( fn.GetFullPath() );

    if( success )
        m_LastLibExportPath = fn.GetPath();

    delete m_library;
    m_library = CurLibTmp;

    if( success )
    {
        if( createLib )
        {
            msg = fn.GetFullPath() + _( " - OK" );
            DisplayInfoMessage( this, _( "This library will not be available \
until it is loaded by Eeschema.\n\nModify the Eeschema library configuration \
if you want to include it as part of this project." ) );
        }
        else
            msg = fn.GetFullPath() + _( " - Export OK" );
    }   // Error
    else
        msg = _( "Error creating " ) + fn.GetFullName();
    SetStatusText( msg );
}
