/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "constraint_rules_list.h"

#include <board.h>
#include <confirm.h>
#include <dialog_shim.h>
#include <pcb_edit_frame.h>
#include <drc/drc_engine.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_rule_parser.h>
#include <drc/rule_editor/drc_re_loaded_rule.h>
#include <base_units.h>
#include <eda_units.h>
#include <drc/rule_editor/drc_re_custom_rule_constraint_data.h>
#include <drc/rule_editor/drc_re_numeric_input_constraint_data.h>
#include <drc/rule_editor/drc_rule_document.h>
#include <drc/rule_editor/drc_rule_editor_utils.h>
#include <drc/rule_editor/panel_drc_rule_editor.h>
#include <netclass.h>
#include <project.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/drc_tool.h>
#include <bitmaps.h>
#include <widgets/paged_dialog.h>
#include <widgets/ui_common.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/dcclient.h>
#include <wx/regex.h>
#include <wx/settings.h>
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/toplevel.h>
#include <algorithm>


CONSTRAINT_RULES_LIST::CONSTRAINT_RULES_LIST( wxWindow* aParent, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                                              DRC_RULE_EDITOR_CONSTRAINT_NAME aType, const wxString& aTitle ) :
        CONSTRAINT_RULES_LIST_BASE( aParent ),
        m_frame( aFrame ),
        m_document( aDocument ),
        m_type( aType ),
        m_title( aTitle ),
        m_titleWindow( nullptr ),
        m_editor( nullptr )
{
    m_list->AppendColumn( wxS( "#" ), wxLIST_FORMAT_RIGHT, 36 );
    m_list->AppendColumn( _( "Name" ), wxLIST_FORMAT_LEFT, 160 );
    m_list->AppendColumn( _( "Condition" ), wxLIST_FORMAT_LEFT, 280 );
    m_list->AppendColumn( _( "Layers" ), wxLIST_FORMAT_LEFT, 120 );
    m_list->AppendColumn( _( "Constraint" ), wxLIST_FORMAT_LEFT, 240 );

    m_emptyLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );

    m_upBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_downBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_duplicateBtn->SetBitmap( KiBitmapBundle( BITMAPS::duplicate ) );
    m_removeBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_primaryBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );

    m_primaryBtn->SetMinSize( wxSize( m_primaryBtn->GetBestSize().x + FromDIP( 16 ), -1 ) );

    m_primaryBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onPrimary, this );
    m_newBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onNewRule, this );
    m_upBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onMoveUp, this );
    m_downBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onMoveDown, this );
    m_duplicateBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onDuplicate, this );
    m_removeBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onRemove, this );

    // The editor is rebuilt per rule, so undo would keep pointers to destroyed controls
    if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
        dlg->ExcludeFromControlUndoRedo( m_editorHost );

    m_list->Bind( wxEVT_SIZE,
                  [this]( wxSizeEvent& aEvent )
                  {
                      // Fitting toggles the horizontal scrollbar, which resizes the list again
                      if( m_list->GetClientSize().x != m_fittedWidth )
                          fitColumns();

                      aEvent.Skip();
                  } );
    m_list->Bind( wxEVT_LIST_ITEM_SELECTED,
                  [this]( wxListEvent& )
                  {
                      if( !m_refreshing && selectedIndex() >= 0 && selectedIndex() != editingIndex()
                          && !EditRule( selectedIndex() ) )
                      {
                          if( editingIndex() >= 0 )
                          {
                              selectIndex( editingIndex() );
                          }
                          else
                          {
                              for( long i = 0; i < m_list->GetItemCount(); ++i )
                                  m_list->SetItemState( i, 0, wxLIST_STATE_SELECTED );
                          }
                      }

                      updateButtons();
                  } );
    m_list->Bind( wxEVT_LIST_ITEM_ACTIVATED,
                  [this]( wxListEvent& aEvent )
                  {
                      long row = aEvent.GetIndex();

                      if( row >= 0 && row < (long) m_rowToIndex.size() && m_rowToIndex[row] == NETCLASS_ROW )
                      {
                          if( PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( this ) )
                              dialog->ShowPage( _( "Net Classes" ) );

                          return;
                      }

                      aEvent.Skip();
                  } );
    m_list->Bind( wxEVT_LIST_ITEM_DESELECTED,
                  [this]( wxListEvent& )
                  {
                      updateButtons();
                  } );
    m_list->Bind( wxEVT_LIST_KEY_DOWN,
                  [this]( wxListEvent& aEvent )
                  {
                      wxCommandEvent dummy;

                      switch( aEvent.GetKeyCode() )
                      {
                      case WXK_DELETE:
                      case WXK_BACK: onRemove( dummy ); break;
                      default: aEvent.Skip();
                      }
                  } );

    showNewRule();
    RefreshRules();
}


void CONSTRAINT_RULES_LIST::RefreshRules()
{
    wxString blocker;

    if( m_frame->GetBoard()->GetDesignRulesPath().IsEmpty() )
        blocker = _( "Design rules cannot be added without a project" );
    else if( m_document->HasUnparsedText() )
        blocker = _( "The custom rules text does not parse. Fix it on the Custom Rules page first." );

    m_refreshing = true;
    m_list->DeleteAllItems();
    m_rowToIndex.clear();

    // A net class value is read before any rule written here, so it is listed above them
    int row = addNetclassRows();
    int rules = 0;

    if( blocker.IsEmpty() )
    {
        for( int index : m_document->IndicesFor( m_type ) )
        {
            const DRC_RE_LOADED_PANEL_ENTRY& entry = m_document->Entries()[index];

            long item = m_list->InsertItem( row, wxString::Format( wxS( "%d" ), ++rules ) );
            m_list->SetItem( item, 1, entry.ruleName );
            m_list->SetItem( item, 2, entry.condition );
            m_list->SetItem( item, 3, layersText( entry ) );
            m_list->SetItem( item, 4,
                             ConstraintText( entry, m_type, m_frame ) + severityText( entry )
                                     + sharedWithText( index ) );

            m_rowToIndex.push_back( index );
            row++;
        }
    }

    bool anyRows = !m_rowToIndex.empty();
    bool anyRules = rules > 0;

    // The count is of the rules this page writes. A net class value is listed, not written here.
    if( m_titleWindow )
        m_titleWindow->SetLabel( anyRules ? wxString::Format( wxS( "%s (%d)" ), m_title, rules ) : m_title );

    m_emptyLabel->SetLabel( blocker.IsEmpty() ? _( "No rules yet." ) : blocker );

    m_list->Show( anyRows );
    m_emptyLabel->Show( !anyRows );
    m_editorHost->Show( blocker.IsEmpty() );
    m_primaryBtn->Show( blocker.IsEmpty() );
    m_newBtn->Show( blocker.IsEmpty() );
    m_upBtn->Show( anyRules );
    m_downBtn->Show( anyRules );
    m_duplicateBtn->Show( anyRules );
    m_removeBtn->Show( anyRules );

    if( editingIndex() >= 0 )
    {
        // A sibling page may have changed the rule's name, condition or layers since the form was filled
        if( m_editor && !m_editor->IsModified() )
        {
            copyRuleData( *m_editor->GetConstraintData(), *m_editingData );
            m_editor->TransferDataToWindow();
        }

        selectIndex( editingIndex() );
    }
    else if( m_editingData && !m_editingData->IsNew() )
    {
        showNewRule();
    }
    else
    {
        for( long i = 0; i < m_list->GetItemCount(); ++i )
            m_list->SetItemState( i, 0, wxLIST_STATE_SELECTED );
    }

    m_refreshing = false;

    fitColumns();
    updateButtons();
    relayout();
}


void CONSTRAINT_RULES_LIST::fitColumns()
{
    m_fittedWidth = m_list->GetClientSize().x;

    wxClientDC dc( m_list );
    dc.SetFont( m_list->GetFont() );

    for( int col : { 1, 3, 4 } )
    {
        wxListItem header;
        header.SetMask( wxLIST_MASK_TEXT );
        m_list->GetColumn( col, header );

        int width = dc.GetTextExtent( header.GetText() ).x;

        for( long row = 0; row < m_list->GetItemCount(); ++row )
            width = std::max( width, dc.GetTextExtent( m_list->GetItemText( row, col ) ).x );

        m_list->SetColumnWidth( col, std::max( width + 40, 90 ) );
    }

    // Give the name and value columns a share of the width and let the condition take what
    // is left, so the last column is never pushed off the edge.
    int client = m_list->GetClientSize().x - 4;

    m_list->SetColumnWidth( 1, std::min( m_list->GetColumnWidth( 1 ), client / 4 ) );
    m_list->SetColumnWidth( 4, std::min( m_list->GetColumnWidth( 4 ), client * 2 / 5 ) );

    int others = m_list->GetColumnWidth( 0 ) + m_list->GetColumnWidth( 1 ) + m_list->GetColumnWidth( 3 )
                 + m_list->GetColumnWidth( 4 );

    m_list->SetColumnWidth( 2, std::max( client - others, 160 ) );
}


void CONSTRAINT_RULES_LIST::SetNetclasses( std::function<std::optional<int>( const NETCLASS& )> aValue,
                                           NETCLASS_CONSTRAINT_PART                             aPart )
{
    m_netclassValue = std::move( aValue );
    m_netclassPart = aPart;
}


int CONSTRAINT_RULES_LIST::addNetclassRows()
{
    if( !m_netclassValue )
        return 0;

    const NET_SETTINGS& netSettings = *m_frame->Prj().GetProjectFile().NetSettings();
    int                 row = 0;

    // The default class first, then the rest, which is the order DRC builds them in
    std::vector<std::shared_ptr<NETCLASS>> classes = { netSettings.GetDefaultNetclass() };

    for( const auto& [name, netclass] : netSettings.GetNetclasses() )
        classes.push_back( netclass );

    for( const std::shared_ptr<NETCLASS>& netclass : classes )
    {
        if( !netclass )
            continue;

        std::optional<int> value = m_netclassValue( *netclass );

        if( !value )
            continue;

        // The condition DRC gives the rule it builds for the class, so the row reads like a rule
        wxString condition = wxString::Format( wxS( "A.hasExactNetclass('%s')" ), netclass->GetName() );

        // A class raises the minimum for some constraints and asks for a value in others, and the
        // row has to say which, since only the first displaces the board default. The clause is
        // written the way the rule rows below it write theirs.
        wxString constraint = wxString::Format( m_netclassPart == NETCLASS_CONSTRAINT_PART::MINIMUM ? wxS( "(min %s)" )
                                                                                                    : wxS( "(opt %s)" ),
                                                m_frame->StringFromValue( *value, true ) );

        long item = m_list->InsertItem( row, wxEmptyString );
        m_list->SetItem( item, 1, wxString::Format( _( "Net class '%s'" ), netclass->GetName() ) );
        m_list->SetItem( item, 2, condition );
        m_list->SetItem( item, 3, _( "Any" ) );
        m_list->SetItem( item, 4, constraint );

        // The row is not a rule of this document and cannot be edited here, so it is muted
        m_list->SetItemTextColour( item, wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

        m_rowToIndex.push_back( NETCLASS_ROW );
        row++;
    }

    return row;
}


wxString CONSTRAINT_RULES_LIST::layersText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry )
{
    return aEntry.layerSource.IsEmpty() ? _( "Any" ) : aEntry.layerSource;
}


wxString CONSTRAINT_RULES_LIST::inUserUnits( const wxString& aClause, UNITS_PROVIDER* aUnits )
{
    // Rule text carries the units it was typed with. Show every number the way the rest of the
    // page shows one, so a column read down does not mix "0.25mm" with "0.13 mm".
    static wxRegEx units( wxS( "([0-9]+(\\.[0-9]+)?)(mm|um|mil|in)\\b" ), wxRE_ADVANCED );

    wxString result;
    wxString rest = aClause;

    while( units.Matches( rest ) )
    {
        size_t start, length;
        units.GetMatch( &start, &length );

        double value =
                EDA_UNIT_UTILS::UI::DoubleValueFromString( pcbIUScale, EDA_UNITS::MM, units.GetMatch( rest, 0 ) );

        result << rest.Left( start ) << aUnits->StringFromValue( value, true );
        rest = rest.Mid( start + length );
    }

    return result + rest;
}


wxString CONSTRAINT_RULES_LIST::ConstraintText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry,
                                                DRC_RULE_EDITOR_CONSTRAINT_NAME aType, UNITS_PROVIDER* aUnits )
{
    if( aEntry.panelType == CUSTOM_RULE )
    {
        auto     custom = std::dynamic_pointer_cast<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( aEntry.constraintData );
        wxString text = custom ? custom->GetRuleText() : aEntry.originalRuleText;
        wxString code = DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( aType );
        size_t   start = DRC_RULE_DOCUMENT::FindConstraintClause( text, code );

        if( start == wxString::npos )
            return _( "Custom rule" );

        size_t body = start + wxString( wxS( "(constraint" ) ).length();

        while( body < text.length() && wxIsspace( text[body] ) )
            body++;

        body += code.length();

        int depth = 0;

        for( size_t i = start; i < text.length(); ++i )
        {
            if( text[i] == '(' )
                depth++;
            else if( text[i] == ')' && --depth == 0 )
            {
                wxString clause = text.Mid( body, i - body );
                clause.Replace( wxS( "\n" ), wxS( " " ) );
                clause.Replace( wxS( "\t" ), wxS( " " ) );
                return inUserUnits( clause.Trim( true ).Trim( false ), aUnits ) + _( "  (text rule)" );
            }
        }

        return _( "Custom rule" );
    }

    RULE_GENERATION_CONTEXT context;
    context.ruleName = aEntry.ruleName;
    context.conditionExpression = aEntry.condition;
    context.constraintCode = DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( aType );

    const wxString        opener = wxS( "(constraint " );
    std::vector<wxString> clauses = aEntry.constraintData->GetConstraintClauses( context );
    wxString              text;

    for( wxString clause : clauses )
    {
        if( clause.StartsWith( opener ) && clause.EndsWith( wxS( ")" ) ) )
        {
            wxString body = clause.Mid( opener.length(), clause.length() - opener.length() - 1 );
            wxString code = body.BeforeFirst( ' ' );
            wxString rest = body.AfterFirst( ' ' );

            clause = clauses.size() > 1 ? code + wxS( " " ) + rest : rest;
        }

        if( !text.IsEmpty() )
            text << wxS( "  " );

        text << clause;
    }

    return inUserUnits( text, aUnits );
}


int CONSTRAINT_RULES_LIST::selectedIndex() const
{
    long row = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED );

    if( row < 0 || row >= (long) m_rowToIndex.size() )
        return -1;

    return m_rowToIndex[row];
}


void CONSTRAINT_RULES_LIST::selectIndex( int aIndex )
{
    // NETCLASS_ROW would match a row that is not a rule of this document
    if( aIndex < 0 )
    {
        updateButtons();
        return;
    }

    for( size_t row = 0; row < m_rowToIndex.size(); ++row )
    {
        if( m_rowToIndex[row] == aIndex )
        {
            m_list->SetItemState( (long) row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
            m_list->EnsureVisible( (long) row );
            break;
        }
    }

    updateButtons();
}


void CONSTRAINT_RULES_LIST::updateButtons()
{
    bool editingExisting = editingIndex() >= 0;
    bool hasSelection = selectedIndex() >= 0;

    wxString label = editingExisting ? _( "Apply" ) : _( "Add Rule" );

    m_primaryBtn->SetToolTip( editingExisting ? _( "Save the form back to the selected rule." )
                                              : _( "Check the form and add it to the list of rules above." ) );

    if( m_primaryBtn->GetLabel() != label )
    {
        m_primaryBtn->SetLabel( label );
        m_primaryBtn->SetBitmap( KiBitmapBundle( editingExisting ? BITMAPS::small_edit : BITMAPS::small_plus ) );
    }

    m_newBtn->Enable( editingExisting );

    m_upBtn->Enable( hasSelection );
    m_downBtn->Enable( hasSelection );
    m_duplicateBtn->Enable( hasSelection );
    m_removeBtn->Enable( hasSelection );
}


void CONSTRAINT_RULES_LIST::onPrimary( wxCommandEvent& aEvent )
{
    if( m_editor )
        m_editor->Save( aEvent );
}


void CONSTRAINT_RULES_LIST::onNewRule( wxCommandEvent& aEvent )
{
    if( !ConfirmPendingRule() )
        return;

    showNewRule();
    RefreshRules();
}


bool CONSTRAINT_RULES_LIST::HasPendingRule() const
{
    return m_editor && m_editor->IsModified();
}


bool CONSTRAINT_RULES_LIST::ApplyPendingRule()
{
    if( !HasPendingRule() )
        return true;

    if( !m_editor->ValidateRule() )
    {
        reportError( m_editor->GetValidationMessage() );
        return false;
    }

    if( editingIndex() >= 0 )
        applyRule();
    else
        addRule();

    return true;
}


bool CONSTRAINT_RULES_LIST::ConfirmPendingRule()
{
    if( !HasPendingRule() )
        return true;

    bool existing = editingIndex() >= 0;

    KICAD_MESSAGE_DIALOG_BASE dlg( this,
                                   existing ? _( "Changes to this rule are still in the form. Only the rules in "
                                                 "the list apply." )
                                            : _( "This rule is still in the form. Only the rules in the "
                                                 "list apply." ),
                                   existing ? _( "Rule Not Applied" ) : _( "Rule Not Added" ),
                                   wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_WARNING | wxCENTRE );

    dlg.SetYesNoCancelLabels( existing ? _( "Apply" ) : _( "Add Rule" ), _( "Discard" ), _( "Cancel" ) );

    int choice = dlg.ShowModal();

    if( choice == wxID_YES )
        return ApplyPendingRule();

    if( choice == wxID_CANCEL )
        return false;

    DiscardPendingRule();

    return true;
}


void CONSTRAINT_RULES_LIST::DiscardPendingRule()
{
    // Taking the values back from the document is what drops anything typed since, and it leaves
    // the page showing what the rule actually holds.
    if( int index = editingIndex(); index >= 0 )
        showRule( index );
    else
        showNewRule();
}


void CONSTRAINT_RULES_LIST::moveSelected( int aDelta )
{
    int index = selectedIndex();

    if( index < 0 )
        return;

    // The move renumbers the entries, so the rule is found again by its data rather than its
    // old index.
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = m_document->Entries()[index].constraintData;

    if( m_document->Move( index, aDelta, m_type ) )
    {
        notifyChanged();
        selectIndex( indexOf( data ) );
    }
}


void CONSTRAINT_RULES_LIST::onMoveUp( wxCommandEvent& aEvent )
{
    moveSelected( -1 );
}


void CONSTRAINT_RULES_LIST::onMoveDown( wxCommandEvent& aEvent )
{
    moveSelected( 1 );
}


void CONSTRAINT_RULES_LIST::onDuplicate( wxCommandEvent& aEvent )
{
    int index = selectedIndex();

    if( index < 0 )
        return;

    // Applying the form can renumber the entries, so the rule is found again by its data
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> source = m_document->Entries()[index].constraintData;

    if( !ConfirmPendingRule() )
        return;

    int copy = m_document->Duplicate( indexOf( source ) );

    if( copy < 0 )
        return;

    showRule( copy );
    notifyChanged();
}


void CONSTRAINT_RULES_LIST::onRemove( wxCommandEvent& aEvent )
{
    int index = selectedIndex();

    if( index < 0 )
        return;

    const DRC_RE_LOADED_PANEL_ENTRY& entry = m_document->Entries()[index];
    wxString                         question;

    if( entry.panelType == CUSTOM_RULE )
    {
        question = _( "Delete rule '%s'? It is a text rule and every constraint it sets goes with it." );
    }
    else if( !sharedWithText( index ).IsEmpty() )
    {
        question = _( "Remove this constraint from rule '%s'? The rule keeps the constraints it shares "
                      "with other pages." );
    }
    else
    {
        question = _( "Delete rule '%s'?" );
    }

    if( !IsOK( this, wxString::Format( question, entry.ruleName ) ) )
        return;

    m_document->Remove( index );
    showNewRule();
    notifyChanged();
}


void CONSTRAINT_RULES_LIST::showOverrideNote()
{
    if( !m_editor )
        return;

    wxString boardDefault;

    // A section with one default is named by the section itself. One with several carries a name
    // per value, or the note would quote one number under both names.
    for( const auto& [name, read] : m_defaults )
    {
        wxString shown = read ? read() : wxString();

        if( shown.IsEmpty() )
            continue;

        wxString value = wxString::Format( wxS( "%s %s" ), name, shown );

        boardDefault = boardDefault.IsEmpty() ? value : wxString::Format( _( "%s and %s" ), boardDefault, value );
    }

    if( boardDefault.IsEmpty() || m_editingType == CUSTOM_RULE )
    {
        m_editor->SetOverrideNote( wxEmptyString );
        return;
    }

    int index = editingIndex();

    // A rule the form is still writing is not in the document, so it displaces nothing yet and
    // there is no scope to describe.
    if( index < 0 || index >= (int) m_document->Entries().size() )
    {
        m_editor->SetOverrideNote( wxEmptyString );
        return;
    }

    // The scope comes from the form rather than from the entry it was loaded from, so the note
    // describes the rule being written rather than the one last applied.
    wxString condition = m_editor->GetCondition();
    wxString layers = m_editor->GetLayerSource();

    wxString note;

    if( condition.IsEmpty() && layers.IsEmpty() )
    {
        note = wxString::Format( _( "Overrides the board default, %s, everywhere" ), boardDefault );
    }
    else if( condition.IsEmpty() )
    {
        note = wxString::Format( _( "Overrides the board default, %s, on %s" ), boardDefault, layers );
    }
    else if( layers.IsEmpty() )
    {
        note = wxString::Format( _( "Overrides the board default, %s, where %s" ), boardDefault, condition );
    }
    else
    {
        note = wxString::Format( _( "Overrides the board default, %s, on %s where %s" ), boardDefault, layers,
                                 condition );
    }

    m_editor->SetOverrideNote( note );
}


void CONSTRAINT_RULES_LIST::copyRuleData( DRC_RE_BASE_CONSTRAINT_DATA& aTarget,
                                          const DRC_RE_BASE_CONSTRAINT_DATA& aSource )
{
    // CopyFrom leaves the name alone so a duplicate can keep its own
    aTarget.CopyFrom( aSource );
    aTarget.SetRuleName( aSource.GetRuleName() );
}


void CONSTRAINT_RULES_LIST::buildEditor( const std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>& aData,
                                         DRC_RULE_EDITOR_CONSTRAINT_NAME                     aPanelType )
{
    if( m_editor )
    {
        m_editorSizer->Detach( m_editor );
        m_editor->Destroy();
    }

    m_editingData = aData;
    m_editingType = aPanelType;

    // The form edits a copy, so a rejected or discarded value never reaches the document
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> draft =
            DRC_RULE_EDITOR_UTILS::CreateConstraintData( aPanelType, *aData );
    copyRuleData( *draft, *aData );

    m_editor = new PANEL_DRC_RULE_EDITOR( m_editorHost, m_frame->GetBoard(), aPanelType, draft );
    m_editorSizer->Insert( 0, m_editor, 0, wxEXPAND | wxRIGHT, 8 );
    m_editor->TransferDataToWindow();

    showOverrideNote();

    // The editor calls back from its own event handlers, so never destroy it inside one.
    m_editor->SetFieldChangeCallback(
            [this]()
            {
                showOverrideNote();
            } );
    m_editor->SetSaveCallback(
            [this]( int )
            {
                CallAfter(
                        [this]()
                        {
                            if( editingIndex() >= 0 )
                                applyRule();
                            else
                                addRule();
                        } );
            } );
    m_editor->SetRuleNameValidationCallback(
            [this]( int, const wxString& aName )
            {
                return m_document->IsRuleNameUnique( aName, m_type, editingIndex() );
            } );
    m_editor->SetShowMatchesCallBack(
            [this]( int )
            {
                return showMatches();
            } );

    relayout();
}


void CONSTRAINT_RULES_LIST::showNewRule()
{
    DRC_RE_BASE_CONSTRAINT_DATA                  base( m_document->NextId(), m_document->UniqueRuleName( m_title ) );
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( m_type, base );

    data->SetConstraintCode( DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( m_type ) );
    data->SetIsNew( true );

    buildEditor( data, m_type );

    if( long selected = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ); selected != wxNOT_FOUND )
        m_list->SetItemState( selected, 0, wxLIST_STATE_SELECTED );

    updateButtons();
}


bool CONSTRAINT_RULES_LIST::EditRule( int aIndex )
{
    if( aIndex < 0 || aIndex >= (int) m_document->Entries().size() )
        return false;

    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> target = m_document->Entries()[aIndex].constraintData;

    if( target != m_editingData && !ConfirmPendingRule() )
        return false;

    // Opened before it is selected, so the selection event finds it already in the editor
    showRule( indexOf( target ) );
    selectIndex( indexOf( target ) );
    return true;
}


void CONSTRAINT_RULES_LIST::showRule( int aIndex )
{
    if( aIndex < 0 || aIndex >= (int) m_document->Entries().size() )
        return;

    const DRC_RE_LOADED_PANEL_ENTRY& entry = m_document->Entries()[aIndex];

    buildEditor( entry.constraintData, entry.panelType );
    updateButtons();
}


void CONSTRAINT_RULES_LIST::addRule()
{
    if( !m_editor->GetIsValidationSucceeded() )
    {
        reportError( m_editor->GetValidationMessage() );
        return;
    }

    m_editor->TransferDataFromWindow();

    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = m_editingData;

    copyRuleData( *data, *m_editor->GetConstraintData() );
    data->SetIsNew( false );

    DRC_RE_LOADED_PANEL_ENTRY entry( m_type, data, data->GetRuleName(), data->GetRuleCondition(), data->GetSeverity(),
                                     LSET( data->GetLayers() ) );

    entry.layerSource = data->GetLayerSource();
    entry.wasEdited = true;
    m_document->Add( entry );
    notifyChanged();
    m_editor->ClearModified();
}


void CONSTRAINT_RULES_LIST::applyRule()
{
    int index = editingIndex();

    if( index < 0 )
        return;

    if( !m_editor->GetIsValidationSucceeded() )
    {
        reportError( m_editor->GetValidationMessage() );
        return;
    }

    m_editor->TransferDataFromWindow();

    DRC_RE_LOADED_PANEL_ENTRY&                   entry = m_document->Entries()[index];
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = entry.constraintData;

    // A rule applied with nothing changed stays as it was read.
    if( !m_editor->IsModified() )
        return;

    copyRuleData( *data, *m_editor->GetConstraintData() );

    DRC_RE_LOADED_PANEL_ENTRY edited = entry;

    edited.ruleName = data->GetRuleName();
    edited.condition = data->GetRuleCondition();
    edited.layerCondition = LSET( data->GetLayers() );
    edited.layerSource = data->GetLayerSource();
    edited.severity = data->GetSeverity();

    m_document->UpdateEntry( index, edited );
    notifyChanged();
    m_editor->ClearModified();
}


int CONSTRAINT_RULES_LIST::showMatches()
{
    if( !m_editor )
        return -1;

    // The form edits a copy of the rule, so taking the typed values does not touch the document
    m_editor->TransferDataFromWindow();

    wxString ruleText = m_editor->GetConstraintData()->GetGeneratedRule();

    if( ruleText.IsEmpty() )
    {
        m_frame->FocusOnItems( {} );
        return 0;
    }

    std::shared_ptr<DRC_RULE> rule;

    try
    {
        std::vector<std::shared_ptr<DRC_RULE>> rules;
        DRC_RULES_PARSER                       parser( wxS( "(version 2)\n" ) + ruleText, wxS( "ShowMatches" ) );
        parser.Parse( rules, nullptr );

        if( rules.empty() )
        {
            m_frame->FocusOnItems( {} );
            return 0;
        }

        rule = rules[0];

        if( rule->m_Condition && !rule->m_Condition->GetExpression().IsEmpty()
            && !rule->m_Condition->Compile( nullptr ) )
        {
            return -1;
        }
    }
    catch( PARSE_ERROR& )
    {
        return -1;
    }

    DRC_TOOL*                drcTool = m_frame->GetToolManager()->GetTool<DRC_TOOL>();
    std::vector<BOARD_ITEM*> matches;

    for( BOARD_ITEM* item : drcTool->GetDRCEngine()->GetItemsMatchingRule( rule, nullptr ) )
    {
        switch( item->Type() )
        {
        case PCB_NETINFO_T:
        case PCB_GENERATOR_T:
        case PCB_GROUP_T: continue;

        default: matches.push_back( item );
        }
    }

    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    if( !matches.empty() )
    {
        std::vector<EDA_ITEM*> selectItems( matches.begin(), matches.end() );

        m_frame->GetToolManager()->RunAction( ACTIONS::selectItems, &selectItems );
        m_frame->GetToolManager()->RunAction( ACTIONS::zoomFitSelection );
    }

    m_frame->FocusOnItems( matches );

    return (int) matches.size();
}


void CONSTRAINT_RULES_LIST::relayout()
{
    Layout();
    PANEL_DRC_RULE_EDITOR::LayoutAncestors( this );
}


void CONSTRAINT_RULES_LIST::reportError( const wxString& aMessage )
{
    if( PAGED_DIALOG* dialog = PAGED_DIALOG::GetDialog( this ) )
        dialog->SetError( aMessage, this, nullptr );
    else
        DisplayErrorMessage( this, aMessage );
}


int CONSTRAINT_RULES_LIST::indexOf( const std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA>& aData ) const
{
    if( !aData )
        return -1;

    const std::vector<DRC_RE_LOADED_PANEL_ENTRY>& entries = m_document->Entries();

    for( size_t i = 0; i < entries.size(); ++i )
    {
        if( entries[i].constraintData == aData )
            return (int) i;
    }

    return -1;
}


int CONSTRAINT_RULES_LIST::editingIndex() const
{
    return indexOf( m_editingData );
}


void CONSTRAINT_RULES_LIST::notifyChanged()
{
    if( m_onChanged )
        m_onChanged();
    else
        RefreshRules();
}


wxString CONSTRAINT_RULES_LIST::severityText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry )
{
    switch( aEntry.severity )
    {
    case RPT_SEVERITY_IGNORE: return _( ", not checked" );
    case RPT_SEVERITY_WARNING: return _( ", warning only" );
    case RPT_SEVERITY_EXCLUSION: return _( ", excluded" );
    default: return wxEmptyString;
    }
}


wxString CONSTRAINT_RULES_LIST::sharedWithText( int aIndex ) const
{
    const std::vector<DRC_RE_LOADED_PANEL_ENTRY>& entries = m_document->Entries();
    const DRC_RE_LOADED_PANEL_ENTRY&              entry = entries[aIndex];
    wxArrayString                                 codes;
    wxRegEx                                       code( wxS( "\\(constraint ([a-z_]+)" ) );

    for( size_t i = 0; i < entries.size(); ++i )
    {
        const DRC_RE_LOADED_PANEL_ENTRY& other = entries[i];

        if( (int) i == aIndex || other.ruleGroupId <= 0 || other.ruleGroupId != entry.ruleGroupId
            || !other.constraintData )
        {
            continue;
        }

        RULE_GENERATION_CONTEXT ctx;
        ctx.ruleName = other.ruleName;
        ctx.conditionExpression = other.condition;
        ctx.constraintCode = other.constraintData->GetConstraintCode();

        wxString text = other.constraintData->GenerateRule( ctx );

        for( const wxString& clause : other.constraintData->GetConstraintClauses( ctx ) )
            text << wxS( "\n" ) << clause;

        while( code.Matches( text ) )
        {
            size_t start, length;
            code.GetMatch( &start, &length );

            if( codes.Index( code.GetMatch( text, 1 ) ) == wxNOT_FOUND )
                codes.Add( code.GetMatch( text, 1 ) );

            text = text.Mid( start + length );
        }
    }

    if( codes.empty() )
        return wxEmptyString;

    wxString joined;

    for( const wxString& item : codes )
        joined << ( joined.IsEmpty() ? wxS( "" ) : wxS( ", " ) ) << item;

    return wxString::Format( _( ", shared with %s" ), joined );
}
