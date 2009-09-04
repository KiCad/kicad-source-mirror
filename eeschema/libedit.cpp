/****************************/
/*  EESchema - libedit.cpp  */
/****************************/

/* Routines de maintenanace des librairies:
 *   sauvegarde, modification de librairies.
 *   creation edition suppression de composants
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
#include "dialog_create_component.h"


/* Affiche dans la zone messages la librairie , et le composant edite */
void WinEDA_LibeditFrame::DisplayLibInfos()
{
    wxString msg = wxT( "Libedit: " );

    msg += CurrentLib ? CurrentLib->m_FullFileName : wxT( "No Lib" );
    SetTitle( msg );

    msg = _( " Part:   " );
    if( CurrentLibEntry == NULL )
    {
        msg += _( "None" );
    }
    else
    {
        msg += CurrentLibEntry->m_Name.m_Text;
        if( !CurrentAliasName.IsEmpty() )
            msg << wxT( "  Alias " ) << CurrentAliasName;
    }
    static wxChar UnitLetter[] = wxT( "?ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    msg << wxT( "   Unit " ) << UnitLetter[CurrentUnit];

    if( CurrentConvert > 1 )
        msg += _( "   Convert" );
    else
        msg += _( "   Normal" );

    if( CurrentLibEntry && (CurrentLibEntry->m_Options == ENTRY_POWER) )
        msg += _( "  (Power Symbol)" );

    SetStatusText( msg, 0 );
}


/* Function to select the current library (working library) */
void WinEDA_LibeditFrame::SelectActiveLibrary()
{
    LibraryStruct* Lib;

    Lib = SelectLibraryFromList( this );
    if( Lib )
    {
        CurrentLib = Lib;
    }
    DisplayLibInfos();
}


/**
 * Function LoadOneLibraryPart()
 * load a library component from the current selected library
 * Prompt user for component name
 * If there is no current selected library,
 * prompt user for library name and make the selected library the current lib.
 */
bool WinEDA_LibeditFrame::LoadOneLibraryPart()
{
    int          i;
    wxString     msg;
    wxString     CmpName;
    LibCmpEntry* LibEntry = NULL;

    if( g_ScreenLib->IsModify() )
    {
        if( !IsOK( this, _( "Current part not saved.\nContinue?" ) ) )
            return FALSE;
    }

    if( CurrentLib == NULL ) // No current lib, ask user for the library to use
    {
        SelectActiveLibrary();
        if( CurrentLib == NULL )
            return FALSE;
    }

    i = GetNameOfPartToLoad( this, CurrentLib, CmpName );
    if( i == 0 )
        return FALSE;

    g_ScreenLib->ClrModify();
    CurrentDrawItem = NULL;

    // Delete previous library component, if any
    if( CurrentLibEntry )
    {
        SAFE_DELETE( CurrentLibEntry );
    }

    /* Load the new library component */
    LibEntry = CurrentLib->FindEntry( CmpName, ALIAS );

    if( LibEntry == NULL )
    {
        msg = _( "Component \"" );
        msg << CmpName << _( "\" not found." );
        DisplayError( this, msg );
        return FALSE;
    }

    GetScreen()->ClearUndoRedoList();

    if( !LoadOneLibraryPartAux( LibEntry, CurrentLib ) )
        return false;

    ReCreateHToolbar();
    Zoom_Automatique( FALSE );
    DrawPanel->Refresh();
    return TRUE;
}


/*
 * Routine Pour Charger en memoire la copie de 1 libpart.
 *  retourne
 *  0 si OK
 *  1 si err
 *  CurrentLibEntry pointe la copie ainsi creee
 */
bool WinEDA_LibeditFrame::LoadOneLibraryPartAux( LibCmpEntry*   LibEntry,
                                                 LibraryStruct* Library )
{
    wxString msg, cmpName, rootName;

    if( ( LibEntry == NULL ) || ( Library == NULL ) )
        return false;

    if( LibEntry->m_Name.m_Text.IsEmpty() )
    {
        wxLogWarning( wxT( "Entry in library <%s> has empty name field." ),
                      (const wxChar*) LibEntry );
        return false;
    }

    cmpName = LibEntry->m_Name.m_Text;
    CurrentAliasName.Empty();

    if( LibEntry->Type != ROOT )
    {
        rootName = ( (EDA_LibCmpAliasStruct*) LibEntry )->m_RootName;
        wxASSERT( !rootName.IsEmpty() );
        wxLogDebug( wxT( "\"<%s>\" is alias of \"<%s>\"" ),
                    (const wxChar*) cmpName, (const wxChar*) rootName );

        LibEntry = Library->FindEntry( rootName, ROOT );

        if( LibEntry == NULL )
        {
            msg.Printf( wxT( "Root entry <%s> for alias <%s> not found in \
library <%s>." ),
                        (const wxChar*) rootName,
                        (const wxChar*) cmpName,
                        (const wxChar*) Library->m_Name );
            DisplayError( this, msg );
            return false;
        }

        CurrentAliasName = cmpName;
    }

    if( CurrentLibEntry )
    {
        SAFE_DELETE( CurrentLibEntry );
    }

    CurrentLibEntry = CopyLibEntryStruct( (EDA_LibComponentStruct*) LibEntry );

    if( CurrentLibEntry == NULL )
    {
        msg.Printf( _( "Could not create copy of part <%s> in library <%s>." ),
                    (const wxChar*) LibEntry->m_Name.m_Text,
                    (const wxChar*) Library->m_Name );
        DisplayError( this, msg );
        return false;
    }

    CurrentUnit    = 1;
    CurrentConvert = 1;
    DisplayLibInfos();

    BuildAliasData( Library, CurrentLibEntry );

    g_ScreenLib->ClrModify();
    g_AsDeMorgan = 0;

    if( LookForConvertPart( CurrentLibEntry ) > 1 )
        g_AsDeMorgan = 1;

    return true;
}


/* Function to redraw the current loaded library component */
void WinEDA_LibeditFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( GetScreen() == NULL )
        return;

    ActiveScreen = GetScreen();

    DC->SetBackground( *wxBLACK_BRUSH );
    DC->SetBackgroundMode( wxTRANSPARENT );
    GRResetPenAndBrush( DC );

    DrawPanel->CursorOff( DC ); // erase cursor
    if( DrawPanel->ManageCurseur )
    {
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    }

    if( EraseBg )
        DrawPanel->EraseScreen( DC );

    DrawPanel->DrawBackGround( DC );

    if( CurrentLibEntry )
        CurrentLibEntry->Draw( DrawPanel, DC, wxPoint( 0, 0 ), CurrentUnit,
                               CurrentConvert, GR_DEFAULT_DRAWMODE );

    DrawPanel->CursorOn( DC ); // redraw cursor

    if( DrawPanel->ManageCurseur )
    {
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );
    }

    GetScreen()->ClrRefreshReq();
    DisplayLibInfos();
    UpdateStatusBar();
}


/*
 * Save (on disk) the current library
 *  if exists the old file is renamed (.bak)
 */
void WinEDA_LibeditFrame::SaveActiveLibrary()
{
    wxFileName fn;
    wxString   msg;

    if( CurrentLib == NULL )
    {
        DisplayError( this, wxT( "No library specified" ) );
        return;
    }

    fn = wxFileName( CurrentLib->m_FullFileName );

    msg = _( "Modify library file \"" ) + fn.GetFullPath() + _( "\"?" );

    if( !IsOK( this, msg ) )
        return;

    bool success = CurrentLib->Save( fn.GetFullPath() );

    MsgPanel->EraseMsgBox();

    if( !success )
    {
        msg = _( "Error while saving library file \"" ) + fn.GetFullPath() +
              _( "\"." );
        Affiche_1_Parametre( this, 1, wxT( " *** ERROR : **" ), msg, BLUE );
        DisplayError( this, msg );
    }
    else
    {
        msg = _( "Library file \"" ) + fn.GetFullName() + wxT( "\" Ok" );
        fn.SetExt( DOC_EXT );
        wxString msg1 = _( "Document file \"" ) + fn.GetFullPath() +
                        wxT( "\" Ok" );
        Affiche_1_Parametre( this, 1, msg, msg1, BLUE );
    }
}


/*
 *  Affiche la documentation du composant selectionne
 *  Utilisïe lors de l'affichage de la liste des composants en librairie
 */
void WinEDA_LibeditFrame::DisplayCmpDoc( const wxString& Name )
{
    LibCmpEntry* CmpEntry;

    if( CurrentLib == NULL )
        return;

    MsgPanel->EraseMsgBox();
    CmpEntry = CurrentLib->FindEntry( Name );

    if( CmpEntry == NULL )
        return;

    AfficheDoc( this, CmpEntry->m_Doc, CmpEntry->m_KeyWord );
}


/*
 * Routine de suppression d'un composant dans la librairie courante
 *  (effacement en memoire uniquement, le fichier n'est pas modifie)
 *  Le composant peut etre un alias, ou la definition de base.
 *  Si c'est un alias:
 *  il est supprime, et la liste des alias de la definition
 *  de base est modifiee
 *  Si c'est le composant de base:
 *  Si la liste des alias est nulle, il est supprime
 *  Sinon le premier alias devient le composant de base, et les autres
 *  alias deviennent dependants de celui ci.
 */
void WinEDA_LibeditFrame::DeleteOnePart()
{
    wxString      CmpName;
    LibCmpEntry*  LibEntry;
    wxArrayString ListNames;
    wxString      msg;

    CurrentDrawItem = NULL;

    if( CurrentLib == NULL )
    {
        SelectActiveLibrary();

        if( CurrentLib == NULL )
        {
            DisplayError( this, _( "Please select a component library." ) );
            return;
        }
    }

    CurrentLib->GetEntryNames( ListNames );

    if( ListNames.IsEmpty() )
    {
        msg.Printf( _( "Component library <%s> does not have any entries to delete." ),
                    ( const wxChar* ) CurrentLib->m_Name );
        wxMessageBox( msg, _( "Delete Entry Error" ),
                      wxID_OK | wxICON_EXCLAMATION, this );
        return;
    }

    msg.Printf( _( "Select 1 of %d components to\ndelete from library <%s>." ),
                ListNames.GetCount(), ( const wxChar* ) CurrentLib->m_Name );

    wxSingleChoiceDialog dlg( this, msg, _( "Delete Component" ), ListNames );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetStringSelection().IsEmpty() )
        return;

    LibEntry = CurrentLib->FindEntry( dlg.GetStringSelection() );

    if( LibEntry == NULL )
    {
        msg.Printf( _( "Entry <%s> not found in library <%s>." ),
                    ( const wxChar* ) dlg.GetStringSelection(),
                    ( const wxChar* ) CurrentLib->m_Name );
        DisplayError( this, msg );
        return;
    }

    msg = _( "Delete component \"" ) + LibEntry->m_Name.m_Text +
        _( "\" from library \"" ) + CurrentLib->m_Name + wxT( "\"?" );

    if( IsOK( this, msg ) )
    {
        CurrentLib->RemoveEntry( LibEntry );
    }
}


/*
 * Routine to create a new library component
 *
 * If an old component is currently in edit, it is deleted.
 */
void WinEDA_LibeditFrame::CreateNewLibraryPart()
{
    wxString msg;
    EDA_LibComponentStruct* NewStruct;
    int      diag;

    if( CurrentLibEntry
       && !IsOK( this,
                _( "Clear old component from screen (changes will be lost)?" ) ) )
        return;

    CurrentDrawItem = NULL;

    WinEDA_CreateCmpDialog Dialogbox( this );
    diag = Dialogbox.ShowModal();
    if( diag != wxID_OK )
        return;
    msg = Dialogbox.ReturnCmpName();
    if( msg.IsEmpty() )
        return;
    msg.MakeUpper();
    msg.Replace( wxT( " " ), wxT( "_" ) );

    /* Test: y a t-il un composant deja de ce nom */
    if( CurrentLib )
    {
        if( CurrentLib->FindEntry( msg ) )
        {
            wxString msg;
            msg << _( "Component \"" ) << Dialogbox.ReturnCmpName() <<
            _( "\" exists in library \"" ) << CurrentLib->m_Name <<
            _( "\"." );
            DisplayError( this, msg );
            return;
        }
    }

    NewStruct = new EDA_LibComponentStruct( msg );
    Dialogbox.SetComponentData( *NewStruct );
    if( NewStruct->m_Prefix.m_Text.IsEmpty() )
        NewStruct->m_Prefix.m_Text = wxT( "U" );
    NewStruct->m_Prefix.m_Text.MakeUpper();

    // Effacement ancien composant affichï¿½
    if( CurrentLibEntry )
    {
        SAFE_DELETE( CurrentLibEntry );
    }
    CurrentLibEntry = NewStruct;
    CurrentUnit     = 1;
    CurrentConvert  = 1;
    ReCreateHToolbar();

    DisplayLibInfos();
}


/*
 * Routine de sauvegarde de la "partlib" courante dans la librairie courante
 *
 * Sauvegarde en memoire uniquement, et PAS sur fichier
 * La routine efface l'ancien composant ( ou / et les alias ) a remplacer
 * s'il existe, et sauve le nouveau et cree les alias correspondants.
 */
void WinEDA_LibeditFrame::SaveOnePartInMemory()
{
    LibCmpEntry*            Entry;
    EDA_LibComponentStruct* Component;
    wxString msg;
    bool     NewCmp = TRUE;

    if( CurrentLibEntry == NULL )
    {
        DisplayError( this, _( "No component to save." ) );
        return;
    }

    if( CurrentLib == NULL )
        SelectActiveLibrary();

    if( CurrentLib == NULL )
    {
        DisplayError( this, _( "No library specified." ) );
        return;
    }

    g_ScreenLib->ClrModify();

    Entry = CurrentLib->FindEntry( CurrentLibEntry->m_Name.m_Text, ROOT );

    if( Entry != NULL )
    {
        msg.Printf( _( "Component \"%s\" exists. Change it?" ),
                    (const wxChar*) Entry->m_Name.m_Text );
        if( !IsOK( this, msg ) )
            return;
        NewCmp = FALSE;
    }

    wxASSERT( CurrentLibEntry->Type == ROOT );

    /* Effacement des alias deja existants en librairie */
    for( unsigned ii = 0;
         ii <  CurrentLibEntry->m_AliasList.GetCount();
         ii += ALIAS_NEXT )
    {
        LibCmpEntry* LocalEntry;
        wxString     aliasname = CurrentLibEntry->m_AliasList[ii];
        LocalEntry = CurrentLib->FindEntry( aliasname, ALIAS );

        while( LocalEntry != NULL && LocalEntry->Type == ALIAS )
        {
            CurrentLib->RemoveEntry( LocalEntry );
        }
    }

    if( !NewCmp )
        CurrentLib->RemoveEntry( Entry );

    Component = CurrentLib->AddComponent( CurrentLibEntry );

    if( Component == NULL )
        return;

    msg.Printf( _( "Component %s saved in library %s" ),
                ( const wxChar* ) Component->m_Name.m_Text,
                ( const wxChar* ) CurrentLib->m_Name );
    Affiche_Message( msg );
}
