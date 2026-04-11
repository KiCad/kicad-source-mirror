/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 Jon Evans <jon@craftyjon.com>
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

#include <ranges>

#include <widgets/footprint_wizard_properties_panel.h>

#include <footprint_wizard.h>
#include <footprint_wizard_frame.h>
#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <widgets/ui_common.h>

#include <wx/stattext.h>


FOOTPRINT_WIZARD_PROPERTIES_PANEL::FOOTPRINT_WIZARD_PROPERTIES_PANEL( wxWindow* aParent,
                                                                      FOOTPRINT_WIZARD_FRAME* aFrame ) :
        PROPERTIES_PANEL( aParent, aFrame ),
        m_frame( aFrame ),
        m_wizard( nullptr ),
        m_unitEditorInstance( nullptr ),
        m_checkboxEditorInstance( nullptr ),
        m_ratioEditorInstance( nullptr )
{
    // AUI panel caption is enough
    m_caption->Hide();

    wxASSERT( wxPGGlobalVars );

    wxString editorKey = PG_UNIT_EDITOR::BuildEditorName( m_frame );
    auto it = wxPGGlobalVars->m_mapEditorClasses.find( editorKey );

    if( it != wxPGGlobalVars->m_mapEditorClasses.end() )
    {
        m_unitEditorInstance = static_cast<PG_UNIT_EDITOR*>( it->second );
        m_unitEditorInstance->UpdateFrame( m_frame );
    }
    else
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
}


FOOTPRINT_WIZARD_PROPERTIES_PANEL::~FOOTPRINT_WIZARD_PROPERTIES_PANEL()
{
    if( m_unitEditorInstance )
        m_unitEditorInstance->UpdateFrame( nullptr );
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::UpdateData()
{
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::RebuildParameters( FOOTPRINT_WIZARD* aWizard )
{
    SUPPRESS_GRID_CHANGED_EVENTS raii( this );

    if( m_grid->IsEditorFocused() )
        m_grid->CommitChangesFromEditor();

    m_grid->Clear();

    if( !aWizard )
        return;

    std::map<kiapi::common::types::WizardParameterCategory, std::vector<WIZARD_PARAMETER*>> params;

    for( const std::unique_ptr<WIZARD_PARAMETER>& param : aWizard->Info().parameters )
        params[param->category].emplace_back( param.get() );

    for( kiapi::common::types::WizardParameterCategory category : params | std::views::keys )
    {
        auto groupItem = new wxPropertyCategory( WIZARD_PARAMETER::ParameterCategoryName( category ) );
        m_grid->Append( groupItem );

        for( WIZARD_PARAMETER* param : params[category] )
        {
            if( wxPGProperty* prop = createPGProperty( param ) )
                m_grid->Append( prop );
        }
    }

    RecalculateSplitterPos();
}


wxPGProperty* FOOTPRINT_WIZARD_PROPERTIES_PANEL::createPGProperty( WIZARD_PARAMETER* aParam ) const
{
    wxPGProperty* ret = nullptr;

    switch( aParam->type )
    {
    case kiapi::common::types::WPDT_DISTANCE:
        ret = new PGPROPERTY_SIZE( m_frame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( m_frame ) );
        break;

    case kiapi::common::types::WPDT_AREA:
        ret = new PGPROPERTY_AREA( m_frame );
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( m_frame ) );
        break;

    case kiapi::common::types::WPDT_VOLUME:
        wxASSERT_MSG( false, "Volume properties are not currently implemented" );
        break;

    case kiapi::common::types::WPDT_TIME:
        ret = new wxFloatProperty();
        break;

    case kiapi::common::types::WPDT_ANGLE:
        ret = new PGPROPERTY_ANGLE();;
        ret->SetEditor( PG_UNIT_EDITOR::BuildEditorName( m_frame ) );
        break;

    case kiapi::common::types::WPDT_STRING:
        ret = new PGPROPERTY_STRING();
        break;

    case kiapi::common::types::WPDT_INTEGER:
        ret = new wxIntProperty();
        break;

    case kiapi::common::types::WPDT_REAL:
        ret = new wxFloatProperty();
        break;

    case kiapi::common::types::WPDT_BOOL:
        ret = new PGPROPERTY_BOOL();
        break;

    // TODO(JE) consider supporting enum properties

    case kiapi::common::types::WPDT_UNKNOWN:
    default:
        break;
    }

    if( ret )
    {
        ret->SetLabel( wxGetTranslation( aParam->name ) );
        ret->SetName( aParam->identifier );
        ret->SetHelpString( wxGetTranslation( aParam->description ) );
        ret->SetClientData( aParam );

        if( auto ip = dynamic_cast<const WIZARD_INT_PARAMETER*>( aParam ) )
            ret->SetValue( ip->value );
        else if( auto rp = dynamic_cast<const WIZARD_REAL_PARAMETER*>( aParam ) )
            ret->SetValue( rp->value );
        else if( auto bp = dynamic_cast<const WIZARD_BOOL_PARAMETER*>( aParam ) )
            ret->SetValue( bp->value );
        else if( auto sp = dynamic_cast<const WIZARD_STRING_PARAMETER*>( aParam ) )
            ret->SetValue( sp->value );
    }

    return ret;
}


WIZARD_PARAMETER* FOOTPRINT_WIZARD_PROPERTIES_PANEL::getParamFromEvent( const wxPropertyGridEvent& aEvent )
{
    return static_cast<WIZARD_PARAMETER*>( aEvent.GetProperty()->GetClientData() );
}


void FOOTPRINT_WIZARD_PROPERTIES_PANEL::valueChanged( wxPropertyGridEvent& aEvent )
{
    WIZARD_PARAMETER* param = getParamFromEvent( aEvent );
    wxCHECK( param, /* void */ );

    wxAny newValue = aEvent.GetPropertyValue();

    if( WIZARD_INT_PARAMETER* ip = dynamic_cast<WIZARD_INT_PARAMETER*>( param ) )
    {
        ip->value = newValue.As<int>();
    }
    else if( WIZARD_REAL_PARAMETER* rp = dynamic_cast<WIZARD_REAL_PARAMETER*>( param ) )
    {
        rp->value = newValue.As<double>();
    }
    else if( WIZARD_BOOL_PARAMETER* bp = dynamic_cast<WIZARD_BOOL_PARAMETER*>( param ) )
    {
        bp->value = newValue.As<bool>();
    }
    else if( WIZARD_STRING_PARAMETER* sp = dynamic_cast<WIZARD_STRING_PARAMETER*>( param ) )
    {
        sp->value = newValue.As<wxString>();
    }

    m_frame->OnWizardParametersChanged();
}
