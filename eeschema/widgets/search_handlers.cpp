/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <ee_actions.h>
#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <sch_text.h>
#include <sch_textbox.h>
#include <schematic.h>
#include <string_utils.h>
#include <tool/tool_manager.h>
#include "search_handlers.h"


SCH_SEARCH_HANDLER::SCH_SEARCH_HANDLER( wxString aName, SCH_EDIT_FRAME* aFrame ) :
        SEARCH_HANDLER( aName ), m_frame( aFrame )
{
}


void SCH_SEARCH_HANDLER::ActivateItem( long aItemRow )
{
    std::vector<long> item = { aItemRow };
    SelectItems( item );
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
            {
                m_hitlist.push_back( { item, sheet } );
            }
        }
    }
}


void SCH_SEARCH_HANDLER::SelectItems( std::vector<long>& aItemRows )
{
    EDA_ITEMS                   selectedItems;
    std::vector<SCH_SEARCH_HIT> selectedHits;

    m_frame->GetToolManager()->RunAction( EE_ACTIONS::clearSelection );

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

    if( allHitsOnSamePage && !selectedHits.empty() )
    {
        if( m_frame->GetCurrentSheet() != *selectedHits.front().sheetPath )
        {
            m_frame->SetCurrentSheet( *selectedHits.front().sheetPath );
            m_frame->DisplayCurrentSheet();
        }

        if( selectedItems.size() )
            m_frame->GetToolManager()->RunAction<EDA_ITEMS*>( EE_ACTIONS::addItemsToSel, &selectedItems );

        m_frame->GetCanvas()->Refresh( false );
    }
}


SYMBOL_SEARCH_HANDLER::SYMBOL_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( wxT( "Symbols" ), aFrame )
{
    m_columns.emplace_back( wxT( "Reference" ), 1 );
    m_columns.emplace_back( wxT( "Value" ), 3 );
    m_columns.emplace_back( wxT( "Footprint" ), 3 );
    m_columns.emplace_back( wxT( "Page" ), 1 );
    m_columns.emplace_back( wxT( "X" ), 2 );
    m_columns.emplace_back( wxT( "Y" ), 2 );
}


int SYMBOL_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    SCH_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
            {
                if( item->Type() == SCH_SYMBOL_T )
                {
                    SCH_SYMBOL* sym = dynamic_cast<SCH_SYMBOL*>( item );

                    if( sym->IsPower() )
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


wxString SYMBOL_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    SCH_SEARCH_HIT hit = m_hitlist[aRow];
    SCH_SYMBOL*    sym = dynamic_cast<SCH_SYMBOL*>( hit.item );

    if( !sym )
        return wxEmptyString;

    if( aCol == 0 )
        return sym->GetRef( hit.sheetPath, true );
    else if( aCol == 1 )
        return sym->GetField( VALUE_FIELD )->GetShownText( hit.sheetPath, false );
    else if( aCol == 2 )
        return sym->GetField( FOOTPRINT_FIELD )->GetShownText( hit.sheetPath, false );
    else if( aCol == 3 )
        return hit.sheetPath->GetPageNumber();
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( sym->GetPosition().x );
    else if( aCol == 5 )
        return m_frame->MessageTextFromValue( sym->GetPosition().y );


    return wxEmptyString;
}


TEXT_SEARCH_HANDLER::TEXT_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( wxT( "Text" ), aFrame )
{
    m_columns.emplace_back( wxT( "Type" ), 1 );
    m_columns.emplace_back( wxT( "Text" ), 5 );
    m_columns.emplace_back( wxT( "Page" ), 1 );
    m_columns.emplace_back( wxT( "X" ), 2 );
    m_columns.emplace_back( wxT( "Y" ), 2 );
}


int TEXT_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    SCH_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
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


wxString TEXT_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    SCH_SEARCH_HIT hit = m_hitlist[aRow];

    if( hit.item->Type() == SCH_TEXT_T )
    {
        SCH_TEXT* txt = dynamic_cast<SCH_TEXT*>( hit.item );

        if( !txt )
            return wxEmptyString;

        if( aCol == 0 )
            return wxS( "Text" );
        else if( aCol == 1 )
            return txt->GetShownText( false );
        else if( aCol == 2 )
            return hit.sheetPath->GetPageNumber();
        else if( aCol == 3 )
            return m_frame->MessageTextFromValue( txt->GetPosition().x );
        else if( aCol == 4 )
            return m_frame->MessageTextFromValue( txt->GetPosition().y );
    }
    else if( hit.item->Type() == SCH_TEXTBOX_T )
    {
        SCH_TEXTBOX* txt = dynamic_cast<SCH_TEXTBOX*>( hit.item );

        if( !txt )
            return wxEmptyString;

        if( aCol == 0 )
            return wxS( "Text" );
        else if( aCol == 1 )
            return txt->GetShownText( false );
        else if( aCol == 2 )
            return hit.sheetPath->GetPageNumber();
        else if( aCol == 3 )
            return m_frame->MessageTextFromValue( txt->GetPosition().x );
        else if( aCol == 4 )
            return m_frame->MessageTextFromValue( txt->GetPosition().y );
    }


    return wxEmptyString;
}


LABEL_SEARCH_HANDLER::LABEL_SEARCH_HANDLER( SCH_EDIT_FRAME* aFrame ) :
        SCH_SEARCH_HANDLER( wxT( "Labels" ), aFrame )
{
    m_columns.emplace_back( wxT( "Type" ), 1 );
    m_columns.emplace_back( wxT( "Name" ), 4 );
    m_columns.emplace_back( wxT( "Page" ), 1 );
    m_columns.emplace_back( wxT( "X" ), 2 );
    m_columns.emplace_back( wxT( "Y" ), 2 );
}


int LABEL_SEARCH_HANDLER::Search( const wxString& aQuery )
{
    m_hitlist.clear();

    SCH_SEARCH_DATA frp;
    frp.findString = aQuery;

    // Try to handle whatever the user throws at us (substring, wildcards, regex, etc.)
    frp.matchMode = EDA_SEARCH_MATCH_MODE::PERMISSIVE;
    frp.searchCurrentSheetOnly = false;

    auto search =
            [frp]( SCH_ITEM* item, SCH_SHEET_PATH* sheet )
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


wxString LABEL_SEARCH_HANDLER::GetResultCell( int aRow, int aCol )
{
    SCH_SEARCH_HIT hit = m_hitlist[aRow];

    SCH_LABEL_BASE* lbl = dynamic_cast<SCH_LABEL_BASE*>( hit.item );

    if( !lbl )
        return wxEmptyString;

    if (aCol == 0)
    {
        if(lbl->Type() == SCH_LABEL_T)
            return wxS( "Local" );
        else if( lbl->Type() == SCH_GLOBAL_LABEL_T )
            return wxS( "Global" );
        else if( lbl->Type() == SCH_HIER_LABEL_T )
            return wxS( "Hierarchal" );
    }
    else if( aCol == 1 )
        return lbl->GetShownText( false );
    else if( aCol == 2 )
        return hit.sheetPath->GetPageNumber();
    else if( aCol == 3 )
        return m_frame->MessageTextFromValue( lbl->GetPosition().x );
    else if( aCol == 4 )
        return m_frame->MessageTextFromValue( lbl->GetPosition().y );

    return wxEmptyString;
}
