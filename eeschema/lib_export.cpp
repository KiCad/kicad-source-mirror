/*****************************/
/* EESchema - lib_export.cpp */
/*****************************/

/* Routines de maintenanace des librariries:
  * sauvegarde, modification de librairies.
  * creation edition suppression de composants
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
#include "libeditfrm.h"
#include "class_library.h"

#include <wx/filename.h>


extern int ExportPartId;


/*************************************************/
/* Routine de lecture de 1 description.
 * Le format est celui des librairies, mais on ne charge que 1 composant
 * ou le 1er composant s'il y en a plusieurs.
 * Si le premier composant est un alias, on chargera la racine correspondante
 */
/*************************************************/
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
                    (const wxChar*) fn.GetFullPath() );
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


/* Routine de creation d'une nouvelle librairie et de sauvegarde du
 * composant courant dans cette librarie
 * si create_lib == TRUE sauvegarde dans le repertoire des libr
 * sinon: sauvegarde sous le nom demande sans modifications.
 *
 * Le format du fichier cree est dans tous les cas le meme.
 */
void WinEDA_LibeditFrame::OnExportPart( wxCommandEvent& event )
{
    wxFileName   fn;
    wxString     msg, title;
    CMP_LIBRARY* CurLibTmp;
    bool         createLib = ( event.GetId() != ExportPartId ) ? false : true;

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

    if( createLib && success )
    {
        msg = fn.GetFullPath() + _( " - OK" );
        DisplayInfoMessage( this, _( "This library will not be available \
until it is loaded by EESchema.\n\nModify the EESchema library configuration \
if you want to include it as part of this project." ) );
    }
    else
        msg = _( "Error creating " ) + fn.GetFullName();
    Affiche_Message( msg );
}
