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

#include "pcb_properties_panel.h"

#include <font/fontconfig.h>
#include <font/kicad_font_name.h>
#include <frame_type.h>
#include <pgm_base.h>
#include <pcb_base_edit_frame.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <properties/property_mgr.h>
#include <properties/pg_editors.h>
#include <board_commit.h>
#include <board_connected_item.h>
#include <board.h>
#include <properties/pg_properties.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <pcb_track.h>
#include <pcb_generator.h>
#include <generators/pcb_tuning_pattern.h>
#include <pad.h>
#include <footprint.h>
#include <pcb_field.h>
#include <template_fieldnames.h>
#include <settings/color_settings.h>
#include <string_utils.h>
#include <widgets/net_selector.h>
#include <widgets/ui_common.h>

static const wxString MISSING_FIELD_SENTINEL = wxS( "\uE000" );

class PCB_FOOTPRINT_FIELD_PROPERTY : public PROPERTY_BASE
{
public:
    PCB_FOOTPRINT_FIELD_PROPERTY( const wxString& aName ) :
            PROPERTY_BASE( aName ),
            m_name( aName )
    {
    }

    size_t OwnerHash() const override { return TYPE_HASH( FOOTPRINT ); }
    size_t BaseHash() const override { return TYPE_HASH( FOOTPRINT ); }
    size_t TypeHash() const override { return TYPE_HASH( wxString ); }

    bool Writeable( INSPECTABLE* aObject ) const override
    {
        return PROPERTY_BASE::Writeable( aObject );
    }

    void setter( void* obj, wxAny& v ) override
    {
        wxString value;

        if( !v.GetAs( &value ) )
            return;

        FOOTPRINT* footprint = reinterpret_cast<FOOTPRINT*>( obj );
        PCB_FIELD* field = footprint->GetField( m_name );

        wxString variantName;

        if( footprint->GetBoard() )
            variantName = footprint->GetBoard()->GetCurrentVariant();

        if( !variantName.IsEmpty() )
        {
            // Store the value as a variant override
            FOOTPRINT_VARIANT* variant = footprint->AddVariant( variantName );

            if( variant )
                variant->SetFieldValue( m_name, value );
        }
        else
        {
            // Set the base field value
            if( !field )
            {
                PCB_FIELD* newField = new PCB_FIELD( footprint, FIELD_T::USER, m_name );
                newField->SetText( value );
                footprint->Add( newField );
            }
            else
            {
                field->SetText( value );
            }
        }
    }

    wxAny getter( const void* obj ) const override
    {
        const FOOTPRINT* footprint = reinterpret_cast<const FOOTPRINT*>( obj );
        PCB_FIELD* field = footprint->GetField( m_name );

        if( field )
        {
            wxString variantName;

            if( footprint->GetBoard() )
                variantName = footprint->GetBoard()->GetCurrentVariant();

            wxString text;

            if( !variantName.IsEmpty() )
                text = footprint->GetFieldValueForVariant( variantName, m_name );
            else
                text = field->GetText();

            return wxAny( text );
        }
        else
        {
            return wxAny( MISSING_FIELD_SENTINEL );
        }
    }

private:
    wxString m_name;
};

std::set<wxString> PCB_PROPERTIES_PANEL::m_currentFieldNames;


class PG_NET_SELECTOR_EDITOR : public wxPGEditor
{
public:
    static const wxString EDITOR_NAME;

    PG_NET_SELECTOR_EDITOR( PCB_BASE_EDIT_FRAME* aFrame ) : m_frame( aFrame )
    {
    }

    wxString GetName() const override { return EDITOR_NAME; }

    wxPGWindowList CreateControls( wxPropertyGrid* aGrid, wxPGProperty* aProperty,
                                   const wxPoint& aPos, const wxSize& aSize ) const override
    {
        NET_SELECTOR* editor = new NET_SELECTOR( aGrid->GetPanel(), wxID_ANY, aPos, aSize, 0 );

        if( BOARD* board = m_frame->GetBoard() )
            editor->SetNetInfo( &board->GetNetInfo() );

        editor->SetIndeterminateString( INDETERMINATE_STATE );
        UpdateControl( aProperty, editor );

        editor->Bind( FILTERED_ITEM_SELECTED,
                      [=]( wxCommandEvent& aEvt )
                      {
                          auto& choices = const_cast<wxPGChoices&>( aProperty->GetChoices() );
                          wxString netname = editor->GetSelectedNetname();

                          if( choices.Index( netname ) == wxNOT_FOUND )
                              choices.Add( netname, editor->GetSelectedNetcode() );

                          wxVariant val( editor->GetSelectedNetcode() );
                          aGrid->ChangePropertyValue( aProperty, val );
                      } );

        return editor;
    }

    void UpdateControl( wxPGProperty* aProperty, wxWindow* aCtrl ) const override
    {
        if( NET_SELECTOR* editor = dynamic_cast<NET_SELECTOR*>( aCtrl ) )
        {
            if( aProperty->IsValueUnspecified() )
                editor->SetIndeterminate();
            else
                editor->SetSelectedNetcode( (int) aProperty->GetValue().GetLong() );
        }
    }

    bool GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty,
                              wxWindow* aCtrl ) const override
    {
        NET_SELECTOR* editor = dynamic_cast<NET_SELECTOR*>( aCtrl );

        if( !editor )
            return false;

        aVariant = static_cast<long>( editor->GetSelectedNetcode() );
        return true;
    }

    bool OnEvent( wxPropertyGrid* aGrid, wxPGProperty* aProperty, wxWindow* aWindow,
                  wxEvent& aEvent ) const override
    {
        return false;
    }

private:
    PCB_BASE_EDIT_FRAME* m_frame;
};

const wxString PG_NET_SELECTOR_EDITOR::EDITOR_NAME = wxS( "PG_NET_SELECTOR_EDITOR" );



PCB_PROPERTIES_PANEL::PCB_PROPERTIES_PANEL( wxWindow* aParent, PCB_BASE_EDIT_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_propMgr( PROPERTY_MANAGER::Instance() )
{
    m_propMgr.Rebuild();
    bool found = false;

    wxASSERT( wxPGGlobalVars );

    wxString editorKey = PG_UNIT_EDITOR::BuildEditorName( m_frame );

    auto it = wxPGGlobalVars->m_mapEditorClasses.find( editorKey );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( it->second );
        m_unitEditorInstance->UpdateFrame( m_frame );
        found = true;
    }

    if( !found )
    {
        PG_UNIT_EDITOR* new_editor = new PG_UNIT_EDITOR( m_frame );
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( wxPropertyGrid::RegisterEditorClass( new_editor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_CHECKBOX_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_CHECKBOX_EDITOR* cbEditor = new PG_CHECKBOX_EDITOR();
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( wxPropertyGrid::RegisterEditorClass( cbEditor ) );
    }
    else
    {
        m_checkboxEditorInstance = static_cast<PG_CHECKBOX_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_RATIO_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_RATIO_EDITOR* ratioEditor = new PG_RATIO_EDITOR();
        m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( wxPropertyGrid::RegisterEditorClass( ratioEditor ) );
    }
    else
    {
        m_ratioEditorInstance = static_cast<PG_RATIO_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_NET_SELECTOR_EDITOR::EDITOR_NAME );

    if( it == wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        PG_NET_SELECTOR_EDITOR* netEditor = new PG_NET_SELECTOR_EDITOR( m_frame );
        m_netSelectorEditorInstance = static_cast<PG_NET_SELECTOR_EDITOR*>( wxPropertyGrid::RegisterEditorClass( netEditor ) );
    }
    else
    {
        m_netSelectorEditorInstance = static_cast<PG_NET_SELECTOR_EDITOR*>( it->second );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_FPID_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( it->second );
        m_fpEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_FPID_EDITOR* fpEditor = new PG_FPID_EDITOR( m_frame );
        m_fpEditorInstance = static_cast<PG_FPID_EDITOR*>( wxPropertyGrid::RegisterEditorClass( fpEditor ) );
    }

    it = wxPGGlobalVars->m_mapEditorClasses.find( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( it->second );
        m_urlEditorInstance->UpdateFrame( m_frame );
    }
    else
    {
        PG_URL_EDITOR* urlEditor = new PG_URL_EDITOR( m_frame );
        m_urlEditorInstance = static_cast<PG_URL_EDITOR*>( wxPropertyGrid::RegisterEditorClass( urlEditor ) );
    }
}


PCB_PROPERTIES_PANEL::~PCB_PROPERTIES_PANEL()
{
    m_unitEditorInstance->UpdateFrame( nullptr );
    m_fpEditorInstance->UpdateFrame( nullptr );
    m_urlEditorInstance->UpdateFrame( nullptr );
}


const SELECTION& PCB_PROPERTIES_PANEL::getSelection( SELECTION& aFallbackSelection )
{
    PCB_SELECTION_TOOL* selectionTool = m_frame->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() && m_frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
    {
        if( BOARD* board = m_frame->GetBoard() )
        {
            if( FOOTPRINT* footprint = board->GetFirstFootprint() )
            {
                aFallbackSelection.Clear();
                aFallbackSelection.Add( footprint );
                return aFallbackSelection;
            }
        }
    }

    return selection;
}


EDA_ITEM* PCB_PROPERTIES_PANEL::getFrontItem()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    return selection.Empty() ? nullptr : selection.Front();
}


void PCB_PROPERTIES_PANEL::UpdateData()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    // TODO perhaps it could be called less often? use PROPERTIES_TOOL and catch MODEL_RELOAD?
    updateLists( static_cast<PCB_EDIT_FRAME*>( m_frame )->GetBoard() );

    // Will actually just be updatePropertyValues() if selection hasn't changed
    rebuildProperties( selection );
}


void PCB_PROPERTIES_PANEL::AfterCommit()
{
    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    rebuildProperties( selection );
}


void PCB_PROPERTIES_PANEL::rebuildProperties( const SELECTION& aSelection )
{
    m_currentFieldNames.clear();

    for( EDA_ITEM* item : aSelection )
    {
        if( item->Type() != PCB_FOOTPRINT_T )
            continue;

        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

        for( PCB_FIELD* field : footprint->GetFields() )
        {
            wxCHECK2( field, continue );

            m_currentFieldNames.insert( field->GetCanonicalName() );
        }
    }

    const wxString groupFields = _HKI( "Fields" );

    for( const wxString& name : m_currentFieldNames )
    {
        if( !m_propMgr.GetProperty( TYPE_HASH( FOOTPRINT ), name ) )
        {
            m_propMgr.AddProperty( new PCB_FOOTPRINT_FIELD_PROPERTY( name ), groupFields )
                    .SetAvailableFunc( [name]( INSPECTABLE* )
                                       {
                                           return PCB_PROPERTIES_PANEL::m_currentFieldNames.count( name );
                                       } );
        }
    }

    PROPERTIES_PANEL::rebuildProperties( aSelection );
}


wxPGProperty* PCB_PROPERTIES_PANEL::createPGProperty( const PROPERTY_BASE* aProperty ) const
{
    if( aProperty->TypeHash() == TYPE_HASH( PCB_LAYER_ID ) )
    {
        wxASSERT( aProperty->HasChoices() );

        const wxPGChoices& canonicalLayers = aProperty->Choices();
        wxArrayString      boardLayerNames;
        wxArrayInt         boardLayerIDs;

        for( int ii = 0; ii < (int) aProperty->Choices().GetCount(); ++ii )
        {
            int layer = canonicalLayers.GetValue( ii );

            boardLayerNames.push_back( m_frame->GetBoard()->GetLayerName( ToLAYER_ID( layer ) ) );
            boardLayerIDs.push_back( canonicalLayers.GetValue( ii ) );
        }

        auto ret = new PGPROPERTY_COLORENUM( new wxPGChoices( boardLayerNames, boardLayerIDs ) );

        ret->SetColorFunc(
                [&]( int aValue ) -> wxColour
                {
                    return m_frame->GetColorSettings()->GetColor( ToLAYER_ID( aValue ) ).ToColour();
                } );

        ret->SetLabel( wxGetTranslation( aProperty->Name() ) );
        ret->SetName( aProperty->Name() );
        ret->SetHelpString( wxGetTranslation( aProperty->Name() ) );
        ret->SetClientData( const_cast<PROPERTY_BASE*>( aProperty ) );

        return ret;
    }

    wxPGProperty* prop = PGPropertyFactory( aProperty, m_frame );

    if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::FOOTPRINT ) )
        prop->SetEditor( PG_FPID_EDITOR::BuildEditorName( m_frame ) );
    else if( aProperty->Name() == GetCanonicalFieldName( FIELD_T::DATASHEET ) )
        prop->SetEditor( PG_URL_EDITOR::BuildEditorName( m_frame ) );

    return prop;
}


PROPERTY_BASE* PCB_PROPERTIES_PANEL::getPropertyFromEvent( const wxPropertyGridEvent& aEvent ) const
{
    EDA_ITEM* item = const_cast<PCB_PROPERTIES_PANEL*>( this )->getFrontItem();

    if( !item || !item->IsBOARD_ITEM() )
        return nullptr;

    BOARD_ITEM* firstItem = static_cast<BOARD_ITEM*>( item );

    wxCHECK_MSG( firstItem, nullptr,
                 wxT( "getPropertyFromEvent for a property with nothing selected!") );

    PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *firstItem ), aEvent.GetPropertyName() );
    wxCHECK_MSG( property, nullptr,
                 wxT( "getPropertyFromEvent for a property not found on the selected item!" ) );

    return property;
}


void PCB_PROPERTIES_PANEL::valueChanging( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents > 0 )
        return;

    EDA_ITEM* item = getFrontItem();

    PROPERTY_BASE* property = getPropertyFromEvent( aEvent );
    wxCHECK( property, /* void */ );
    wxCHECK( item, /* void */ );

    wxVariant newValue = aEvent.GetPropertyValue();

    if( VALIDATOR_RESULT validationFailure = property->Validate( newValue.GetAny(), item ) )
    {
        wxString errorMsg = wxString::Format( wxS( "%s: %s" ), wxGetTranslation( property->Name() ),
                                              validationFailure->get()->Format( m_frame ) );
        m_frame->ShowInfoBarError( errorMsg );
        aEvent.Veto();
        return;
    }

    aEvent.Skip();
}


void PCB_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    if( m_SuppressGridChangeEvents > 0 )
        return;

    SELECTION fallbackSelection;
    const SELECTION& selection = getSelection( fallbackSelection );

    wxCHECK( getPropertyFromEvent( aEvent ), /* void */ );

    wxVariant newValue = aEvent.GetPropertyValue();
    BOARD_COMMIT changes( m_frame );

    PROPERTY_COMMIT_HANDLER handler( &changes );

    for( EDA_ITEM* edaItem : selection )
    {
        if( !edaItem->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* item = static_cast<BOARD_ITEM*>( edaItem );
        PROPERTY_BASE* property = m_propMgr.GetProperty( TYPE_HASH( *item ), aEvent.GetPropertyName() );
        wxCHECK( property, /* void */ );

        if( item->Type() == PCB_TABLECELL_T )
            changes.Modify( item->GetParent(), nullptr, RECURSE_MODE::NO_RECURSE );
        else if( item->Type() == PCB_GENERATOR_T )
            changes.Modify( item, nullptr, RECURSE_MODE::RECURSE );
        else
            changes.Modify( item, nullptr, RECURSE_MODE::NO_RECURSE );

        // In the PCB Editor, we generally restrict pad movement to the footprint (like dragging)
        if( item->Type() == PCB_PAD_T && m_frame
                && m_frame->IsType( FRAME_PCB_EDITOR )
                && !m_frame->GetPcbNewSettings()->m_AllowFreePads
                && ( aEvent.GetPropertyName() == _HKI( "Position X" )
                     || aEvent.GetPropertyName() == _HKI( "Position Y" ) ) )
        {
            PAD* pad = static_cast<PAD*>( item );
            FOOTPRINT* fp = pad->GetParentFootprint();

            if( fp )
            {
                VECTOR2I oldPos = pad->GetPosition();
                VECTOR2I newPos = oldPos;

                if( aEvent.GetPropertyName() == _HKI( "Position X" ) )
                    newPos.x = (int) newValue.GetLong();
                else
                    newPos.y = (int) newValue.GetLong();

                VECTOR2I delta = newPos - oldPos;

                if( delta.x != 0 || delta.y != 0 )
                {
                    changes.Modify( fp );
                    fp->Move( delta );
                }
            }

            continue;
        }

        // Handle variant-aware boolean properties for footprints
        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );
            wxString   variantName;

            if( footprint->GetBoard() )
                variantName = footprint->GetBoard()->GetCurrentVariant();

            if( !variantName.IsEmpty() )
            {
                wxString propName = aEvent.GetPropertyName();

                if( propName == _HKI( "Do not Populate" )
                    || propName == _HKI( "Exclude From Bill of Materials" )
                    || propName == _HKI( "Exclude From Position Files" ) )
                {
                    FOOTPRINT_VARIANT* variant = footprint->GetVariant( variantName );

                    if( !variant )
                        variant = footprint->AddVariant( variantName );

                    if( variant )
                    {
                        bool boolValue = newValue.GetBool();

                        if( propName == _HKI( "Do not Populate" ) )
                            variant->SetDNP( boolValue );
                        else if( propName == _HKI( "Exclude From Bill of Materials" ) )
                            variant->SetExcludedFromBOM( boolValue );
                        else if( propName == _HKI( "Exclude From Position Files" ) )
                            variant->SetExcludedFromPosFiles( boolValue );

                        continue;
                    }
                }
            }
        }

        item->Set( property, newValue );
    }

    changes.Push( _( "Edit Properties" ) );

    m_frame->Refresh();

    // Perform grid updates as necessary based on value change
    AfterCommit();

    // PointEditor may need to update if locked/unlocked
    if( aEvent.GetPropertyName() == _HKI( "Locked" ) )
        m_frame->GetToolManager()->ProcessEvent( EVENTS::SelectedEvent );

    aEvent.Skip();
}


void PCB_PROPERTIES_PANEL::updateLists( const BOARD* aBoard )
{
    wxPGChoices layersAll;
    wxPGChoices layersCu;
    wxPGChoices nets;
    wxPGChoices fonts;

    // Regenerate all layers
    for( PCB_LAYER_ID layer : aBoard->GetEnabledLayers().UIOrder() )
        layersAll.Add( LSET::Name( layer ), layer );

    for( PCB_LAYER_ID layer : LSET( aBoard->GetEnabledLayers() & LSET::AllCuMask() ).UIOrder() )
        layersCu.Add( LSET::Name( layer ), layer );

    m_propMgr.GetProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ) )->SetChoices( layersAll );
    m_propMgr.GetProperty( TYPE_HASH( PCB_SHAPE ), _HKI( "Layer" ) )->SetChoices( layersAll );

    // Copper only properties
    m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Layer" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PAD ), _HKI( "Bottom Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PAD ), _HKI( "Top Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Top" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Layer Bottom" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Bottom Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_VIA ), _HKI( "Top Backdrill Must-Cut" ) )->SetChoices( layersCu );
    m_propMgr.GetProperty( TYPE_HASH( PCB_TUNING_PATTERN ), _HKI( "Layer" ) )->SetChoices( layersCu );

    // Regenerate nets

    std::vector<std::pair<wxString, int>> netNames;
    netNames.reserve( aBoard->GetNetInfo().NetsByNetcode().size() );

    for( const auto& [ netCode, netInfo ] : aBoard->GetNetInfo().NetsByNetcode() )
        netNames.emplace_back( UnescapeString( netInfo->GetNetname() ), netCode );

    std::sort( netNames.begin(), netNames.end(),
               []( const auto& a, const auto& b )
               {
                   return a.first.CmpNoCase( b.first ) < 0;
               } );

    for( const auto& [ netName, netCode ] : netNames )
        nets.Add( netName, netCode );

    auto netProperty = m_propMgr.GetProperty( TYPE_HASH( BOARD_CONNECTED_ITEM ), _HKI( "Net" ) );
    netProperty->SetChoices( nets );

    auto tuningNet = m_propMgr.GetProperty( TYPE_HASH( PCB_TUNING_PATTERN ), _HKI( "Net" ) );
    tuningNet->SetChoices( nets );
}


bool PCB_PROPERTIES_PANEL::getItemValue( EDA_ITEM* aItem, PROPERTY_BASE* aProperty, wxVariant& aValue )
{
    // For FOOTPRINT variant-aware boolean properties, return variant-specific values
    if( aItem->Type() == PCB_FOOTPRINT_T )
    {
        FOOTPRINT* footprint = static_cast<FOOTPRINT*>( aItem );
        wxString   variantName;

        if( footprint->GetBoard() )
            variantName = footprint->GetBoard()->GetCurrentVariant();

        if( !variantName.IsEmpty() )
        {
            wxString propName = aProperty->Name();

            if( propName == _HKI( "Do not Populate" ) )
            {
                aValue = wxVariant( footprint->GetDNPForVariant( variantName ) );
                return true;
            }
            else if( propName == _HKI( "Exclude From Bill of Materials" ) )
            {
                aValue = wxVariant( footprint->GetExcludedFromBOMForVariant( variantName ) );
                return true;
            }
            else if( propName == _HKI( "Exclude From Position Files" ) )
            {
                aValue = wxVariant( footprint->GetExcludedFromPosFilesForVariant( variantName ) );
                return true;
            }
        }
    }

    return PROPERTIES_PANEL::getItemValue( aItem, aProperty, aValue );
}
