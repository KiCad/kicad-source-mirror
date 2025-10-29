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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_actions.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <sch_symbol.h>
#include <sch_group.h>
#include <sch_label.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <schematic.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include "search_handlers.h"


void SCH_SEARCH_HANDLER::ActivateItem( long aItemRow )
{
    std::vector<long> item = { aItemRow };
    SelectItems( item );

    m_frame->GetToolManager()->RunAction( SCH_ACTIONS::properties, true );
}


void SCH_SEARCH_HANDLER::FindAll( const std::function<bool( SCH_ITEM*, SCH_SHEET_PATH* )>& aCollector )
{
    SCH_SCREENS                  screens( m_frame->Schematic().Root() );
    std::vector<SCH_SHEET_PATH*> paths;

    m_hitlist.clear();

    screens.BuildClientSheetPathList();

    for( SCH_SCREEN* screen = screens.GetFirst(); screen; screen = screens.GetNext() )
    {
        for( SCH_SHEET_PATH& sheet : screen->GetClientSheetPaths() )
            paths.push_back( &sheet );
    }

    for( SCH_SHEET_PATH* sheet : paths )
    {
        for( SCH_ITEM* item : sheet->LastScreen()->Items() )
        {
            if( aCollector( item, sheet ) )
                m_hitlist.push_back( { item, sheet } );
        }
    }
}


void SCH_SEARCH_HANDLER::Sort( int aCol, bool aAscending, std::vector<long>* aSelection )
{
    std::vector<SCH_ITEM*> selection;

    for( long i = 0; i < (long) m_hitlist.size(); ++i )
    {
        if( alg::contains( *aSelection, i ) )
            selection.push_back( m_hitlist[i].item );
    }

    int col = std::max( 0, aCol );  // Provide a stable order by sorting on first column if no
                                    // sort column provided.

    std::sort( m_hitlist.begin(), m_hitlist.end(),
            [&]( const SCH_SEARCH_HIT& a, const SCH_SEARCH_HIT& b ) -> bool
            {
                // N.B. To meet the iterator sort conditions, we cannot simply invert the truth
                // to get the opposite sort.  i.e. ~(a<b) != (a>b)
                if( aAscending )
                    return StrNumCmp( getResultCell( a, col ), getResultCell( b, col ), true ) < 0;
                else
                    return StrNumCmp( getResultCell( b, col ), getResultCell( a, col ), true ) < 0;
            } );

    aSelection->clear();

    for( long i = 0; i < (long) m_hitlist.size(); ++i )
    {
        if( alg::contains( selection, m_hitlist[i].item ) )
            aSelection->push_back( i );
    }
}


void SCH_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    EDA_ITEMS                   selectedItems;
    std::vector<SCH_SEARCH_HIT> selectedHits;

    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    for( long row : aItemRows )
    {
        if( row >= 0 && row < (long) m_hitlist.size() )
        {
            selectedHits.emplace_back( m_hitlist[row] );
            selectedItems.emplace_back( m_hitlist[row].item );
        }
    }

    if( selectedHits.empty() )
        return;

    bool allHitsOnSamePage = std::all_of( selectedHits.begin() + 1, selectedHits.end(),
                                          [&]( const SCH_SEARCH_HIT& r )
                                          {
                                              return r.sheetPath == selectedHits.front().sheetPath;
                                          } );

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;

    if( allHitsOnSamePage && !selectedHits.empty() )
    {
        SCH_SHEET_PATH* sheet = selectedHits.front().sheetPath;

        if( m_frame->GetCurrentSheet() != *sheet )
            m_frame->GetToolManager()->RunAction<SCH_SHEET_PATH*>( SCH_ACTIONS::changeSheet, sheet );

        if( selectedItems.size() )
            m_frame->GetToolManager()->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &selectedItems );

        switch( settings.selection_zoom )
        {
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::PAN:
            m_frame->GetToolManager()->RunAction( ACTIONS::centerSelection );
            break;
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::ZOOM:
            m_frame->GetToolManager()->RunAction( ACTIONS::zoomFitSelection );
            break;
        case APP_SETTINGS_BASE::SEARCH_PANE::SELECTION_ZOOM::NONE:
            break;
        }

        m_frame->GetCanvas()->Refresh( false );
    }
}


SYMBOL_SEARCH_HANDLER::SYMBOL_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( _HKI( "Symbols" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Reference" ),            2, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Value" ),                6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Footprint" ),           10, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Page" ),                 1, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back(  wxT( "X" ),                    3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back(  wxT( "Y" ),                    3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Excl. Sim" ),            2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Excl. BOM" ),            2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Excl. Board" ),          2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "DNP" ),                  2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( _HKI( "Library Link" ),         8, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Library Description" ), 10, wxLIST_FORMAT_LEFT );
}


int SYMBOL_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    SCH_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [&frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
            {
                if( item && item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

                    // IsPower depends on non-missing lib symbol association
                    if( !sym->IsMissingLibSymbol() && sym->IsPower() )
                        return false;

                    for( SCH_FIELD& field : sym->GetFields() )
                    {
                        if( frp.findString.IsEmpty() || field.Matches( frp, sheet ) )
                            return true;
                    }
                }

                return false;
            };

    FindAll( search );

    return (int) m_hitlist.size();
}


wxString SYMBOL_SEARCH_HANDLER::getResultCell( const SCH_SEARCH_HIT& aHit, int aCol )
{
    SCH_SYMBOL*sym = dynamic_cast<SCH_SYMBOL*>( aHit.item );

    if( !sym )
        return wxEmptyString;

    if( aCol == 0 )
        return sym->GetRef( aHit.sheetPath, true );
    else if( aCol == 1 )
        return sym->GetField( FIELD_T::VALUE )->GetShownText( aHit.sheetPath, false );
    else if( aCol == 2 )
        return sym->GetField( FIELD_T::FOOTPRINT )->GetShownText( aHit.sheetPath, false );
    else if( aCol == 3 )
        return aHit.sheetPath->GetPageNumber();
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( sym->GetPosition().x );
    else if( aCol == 5 )
        return m_frame->MessageTextFromValue( sym->GetPosition().y );
    else if( aCol == 6 )
        return sym->ResolveExcludedFromSim() ? wxS( "X" ) : wxS( " " );
    else if( aCol == 7 )
        return sym->ResolveExcludedFromBOM() ? wxS( "X" ) : wxS( " " );
    else if( aCol == 8 )
        return sym->ResolveExcludedFromBoard() ? wxS( "X" ) : wxS( " " );
    else if( aCol == 9 )
        return sym->ResolveDNP() ? wxS( "X" ) : wxS( " " );
    else if( aCol == 10 )
        return sym->GetLibId().Format();
    else if( aCol == 11 )
        return sym->GetShownDescription();

    return wxEmptyString;
}


POWER_SEARCH_HANDLER::POWER_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( _HKI( "Power" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Reference" ),   2,  wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Value" ),       6,  wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Page" ),        1,  wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),            3,  wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),            3,  wxLIST_FORMAT_CENTER );
}


int POWER_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    SCH_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [&frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
            {
                if( item && item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

                    // IsPower depends on non-missing lib symbol association
                    if( sym->IsMissingLibSymbol() || !sym->IsPower() )
                        return false;

                    for( SCH_FIELD& field : sym->GetFields() )
                    {
                        if( frp.findString.IsEmpty() || field.Matches( frp, sheet ) )
                            return true;
                    }
                }

                return false;
            };

    FindAll( search );

    return (int) m_hitlist.size();
}


wxString POWER_SEARCH_HANDLER::getResultCell( const SCH_SEARCH_HIT& aHit, int aCol )
{
    SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( aHit.item );

    if( !sym )
        return wxEmptyString;

    if( aCol == 0 )
        return sym->GetRef( aHit.sheetPath, true );
    else if( aCol == 1 )
        return sym->GetField( FIELD_T::VALUE )->GetShownText( aHit.sheetPath, false );
    else if( aCol == 2 )
        return aHit.sheetPath->GetPageNumber();
    else if( aCol == 3 )
        return m_frame->MessageTextFromValue( sym->GetPosition().x );
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( sym->GetPosition().y );

    return wxEmptyString;
}


TEXT_SEARCH_HANDLER::TEXT_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( _HKI( "Text" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Type" ), 2, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Text" ), 12, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Page" ), 1, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),     3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),     3, wxLIST_FORMAT_CENTER );
}


int TEXT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    SCH_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [&frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
            {
                if( item->Type() == SCH_TEXT_T || item->Type() == SCH_TEXTBOX_T )
                {
                    if( frp.findString.IsEmpty() || item->Matches( frp, sheet ) )
                        return true;
                }

                return false;
            };

    FindAll( search );

    return (int) m_hitlist.size();
}


wxString TEXT_SEARCH_HANDLER::getResultCell( const SCH_SEARCH_HIT& aHit, int aCol )
{
    if( aHit.item->Type() == SCH_TEXT_T )
    {
        SCH_TEXT* txt = dynamic_cast<SCH_TEXT*>( aHit.item );

        if( !txt )
            return wxEmptyString;

        if( aCol == 0 )
            return _( "Text" );
        else if( aCol == 1 )
            return txt->GetShownText( false );
        else if( aCol == 2 )
            return aHit.sheetPath->GetPageNumber();
        else if( aCol == 3 )
            return m_frame->MessageTextFromValue( txt->GetPosition().x );
        else if( aCol == 4 )
            return m_frame->MessageTextFromValue( txt->GetPosition().y );
    }
    else if( aHit.item->Type() == SCH_TEXTBOX_T )
    {
        SCH_TEXTBOX* txt = dynamic_cast<SCH_TEXTBOX*>( aHit.item );

        if( !txt )
            return wxEmptyString;

        if( aCol == 0 )
            return _( "Text Box" );
        else if( aCol == 1 )
            return txt->GetShownText( false );
        else if( aCol == 2 )
            return aHit.sheetPath->GetPageNumber();
        else if( aCol == 3 )
            return m_frame->MessageTextFromValue( txt->GetPosition().x );
        else if( aCol == 4 )
            return m_frame->MessageTextFromValue( txt->GetPosition().y );
    }


    return wxEmptyString;
}


LABEL_SEARCH_HANDLER::LABEL_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( _HKI( "Labels" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Type" ), 2, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Name" ), 6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Page" ), 2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),     3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),     3 , wxLIST_FORMAT_CENTER);
}


int LABEL_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    SCH_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [&frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
            {
                if( item->IsType( { SCH_LABEL_LOCATE_ANY_T } ) )
                {
                    SCH_LABEL_BASE* lbl = dynamic_cast<SCH_LABEL_BASE*>( item );

                    wxCHECK( lbl, false );

                    if( frp.findString.IsEmpty() || lbl->Matches( frp, sheet ) )
                        return true;
                }

                return false;
            };

    FindAll( search );

    return (int) m_hitlist.size();
}


wxString LABEL_SEARCH_HANDLER::getResultCell( const SCH_SEARCH_HIT& aHit, int aCol )
{
    SCH_LABEL_BASE* lbl = dynamic_cast<SCH_LABEL_BASE*>( aHit.item );

    if( !lbl )
        return wxEmptyString;

    if (aCol == 0)
    {
        if(lbl->Type() == SCH_LABEL_T)
            return _HKI( "Local" );
        else if( lbl->Type() == SCH_GLOBAL_LABEL_T )
            return _HKI( "Global" );
        else if( lbl->Type() == SCH_HIER_LABEL_T )
            return _HKI( "Hierarchical" );
        else if( lbl->Type() == SCH_DIRECTIVE_LABEL_T )
            return _HKI( "Directive" );
    }
    else if( aCol == 1 )
        return lbl->GetShownText( false );
    else if( aCol == 2 )
        return aHit.sheetPath->GetPageNumber();
    else if( aCol == 3 )
        return m_frame->MessageTextFromValue( lbl->GetPosition().x );
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( lbl->GetPosition().y );

    return wxEmptyString;
}


GROUP_SEARCH_HANDLER::GROUP_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( _HKI( "Groups" ), aFrame )
{
    m_columns.emplace_back( _HKI( "Name" ), 6, wxLIST_FORMAT_LEFT );
    m_columns.emplace_back( _HKI( "Page" ), 2, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "X" ),     3, wxLIST_FORMAT_CENTER );
    m_columns.emplace_back( wxT( "Y" ),     3, wxLIST_FORMAT_CENTER);
}


int GROUP_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    APP_SETTINGS_BASE::SEARCH_PANE& settings = m_frame->config()->m_SearchPane;
    SCH_SEARCH_DATA                 frp;

    frp.searchAllFields = settings.search_hidden_fields;
    frp.searchMetadata = settings.search_metadata;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [&frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
            {
                if( item->IsType( { SCH_GROUP_T } ) )
                {
                    SCH_GROUP* group = static_cast<SCH_GROUP*>( item );

                    if( frp.findString.IsEmpty() || group->Matches( frp, sheet ) )
                        return true;
                }

                return false;
            };

    FindAll( search );

    return (int) m_hitlist.size();
}


wxString GROUP_SEARCH_HANDLER::getResultCell( const SCH_SEARCH_HIT& aHit, int aCol )
{
    SCH_GROUP* group = dynamic_cast<SCH_GROUP*>( aHit.item );

    if( !group )
        return wxEmptyString;

    if( aCol == 0 )
        return group->GetName();
    else if( aCol == 1 )
        return aHit.sheetPath->GetPageNumber();
    else if( aCol == 2 )
        return m_frame->MessageTextFromValue( group->GetPosition().x );
    else if( aCol == 3 )
        return m_frame->MessageTextFromValue( group->GetPosition().y );

    return wxEmptyString;
}
