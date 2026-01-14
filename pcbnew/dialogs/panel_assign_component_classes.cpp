/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#include <dialogs/panel_assign_component_classes.h>

#include <bitmaps.h>
#include <board.h>
#include <component_classes/component_class_assignment_rule.h>
#include <component_classes/component_class_manager.h>
#include <footprint.h>
#include <kiway.h>
#include <pcb_edit_frame.h>
#include <project/project_file.h>
#include <tool/selection_tool.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/paged_dialog.h>
#include <widgets/std_bitmap_button.h>


/**************************************************************************************************
 *
 * PANEL_ASSIGN_COMPONENT_CLASSES implementation
 * This is the top-level panel for component class configuration
 *
 *************************************************************************************************/

PANEL_ASSIGN_COMPONENT_CLASSES::PANEL_ASSIGN_COMPONENT_CLASSES(
        wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame,
        std::shared_ptr<COMPONENT_CLASS_SETTINGS> aSettings, DIALOG_SHIM* aDlg ) :
        PANEL_ASSIGN_COMPONENT_CLASSES_BASE( aParentWindow ), m_dlg( aDlg ), m_frame( aFrame ),
        m_componentClassSettings( std::move( aSettings ) ),
        m_assignmentsList( static_cast<wxBoxSizer*>( m_assignmentsScrollWindow->GetSizer() ) )
{
    // Load footprint fields and sheet names
    const BOARD*       board = dynamic_cast<BOARD*>( m_frame->GetModel() );
    std::set<wxString> fieldsSet;
    std::set<wxString> sheetsSet;

    for( const FOOTPRINT* fp : board->Footprints() )
    {
        wxString sheetName = fp->GetSheetname();

        if( !sheetName.empty() )
        {
            sheetName.Replace( wxT( "\"" ), wxT( "" ) );
            sheetName.Replace( wxT( "'" ), wxT( "" ) );
            sheetsSet.insert( fp->GetSheetname() );
        }

        for( const PCB_FIELD* field : fp->GetFields() )
        {
            wxCHECK2( field, continue );

            fieldsSet.insert( field->GetName() );
        }
    }

    // Sort field names
    std::vector<wxString> fieldNames( fieldsSet.begin(), fieldsSet.end() );
    std::ranges::sort( fieldNames,
                       []( const wxString& a, const wxString& b )
                       {
                           return a.Cmp( b ) < 0;
                       } );

    m_fieldNames = std::move( fieldNames );

    // Sort sheet names
    std::vector<wxString> sheetNames( sheetsSet.begin(), sheetsSet.end() );
    std::ranges::sort( sheetNames,
                       []( const wxString& a, const wxString& b )
                       {
                           return a.Cmp( b ) < 0;
                       } );

    m_sheetNames = std::move( sheetNames );

    // Get references of currently selected items
    std::set<wxString> refsSet;

    for( const EDA_ITEM* item : m_frame->GetCurrentSelection() )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            continue;

        const FOOTPRINT* fp = static_cast<const FOOTPRINT*>( item );
        wxString         ref = fp->GetReferenceAsString();
        refsSet.insert( ref );
    }

    std::vector<wxString> refs( refsSet.begin(), refsSet.end() );
    std::ranges::sort( refs,
                       []( const wxString& a, const wxString& b )
                       {
                           return a.Cmp( b ) < 0;
                       } );

    m_selectionRefs = std::move( refs );
}


PANEL_ASSIGN_COMPONENT_CLASSES::~PANEL_ASSIGN_COMPONENT_CLASSES()
{
    BOARD*              board = dynamic_cast<BOARD*>( m_frame->GetModel() );
    PCB_SELECTION_TOOL* selTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();

    for( FOOTPRINT* fp : board->Footprints() )
        selTool->UnbrightenItem( fp );

    m_frame->GetCanvas()->Refresh();
}


bool PANEL_ASSIGN_COMPONENT_CLASSES::TransferDataToWindow()
{
    // Load sheet-level settings
    m_assignSheetClasses->SetValue( m_componentClassSettings->GetEnableSheetComponentClasses() );

    // Load dynamic component class assignments
    for( const COMPONENT_CLASS_ASSIGNMENT_DATA& assignmentData :
         m_componentClassSettings->GetComponentClassAssignments() )
    {
        PANEL_COMPONENT_CLASS_ASSIGNMENT* assignment = addAssignment();
        assignment->SetComponentClass( assignmentData.GetComponentClass() );
        assignment->SetConditionsOperator( assignmentData.GetConditionsOperator() );

        for( const auto& [conditionType, conditionData] : assignmentData.GetConditions() )
        {
            CONDITION_DATA* match = assignment->AddCondition( conditionType );
            match->SetPrimaryField( conditionData.first );
            match->SetSecondaryField( conditionData.second );
        }
    }

    return true;
}


bool PANEL_ASSIGN_COMPONENT_CLASSES::TransferDataFromWindow()
{
    if( !Validate() )
        return false;

    // Save sheet-level settings
    m_componentClassSettings->SetEnableSheetComponentClasses( m_assignSheetClasses->GetValue() );

    // Save dynamic component class assignments
    m_componentClassSettings->ClearComponentClassAssignments();

    for( const auto assignment : m_assignments )
    {
        COMPONENT_CLASS_ASSIGNMENT_DATA assignmentData = assignment->GenerateAssignmentData();
        m_componentClassSettings->AddComponentClassAssignment( assignmentData );
    }

    return true;
}


bool PANEL_ASSIGN_COMPONENT_CLASSES::Validate()
{
    PCB_EDIT_FRAME* frame = GetFrame();
    BOARD*          board = dynamic_cast<BOARD*>( frame->GetModel() );

    frame->GetCanvas()->Refresh();

    for( PANEL_COMPONENT_CLASS_ASSIGNMENT* assignment : m_assignments )
    {
        const COMPONENT_CLASS_ASSIGNMENT_DATA assignmentData = assignment->GenerateAssignmentData();
        const std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE> rule =
                board->GetComponentClassManager().CompileAssignmentRule( assignmentData );

        if( !rule )
        {
            const wxString msg = wxString::Format(
                    _( "Error with conditions for component class assignment %s" ),
                    assignment->GetComponentClass() );
            PAGED_DIALOG::GetDialog( this )->SetError( msg, this, assignment );
            scrollToAssignment( assignment );
            assignment->SetFocus();
            return false;
        }
    }

    return true;
}


void PANEL_ASSIGN_COMPONENT_CLASSES::OnAddAssignmentClick( wxCommandEvent& event )
{
    PANEL_COMPONENT_CLASS_ASSIGNMENT* assignment = addAssignment();
    scrollToAssignment( assignment );
}


PANEL_COMPONENT_CLASS_ASSIGNMENT* PANEL_ASSIGN_COMPONENT_CLASSES::addAssignment()
{
    PANEL_COMPONENT_CLASS_ASSIGNMENT* assignmentPanel =
            new PANEL_COMPONENT_CLASS_ASSIGNMENT( m_assignmentsScrollWindow, this, m_dlg );

#if __OSX__
    m_assignmentsList->Add( assignmentPanel, 0, wxEXPAND, 5 );
#else
    m_assignmentsList->Add( assignmentPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5 );
#endif

    Layout();

    m_assignments.push_back( assignmentPanel );

    return assignmentPanel;
}


void PANEL_ASSIGN_COMPONENT_CLASSES::scrollToAssignment(
        const PANEL_COMPONENT_CLASS_ASSIGNMENT* aAssignment ) const
{
    const wxPoint viewStart = m_assignmentsScrollWindow->GetViewStart();
    const wxPoint panelPosition = aAssignment->GetPosition();
    const wxSize  panelSize = aAssignment->GetClientSize();

    m_assignmentsScrollWindow->Scroll( viewStart.x, panelPosition.y + panelSize.y );
}


void PANEL_ASSIGN_COMPONENT_CLASSES::RemoveAssignment( PANEL_COMPONENT_CLASS_ASSIGNMENT* aPanel )
{
    m_assignments.erase( std::ranges::find( m_assignments, aPanel ) );
    m_assignmentsList->Detach( aPanel );
    aPanel->Destroy();
    Layout();
}


void PANEL_ASSIGN_COMPONENT_CLASSES::ImportSettingsFrom(
        const std::shared_ptr<COMPONENT_CLASS_SETTINGS>& aOtherSettings )
{
    std::shared_ptr<COMPONENT_CLASS_SETTINGS> savedSettings = m_componentClassSettings;

    m_componentClassSettings = aOtherSettings;
    TransferDataToWindow();

    m_componentClassSettings = std::move( savedSettings );
}


/**************************************************************************************************
 *
 * CONDITION_DATA implementation
 * Provides a common interface for all condition panel types
 *
 *************************************************************************************************/

wxString CONDITION_DATA::GetPrimaryField() const
{
    wxString data = m_primaryCtrl->GetValue();
    data.Trim( true );
    data.Trim( false );

    if( m_conditionType != COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM )
    {
        data.Replace( wxT( "\"" ), wxT( "" ) );
        data.Replace( wxT( "'" ), wxT( "" ) );
    }

    return data;
}


void CONDITION_DATA::SetPrimaryField( const wxString& aVal )
{
    m_primaryCtrl->SetValue( aVal );
}


wxString CONDITION_DATA::GetSecondaryField() const
{
    if( !m_secondaryCtrl )
        return wxEmptyString;

    wxString data = m_secondaryCtrl->GetValue();
    data.Trim( true );
    data.Trim( false );

    if( m_conditionType != COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM )
    {
        data.Replace( wxT( "\"" ), wxT( "" ) );
        data.Replace( wxT( "'" ), wxT( "" ) );
    }

    return data;
}


void CONDITION_DATA::SetSecondaryField( const wxString& aVal )
{
    if( m_secondaryCtrl )
        m_secondaryCtrl->SetValue( aVal );
};

/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_ASSIGNMENT implementation
 * This is the dynamically-added panel for each component class assignment rule set
 *
 *************************************************************************************************/

PANEL_COMPONENT_CLASS_ASSIGNMENT::PANEL_COMPONENT_CLASS_ASSIGNMENT(
        wxWindow* aParent, PANEL_ASSIGN_COMPONENT_CLASSES* aPanelParent, DIALOG_SHIM* aDlg ) :
        PANEL_COMPONENT_CLASS_ASSIGNMENT_BASE( aParent ), m_parentPanel( aPanelParent ),
        m_matchesList( static_cast<wxStaticBoxSizer*>( GetSizer() ) ), m_dlg( aDlg )
{
    m_buttonAddCondition->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_buttonDeleteAssignment->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_buttonHighlightItems->SetBitmap( KiBitmapBundle( BITMAPS::net_highlight ) );

    Connect( wxEVT_MENU, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT::onMenu ), nullptr,
             this );
}


PANEL_COMPONENT_CLASS_ASSIGNMENT::~PANEL_COMPONENT_CLASS_ASSIGNMENT()
{
    Disconnect( wxEVT_MENU, wxCommandEventHandler( PANEL_COMPONENT_CLASS_ASSIGNMENT::onMenu ),
                nullptr, this );
}


COMPONENT_CLASS_ASSIGNMENT_DATA PANEL_COMPONENT_CLASS_ASSIGNMENT::GenerateAssignmentData() const
{
    COMPONENT_CLASS_ASSIGNMENT_DATA assignmentData;
    assignmentData.SetComponentClass( GetComponentClass() );
    assignmentData.SetConditionsOperation( GetConditionsOperator() );

    for( const auto& condition : GetConditions() )
    {
        assignmentData.SetCondition( condition->GetConditionType(), condition->GetPrimaryField(),
                                     condition->GetSecondaryField() );
    }

    return assignmentData;
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::OnAddConditionClick( wxCommandEvent& event )
{
    auto hasCondition = [this]( const COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE aCondition )
    {
        return m_conditionTypes.contains( aCondition );
    };

    wxMenu menu;
    menu.Append( ID_REFERENCE, _( "Reference..." ) );
    menu.Append( ID_FOOTPRINT, _( "Footprint..." ) );
    menu.Append( ID_SIDE, _( "Side..." ) );
    menu.Append( ID_ROTATION, _( "Rotation..." ) );
    menu.Append( ID_FOOTPRINT_FIELD, _( "Footprint Field..." ) );
    menu.Append( ID_SHEET_NAME, _( "Sheet Name..." ) );
    menu.Append( ID_CUSTOM, _( "Custom..." ) );
    menu.Enable( ID_REFERENCE,
                 !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE ) );
    menu.Enable( ID_FOOTPRINT,
                 !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT ) );
    menu.Enable( ID_SIDE, !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SIDE ) );
    menu.Enable( ID_ROTATION,
                 !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::ROTATION ) );
    menu.Enable(
            ID_FOOTPRINT_FIELD,
            !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT_FIELD ) );
    menu.Enable( ID_CUSTOM,
                 !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM ) );
    menu.Enable( ID_SHEET_NAME,
                 !hasCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SHEET_NAME ) );
    PopupMenu( &menu );
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::OnDeleteAssignmentClick( wxCommandEvent& event )
{
    m_parentPanel->RemoveAssignment( this );
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::SetComponentClass( const wxString& aComponentClass ) const
{
    m_componentClass->SetValue( aComponentClass );
}


const wxString PANEL_COMPONENT_CLASS_ASSIGNMENT::GetComponentClass() const
{
    return m_componentClass->GetValue();
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::onMenu( wxCommandEvent& aEvent )
{
    switch( aEvent.GetId() )
    {
    case ID_REFERENCE:
        AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE );
        break;
    case ID_FOOTPRINT:
        AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT );
        break;
    case ID_SIDE: AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SIDE ); break;
    case ID_ROTATION:
        AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::ROTATION );
        break;
    case ID_FOOTPRINT_FIELD:
        AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT_FIELD );
        break;
    case ID_CUSTOM: AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM ); break;
    case ID_SHEET_NAME:
        AddCondition( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SHEET_NAME );
        break;

    default: wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );
    }
}


CONDITION_DATA* PANEL_COMPONENT_CLASS_ASSIGNMENT::AddCondition(
        const COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE aCondition )
{
    wxASSERT_MSG( !m_conditionTypes.contains( aCondition ), "Condition type already exists" );

    wxPanel* panelToAdd = nullptr;

    switch( aCondition )
    {
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE:
    {
        PANEL_COMPONENT_CLASS_CONDITION_REFERENCE* refsPanel =
                new PANEL_COMPONENT_CLASS_CONDITION_REFERENCE( this );
        refsPanel->SetSelectionRefs( m_parentPanel->GetSelectionRefs() );
        panelToAdd = refsPanel;
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE );
        break;
    }
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT:
    {
        panelToAdd = new PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT( this, m_dlg );
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT );
        break;
    }
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SIDE:
    {
        panelToAdd = new PANEL_COMPONENT_CLASS_CONDITION_SIDE( this );
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SIDE );
        break;
    }
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::ROTATION:
    {
        panelToAdd = new PANEL_COMPONENT_CLASS_CONDITION_ROTATION( this );
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::ROTATION );
        break;
    }
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT_FIELD:
    {
        PANEL_COMPONENT_CLASS_CONDITION_FIELD* fieldPanel =
                new PANEL_COMPONENT_CLASS_CONDITION_FIELD( this );
        fieldPanel->SetFieldsList( m_parentPanel->GetFieldNames() );
        panelToAdd = fieldPanel;
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT_FIELD );
        break;
    }
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM:
    {
        panelToAdd = new PANEL_COMPONENT_CLASS_CONDITION_CUSTOM( this );
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM );
        break;
    }
    case COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SHEET_NAME:
    {
        PANEL_COMPONENT_CLASS_CONDITION_SHEET* sheetPanel =
                new PANEL_COMPONENT_CLASS_CONDITION_SHEET( this );
        sheetPanel->SetSheetsList( m_parentPanel->GetSheetNames() );
        panelToAdd = sheetPanel;
        m_conditionTypes.insert( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SHEET_NAME );
        break;
    }
    }

    const size_t numItems = m_matchesList->GetItemCount();
    m_matchesList->Insert( numItems - 1, panelToAdd, 0, wxEXPAND | wxTOP, 5 );
    Layout();
    GetParent()->Layout();
    m_parentPanel->Layout();

    CONDITION_DATA* conditionIface = dynamic_cast<CONDITION_DATA*>( panelToAdd );
    m_matches.push_back( conditionIface );

    return conditionIface;
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::RemoveCondition( wxPanel* aMatch )
{
    if( CONDITION_DATA* matchData = dynamic_cast<CONDITION_DATA*>( aMatch ) )
    {
        m_conditionTypes.erase( matchData->GetConditionType() );
        m_matches.erase( std::ranges::find( m_matches, matchData ) );
    }

    m_matchesList->Detach( aMatch );
    aMatch->Destroy();
    Layout();
    m_parentPanel->Layout();
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::SetConditionsOperator(
        const COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR aCondition ) const
{
    if( aCondition == COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR::ALL )
    {
        m_radioAll->SetValue( true );
        m_radioAny->SetValue( false );
    }
    else
    {
        m_radioAll->SetValue( false );
        m_radioAny->SetValue( true );
    }
}


COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR
PANEL_COMPONENT_CLASS_ASSIGNMENT::GetConditionsOperator() const
{
    if( m_radioAll->GetValue() == true )
        return COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR::ALL;

    return COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITIONS_OPERATOR::ANY;
}


void PANEL_COMPONENT_CLASS_ASSIGNMENT::OnHighlightItemsClick( wxCommandEvent& event )
{
    PCB_EDIT_FRAME*     frame = m_parentPanel->GetFrame();
    BOARD*              board = dynamic_cast<BOARD*>( frame->GetModel() );
    PCB_SELECTION_TOOL* selTool = frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();

    const COMPONENT_CLASS_ASSIGNMENT_DATA                  assignment = GenerateAssignmentData();
    const std::shared_ptr<COMPONENT_CLASS_ASSIGNMENT_RULE> rule =
            board->GetComponentClassManager().CompileAssignmentRule( assignment );

    if( !rule )
        return;

    for( FOOTPRINT* fp : board->Footprints() )
    {
        if( rule->Matches( fp ) )
            selTool->BrightenItem( fp );
        else
            selTool->UnbrightenItem( fp );
    }

    frame->GetCanvas()->Refresh();
}


/**************************************************************************************************
 *
 * PANEL_COMPONENT_CLASS_CONDITION_* implementations
 * These are the dynamically-added panels for each condition / match type
 *
 *************************************************************************************************/

PANEL_COMPONENT_CLASS_CONDITION_REFERENCE::PANEL_COMPONENT_CLASS_CONDITION_REFERENCE(
        wxWindow* aParent ) :
        PANEL_COMPONENT_CLASS_CONDITION_REFERENCE_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::REFERENCE, m_refs ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );

    m_buttonImportRefs->SetBitmap( KiBitmapBundle( BITMAPS::config ) );
    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    Connect( wxEVT_MENU, wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE::onMenu ),
             nullptr, this );
}


void PANEL_COMPONENT_CLASS_CONDITION_REFERENCE::OnDeleteConditionClick( wxCommandEvent& event )
{
    Disconnect( wxEVT_MENU,
                wxCommandEventHandler( PANEL_COMPONENT_CLASS_CONDITION_REFERENCE::onMenu ), nullptr,
                this );
    m_panelParent->RemoveCondition( this );
}


void PANEL_COMPONENT_CLASS_CONDITION_REFERENCE::OnImportRefsClick( wxCommandEvent& event )
{
    wxMenu menu;
    menu.Append( ID_IMPORT_REFS, _( "Import references from selection" ) );
    menu.Enable( ID_IMPORT_REFS, !m_selectionRefs.empty() );
    PopupMenu( &menu );
}


void PANEL_COMPONENT_CLASS_CONDITION_REFERENCE::onMenu( wxCommandEvent& aEvent )
{
    if( aEvent.GetId() != ID_IMPORT_REFS )
        wxFAIL_MSG( wxT( "Unknown ID in context menu event" ) );

    if( m_selectionRefs.empty() )
        return;

    wxString refs = m_selectionRefs[0];

    for( size_t i = 1; i < m_selectionRefs.size(); i++ )
    {
        refs += wxT( "," );
        refs += m_selectionRefs[i];
    }

    m_refs->SetValue( refs );
}


PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT::PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT(
        wxWindow* aParent, DIALOG_SHIM* aDlg ) :
        PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT, m_footprint ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) ), m_dlg( aDlg )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );

    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_buttonShowLibrary->SetBitmap( KiBitmapBundle( BITMAPS::small_library ) );
}


void PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT::OnDeleteConditionClick( wxCommandEvent& event )
{
    m_panelParent->RemoveCondition( this );
}


void PANEL_COMPONENT_CLASS_CONDITION_FOOTPRINT::OnShowLibraryClick( wxCommandEvent& event )
{
    wxString fpId = m_footprint->GetValue();

    if( KIWAY_PLAYER* frame = m_dlg->Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, m_dlg ) )
    {
        if( frame->ShowModal( &fpId, this ) )
        {
            m_footprint->SetValue( fpId );
        }

        frame->Destroy();
    }
}


PANEL_COMPONENT_CLASS_CONDITION_SIDE::PANEL_COMPONENT_CLASS_CONDITION_SIDE( wxWindow* aParent ) :
        PANEL_COMPONENT_CLASS_CONDITION_SIDE_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SIDE, m_side ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );

    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_side->Append( wxT( "Any" ) );
    m_side->Append( wxT( "Front" ) );
    m_side->Append( wxT( "Back" ) );
    m_side->SetValue( wxT( "Any" ) );
}


void PANEL_COMPONENT_CLASS_CONDITION_SIDE::OnDeleteConditionClick( wxCommandEvent& event )
{
    m_panelParent->RemoveCondition( this );
}


PANEL_COMPONENT_CLASS_CONDITION_ROTATION::PANEL_COMPONENT_CLASS_CONDITION_ROTATION(
        wxWindow* aParent ) :
        PANEL_COMPONENT_CLASS_CONDITION_ROTATION_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::ROTATION, m_rotation ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );
    m_rotUnit->SetLabel( wxT( "°" ) );

    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_rotation->Append( wxT( "Any" ) );
    m_rotation->Append( wxT( "0" ) );
    m_rotation->Append( wxT( "90" ) );
    m_rotation->Append( wxT( "180" ) );
    m_rotation->Append( wxT( "-90" ) );
    m_rotation->SetValue( wxT( "Any" ) );
}


void PANEL_COMPONENT_CLASS_CONDITION_ROTATION::OnDeleteConditionClick( wxCommandEvent& event )
{
    m_panelParent->RemoveCondition( this );
}


PANEL_COMPONENT_CLASS_CONDITION_FIELD::PANEL_COMPONENT_CLASS_CONDITION_FIELD( wxWindow* aParent ) :
        PANEL_COMPONENT_CLASS_CONDITION_FIELD_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::FOOTPRINT_FIELD,
                        m_fieldName, m_fieldValue ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );

    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
}


void PANEL_COMPONENT_CLASS_CONDITION_FIELD::OnDeleteConditionClick( wxCommandEvent& event )
{
    m_panelParent->RemoveCondition( this );
}


void PANEL_COMPONENT_CLASS_CONDITION_FIELD::SetFieldsList( const std::vector<wxString>& aFields )
{
    m_fieldName->Append( aFields );
}


PANEL_COMPONENT_CLASS_CONDITION_CUSTOM::PANEL_COMPONENT_CLASS_CONDITION_CUSTOM(
        wxWindow* aParent ) :
        PANEL_COMPONENT_CLASS_CONDITION_CUSTOM_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::CUSTOM,
                        m_customCondition ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );

    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
}


void PANEL_COMPONENT_CLASS_CONDITION_CUSTOM::OnDeleteConditionClick( wxCommandEvent& event )
{
    m_panelParent->RemoveCondition( this );
}


PANEL_COMPONENT_CLASS_CONDITION_SHEET::PANEL_COMPONENT_CLASS_CONDITION_SHEET( wxWindow* aParent ) :
        PANEL_COMPONENT_CLASS_CONDITION_SHEET_BASE( aParent ),
        CONDITION_DATA( COMPONENT_CLASS_ASSIGNMENT_DATA::CONDITION_TYPE::SHEET_NAME, m_sheetName ),
        m_panelParent( static_cast<PANEL_COMPONENT_CLASS_ASSIGNMENT*>( aParent ) )
{
    m_title->SetMinSize( { GetTextExtent( _( "Footprint Field:" ) ).x, -1 } );

    m_buttonDeleteMatch->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
}


void PANEL_COMPONENT_CLASS_CONDITION_SHEET::OnDeleteConditionClick( wxCommandEvent& event )
{
    m_panelParent->RemoveCondition( this );
}


void PANEL_COMPONENT_CLASS_CONDITION_SHEET::SetSheetsList( const std::vector<wxString>& aSheets )
{
    m_sheetName->Append( aSheets );
}
