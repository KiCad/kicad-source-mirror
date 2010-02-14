/**************************************************************/
/*  librairy editor: edition of component general properties  */
/**************************************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
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
    DrawPanel->Refresh();
}


void WinEDA_LibeditFrame::EditComponentProperties()
{
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

    m_Doc->SetValue( entry->GetDescription() );
    m_Keywords->SetValue( entry->GetKeyWords() );
    m_Docfile->SetValue( entry->GetDocFileName() );
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
    m_SelNumberOfUnits->SetValue( component->GetPartCount() );
    m_SetSkew->SetValue( component->m_TextInside );
    m_OptionPower->SetValue( component->isPower() );
    m_OptionPartsLocked->SetValue( component->m_UnitSelectionLocked );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnOkClick( wxCommandEvent& event )
{

    /* Update the doc, keyword and doc filename strings */
    size_t i;
    int index;
    CMP_LIB_ENTRY* entry = NULL;
    LIB_COMPONENT* component = m_Parent->GetComponent();
    if( component == NULL )
    {
        EndModal( wxID_CANCEL );
        return;
    }

    CMP_LIBRARY* library = m_Parent->GetLibrary();

    if( m_Parent->GetAliasName().IsEmpty() )
    {
        entry = (CMP_LIB_ENTRY*) component;
    }
    else if ( library )
    {
        entry = library->FindEntry( m_Parent->GetAliasName() );
    }

    if( entry == NULL )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> not found for component <%s> in library \
<%s>." ),
                    GetChars( m_Parent->GetAliasName() ),
                    GetChars( component->GetName() ),
                    GetChars( library->GetName() ) );
        wxMessageBox( msg, _( "Component Library Error" ),
                      wxID_OK | wxICON_ERROR, this );
    }
    else
    {
        entry->SetDescription( m_Doc->GetValue() );
        entry->SetKeyWords( m_Keywords->GetValue() );
        entry->SetDocFileName( m_Docfile->GetValue() );
    }

    if( m_PartAliasList->GetStrings() != component->m_AliasList )
    {
        wxArrayString aliases = m_PartAliasList->GetStrings();

        /* Add names not existing in the current component alias list. */
        for( i = 0; i < aliases.GetCount(); i++ )
        {
            index = component->m_AliasList.Index( aliases[ i ], false );

            if( index != wxNOT_FOUND )
                continue;

            component->m_AliasList.Add( aliases[ i ] );
            }

        /* Remove names in the current component that are not in the new alias list. */
        for( i = 0; i < component->m_AliasList.GetCount(); i++ )
        {
            index = aliases.Index( component->m_AliasList[ i ], false );

            if( index == wxNOT_FOUND )
                continue;

            component->m_AliasList.RemoveAt( i );
            i--;
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

    if( m_PinsNameInsideButt->GetValue() == false )
        component->m_TextInside = 0;        // pin text outside the body (name is on the pin)
    else
    {
        component->m_TextInside = m_SetSkew->GetValue();
        // Ensure component->m_TextInside != 0, because the meaning is "text outside".
        if( component->m_TextInside == 0 )
            component->m_TextInside = 20;       // give a reasonnable value
    }

    if( m_OptionPower->GetValue() == true )
        component->SetPower();
    else
        component->SetNormal();

    /* Set the option "Units locked".
     *  Obviously, cannot be true if there is only one part */
    component->m_UnitSelectionLocked = m_OptionPartsLocked->GetValue();
    if( component->GetPartCount() <= 1 )
        component->m_UnitSelectionLocked = false;

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

    m_Doc->SetValue( component->GetDescription() );
    m_Docfile->SetValue( component->GetDocFileName() );
    m_Keywords->SetValue( component->GetKeyWords() );
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
                    GetChars( m_Parent->GetAliasName() ) );
        DisplayError( this, msg );
        return;
    }

    m_Parent->GetAliasName().Empty();

    if( IsOK( this, _( "Remove all aliases from list?" ) ) )
    {
        m_PartAliasList->Clear();
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
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
                    GetChars( aliasname ),
                    GetChars( library->GetName() ) );
        DisplayError( this,  msg );
        return;
    }

    m_PartAliasList->Append( aliasname );
    if( m_Parent->GetAliasName().IsEmpty() )
        m_ButtonDeleteAllAlias->Enable( true );
    m_ButtonDeleteOneAlias->Enable( true );
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
                    GetChars( aliasname ) );
        DisplayError( this, msg );
        return;
    }

    m_PartAliasList->Delete( m_PartAliasList->GetSelection() );

    if( m_PartAliasList->IsEmpty() )
    {
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
    }
}


/*
 * Change the number of parts per package.
 */
bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::ChangeNbUnitsPerPackage( int MaxUnit )
{
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL || component->GetPartCount() == MaxUnit
        || MaxUnit < 1 )
        return false;

    if( MaxUnit < component->GetPartCount()
        && !IsOK( this, _( "Delete extra parts from component?" ) ) )
        return false;

    component->SetPartCount( MaxUnit );
    return true;
}


/*
 * Set or clear the component alternate body style ( DeMorgan ).
 */
bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::SetUnsetConvert()
{
    LIB_COMPONENT* component = m_Parent->GetComponent();

    if( component == NULL
        || ( m_Parent->GetShowDeMorgan() == component->HasConversion() ) )
        return false;

    if( m_Parent->GetShowDeMorgan() )
    {
       if( !IsOK( this, _( "Add new pins for alternate body style \
( DeMorgan ) to component?" ) ) )
        return false;
    }

    else if(  component->HasConversion() )
    {
        if( !IsOK( this, _( "Delete alternate body style (DeMorgan) draw items from component?" ) ) )
        {
            m_Parent->SetShowDeMorgan( true );
            return false;
        }
    }

    component->SetConversion( m_Parent->GetShowDeMorgan() );
    m_Parent->GetScreen()->SetModify();

    return true;
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
                                     true
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
    // Filenames are always stored in unix like mode, ie separator "\" is stored as "/"
    // to ensure files are identical under unices and windows
#ifdef __WINDOWS__
    filename.Replace(wxT("\\"), wxT("/") );
#endif
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
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
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
                    GetChars( Line ) );
        DisplayError( this, msg );
        return;
    }

    m_FootprintFilterListBox->Append( Line );
    m_ButtonDeleteAllFootprintFilter->Enable( true );
    m_ButtonDeleteOneFootprintFilter->Enable( true );
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
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
    }
}
