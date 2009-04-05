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

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"

#include "id.h"

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
    wxFileName              fn;
    LibraryStruct*          LibTmp;
    EDA_LibComponentStruct* LibEntry;
    int err = 1;

    LibItemToRepeat = NULL;

    wxFileDialog dlg( this, _( "Import Component" ), m_LastLibImportPath,
                      wxEmptyString, CompLibFileWildcard,
                      wxFD_OPEN | wxFILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    LibTmp = g_LibraryList;
    g_LibraryList = NULL;

    LoadLibraryName( this, dlg.GetPath(), wxT( "$tmplib$" ) );

    if( g_LibraryList )
    {
        LibEntry = (EDA_LibComponentStruct*) PQFirst( &g_LibraryList->m_Entries,
                                                      false );

        if( LibEntry )
            err = LoadOneLibraryPartAux( LibEntry, g_LibraryList, 1 );
        FreeCmpLibrary( this, g_LibraryList->m_Name );

        if( err == 0 )
        {
            fn = dlg.GetPath();
            m_LastLibImportPath = fn.GetPath();
            ReCreateHToolbar();
            DisplayLibInfos();
            GetScreen()->ClearUndoRedoList();
            DrawPanel->Refresh();
        }
        else
            DisplayError( this, _( "File is empty" ), 30 );
    }

    g_LibraryList = LibTmp;
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
    wxFileName     fn;
    wxString       Name, mask, title;
    LibraryStruct* NewLib, * LibTmp, * CurLibTmp;
    bool createLib = ( event.GetId() == ExportPartId ) ? false : true;

    if( CurrentLibEntry == NULL )
    {
        DisplayError( this, _( "No Part to Save" ), 10 );
        return;
    }

    fn = CurrentLibEntry->m_Name.m_Text.Lower();
    fn.SetExt( CompLibFileExtension );

    title = createLib ? _( "New Library" ) : _( "Export Component" );

    wxFileDialog dlg( this, title, wxGetCwd(), fn.GetFullName(),
                      CompLibFileWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    /* Creation d'une librairie standard pour sauvegarde */

    LibTmp    = g_LibraryList;
    CurLibTmp = CurrentLib;

    NewLib = new LibraryStruct( LIBRARY_TYPE_EESCHEMA, wxT( "$libTmp$" ),
                                fn.GetFullName() );

    g_LibraryList = NewLib;

    /* Sauvegarde du composant: */
    CurrentLib = NewLib;
    SaveOnePartInMemory();
    bool success = NewLib->SaveLibrary( fn.GetFullPath() );

    if( success )
    {
        m_LastLibExportPath = fn.GetPath();
    }

    /* Suppression de la librarie temporaire */
    FreeCmpLibrary( this, NewLib->m_Name );
    g_LibraryList = LibTmp;
    CurrentLib    = CurLibTmp;

    wxString msg;
    if( createLib && success )
    {
        msg = fn.GetFullPath() + _( " - OK" );
        DisplayInfo( this, _( "Note: this new library will be available " \
                              "only if it is loaded by eeschema.\nModify "
                              "eeschema config if you want use it." ) );
    }
    else
        msg = _( "Error creating " ) + fn.GetFullName();
    Affiche_Message( msg );
}
