/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "properties_panel.h"
#include <tool/selection.h>
#include <eda_base_frame.h>
#include <eda_item.h>

#include <algorithm>
#include <set>

using namespace std;

extern wxPGGlobalVarsClass* wxPGGlobalVars;

PROPERTIES_PANEL::PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame )
    : wxPanel( aParent ), m_frame( aFrame )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // on some platforms wxPGGlobalVars is initialized automatically,
    // but others need an explicit init
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();

    m_grid = new wxPropertyGrid( this, wxID_ANY, wxDefaultPosition, wxSize( 300, 400 ),
            wxPG_AUTO_SORT | wxPG_SPLITTER_AUTO_CENTER | wxPG_DEFAULT_STYLE );
    m_grid->SetUnspecifiedValueAppearance( wxPGCell( "<...>" ) );
    mainSizer->Add( m_grid, 1, wxALL | wxEXPAND, 5 );

    SetSizer( mainSizer );
    Layout();

    Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( PROPERTIES_PANEL::valueChanged ), NULL, this );
    Connect( wxEVT_PG_CHANGING, wxPropertyGridEventHandler( PROPERTIES_PANEL::valueChanging ), NULL, this );
    Connect( wxEVT_SHOW, wxShowEventHandler( PROPERTIES_PANEL::onShow ), NULL, this );
}


void PROPERTIES_PANEL::update( const SELECTION& aSelection )
{
    m_grid->Clear();
    m_displayed.clear();

    if( aSelection.Empty() )
        return;

    // Get all the selected types
    set<TYPE_ID> types;

    for( EDA_ITEM* item : aSelection )
        types.insert( TYPE_HASH( *item ) );

    wxCHECK( !types.empty(), /* void */ );
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.SetUnits( m_frame->GetUserUnits() );

    set<PROPERTY_BASE*> commonProps;
    const PROPERTY_LIST& allProperties = propMgr.GetProperties( *types.begin() );
    copy( allProperties.begin(), allProperties.end(), inserter( commonProps, commonProps.begin() ) );

    // Get all possible properties
    for( const auto& type : types )
    {
        const PROPERTY_LIST& itemProps = propMgr.GetProperties( type );

        for( auto it = commonProps.begin(); it != commonProps.end(); /* ++it in the loop */ )
        {
            if( !binary_search( itemProps.begin(), itemProps.end(), *it ) )
                it = commonProps.erase( it );
            else
                ++it;
        }
    }

    // Find a set of properties that is common to all selected items
    for( const auto& property : commonProps )
    {
        if( !property->Available( aSelection.Front() ) )
            continue;

        // Either determine the common value for a property or "<...>" to indicate multiple values
        bool available = true;
        wxVariant commonVal, itemVal;

        for( EDA_ITEM* item : aSelection )
        {
            if( !property->Available( item ) )
                break; // there is an item that does not have this property, so do not display it

            wxVariant& value = commonVal.IsNull() ? commonVal : itemVal;
            const wxAny& any = item->Get( property );
            bool converted = false;

            if( property->HasChoices() )
            {
                // handle enums as ints, since there are no default conversion functions for wxAny
                int tmp;
                converted = any.GetAs<int>( &tmp );

                if( converted )
                    value = wxVariant( tmp );
            }

            if( !converted )                // all other types
                converted = any.GetAs( &value );

            if( !converted )
            {
                wxFAIL_MSG( "Could not convert wxAny to wxVariant" );
                available = false;
                break;
            }

            if( !commonVal.IsNull() && value != commonVal )
            {
                commonVal.MakeNull();       // items have different values for this property
                break;
            }
        }

        if( available )
        {
            wxPGProperty* pgProp = createPGProperty( property );

            if( pgProp )
            {
                pgProp->SetValue( commonVal );
                m_grid->Append( pgProp );
                m_displayed.push_back( property );
            }
        }
    }

    m_grid->FitColumns();
}


void PROPERTIES_PANEL::onShow( wxShowEvent& aEvent )
{
    if( aEvent.IsShown() )
        UpdateData();
}
