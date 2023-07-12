/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2020-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <import_export.h>
#include <properties/pg_cell_renderer.h>

#include <algorithm>
#include <set>

#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/propgrid/advprops.h>


// This is provided by wx >3.3.0
#if !wxCHECK_VERSION( 3, 3, 0 )
extern APIIMPORT wxPGGlobalVarsClass* wxPGGlobalVars;
#endif

PROPERTIES_PANEL::PROPERTIES_PANEL( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        wxPanel( aParent ),
        m_frame( aFrame ),
        m_splitter_key_proportion( -1 )
{
    wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

    // on some platforms wxPGGlobalVars is initialized automatically,
    // but others need an explicit init
    if( !wxPGGlobalVars )
        wxPGInitResourceModule();

    // See https://gitlab.com/kicad/code/kicad/-/issues/12297
    // and https://github.com/wxWidgets/wxWidgets/issues/11787
    if( wxPGGlobalVars->m_mapEditorClasses.empty() )
    {
        wxPGEditor_TextCtrl = nullptr;
        wxPGEditor_Choice = nullptr;
        wxPGEditor_ComboBox = nullptr;
        wxPGEditor_TextCtrlAndButton = nullptr;
        wxPGEditor_CheckBox = nullptr;
        wxPGEditor_ChoiceAndButton = nullptr;
        wxPGEditor_SpinCtrl = nullptr;
        wxPGEditor_DatePickerCtrl = nullptr;
    }

    if( !dynamic_cast<PG_CELL_RENDERER*>( wxPGGlobalVars->m_defaultRenderer ) )
    {
        delete wxPGGlobalVars->m_defaultRenderer;
        wxPGGlobalVars->m_defaultRenderer = new PG_CELL_RENDERER();
    }

    m_caption = new wxStaticText( this, wxID_ANY, _( "No objects selected" ) );
    mainSizer->Add( m_caption, 0, wxALL | wxEXPAND, 5 );

    m_grid = new wxPropertyGrid( this, wxID_ANY, wxDefaultPosition, wxSize( 300, 400 ),
                                 wxPG_DEFAULT_STYLE );
    m_grid->SetUnspecifiedValueAppearance( wxPGCell( wxT( "<...>" ) ) );
    m_grid->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );

#if wxCHECK_VERSION( 3, 3, 0 )
    m_grid->SetValidationFailureBehavior( wxPGVFBFlags::MarkCell );
#else
    m_grid->SetValidationFailureBehavior( wxPG_VFB_MARK_CELL );
#endif

    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_NUMPAD_ENTER );
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_DOWN );
    m_grid->AddActionTrigger( wxPG_ACTION_PREV_PROPERTY, WXK_UP );
    m_grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_SPACE );
    m_grid->DedicateKey( WXK_RETURN );
    m_grid->DedicateKey( WXK_NUMPAD_ENTER );
    m_grid->DedicateKey( WXK_DOWN );
    m_grid->DedicateKey( WXK_UP );
    mainSizer->Add( m_grid, 1, wxEXPAND, 5 );

    m_grid->SetCellDisabledTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_GRAYTEXT ) );

#ifdef __WXGTK__
    // Needed for dark mode, on wx 3.0 at least.
    m_grid->SetCaptionTextColour( wxSystemSettings::GetColour( wxSYS_COLOUR_CAPTIONTEXT ) );
#endif

    SetFont( KIUI::GetDockedPaneFont( this ) );

    SetSizer( mainSizer );
    Layout();

    m_grid->CenterSplitter();

    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( PROPERTIES_PANEL::onCharHook ), nullptr, this );
    Connect( wxEVT_PG_CHANGED, wxPropertyGridEventHandler( PROPERTIES_PANEL::valueChanged ), nullptr, this );
    Connect( wxEVT_PG_CHANGING, wxPropertyGridEventHandler( PROPERTIES_PANEL::valueChanging ), nullptr, this );
    Connect( wxEVT_SHOW, wxShowEventHandler( PROPERTIES_PANEL::onShow ), nullptr, this );

    Bind( wxEVT_PG_COL_END_DRAG,
          [&]( wxPropertyGridEvent& )
          {
              m_splitter_key_proportion =
                      static_cast<float>( m_grid->GetSplitterPosition() ) / m_grid->GetSize().x;
          } );

    Bind( wxEVT_SIZE,
          [&]( wxSizeEvent& aEvent )
          {
              CallAfter( [&]()
                         {
                            RecalculateSplitterPos();
                         } );
              aEvent.Skip();
          } );
}


void PROPERTIES_PANEL::OnLanguageChanged()
{
    UpdateData();
}


void PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    auto reset =
            [&]()
            {
                if( m_grid->IsEditorFocused() )
                    m_grid->CommitChangesFromEditor();

                m_grid->Clear();
                m_displayed.clear();
            };

    if( aSelection.Empty() )
    {
        m_caption->SetLabel( _( "No objects selected" ) );
        reset();
        return;
    }

    // Get all the selected types
    std::set<TYPE_ID> types;

    for( EDA_ITEM* item : aSelection )
        types.insert( TYPE_HASH( *item ) );

    wxCHECK( !types.empty(), /* void */ );

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.SetUnits( m_frame->GetUserUnits() );
    propMgr.SetIuScale( &m_frame->GetIuScale() );
    propMgr.SetTransforms( &m_frame->GetOriginTransforms() );

    std::set<PROPERTY_BASE*> commonProps;
    const PROPERTY_LIST& allProperties = propMgr.GetProperties( *types.begin() );
    copy( allProperties.begin(), allProperties.end(), inserter( commonProps, commonProps.begin() ) );

    PROPERTY_DISPLAY_ORDER displayOrder = propMgr.GetDisplayOrder( *types.begin() );

    std::vector<wxString> groupDisplayOrder = propMgr.GetGroupDisplayOrder( *types.begin() );
    std::set<wxString> groups( groupDisplayOrder.begin(), groupDisplayOrder.end() );

    std::set<PROPERTY_BASE*> availableProps;

    // Get all possible properties
    for( const TYPE_ID& type : types )
    {
        const PROPERTY_LIST& itemProps = propMgr.GetProperties( type );

        const PROPERTY_DISPLAY_ORDER& itemDisplayOrder = propMgr.GetDisplayOrder( type );

        copy( itemDisplayOrder.begin(), itemDisplayOrder.end(),
              inserter( displayOrder, displayOrder.begin() ) );

        const std::vector<wxString>& itemGroups = propMgr.GetGroupDisplayOrder( type );

        for( const wxString& group : itemGroups )
        {
            if( !groups.count( group ) )
            {
                groupDisplayOrder.emplace_back( group );
                groups.insert( group );
            }
        }

        for( auto it = commonProps.begin(); it != commonProps.end(); /* ++it in the loop */ )
        {
            if( !binary_search( itemProps.begin(), itemProps.end(), *it ) )
                it = commonProps.erase( it );
            else
                ++it;
        }
    }

    EDA_ITEM* firstItem = aSelection.Front();
    bool isFootprintEditor = m_frame->IsType( FRAME_FOOTPRINT_EDITOR );

    // Find a set of properties that is common to all selected items
    for( PROPERTY_BASE* property : commonProps )
    {
        if( property->IsHiddenFromPropertiesManager() )
            continue;

        if( isFootprintEditor && property->IsHiddenFromLibraryEditors() )
            continue;

        if( propMgr.IsAvailableFor( TYPE_HASH( *firstItem ), property, firstItem ) )
            availableProps.insert( property );
    }

    bool writeable = true;
    std::set<PROPERTY_BASE*> existingProps;

    for( wxPropertyGridIterator it = m_grid->GetIterator(); !it.AtEnd(); it.Next() )
    {
        wxPGProperty*  pgProp   = it.GetProperty();
        PROPERTY_BASE* property = propMgr.GetProperty( TYPE_HASH( *firstItem ), pgProp->GetName() );

        // Switching item types?  Property may no longer be valid
        if( !property )
            continue;

        wxVariant commonVal;

        extractValueAndWritability( aSelection, property, commonVal, writeable );
        pgProp->SetValue( commonVal );
        pgProp->Enable( writeable );

        existingProps.insert( property );
    }

    if( !existingProps.empty() && existingProps == availableProps )
        return;

    // Some difference exists:  start from scratch
    reset();

    std::map<wxPGProperty*, int> pgPropOrders;
    std::map<wxString, std::vector<wxPGProperty*>> pgPropGroups;

    for( PROPERTY_BASE* property : availableProps )
    {
        wxPGProperty* pgProp = createPGProperty( property );
        wxVariant commonVal;

        if( !extractValueAndWritability( aSelection, property, commonVal, writeable ) )
            continue;

        if( pgProp )
        {
            pgProp->SetValue( commonVal );
            pgProp->Enable( writeable );
            m_displayed.push_back( property );

            wxASSERT( displayOrder.count( property ) );
            pgPropOrders[pgProp] = displayOrder[property];
            pgPropGroups[property->Group()].emplace_back( pgProp );
        }
    }

    if( aSelection.Size() > 1 )
    {
        m_caption->SetLabel( wxString::Format( _( "%d objects selected" ), aSelection.Size() ) );
    }
    else
    {
        m_caption->SetLabel( aSelection.Front()->GetFriendlyName() );
    }

    const wxString unspecifiedGroupCaption = _( "Basic Properties" );

    for( const wxString& groupName : groupDisplayOrder )
    {
        if( !pgPropGroups.count( groupName ) )
            continue;

        std::vector<wxPGProperty*>& properties = pgPropGroups[groupName];
        wxString groupCaption = wxGetTranslation( groupName );

        auto groupItem = new wxPropertyCategory( groupName.IsEmpty() ? unspecifiedGroupCaption
                                                                     : groupCaption );

        m_grid->Append( groupItem );

        std::sort( properties.begin(), properties.end(),
                   [&]( wxPGProperty*& aFirst, wxPGProperty*& aSecond )
                   {
                       return pgPropOrders[aFirst] < pgPropOrders[aSecond];
                   } );

        for( wxPGProperty* property : properties )
            m_grid->Append( property );
    }

    RecalculateSplitterPos();
}


bool PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    const wxAny& any = aItem->Get( aProperty );
    bool converted = false;

    if( aProperty->HasChoices() )
    {
        // handle enums as ints, since there are no default conversion functions for wxAny
        int tmp;
        converted = any.GetAs<int>( &tmp );

        if( converted )
            aValue = wxVariant( tmp );
    }

    if( !converted )                // all other types
        converted = any.GetAs( &aValue );

    if( !converted )
        wxFAIL_MSG( wxS( "Could not convert wxAny to wxVariant" ) );

    return converted;
}


bool PROPERTIES_PANEL::extractValueAndWritability( const SELECTION& aSelection,
                                                   PROPERTY_BASE* aProperty,
                                                   wxVariant& aValue, bool& aWritable )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.SetUnits( m_frame->GetUserUnits() );
    propMgr.SetTransforms( &m_frame->GetOriginTransforms() );

    bool different = false;
    wxVariant commonVal;

    aWritable = true;

    for( EDA_ITEM* item : aSelection )
    {
        if( !propMgr.IsAvailableFor( TYPE_HASH( *item ), aProperty, item ) )
            return false;

        // If read-only for any of the selection, read-only for the whole selection.
        if( !propMgr.IsWriteableFor( TYPE_HASH( *item ), aProperty, item ) )
            aWritable = false;

        wxVariant value;

        if( getItemValue( item, aProperty, value ) )
        {
            // Null value indicates different property values between items
            if( !different && !aValue.IsNull() && value != aValue )
            {
                different = true;
                aValue.MakeNull();
            }
            else if( !different )
            {
                aValue = value;
            }
        }
        else
        {
            // getItemValue returned false -- not available for this item
            return false;
        }
    }

    return true;
}


void PROPERTIES_PANEL::onShow( wxShowEvent& aEvent )
{
    if( aEvent.IsShown() )
        UpdateData();
}


void PROPERTIES_PANEL::onCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_TAB && !aEvent.ShiftDown() )
    {
        m_grid->CommitChangesFromEditor();
        return;
    }

    if( aEvent.GetKeyCode() == WXK_SPACE )
    {
        if( wxPGProperty* prop = m_grid->GetSelectedProperty() )
        {
            if( prop->GetValueType() == wxT( "bool" ) )
            {
                m_grid->SetPropertyValue( prop, !prop->GetValue().GetBool() );
                return;
            }
        }
    }

    if( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER )
    {
        m_grid->CommitChangesFromEditor();
        /* don't skip this one; if we're not the last property we'll also go to the next row */
    }

    aEvent.Skip();
}


void PROPERTIES_PANEL::RecalculateSplitterPos()
{
    if( m_splitter_key_proportion < 0 )
        m_grid->CenterSplitter();
    else
        m_grid->SetSplitterPosition( m_splitter_key_proportion * m_grid->GetSize().x );
}


void PROPERTIES_PANEL::SetSplitterProportion( float aProportion )
{
    m_splitter_key_proportion = aProportion;
    RecalculateSplitterPos();
}
