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
#include <pcb_edit_frame.h>
#include <drc/drc_engine.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_condition.h>
#include <drc/drc_rule_parser.h>
#include <drc/rule_editor/drc_re_loaded_rule.h>
#include <base_units.h>
#include <drc/rule_editor/drc_re_custom_rule_constraint_data.h>
#include <drc/rule_editor/drc_re_numeric_input_constraint_data.h>
#include <drc/rule_editor/drc_rule_document.h>
#include <drc/rule_editor/drc_rule_editor_utils.h>
#include <drc/rule_editor/panel_drc_rule_editor.h>
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
#include <wx/listctrl.h>
#include <wx/scrolwin.h>
#include <wx/sizer.h>
#include <wx/toplevel.h>
#include <algorithm>


CONSTRAINT_RULES_LIST::CONSTRAINT_RULES_LIST( wxWindow* aParent, PCB_EDIT_FRAME* aFrame, DRC_RULE_DOCUMENT* aDocument,
                                              DRC_RULE_EDITOR_CONSTRAINT_NAME aType, const wxString& aTitle ) :
        wxPanel( aParent, wxID_ANY ),
        m_frame( aFrame ),
        m_document( aDocument ),
        m_type( aType ),
        m_title( aTitle ),
        m_titleWindow( nullptr ),
        m_editor( nullptr )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    m_list = new wxListCtrl( this, wxID_ANY, wxDefaultPosition, wxSize( -1, 140 ), wxLC_REPORT | wxLC_SINGLE_SEL );
    m_list->AppendColumn( wxS( "#" ), wxLIST_FORMAT_RIGHT, 36 );
    m_list->AppendColumn( _( "Name" ), wxLIST_FORMAT_LEFT, 160 );
    m_list->AppendColumn( _( "Condition" ), wxLIST_FORMAT_LEFT, 280 );
    m_list->AppendColumn( _( "Layers" ), wxLIST_FORMAT_LEFT, 120 );
    m_list->AppendColumn( _( "Constraint" ), wxLIST_FORMAT_LEFT, 240 );
    mainSizer->Add( m_list, 1, wxEXPAND | wxBOTTOM, 4 );

    m_emptyLabel = new wxStaticText( this, wxID_ANY, _( "No rules yet." ) );
    m_emptyLabel->SetFont( KIUI::GetInfoFont( this ).Italic() );
    mainSizer->Add( m_emptyLabel, 0, wxLEFT | wxBOTTOM, 4 );

    wxBoxSizer* tableButtons = new wxBoxSizer( wxHORIZONTAL );
    m_upBtn = new wxButton( this, wxID_ANY, _( "Move Up" ) );
    m_downBtn = new wxButton( this, wxID_ANY, _( "Move Down" ) );
    m_duplicateBtn = new wxButton( this, wxID_ANY, _( "Duplicate" ) );
    m_removeBtn = new wxButton( this, wxID_ANY, _( "Remove" ) );

    m_upBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_downBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );
    m_duplicateBtn->SetBitmap( KiBitmapBundle( BITMAPS::duplicate ) );
    m_removeBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_upBtn->SetToolTip( _( "Move the selected rule one place earlier. Rules are read from top to bottom "
                            "and the last one that matches decides the value, so a rule lower down wins." ) );
    m_downBtn->SetToolTip( _( "Move the selected rule one place later. Rules are read from top to bottom "
                              "and the last one that matches decides the value, so a rule lower down wins." ) );
    m_duplicateBtn->SetToolTip( _( "Copy the selected rule and place the copy after it." ) );
    m_removeBtn->SetToolTip( _( "Delete the selected rule." ) );
    m_list->SetToolTip( _( "The rules for this constraint, in the order they are read. Select one to load "
                           "it into the form below." ) );
    tableButtons->Add( m_upBtn, 0, wxRIGHT, 4 );
    tableButtons->Add( m_downBtn, 0, wxRIGHT, 4 );
    tableButtons->Add( m_duplicateBtn, 0, wxRIGHT, 4 );
    tableButtons->Add( m_removeBtn, 0, wxRIGHT, 4 );
    mainSizer->Add( tableButtons, 0, wxBOTTOM, 4 );

    m_editorHost = new wxPanel( this, wxID_ANY );
    m_editorSizer = new wxBoxSizer( wxVERTICAL );
    m_editorHost->SetSizer( m_editorSizer );
    mainSizer->Add( m_editorHost, 0, wxEXPAND | wxTOP, 8 );

    wxBoxSizer* formButtons = new wxBoxSizer( wxHORIZONTAL );
    m_primaryBtn = new wxButton( this, wxID_ANY, _( "Add Rule" ) );
    m_newBtn = new wxButton( this, wxID_ANY, _( "New Rule" ) );

    m_primaryBtn->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_primaryBtn->SetMinSize( wxSize( m_primaryBtn->GetBestSize().x + FromDIP( 16 ), -1 ) );
    m_newBtn->SetToolTip( _( "Empty the form so another rule can be written. Anything typed and not "
                             "added is discarded." ) );
    formButtons->Add( m_primaryBtn, 0, wxRIGHT, 4 );
    formButtons->Add( m_newBtn, 0, wxRIGHT, 4 );
    mainSizer->Add( formButtons, 0, wxTOP | wxBOTTOM, 6 );

    SetSizer( mainSizer );

    m_primaryBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onPrimary, this );
    m_newBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onNewRule, this );
    m_upBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onMoveUp, this );
    m_downBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onMoveDown, this );
    m_duplicateBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onDuplicate, this );
    m_removeBtn->Bind( wxEVT_BUTTON, &CONSTRAINT_RULES_LIST::onRemove, this );

    m_list->Bind( wxEVT_SIZE,
                  [this]( wxSizeEvent& aEvent )
                  {
                      fitColumns();
                      aEvent.Skip();
                  } );
    m_list->Bind( wxEVT_LIST_ITEM_SELECTED,
                  [this]( wxListEvent& )
                  {
                      if( !m_refreshing && selectedIndex() >= 0 && selectedIndex() != editingIndex() )
                          showRule( selectedIndex() );

                      updateButtons();
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

    int row = 0;

    if( blocker.IsEmpty() )
    {
        for( int index : m_document->IndicesFor( m_type ) )
        {
            const DRC_RE_LOADED_PANEL_ENTRY& entry = m_document->Entries()[index];

            long item = m_list->InsertItem( row, wxString::Format( wxS( "%d" ), row + 1 ) );
            m_list->SetItem( item, 1, entry.ruleName );
            m_list->SetItem( item, 2, entry.condition );
            m_list->SetItem( item, 3, layersText( entry ) );
            m_list->SetItem( item, 4, constraintText( entry ) + severityText( entry ) + sharedWithText( index ) );

            m_rowToIndex.push_back( index );
            row++;
        }
    }

    bool any = !m_rowToIndex.empty();

    if( m_titleWindow )
    {
        m_titleWindow->SetLabel( any ? wxString::Format( wxS( "%s (%d)" ), m_title, (int) m_rowToIndex.size() )
                                     : m_title );
    }

    m_emptyLabel->SetLabel( blocker.IsEmpty() ? _( "No rules yet." ) : blocker );

    m_list->Show( any );
    m_emptyLabel->Show( !any );
    m_editorHost->Show( blocker.IsEmpty() );
    m_primaryBtn->Show( blocker.IsEmpty() );
    m_newBtn->Show( blocker.IsEmpty() );
    m_upBtn->Show( any );
    m_downBtn->Show( any );
    m_removeBtn->Show( any );

    if( editingIndex() >= 0 )
    {
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

    // Long names or values must not push the last column off the edge, so they get a share of
    // the width and the condition takes what is left.
    int client = m_list->GetClientSize().x - 4;

    m_list->SetColumnWidth( 1, std::min( m_list->GetColumnWidth( 1 ), client / 4 ) );
    m_list->SetColumnWidth( 4, std::min( m_list->GetColumnWidth( 4 ), client * 2 / 5 ) );

    int others = m_list->GetColumnWidth( 0 ) + m_list->GetColumnWidth( 1 ) + m_list->GetColumnWidth( 3 )
                 + m_list->GetColumnWidth( 4 );

    m_list->SetColumnWidth( 2, std::max( client - others, 160 ) );
}


wxString CONSTRAINT_RULES_LIST::layersText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry )
{
    return aEntry.layerSource.IsEmpty() ? _( "Any" ) : aEntry.layerSource;
}


wxString CONSTRAINT_RULES_LIST::inUserUnits( const wxString& aClause ) const
{
    if( m_frame->GetUserUnits() == EDA_UNITS::MM )
        return aClause;

    // Rule text carries its own units. Show the numbers the way the rest of the page does.
    static wxRegEx units( wxS( "([0-9]+(\\.[0-9]+)?)(mm|um|mil|in)\\b" ), wxRE_ADVANCED );

    wxString result;
    wxString rest = aClause;

    while( units.Matches( rest ) )
    {
        size_t start, length;
        units.GetMatch( &start, &length );

        double   value = 0.0;
        wxString unit = units.GetMatch( rest, 3 );
        units.GetMatch( rest, 1 ).ToCDouble( &value );

        if( unit == wxS( "um" ) )
            value /= 1000.0;
        else if( unit == wxS( "mil" ) )
            value *= 0.0254;
        else if( unit == wxS( "in" ) )
            value *= 25.4;

        result << rest.Left( start ) << m_frame->StringFromValue( pcbIUScale.mmToIU( value ), true );
        rest = rest.Mid( start + length );
    }

    return result + rest;
}


wxString CONSTRAINT_RULES_LIST::constraintText( const DRC_RE_LOADED_PANEL_ENTRY& aEntry ) const
{
    DRC_RULE_EDITOR_CONSTRAINT_NAME type = m_type;

    if( aEntry.panelType == CUSTOM_RULE )
    {
        auto     custom = std::dynamic_pointer_cast<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA>( aEntry.constraintData );
        wxString text = custom ? custom->GetRuleText() : aEntry.originalRuleText;
        wxString code = DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( type );
        wxString prefix = wxString::Format( wxS( "(constraint %s" ), code );
        size_t   start = DRC_RULE_DOCUMENT::FindConstraintClause( text, code );

        if( start == wxString::npos )
            return _( "Custom rule" );

        int depth = 0;

        for( size_t i = start; i < text.length(); ++i )
        {
            if( text[i] == '(' )
                depth++;
            else if( text[i] == ')' && --depth == 0 )
            {
                wxString clause = text.Mid( start + prefix.length(), i - start - prefix.length() );
                clause.Replace( wxS( "\n" ), wxS( " " ) );
                clause.Replace( wxS( "\t" ), wxS( " " ) );
                return inUserUnits( clause.Trim( true ).Trim( false ) ) + _( "  (text rule)" );
            }
        }

        return _( "Custom rule" );
    }

    RULE_GENERATION_CONTEXT context;
    context.ruleName = aEntry.ruleName;
    context.conditionExpression = aEntry.condition;
    context.constraintCode = DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( type );

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

    return inUserUnits( text );
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
    showNewRule();
    RefreshRules();
}


void CONSTRAINT_RULES_LIST::onMoveUp( wxCommandEvent& aEvent )
{
    int index = selectedIndex();

    if( index < 0 )
        return;

    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = m_document->Entries()[index].constraintData;

    if( m_document->Move( index, -1, m_type ) )
    {
        notifyChanged();

        for( int candidate : m_rowToIndex )
        {
            if( m_document->Entries()[candidate].constraintData == data )
                selectIndex( candidate );
        }
    }
}


void CONSTRAINT_RULES_LIST::onMoveDown( wxCommandEvent& aEvent )
{
    int index = selectedIndex();

    if( index < 0 )
        return;

    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = m_document->Entries()[index].constraintData;

    if( m_document->Move( index, 1, m_type ) )
    {
        notifyChanged();

        for( int candidate : m_rowToIndex )
        {
            if( m_document->Entries()[candidate].constraintData == data )
                selectIndex( candidate );
        }
    }
}


void CONSTRAINT_RULES_LIST::onDuplicate( wxCommandEvent& aEvent )
{
    int index = selectedIndex();

    if( index < 0 )
        return;

    int copy = m_document->Duplicate( index );

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

    m_editor = new PANEL_DRC_RULE_EDITOR( m_editorHost, m_frame->GetBoard(), aPanelType, aData );
    m_editorSizer->Insert( 0, m_editor, 0, wxEXPAND | wxRIGHT, 8 );
    m_editor->TransferDataToWindow();

    // The editor calls back from inside its own event handlers, so it is never destroyed
    // synchronously from a callback.
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
    DRC_RE_BASE_CONSTRAINT_DATA                  base( m_document->NextId(), 0, m_document->UniqueRuleName( m_title ) );
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = DRC_RULE_EDITOR_UTILS::CreateConstraintData( m_type, base );

    data->SetConstraintCode( DRC_RULE_EDITOR_UTILS::ConstraintToKicadDrc( m_type ) );
    data->SetIsNew( true );

    buildEditor( data, m_type );

    if( long selected = m_list->GetNextItem( -1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED ); selected != wxNOT_FOUND )
        m_list->SetItemState( selected, 0, wxLIST_STATE_SELECTED );

    updateButtons();
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

    data->SetIsNew( false );

    DRC_RE_LOADED_PANEL_ENTRY entry( m_type, data, data->GetRuleName(), data->GetRuleCondition(), data->GetSeverity(),
                                     LSET( data->GetLayers() ) );

    entry.layerSource = data->GetLayerSource();
    entry.wasEdited = true;
    m_document->Add( entry );
    notifyChanged();
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
    DRC_RE_LOADED_PANEL_ENTRY                    edited = entry;

    edited.ruleName = data->GetRuleName();
    edited.condition = data->GetRuleCondition();
    edited.layerCondition = LSET( data->GetLayers() );
    edited.layerSource = data->GetLayerSource();
    edited.severity = data->GetSeverity();

    // A rule applied with nothing changed stays as it was read.
    if( m_editor->IsModified() )
    {
        m_document->UpdateEntry( index, edited );
        notifyChanged();
    }
}


int CONSTRAINT_RULES_LIST::showMatches()
{
    if( !m_editor )
        return -1;

    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> data = m_editor->GetConstraintData();
    std::shared_ptr<DRC_RE_BASE_CONSTRAINT_DATA> saved =
            DRC_RULE_EDITOR_UTILS::CreateConstraintData( m_editingType, *data );

    saved->CopyFrom( *data );
    m_editor->TransferDataFromWindow();

    wxString ruleText = data->GetGeneratedRule();

    data->CopyFrom( *saved );

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

    for( wxWindow* window = GetParent(); window; window = window->GetParent() )
    {
        window->Layout();

        if( wxScrolledWindow* scrolled = dynamic_cast<wxScrolledWindow*>( window ) )
        {
            scrolled->FitInside();

            // Only the height scrolls. Holding the virtual width to the client width keeps the
            // editor's own buttons inside the visible area instead of under the scrollbar.
            wxSize virtualSize = scrolled->GetVirtualSize();
            virtualSize.x = scrolled->GetClientSize().x;
            scrolled->SetVirtualSize( virtualSize );
            scrolled->Layout();
            break;
        }

        if( dynamic_cast<wxTopLevelWindow*>( window ) )
            break;
    }
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
