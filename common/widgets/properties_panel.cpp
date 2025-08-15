/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pgm_base.h>
#include <properties/pg_cell_renderer.h>

#include <algorithm>
#include <iterator>
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
        m_SuppressGridChangeEvents( 0 ),
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

    if( !Pgm().m_PropertyGridInitialized )
    {
        delete wxPGGlobalVars->m_defaultRenderer;
        wxPGGlobalVars->m_defaultRenderer = new PG_CELL_RENDERER();
        Pgm().m_PropertyGridInitialized = true;
    }

    m_caption = new wxStaticText( this, wxID_ANY, _( "No objects selected" ) );
    mainSizer->Add( m_caption, 0, wxALL | wxEXPAND, 5 );

    m_grid = new wxPropertyGrid( this );
    m_grid->SetUnspecifiedValueAppearance( wxPGCell( wxT( "<...>" ) ) );
    m_grid->SetExtraStyle( wxPG_EX_HELP_AS_TOOLTIPS );

#if wxCHECK_VERSION( 3, 3, 0 )
    m_grid->SetValidationFailureBehavior( wxPGVFBFlags::MarkCell );
#else
    m_grid->SetValidationFailureBehavior( wxPG_VFB_MARK_CELL );
#endif

#if wxCHECK_VERSION( 3, 3, 0 )
    m_grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_RETURN );
    m_grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_NUMPAD_ENTER );
    m_grid->AddActionTrigger( wxPGKeyboardAction::NextProperty, WXK_DOWN );
    m_grid->AddActionTrigger( wxPGKeyboardAction::PrevProperty, WXK_UP );
    m_grid->AddActionTrigger( wxPGKeyboardAction::Edit, WXK_SPACE );
#else
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_RETURN );
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_NUMPAD_ENTER );
    m_grid->AddActionTrigger( wxPG_ACTION_NEXT_PROPERTY, WXK_DOWN );
    m_grid->AddActionTrigger( wxPG_ACTION_PREV_PROPERTY, WXK_UP );
    m_grid->AddActionTrigger( wxPG_ACTION_EDIT, WXK_SPACE );
#endif

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
              m_splitter_key_proportion = static_cast<float>( m_grid->GetSplitterPosition() ) / m_grid->GetSize().x;
          } );

    Bind( wxEVT_SIZE,
          [&]( wxSizeEvent& aEvent )
          {
              CallAfter( [this]()
                         {
                            RecalculateSplitterPos();
                         } );
              aEvent.Skip();
          } );

    m_frame->Bind( EDA_LANG_CHANGED, &PROPERTIES_PANEL::OnLanguageChanged, this );
}


PROPERTIES_PANEL::~PROPERTIES_PANEL()
{
    m_frame->Unbind( EDA_LANG_CHANGED, &PROPERTIES_PANEL::OnLanguageChanged, this );
}


void PROPERTIES_PANEL::OnLanguageChanged( wxCommandEvent& aEvent )
{
    if( m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();

    m_grid->Clear();
    m_displayed.clear();    // no ownership of pointers

    UpdateData();

    aEvent.Skip();
}


class SUPPRESS_GRID_CHANGED_EVENTS
{
public:
    SUPPRESS_GRID_CHANGED_EVENTS( PROPERTIES_PANEL* aPanel ) :
            m_panel( aPanel )
    {
        m_panel->m_SuppressGridChangeEvents++;
    }

    ~SUPPRESS_GRID_CHANGED_EVENTS()
    {
        m_panel->m_SuppressGridChangeEvents--;
    }

private:
    PROPERTIES_PANEL* m_panel;
};


void PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    SUPPRESS_GRID_CHANGED_EVENTS raii( this );

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
    else if( aSelection.Size() == 1 )
    {
        m_caption->SetLabel( aSelection.Front()->GetFriendlyName() );
    }
    else
    {
        m_caption->SetLabel( wxString::Format( _( "%d objects selected" ), aSelection.Size() ) );
    }

    // Get all the selected types
    std::set<TYPE_ID> types;

    for( EDA_ITEM* item : aSelection )
        types.insert( TYPE_HASH( *item ) );

    wxCHECK( !types.empty(), /* void */ );  // already guarded above, but Coverity doesn't know that

    PROPERTY_MANAGER&                  propMgr = PROPERTY_MANAGER::Instance();
    std::map<wxString, PROPERTY_BASE*> commonProps;
    const std::vector<PROPERTY_BASE*>& allProperties = propMgr.GetProperties( *types.begin() );

    for( PROPERTY_BASE* property : allProperties )
        commonProps.emplace( property->Name(), property );

    std::map<wxString, int> displayOrder;
    for( const auto& entry : propMgr.GetDisplayOrder( *types.begin() ) )
        displayOrder.emplace( entry.first->Name(), entry.second );

    std::vector<wxString> groupDisplayOrder = propMgr.GetGroupDisplayOrder( *types.begin() );
    std::set<wxString>    groups( groupDisplayOrder.begin(), groupDisplayOrder.end() );

    // Get all possible properties
    for( auto itType = std::next( types.begin() ); itType != types.end(); ++itType )
    {
        TYPE_ID type = *itType;

        for( const wxString& group : propMgr.GetGroupDisplayOrder( type ) )
        {
            if( !groups.count( group ) )
            {
                groupDisplayOrder.emplace_back( group );
                groups.insert( group );
            }
        }

        for( auto it = commonProps.begin(); it != commonProps.end(); )
        {
            if( !propMgr.GetProperty( type, it->first ) )
                it = commonProps.erase( it );
            else
                ++it;
        }
    }

    bool isLibraryEditor = m_frame->IsType( FRAME_FOOTPRINT_EDITOR )
                        || m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

    bool isDesignEditor = m_frame->IsType( FRAME_PCB_EDITOR )
                       || m_frame->IsType( FRAME_SCH );

    std::set<wxString> availableProps;

    // Find a set of properties that is common to all selected items
    for( auto& [name, property] : commonProps )
    {
        if( property->IsHiddenFromPropertiesManager() )
            continue;

        if( isLibraryEditor && property->IsHiddenFromLibraryEditors() )
            continue;

        if( isDesignEditor && property->IsHiddenFromDesignEditors() )
            continue;

        wxVariant   dummy;
        wxPGChoices choices;
        bool        writable;

        if( extractValueAndWritability( aSelection, name, dummy, writable, choices ) )
            availableProps.insert( name );
    }

    bool             writeable = true;
    std::set<wxString> existingProps;

    for( wxPropertyGridIterator it = m_grid->GetIterator(); !it.AtEnd(); it.Next() )
    {
        wxPGProperty* pgProp = it.GetProperty();
        wxString      name   = pgProp->GetName();

        if( !availableProps.count( name ) )
            continue;

        wxVariant   commonVal;
        wxPGChoices choices;

        extractValueAndWritability( aSelection, name, commonVal, writeable, choices );
        pgProp->SetValue( commonVal );
        pgProp->Enable( writeable );

        existingProps.insert( name );
    }

    if( !existingProps.empty() && existingProps == availableProps )
        return;

    // Some difference exists:  start from scratch
    reset();

    std::map<wxPGProperty*, int>                        pgPropOrders;
    std::map<wxString, std::vector<wxPGProperty*>> pgPropGroups;

    for( const wxString& name : availableProps )
    {
        PROPERTY_BASE* property = commonProps[name];
        wxPGProperty*  pgProp   = createPGProperty( property );
        wxVariant      commonVal;
        wxPGChoices    choices;

        if( !extractValueAndWritability( aSelection, name, commonVal, writeable, choices ) )
            continue;

        if( pgProp )
        {
            if( choices.GetCount() )
                pgProp->SetChoices( choices );

            pgProp->SetValue( commonVal );
            pgProp->Enable( writeable );
            m_displayed.push_back( property );

            wxASSERT( displayOrder.count( name ) );
            pgPropOrders[pgProp] = displayOrder[name];
            pgPropGroups[property->Group()].emplace_back( pgProp );
        }
    }

    const wxString unspecifiedGroupCaption = _( "Basic Properties" );

    for( const wxString& groupName : groupDisplayOrder )
    {
        if( !pgPropGroups.count( groupName ) )
            continue;

        std::vector<wxPGProperty*>& properties = pgPropGroups[groupName];
        wxString                    groupCaption = wxGetTranslation( groupName );

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
    {
        wxString propName = aProperty->Name();
        propName.Replace( ' ', '_' );
        wxFAIL_MSG( wxString::Format( wxS( "Could not convert wxAny to wxVariant for %s::%s" ),
                                      aItem->GetClass(),
                                      propName ) );
    }

    return converted;
}


bool PROPERTIES_PANEL::extractValueAndWritability( const SELECTION& aSelection, const wxString& aPropName,
                                                   wxVariant& aValue, bool& aWritable, wxPGChoices& aChoices )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    bool              different = false;
    bool              first = true;

    aWritable = true;

    for( EDA_ITEM* item : aSelection )
    {
        PROPERTY_BASE* property = propMgr.GetProperty( TYPE_HASH( *item ), aPropName );

        if( !property )
            return false;

        if( !propMgr.IsAvailableFor( TYPE_HASH( *item ), property, item ) )
            return false;

        if( property->IsHiddenFromPropertiesManager() )
            return false;

        wxPGChoices choices = property->GetChoices( item );

        if( first )
        {
            aChoices = choices;
            first = false;
        }
        else
        {
            wxArrayString labels = choices.GetLabels();
            wxArrayInt    values = choices.GetValuesForStrings( labels );

            if( labels != aChoices.GetLabels() || values != aChoices.GetValuesForStrings( labels ) )
                return false;
        }

        // If read-only for any of the selection, read-only for the whole selection.
        if( !propMgr.IsWriteableFor( TYPE_HASH( *item ), property, item ) )
            aWritable = false;

        wxVariant value;

        if( getItemValue( item, property, value ) )
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

    aEvent.Skip();
}


void PROPERTIES_PANEL::onCharHook( wxKeyEvent& aEvent )
{
    // m_grid->IsAnyModified() doesn't work for the first modification
    if( aEvent.GetKeyCode() == WXK_TAB && !aEvent.ShiftDown() )
    {
        wxVariant oldValue;

        if( wxPGProperty* prop = m_grid->GetSelectedProperty() )
            oldValue = prop->GetValue();

        m_grid->CommitChangesFromEditor();

        // If there was no change, treat it as a navigation key
        if( wxPGProperty* prop = m_grid->GetSelectedProperty() )
        {
            if( prop->GetValue() == oldValue )
                aEvent.Skip();
        }

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

    if( aEvent.GetKeyCode() == WXK_RETURN || aEvent.GetKeyCode() == WXK_NUMPAD_ENTER
        || aEvent.GetKeyCode() == WXK_DOWN || aEvent.GetKeyCode() == WXK_UP )
    {
        m_grid->CommitChangesFromEditor();

        CallAfter( [this]()
                   {
                       m_grid->SelectProperty( m_grid->GetSelectedProperty(), true );
                   } );
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
