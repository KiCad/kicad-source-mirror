/**************************************************************/
/*  librairy editor: edition of component general properties  */
/**************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "libcmp.h"
#include "general.h"

#include "protos.h"


extern int CurrentUnit;

/* Dialog box to edit a libentry (a component in library) properties */

/* Creates a NoteBook dialog
 *  Edition:
 *  Doc and keys words
 *  Parts per package
 *  General properties
 * Fields are NOT edited here. There is a specific dialog box to do that
 */

#include "dialog_edit_component_in_lib.h"


void WinEDA_LibeditFrame::OnEditComponentProperties( wxCommandEvent& event )
{
    EditComponentProperties();
}


void WinEDA_LibeditFrame::EditComponentProperties()
{
    wxASSERT( CurrentLibEntry != NULL && CurrentLib != NULL );

    DIALOG_EDIT_COMPONENT_IN_LIBRARY dlg( this );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    UpdateAliasSelectList();
    UpdatePartSelectList();
    DisplayLibInfos();
    GetScreen()->SetModify();
    SaveCopyInUndoList( CurrentLibEntry );
}



void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitPanelDoc()
{
    CMP_LIB_ENTRY* entry;

    if( CurrentLibEntry == NULL )
        return;

    if( CurrentAliasName.IsEmpty() )
    {
        entry = CurrentLibEntry;
    }
    else
    {
        entry = ( CMP_LIB_ENTRY* ) CurrentLib->FindAlias( CurrentAliasName );

        if( entry == NULL )
            return;
    }

    m_Doc->SetValue( entry->m_Doc );
    m_Keywords->SetValue( entry->m_KeyWord );
    m_Docfile->SetValue( entry->m_DocFile );
}


/*
 * create the basic panel for component properties editing
 */
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitBasicPanel()
{
    if( g_AsDeMorgan )
        m_AsConvertButt->SetValue( TRUE );
    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_DrawPinNum )
            m_ShowPinNumButt->SetValue( TRUE );
    }
    else
        m_ShowPinNumButt->SetValue( TRUE );

    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_DrawPinName )
            m_ShowPinNameButt->SetValue( TRUE );
    }
    else
        m_ShowPinNameButt->SetValue( TRUE );


    if( CurrentLibEntry )
    {
        if( CurrentLibEntry->m_TextInside )
            m_PinsNameInsideButt->SetValue( TRUE );
    }
    else
        m_PinsNameInsideButt->SetValue( TRUE );

    int number, number_of_units;
    if( CurrentLibEntry )
        number_of_units = CurrentLibEntry->m_UnitCount;
    else
        number_of_units = 1;
    m_SelNumberOfUnits->SetValue( number_of_units );

    if( CurrentLibEntry && CurrentLibEntry->m_TextInside )
        number = CurrentLibEntry->m_TextInside;
    else
        number = 40;
    m_SetSkew->SetValue( number );

    if( CurrentLibEntry && CurrentLibEntry->m_Options == ENTRY_POWER )
        m_OptionPower->SetValue( TRUE );

    if( CurrentLibEntry && CurrentLibEntry->m_UnitSelectionLocked )
        m_OptionPartsLocked->SetValue( TRUE );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnOkClick( wxCommandEvent& event )
{

    /* Update the doc, keyword and doc filename strings */
    size_t i;
    int index;
    CMP_LIB_ENTRY* entry;

    if( CurrentAliasName.IsEmpty() )
    {
        entry = CurrentLibEntry;
    }
    else
    {
        entry = CurrentLib->FindEntry( CurrentAliasName );
    }

    if( entry == NULL )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> not found for component <%s> in library <%s>." ),
                    (const wxChar*) CurrentAliasName,
                    (const wxChar*) CurrentLibEntry->GetName(),
                    (const wxChar*) CurrentLib->GetName() );
        wxMessageBox( msg, _( "Component Library Error" ),
                      wxID_OK | wxICON_ERROR, this );
    }
    else
    {
        entry->m_Doc = m_Doc->GetValue();
        entry->m_KeyWord = m_Keywords->GetValue();
        entry->m_DocFile = m_Docfile->GetValue();
    }

    if( m_PartAliasList->GetStrings() != CurrentLibEntry->m_AliasList )
    {
        LIB_ALIAS* alias;
        wxArrayString aliases = m_PartAliasList->GetStrings();

        /* Add names not existing in the old alias list. */
        for( i = 0; i < aliases.GetCount(); i++ )
        {
            index = CurrentLibEntry->m_AliasList.Index( aliases[ i ], false );

            if( index != wxNOT_FOUND )
                continue;

            alias = new LIB_ALIAS( aliases[ i ], CurrentLibEntry );

            if( !CurrentLib->AddAlias( alias ) )
            {
                delete alias;
                alias = NULL;
            }
        }

        /* Remove names and library alias entries not in the new alias list. */
        for( i = 0; CurrentLibEntry->m_AliasList.GetCount(); i++ )
        {
            index = aliases.Index( CurrentLibEntry->m_AliasList[ i ], false );

            if( index == wxNOT_FOUND )
                continue;

            CMP_LIB_ENTRY* alias =
                CurrentLib->FindAlias( CurrentLibEntry->m_AliasList[ i ] );
            if( alias != NULL )
                CurrentLib->RemoveEntry( alias );
        }

        CurrentLibEntry->m_AliasList = aliases;
    }

    index = m_SelNumberOfUnits->GetValue();
    ChangeNbUnitsPerPackage( index );

    if( m_AsConvertButt->GetValue() )
    {
        if( !g_AsDeMorgan )
        {
            g_AsDeMorgan = 1;
            SetUnsetConvert();
        }
    }
    else
    {
        if( g_AsDeMorgan )
        {
            g_AsDeMorgan = 0;
            SetUnsetConvert();
        }
    }

    CurrentLibEntry->m_DrawPinNum  = m_ShowPinNumButt->GetValue() ? 1 : 0;
    CurrentLibEntry->m_DrawPinName = m_ShowPinNameButt->GetValue() ? 1 : 0;

    if( m_PinsNameInsideButt->GetValue() == FALSE )
        CurrentLibEntry->m_TextInside = 0;
    else
        CurrentLibEntry->m_TextInside = m_SetSkew->GetValue();

    if( m_OptionPower->GetValue() == TRUE )
        CurrentLibEntry->m_Options = ENTRY_POWER;
    else
        CurrentLibEntry->m_Options = ENTRY_NORMAL;

    /* Set the option "Units locked".
     *  Obviously, cannot be TRUE if there is only one part */
    CurrentLibEntry->m_UnitSelectionLocked = m_OptionPartsLocked->GetValue();
    if( CurrentLibEntry->m_UnitCount <= 1 )
        CurrentLibEntry->m_UnitSelectionLocked = FALSE;

    /* Update the footprint filter list */
    CurrentLibEntry->m_FootprintList.Clear();
    CurrentLibEntry->m_FootprintList = m_FootprintFilterListBox->GetStrings();

    EndModal( wxID_OK );
}


/*******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::CopyDocToAlias( wxCommandEvent& WXUNUSED (event) )
/******************************************************************************/
{
    if( CurrentLibEntry == NULL || CurrentAliasName.IsEmpty() )
        return;

    m_Doc->SetValue( CurrentLibEntry->m_Doc );
    m_Docfile->SetValue( CurrentLibEntry->m_DocFile );
    m_Keywords->SetValue( CurrentLibEntry->m_KeyWord );
}


/**********************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAllAliasOfPart(
    wxCommandEvent& WXUNUSED (event) )
/**********************************************************/
{
    if( m_PartAliasList->FindString( CurrentAliasName ) != wxNOT_FOUND )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> cannot be removed while it is being \
edited!" ),
                    (const wxChar*) CurrentAliasName );
        DisplayError( this, msg );
        return;
    }

    CurrentAliasName.Empty();

    if( IsOK( this, _( "Remove all aliases from list?" ) ) )
    {
        m_PartAliasList->Clear();
        m_ButtonDeleteAllAlias->Enable( FALSE );
        m_ButtonDeleteOneAlias->Enable( FALSE );
    }
}


/*******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::AddAliasOfPart( wxCommandEvent& WXUNUSED (event) )
/*******************************************************************************/

/* Add a new name to the alias list box
 *  New name cannot be the root name, and must not exists
 */
{
    wxString Line;
    wxString aliasname;

    if( CurrentLibEntry == NULL )
        return;

    if( Get_Message( _( "New alias:" ), _( "Component Alias" ), Line, this ) != 0 )
        return;

    Line.Replace( wxT( " " ), wxT( "_" ) );
    aliasname = Line;

    if( m_PartAliasList->FindString( aliasname ) != wxNOT_FOUND
        || CurrentLib->FindEntry( aliasname ) != NULL )
    {
        wxString msg;
        msg.Printf( _( "Alias or component name <%s> already exists in \
library <%s>." ),
                    (const wxChar*) aliasname,
                    (const wxChar*) CurrentLib->GetName() );
        DisplayError( this,  msg );
        return;
    }

    m_PartAliasList->Append( aliasname );
    if( CurrentAliasName.IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( TRUE );
    m_ButtonDeleteOneAlias->Enable( TRUE );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAliasOfPart(
    wxCommandEvent& WXUNUSED (event) )
{
    wxString aliasname = m_PartAliasList->GetStringSelection();

    if( aliasname.IsEmpty() )
        return;
    if( aliasname.CmpNoCase( CurrentAliasName ) == 0 )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> cannot be removed while it is being \
edited!" ),
                    (const wxChar*) aliasname );
        DisplayError( this, msg );
        return;
    }

    m_PartAliasList->Delete( m_PartAliasList->GetSelection() );

    if( m_PartAliasList->IsEmpty() )
    {
        m_ButtonDeleteAllAlias->Enable( FALSE );
        m_ButtonDeleteOneAlias->Enable( FALSE );
    }
}


/********************************************************************/
bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::ChangeNbUnitsPerPackage( int MaxUnit )
/********************************************************************/

/* Routine de modification du nombre d'unites par package pour le
 *  composant courant;
 */
{
    int OldNumUnits, ii, FlagDel = -1;
    LibEDA_BaseStruct* DrawItem, * NextDrawItem;

    if( CurrentLibEntry == NULL )
        return FALSE;

    /* Si pas de changement: termine */
    if( CurrentLibEntry->m_UnitCount == MaxUnit )
        return FALSE;

    OldNumUnits = CurrentLibEntry->m_UnitCount;
    if( OldNumUnits < 1 )
        OldNumUnits = 1;

    CurrentLibEntry->m_UnitCount = MaxUnit;


    /* Traitement des unites enlevees ou rajoutees */
    if( OldNumUnits > CurrentLibEntry->m_UnitCount )
    {
        DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = NextDrawItem )
        {
            NextDrawItem = DrawItem->Next();
            if( DrawItem->m_Unit > MaxUnit )  /* Item a effacer */
            {
                if( FlagDel < 0 )
                {
                    if( IsOK( this, _( "Delete units" ) ) )
                    {
                        /* Si part selectee n'existe plus: selection 1ere unit */
                        if( CurrentUnit > MaxUnit )
                            CurrentUnit = 1;
                        FlagDel = 1;
                    }
                    else
                    {
                        FlagDel = 0;
                        MaxUnit = OldNumUnits;
                        CurrentLibEntry->m_UnitCount = MaxUnit;
                        return FALSE;
                    }
                }

                CurrentLibEntry->RemoveDrawItem( DrawItem );
            }
        }

        return TRUE;
    }

    if( OldNumUnits < CurrentLibEntry->m_UnitCount )
    {
        DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        {
            /* Duplication des items pour autres elements */
            if( DrawItem->m_Unit == 1 )
            {
                for( ii = OldNumUnits + 1; ii <= MaxUnit; ii++ )
                {
                    NextDrawItem = DrawItem->GenCopy();
                    NextDrawItem->SetNext( CurrentLibEntry->m_Drawings );
                    CurrentLibEntry->m_Drawings = NextDrawItem;
                    NextDrawItem->m_Unit = ii;
                }
            }
        }
    }
    return TRUE;
}


/*****************************************************/
bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::SetUnsetConvert()
/*****************************************************/

/* cr�e ou efface (selon option AsConvert) les �l�ments
 *  de la repr�sentation convertie d'un composant
 */
{
    int FlagDel = 0;
    LibEDA_BaseStruct* DrawItem = NULL, * NextDrawItem;

    if( g_AsDeMorgan )  /* Representation convertie a creer */
    {
        /* Traitement des elements a ajouter ( pins seulement ) */
        if( CurrentLibEntry )
            DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        {
            /* Duplication des items pour autres elements */
            if( DrawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( DrawItem->m_Convert == 1 )
            {
                if( FlagDel == 0 )
                {
                    if( IsOK( this, _( "Create pins for Convert items" ) ) )
                        FlagDel = 1;
                    else
                    {
                        if( IsOK( this, _( "Part as \"De Morgan\" anymore" ) ) )
                            return TRUE;

                        g_AsDeMorgan = 0; return FALSE;
                    }
                }
                NextDrawItem = DrawItem->GenCopy();
                NextDrawItem->SetNext( CurrentLibEntry->m_Drawings );
                CurrentLibEntry->m_Drawings = NextDrawItem;
                NextDrawItem->m_Convert     = 2;
            }
        }
    }
    else               /* Representation convertie a supprimer */
    {
        /* Traitement des elements � supprimer */
        if( CurrentLibEntry )
            DrawItem = CurrentLibEntry->m_Drawings;
        for( ; DrawItem != NULL; DrawItem = NextDrawItem )
        {
            NextDrawItem = DrawItem->Next();
            if( DrawItem->m_Convert > 1 )  /* Item a effacer */
            {
                if( FlagDel == 0 )
                {
                    if( IsOK( this, _( "Delete Convert items" ) ) )
                    {
                        CurrentConvert = 1;
                        FlagDel = 1;
                    }
                    else
                    {
                        g_AsDeMorgan = 1;
                        return FALSE;
                    }
                }

                m_Parent->GetScreen()->SetModify();
                CurrentLibEntry->RemoveDrawItem( DrawItem );
            }
        }
    }
    return TRUE;
}


/****************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::BrowseAndSelectDocFile( wxCommandEvent& event )
/****************************************************************************/
{
    wxString FullFileName, mask;
    wxString docpath, filename;

    docpath = wxGetApp().ReturnLastVisitedLibraryPath(wxT( "doc" ));

    mask = wxT( "*" );
    FullFileName = EDA_FileSelector( _( "Doc Files" ),
                                     docpath,       /* Chemin par defaut */
                                     wxEmptyString, /* nom fichier par defaut */
                                     wxEmptyString, /* extension par defaut */
                                     mask,          /* Masque d'affichage */
                                     this,
                                     wxFD_OPEN,
                                     TRUE
                                     );
    if( FullFileName.IsEmpty() )
        return;

    /* If the path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of these default paths
     */
    wxFileName fn = FullFileName;
    wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );

    filename = wxGetApp().ReturnFilenameWithRelativePathInLibPath(FullFileName);
    m_Docfile->SetValue( filename );
}


/**********************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAllFootprintFilter(
    wxCommandEvent& WXUNUSED (event) )
/**********************************************************/
{
    if( IsOK( this, _( "Ok to Delete FootprintFilter LIST" ) ) )
    {
        m_FootprintFilterListBox->Clear();
        m_ButtonDeleteAllFootprintFilter->Enable( FALSE );
        m_ButtonDeleteOneFootprintFilter->Enable( FALSE );
    }
}


/*******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::AddFootprintFilter( wxCommandEvent& WXUNUSED (event) )
/*******************************************************************************/

/* Add a new name to the alias list box
 *  New name cannot be the root name, and must not exists
 */
{
    wxString Line;

    if( CurrentLibEntry == NULL )
        return;

    if( Get_Message( _( "Add Footprint Filter" ), _( "Footprint Filter" ),
                     Line, this ) != 0 )
        return;

    Line.Replace( wxT( " " ), wxT( "_" ) );

    /* test for an existing name: */
    int index = m_FootprintFilterListBox->FindString( Line );

    if( index != wxNOT_FOUND )
    {
        wxString msg;

        msg.Printf( _( "Foot print filter <%s> is already defined." ),
                    (const wxChar*) Line );
        DisplayError( this, msg );
        return;
    }

    m_FootprintFilterListBox->Append( Line );
    m_ButtonDeleteAllFootprintFilter->Enable( TRUE );
    m_ButtonDeleteOneFootprintFilter->Enable( TRUE );
}


/********************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteOneFootprintFilter(
    wxCommandEvent& WXUNUSED (event) )
/********************************************************/
{
    int ii = m_FootprintFilterListBox->GetSelection();

    m_FootprintFilterListBox->Delete( ii );

    if( !CurrentLibEntry || (m_FootprintFilterListBox->GetCount() == 0) )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( FALSE );
        m_ButtonDeleteOneFootprintFilter->Enable( FALSE );
    }
}
