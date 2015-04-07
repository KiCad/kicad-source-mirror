/**
 * @file dialog_edit_component_in_lib.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiway.h>
#include <common.h>
#include <confirm.h>
#include <gestfich.h>
#include <pgm_base.h>

#include <general.h>
#include <libeditframe.h>
#include <class_library.h>
#include <eeschema_id.h>    // for ID_POPUP_SCH_SELECT_UNIT_CMP_MAX and ID_POPUP_SCH_SELECT_UNIT1

#include <dialog_edit_component_in_lib.h>

int DIALOG_EDIT_COMPONENT_IN_LIBRARY::m_lastOpenedPage = 0;

DIALOG_EDIT_COMPONENT_IN_LIBRARY::DIALOG_EDIT_COMPONENT_IN_LIBRARY( LIB_EDIT_FRAME* aParent ):
    DIALOG_EDIT_COMPONENT_IN_LIBRARY_BASE( aParent )
{
    m_Parent = aParent;
    m_RecreateToolbar = false;

    initDlg();

    GetSizer()->SetSizeHints( this );
    Center();
}


DIALOG_EDIT_COMPONENT_IN_LIBRARY::~DIALOG_EDIT_COMPONENT_IN_LIBRARY()
{
    m_lastOpenedPage = m_NoteBook->GetSelection( );
}

/* Initialize state of check boxes and texts
*/
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::initDlg()
{
    m_AliasLocation = -1;

    LIB_PART*      component = m_Parent->GetCurPart();

    if( component == NULL )
    {
        SetTitle( _( "Library Component Properties" ) );
        return;
    }

    wxString title;
    bool isRoot = m_Parent->GetAliasName().CmpNoCase( component->GetName() ) == 0;

    if( !isRoot )
    {
        title.Printf( _( "Properties for %s (alias of %s)" ),
                      GetChars( m_Parent->GetAliasName() ),
                      GetChars( component->GetName() ) );
    }
    else
        title.Printf( _( "Properties for %s" ), GetChars( component->GetName() ) );

    SetTitle( title );
    InitPanelDoc();
    InitBasicPanel();

    if( isRoot && component->GetAliasCount() == 1 )
        m_ButtonDeleteAllAlias->Enable( false );

    /* Place list of alias names in listbox */
    m_PartAliasListCtrl->Append( component->GetAliasNames( false ) );

    if( component->GetAliasCount() <= 1 )
    {
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
    }

    /* Read the Footprint Filter list */
    m_FootprintFilterListBox->Append( component->GetFootPrints() );

    if( component->GetFootPrints().GetCount() == 0 )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
        m_buttonEditOneFootprintFilter->Enable( false );
    }

    m_NoteBook->SetSelection( m_lastOpenedPage );

    m_stdSizerButtonOK->SetDefault();
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnCancelClick( wxCommandEvent& event )
{
    EndModal( wxID_CANCEL );
}



void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitPanelDoc()
{
    LIB_ALIAS* alias;
    LIB_PART*      component = m_Parent->GetCurPart();

    if( component == NULL )
        return;

    wxString aliasname = m_Parent->GetAliasName();

    if( aliasname.IsEmpty() )
        return;

    alias = component->GetAlias( aliasname );

    if( alias != NULL )
    {
        m_DocCtrl->SetValue( alias->GetDescription() );
        m_KeywordsCtrl->SetValue( alias->GetKeyWords() );
        m_DocfileCtrl->SetValue( alias->GetDocFileName() );
    }
}


/*
 * create the basic panel for component properties editing
 */
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::InitBasicPanel()
{
    LIB_PART*      component = m_Parent->GetCurPart();

    if( m_Parent->GetShowDeMorgan() )
        m_AsConvertButt->SetValue( true );

    int maxUnits = ID_POPUP_SCH_SELECT_UNIT_CMP_MAX - ID_POPUP_SCH_SELECT_UNIT1;
    m_SelNumberOfUnits->SetRange (1, maxUnits );

    m_staticTextNbUnits->SetLabel( wxString::Format(
                            _( "Number of Units (max allowed %d)" ), maxUnits ) );


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

    m_ShowPinNumButt->SetValue( component->ShowPinNumbers() );
    m_ShowPinNameButt->SetValue( component->ShowPinNames() );
    m_PinsNameInsideButt->SetValue( component->GetPinNameOffset() != 0 );
    m_SelNumberOfUnits->SetValue( component->GetUnitCount() );
    m_SetSkew->SetValue( component->GetPinNameOffset() );
    m_OptionPower->SetValue( component->IsPower() );
    m_OptionPartsLocked->SetValue( component->UnitsLocked() && component->GetUnitCount() > 1 );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::OnOkClick( wxCommandEvent& event )
{
    /* Update the doc, keyword and doc filename strings */
    LIB_ALIAS* alias;
    LIB_PART*      component = m_Parent->GetCurPart();

    if( component == NULL )
    {
        EndModal( wxID_CANCEL );
        return;
    }

    m_Parent->SaveCopyInUndoList( component );

    alias = component->GetAlias( m_Parent->GetAliasName() );

    wxCHECK_RET( alias != NULL,
                 wxT( "Alias \"" ) + m_Parent->GetAliasName() + wxT( "\" of component \"" ) +
                 component->GetName() + wxT( "\" does not exist." ) );

    alias->SetDescription( m_DocCtrl->GetValue() );
    alias->SetKeyWords( m_KeywordsCtrl->GetValue() );
    alias->SetDocFileName( m_DocfileCtrl->GetValue() );

    component->SetAliases( m_PartAliasListCtrl->GetStrings() );

    int unitCount = m_SelNumberOfUnits->GetValue();
    ChangeNbUnitsPerPackage( unitCount );

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

    component->SetShowPinNumbers( m_ShowPinNumButt->GetValue() );
    component->SetShowPinNames( m_ShowPinNameButt->GetValue() );

    if( m_PinsNameInsideButt->GetValue() == false )
        component->SetPinNameOffset( 0 );       // pin text outside the body (name is on the pin)
    else
    {
        component->SetPinNameOffset( m_SetSkew->GetValue() );
        // Ensure component->m_TextInside != 0, because the meaning is "text outside".
        if( component->GetPinNameOffset() == 0 )
            component->SetPinNameOffset( 20 );  // give a reasonnable value
    }

    if( m_OptionPower->GetValue() == true )
        component->SetPower();
    else
        component->SetNormal();

    /* Set the option "Units locked".
     *  Obviously, cannot be true if there is only one part */
    component->LockUnits( m_OptionPartsLocked->GetValue() );

    if( component->GetUnitCount() <= 1 )
        component->LockUnits( false );

    /* Update the footprint filter list */
    component->GetFootPrints().Clear();
    component->GetFootPrints() = m_FootprintFilterListBox->GetStrings();

    EndModal( wxID_OK );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::CopyDocFromRootToAlias( wxCommandEvent& event )
{
    if( m_Parent == NULL )
        return;

    LIB_ALIAS* parent_alias;
    LIB_PART*      component = m_Parent->GetCurPart();

    if( component == NULL )
        return;

    // search for the main alias: this is the first alias in alias list
    // something like the main component
    parent_alias = component->GetAlias( 0 );

    if( parent_alias == NULL )  // Should never occur (bug)
        return;

    m_DocCtrl->SetValue( parent_alias->GetDescription() );
    m_DocfileCtrl->SetValue( parent_alias->GetDocFileName() );
    m_KeywordsCtrl->SetValue( parent_alias->GetKeyWords() );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAllAliasOfPart( wxCommandEvent& event )
{
    if( m_PartAliasListCtrl->FindString( m_Parent->GetAliasName() ) != wxNOT_FOUND )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> cannot be removed while it is being edited!" ),
                    GetChars( m_Parent->GetAliasName() ) );
        DisplayError( this, msg );
        return;
    }

    if( IsOK( this, _( "Remove all aliases from list?" ) ) )
    {
        m_PartAliasListCtrl->Clear();
        m_ButtonDeleteAllAlias->Enable( false );
        m_ButtonDeleteOneAlias->Enable( false );
    }
}


/* Add a new name to the alias list box
 *  New name cannot be the root name, and must not exists
 */
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::AddAliasOfPart( wxCommandEvent& event )
{
    wxString aliasname;
    LIB_PART*      component = m_Parent->GetCurPart();
    PART_LIB* library = m_Parent->GetCurLib();

    if( component == NULL )
        return;

    wxTextEntryDialog dlg( this, _( "New alias:" ), _( "Component Alias" ), aliasname );

    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    aliasname = dlg.GetValue( );

    aliasname.Replace( wxT( " " ), wxT( "_" ) );
    if( aliasname.IsEmpty() )
        return;

    if( m_PartAliasListCtrl->FindString( aliasname ) != wxNOT_FOUND )
    {
        wxString msg;
        msg.Printf( _( "Alias or component name <%s> already in use." ),
                    GetChars( aliasname ) );
        DisplayError( this, msg );
        return;
    }

    if( library && library->FindEntry( aliasname ) != NULL )
    {
        wxString msg;
        msg.Printf( _( "Alias or component name <%s> already exists in library <%s>." ),
                    GetChars( aliasname ),
                    GetChars( library->GetName() ) );
        DisplayError( this, msg );
        return;
    }

    m_PartAliasListCtrl->Append( aliasname );

    if( m_Parent->GetAliasName().CmpNoCase( component->GetName() ) == 0 )
        m_ButtonDeleteAllAlias->Enable( true );

    m_ButtonDeleteOneAlias->Enable( true );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAliasOfPart( wxCommandEvent& event )
{
    wxString aliasname = m_PartAliasListCtrl->GetStringSelection();

    if( aliasname.IsEmpty() )
        return;

    if( aliasname.CmpNoCase( m_Parent->GetAliasName() ) == 0 )
    {
        wxString msg;
        msg.Printf( _( "Alias <%s> cannot be removed while it is being edited!" ),
                    GetChars( aliasname ) );
        DisplayError( this, msg );
        return;
    }

    m_PartAliasListCtrl->Delete( m_PartAliasListCtrl->GetSelection() );
    LIB_PART*      component = m_Parent->GetCurPart();

    if( component )
        component->RemoveAlias( aliasname );

    if( m_PartAliasListCtrl->IsEmpty() )
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
    LIB_PART*      part = m_Parent->GetCurPart();

    if( !part || part->GetUnitCount() == MaxUnit || MaxUnit < 1 )
        return false;

    if( MaxUnit < part->GetUnitCount()
        && !IsOK( this, _( "Delete extra parts from component?" ) ) )
        return false;

    part->SetUnitCount( MaxUnit );
    return true;
}


/*
 * Set or clear the component alternate body style ( DeMorgan ).
 */
bool DIALOG_EDIT_COMPONENT_IN_LIBRARY::SetUnsetConvert()
{
    LIB_PART*      component = m_Parent->GetCurPart();

    if( component == NULL || ( m_Parent->GetShowDeMorgan() == component->HasConversion() ) )
        return false;

    if( m_Parent->GetShowDeMorgan() )
    {
        if( !IsOK( this, _( "Add new pins for alternate body style ( DeMorgan ) to component?" ) ) )
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
    m_Parent->OnModify();

    return true;
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::BrowseAndSelectDocFile( wxCommandEvent& event )
{
    PROJECT&        prj = Prj();
    SEARCH_STACK*   search = prj.SchSearchS();

    wxString    mask = wxT( "*" );
    wxString    docpath = prj.GetRString( PROJECT::DOC_PATH );

    if( !docpath )
        docpath = search->LastVisitedPath( wxT( "doc" ) );

    wxString    fullFileName = EDA_FileSelector( _( "Doc Files" ),
                                     docpath,
                                     wxEmptyString,
                                     wxEmptyString,
                                     mask,
                                     this,
                                     wxFD_OPEN,
                                     true
                                     );
    if( fullFileName.IsEmpty() )
        return;

    /* If the path is already in the library search paths
     * list, just add the library name to the list.  Otherwise, add
     * the library name with the full or relative path.
     * the relative path, when possible is preferable,
     * because it preserve use of default libraries paths, when the path is a sub path of
     * these default paths
     */
    wxFileName fn = fullFileName;

    prj.SetRString( PROJECT::DOC_PATH, fn.GetPath() );

    wxString filename = search->FilenameWithRelativePathInSearchList(
            fullFileName, wxPathOnly( Prj().GetProjectFullName() ) );

    // Filenames are always stored in unix like mode, ie separator "\" is stored as "/"
    // to ensure files are identical under unices and windows
#ifdef __WINDOWS__
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif
    m_DocfileCtrl->SetValue( filename );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteAllFootprintFilter( wxCommandEvent& event )
{
    if( IsOK( this, _( "OK to delete the footprint filter list ?" ) ) )
    {
        m_FootprintFilterListBox->Clear();
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
        m_buttonEditOneFootprintFilter->Enable( false );
    }
}


/* Add a new name to the footprint filter list box
 * Obvioulsy, cannot be void
 */
void DIALOG_EDIT_COMPONENT_IN_LIBRARY::AddFootprintFilter( wxCommandEvent& event )
{
    wxString Line;
    LIB_PART*      component = m_Parent->GetCurPart();

    if( component == NULL )
        return;

    wxTextEntryDialog dlg( this, _( "Add Footprint Filter" ), _( "Footprint Filter" ), Line );
    if( dlg.ShowModal() != wxID_OK )
        return; // cancelled by user

    Line = dlg.GetValue();
    Line.Replace( wxT( " " ), wxT( "_" ) );

    if( Line.IsEmpty() )
        return;

    /* test for an existing name: */
    int index = m_FootprintFilterListBox->FindString( Line );

    if( index != wxNOT_FOUND )
    {
        wxString msg;

        msg.Printf( _( "Foot print filter <%s> is already defined." ), GetChars( Line ) );
        DisplayError( this, msg );
        return;
    }

    m_FootprintFilterListBox->Append( Line );
    m_ButtonDeleteAllFootprintFilter->Enable( true );
    m_ButtonDeleteOneFootprintFilter->Enable( true );
    m_buttonEditOneFootprintFilter->Enable( true );
}


void DIALOG_EDIT_COMPONENT_IN_LIBRARY::DeleteOneFootprintFilter( wxCommandEvent& event )
{
    LIB_PART*      component = m_Parent->GetCurPart();
    int ii = m_FootprintFilterListBox->GetSelection();

    m_FootprintFilterListBox->Delete( ii );

    if( !component || ( m_FootprintFilterListBox->GetCount() == 0 ) )
    {
        m_ButtonDeleteAllFootprintFilter->Enable( false );
        m_ButtonDeleteOneFootprintFilter->Enable( false );
        m_buttonEditOneFootprintFilter->Enable( false );
    }
}

void DIALOG_EDIT_COMPONENT_IN_LIBRARY::EditOneFootprintFilter( wxCommandEvent& event )
{
    int idx = m_FootprintFilterListBox->GetSelection();

    if( idx < 0 )
        return;

    wxString filter = m_FootprintFilterListBox->GetStringSelection();

    wxTextEntryDialog dlg( this, wxEmptyString, _( "Edit footprint filter" ), filter );

    if( dlg.ShowModal() != wxID_OK )
        return;    // Aborted by user

    filter = dlg.GetValue();

    if( filter.IsEmpty() )
        return;    // do not accept blank filter.

    m_FootprintFilterListBox->SetString( idx, filter );
}
