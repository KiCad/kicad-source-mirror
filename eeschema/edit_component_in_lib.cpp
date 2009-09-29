/**************************************************************/
/*  librairy editor: edition of component general properties  */
/**************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditfrm.h"
#include "class_library.h"


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
    wxASSERT( m_component != NULL && m_library != NULL );

    DIALOG_EDIT_COMPONENT_IN_LIBRARY dlg( this );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    UpdateAliasSelectList();
    UpdatePartSelectList();
    DisplayLibInfos();
    DisplayCmpDoc();
    GetScreen()->SetModify();
    SaveCopyInUndoList( m_component );
}



void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitPanelDoc()
{
    CMP_LIB_ENTRY* entry;
    LIB_COMPONENT* component = m_Parent->GetComponent();
    CMP_LIBRARY* library = m_Parent->GetLibrary();

    if( component == NULL )
        return;

    if( m_Parent->GetAliasName().IsEmpty() )
    {
        entry = component;
    }
    else
    {
        entry =
            ( CMP_LIB_ENTRY* ) library->FindAlias( m_Parent->GetAliasName() );

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
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( m_Parent->GetShowDeMorgan() )
        m_AsConvertButt->SetValue( true );

    /* Default values for a new component. */
    if( component == NULL )
    {
        m_ShowPinNumButt->SetValue( true );
        m_ShowPinNameButt->SetValue( true );
        m_PinsNameInsideButt->SetValue( true );
        m_SelNumberOfUnits->SetValue( 1 );
        m_SetSkew->SetValue( 40 );
        m_OptionPower->SetValue( false );
        m_OptionPartsLocked->SetValue( false );
        return;
    }

    m_ShowPinNumButt->SetValue( component->m_DrawPinNum );
    m_ShowPinNameButt->SetValue( component->m_DrawPinName );
    m_PinsNameInsideButt->SetValue( component->m_TextInside != 0 );
    m_SelNumberOfUnits->SetValue( component->m_UnitCount );
    m_SetSkew->SetValue( component->m_TextInside );
    m_OptionPower->SetValue( component->m_Options == ENTRY_POWER );
    m_OptionPartsLocked->SetValue( component->m_UnitSelectionLocked );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnOkClick( wxCommandEvent& event )
{

    /* Update the doc, keyword and doc filename strings */
    size_t i;
    int index;
    CMP_LIB_ENTRY* entry;
    LIB_COMPONENT* component = m_Parent->GetComponent();
    CMP_LIBRARY* library = m_Parent->GetLibrary();

    if( m_Parent->GetAliasName().IsEmpty() )
    {
        entry = (CMP_LIB_ENTRY*) component;
    }
    else
    {
        entry = library->FindEntry( m_Parent->GetAliasName() );
    }

    if( entry == NULL )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> not found for component <%s> in library \
<%s>." ),
                    (const wxChar*) m_Parent->GetAliasName(),
                    (const wxChar*) component->GetName(),
                    (const wxChar*) library->GetName() );
        wxMessageBox( msg, _( "Component Library Error" ),
                      wxID_OK | wxICON_ERROR, this );
    }
    else
    {
        entry->m_Doc = m_Doc->GetValue();
        entry->m_KeyWord = m_Keywords->GetValue();
        entry->m_DocFile = m_Docfile->GetValue();
    }

    if( m_PartAliasList->GetStrings() != component->m_AliasList )
    {
        LIB_ALIAS* alias;
        wxArrayString aliases = m_PartAliasList->GetStrings();

        /* Add names not existing in the old alias list. */
        for( i = 0; i < aliases.GetCount(); i++ )
        {
            index = component->m_AliasList.Index( aliases[ i ], false );

            if( index != wxNOT_FOUND )
                continue;

            alias = new LIB_ALIAS( aliases[ i ], component );

            if( !library->AddAlias( alias ) )
            {
                delete alias;
                alias = NULL;
            }
        }

        /* Remove names and library alias entries not in the new alias list. */
        for( i = 0; component->m_AliasList.GetCount(); i++ )
        {
            index = aliases.Index( component->m_AliasList[ i ], false );

            if( index == wxNOT_FOUND )
                continue;

            CMP_LIB_ENTRY* alias =
                library->FindAlias( component->m_AliasList[ i ] );
            if( alias != NULL )
                library->RemoveEntry( alias );
        }

        component->m_AliasList = aliases;
    }

    index = m_SelNumberOfUnits->GetValue();
    ChangeNbUnitsPerPackage( index );

    if( m_AsConvertButt->GetValue() )
    {
        if( !m_Parent->GetShowDeMorgan() )
        {
            m_Parent->SetShowDeMorgan( true );
            SetUnsetConvert();
        }
    }
    else
    {
        if( m_Parent->GetShowDeMorgan() )
        {
            m_Parent->SetShowDeMorgan( false );
            SetUnsetConvert();
        }
    }

    component->m_DrawPinNum  = m_ShowPinNumButt->GetValue() ? 1 : 0;
    component->m_DrawPinName = m_ShowPinNameButt->GetValue() ? 1 : 0;

    if( m_PinsNameInsideButt->GetValue() == FALSE )
        component->m_TextInside = 0;
    else
        component->m_TextInside = m_SetSkew->GetValue();

    if( m_OptionPower->GetValue() == TRUE )
        component->m_Options = ENTRY_POWER;
    else
        component->m_Options = ENTRY_NORMAL;

    /* Set the option "Units locked".
     *  Obviously, cannot be TRUE if there is only one part */
    component->m_UnitSelectionLocked = m_OptionPartsLocked->GetValue();
    if( component->m_UnitCount <= 1 )
        component->m_UnitSelectionLocked = FALSE;

    /* Update the footprint filter list */
    component->m_FootprintList.Clear();
    component->m_FootprintList = m_FootprintFilterListBox->GetStrings();

    EndModal( wxID_OK );
}


/*******************************************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::CopyDocToAlias( wxCommandEvent& WXUNUSED (event) )
/******************************************************************************/
{
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL || m_Parent->GetAliasName().IsEmpty() )
        return;

    m_Doc->SetValue( component->m_Doc );
    m_Docfile->SetValue( component->m_DocFile );
    m_Keywords->SetValue( component->m_KeyWord );
}


/**********************************************************/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAllAliasOfPart(
    wxCommandEvent& WXUNUSED (event) )
/**********************************************************/
{
    if( m_PartAliasList->FindString( m_Parent->GetAliasName() )
        != wxNOT_FOUND )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> cannot be removed while it is being \
edited!" ),
                    (const wxChar*) m_Parent->GetAliasName() );
        DisplayError( this, msg );
        return;
    }

    m_Parent->GetAliasName().Empty();

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
    LIB_COMPONENT* component = m_Parent->GetComponent();
    CMP_LIBRARY* library = m_Parent->GetLibrary();

    if( component == NULL )
        return;

    if( Get_Message( _( "New alias:" ),
                     _( "Component Alias" ), Line, this ) != 0 )
        return;

    Line.Replace( wxT( " " ), wxT( "_" ) );
    aliasname = Line;

    if( m_PartAliasList->FindString( aliasname ) != wxNOT_FOUND
        || library->FindEntry( aliasname ) != NULL )
    {
        wxString msg;
        msg.Printf( _( "Alias or component name <%s> already exists in \
library <%s>." ),
                    (const wxChar*) aliasname,
                    (const wxChar*) library->GetName() );
        DisplayError( this,  msg );
        return;
    }

    m_PartAliasList->Append( aliasname );
    if( m_Parent->GetAliasName().IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( TRUE );
    m_ButtonDeleteOneAlias->Enable( TRUE );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAliasOfPart(
    wxCommandEvent& WXUNUSED (event) )
{
    wxString aliasname = m_PartAliasList->GetStringSelection();

    if( aliasname.IsEmpty() )
        return;
    if( aliasname.CmpNoCase( m_Parent->GetAliasName() ) == 0 )
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
    LIB_DRAW_ITEM* DrawItem, * NextDrawItem;
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL )
        return FALSE;

    /* Si pas de changement: termine */
    if( component->m_UnitCount == MaxUnit )
        return FALSE;

    OldNumUnits = component->m_UnitCount;
    if( OldNumUnits < 1 )
        OldNumUnits = 1;

    component->m_UnitCount = MaxUnit;


    /* Traitement des unites enlevees ou rajoutees */
    if( OldNumUnits > component->m_UnitCount )
    {
        DrawItem = component->GetNextDrawItem();
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
                        if( m_Parent->GetUnit() > MaxUnit )
                            m_Parent->SetUnit( 1 );
                        FlagDel = 1;
                    }
                    else
                    {
                        FlagDel = 0;
                        MaxUnit = OldNumUnits;
                        component->m_UnitCount = MaxUnit;
                        return FALSE;
                    }
                }

                component->RemoveDrawItem( DrawItem );
            }
        }

        return TRUE;
    }

    if( OldNumUnits < component->m_UnitCount )
    {
        DrawItem = component->GetNextDrawItem();
        for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        {
            /* Duplication des items pour autres elements */
            if( DrawItem->m_Unit == 1 )
            {
                for( ii = OldNumUnits + 1; ii <= MaxUnit; ii++ )
                {
                    NextDrawItem = DrawItem->GenCopy();
                    NextDrawItem->m_Unit = ii;
                    component->AddDrawItem( NextDrawItem );
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
    LIB_DRAW_ITEM* DrawItem = NULL, * NextDrawItem;
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( m_Parent->GetShowDeMorgan() )  /* Representation convertie a creer */
    {
        /* Traitement des elements a ajouter ( pins seulement ) */
        if( component )
            DrawItem = component->GetNextDrawItem();

        for( ; DrawItem != NULL; DrawItem = DrawItem->Next() )
        {
            /* Duplication des items pour autres elements */
            if( DrawItem->Type() != COMPONENT_PIN_DRAW_TYPE )
                continue;
            if( DrawItem->m_Convert == 1 )
            {
                if( FlagDel == 0 )
                {
                    if( IsOK( this, _( "Create pins for convert items." ) ) )
                        FlagDel = 1;
                    else
                    {
                        if( IsOK( this, _( "Part as \"De Morgan\" anymore" ) ) )
                            return TRUE;

                        m_Parent->SetShowDeMorgan( false );
                        return FALSE;
                    }
                }

                NextDrawItem = DrawItem->GenCopy();
                NextDrawItem->m_Convert = 2;
                component->AddDrawItem( NextDrawItem );
            }
        }
    }
    else               /* Representation convertie a supprimer */
    {
        /* Traitement des elements � supprimer */
        if( component )
            DrawItem = component->GetNextDrawItem();
        for( ; DrawItem != NULL; DrawItem = NextDrawItem )
        {
            NextDrawItem = DrawItem->Next();
            if( DrawItem->m_Convert > 1 )  /* Item a effacer */
            {
                if( FlagDel == 0 )
                {
                    if( IsOK( this, _( "Delete Convert items" ) ) )
                    {
                        m_Parent->SetConvert( 1 );
                        FlagDel = 1;
                    }
                    else
                    {
                        m_Parent->SetShowDeMorgan( true );
                        return FALSE;
                    }
                }

                m_Parent->GetScreen()->SetModify();
                component->RemoveDrawItem( DrawItem );
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
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL )
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
    LIB_COMPONENT* component = m_Parent->GetComponent();
    int ii = m_FootprintFilterListBox->GetSelection();

    m_FootprintFilterListBox->Delete( ii );

    if( !component || (m_FootprintFilterListBox->GetCount() == 0) )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( FALSE );
        m_ButtonDeleteOneFootprintFilter->Enable( FALSE );
    }
}
