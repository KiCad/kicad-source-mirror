/*****************************/
/* EESchema - lib_export.cpp */
/*****************************/

/* Library maintenance routines.
 * Backup modified libraries.
 * Create, edit, and delete components.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"

#include <wx/filename.h>


extern int ExportPartId;


/* Routine to read one part.
 * The format is that of libraries, but it loads only 1 component.
 * Or 1 component if there are several.
 * If the first component is an alias, it will load the corresponding root.
 */
void WinEDA_LibeditFrame::OnImportPart( wxCommandEvent& event )
{
    wxString       errMsg;
    wxFileName     fn;
    CMP_LIBRARY*   LibTmp;
    CMP_LIB_ENTRY* LibEntry;

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

        msg.Printf( _( "Component library file <%s> is empty." ),
                    GetChars( fn.GetFullPath() ) );
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


/* Routine to create a new library and backup the current component in this
 * library or export the component of the current library.
 * createLib == TRUE if we are creating a new library.
 * If not: export the library component.
 * Basically these 2 options do the same thing, but for user's convenience
 * > When creating a new lib, the user is prompted to add the new lib to
 * current eeschema config library list
 * > When exporting there is no message (it is expected the user does not want to add the
 * new created lib
 *
 * The file format is created in all cases the same.
 */
void WinEDA_LibeditFrame::OnExportPart( wxCommandEvent& event )
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
until it is loaded by EESchema.\n\nModify the EESchema library configuration \
if you want to include it as part of this project." ) );
        }
        else
            msg = fn.GetFullPath() + _( " - Export OK" );
    }   // Error
    else
        msg = _( "Error creating " ) + fn.GetFullName();
    Affiche_Message( msg );
}
