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

#include <api/api_plugin_manager.h>
#include <api/api_utils.h>
#include <footprint.h>
#include <pgm_base.h>
#include <wx/log.h>

#include <google/protobuf/util/json_util.h>

#include "footprint_wizard.h"


void FOOTPRINT_WIZARD::ResetParameters()
{
    for( const std::unique_ptr<WIZARD_PARAMETER>& param : m_info.parameters )
        param->Reset();
}


void FOOTPRINT_WIZARD_MANAGER::ReloadWizards()
{
    m_wizards.clear();

#ifdef KICAD_IPC_API
    API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();
    std::vector<const PLUGIN_ACTION*> actions = manager.GetActionsForScope( PLUGIN_ACTION_SCOPE::FOOTPRINT_WIZARD );

    for( const PLUGIN_ACTION* action : actions )
    {
        std::unique_ptr<FOOTPRINT_WIZARD> wizard = std::make_unique<FOOTPRINT_WIZARD>();
        wizard->SetIdentifier( action->identifier );

        if( RefreshInfo( wizard.get() ) )
            m_wizards[wizard->Identifier()] = std::move( wizard );
    }
#endif
}


std::vector<FOOTPRINT_WIZARD*> FOOTPRINT_WIZARD_MANAGER::Wizards() const
{
    std::vector<FOOTPRINT_WIZARD*> wizards;

    for( const std::unique_ptr<FOOTPRINT_WIZARD>& wizard : m_wizards | std::views::values )
        wizards.emplace_back( wizard.get() );

    std::ranges::sort( wizards,
                       []( FOOTPRINT_WIZARD* const& lhs,
                           FOOTPRINT_WIZARD* const& rhs ) -> bool
                       {
                           if( !lhs || !rhs )
                               return false;

                           return lhs->Info().meta.name < rhs->Info().meta.name;
                       } );

    return wizards;
}


std::optional<FOOTPRINT_WIZARD*> FOOTPRINT_WIZARD_MANAGER::GetWizard( const wxString& aIdentifier )
{
    if( m_wizards.contains( aIdentifier ) )
        return m_wizards[aIdentifier].get();

    return std::nullopt;
}


bool FOOTPRINT_WIZARD_MANAGER::RefreshInfo( FOOTPRINT_WIZARD* aWizard )
{
    wxCHECK( aWizard, false );
#ifdef KICAD_IPC_API
    API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();

    const wxLanguageInfo* lang = wxLocale::GetLanguageInfo( Pgm().GetSelectedLanguageIdentifier() );

    std::vector<wxString> args = {
        wxS( "--get-info" ),
        wxS( "--lang" ),
        lang->CanonicalName
    };

    wxString out, err;
    int ret = manager.InvokeActionSync( aWizard->Identifier(), args, &out, &err );

    if( ret != 0 )
        return false;

    kiapi::common::types::WizardInfo info;

    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;

    if( !google::protobuf::util::JsonStringToMessage( out.ToStdString(), &info, options ).ok() )
        return false;

    aWizard->Info().FromProto( info );
    return true;
#else
    return false;
#endif
}


tl::expected<FOOTPRINT*, wxString> FOOTPRINT_WIZARD_MANAGER::Generate( FOOTPRINT_WIZARD* aWizard )
{
#ifdef KICAD_IPC_API
    wxCHECK( aWizard, tl::unexpected( _( "Unexpected error with footprint wizard" ) ) );
    API_PLUGIN_MANAGER& manager = Pgm().GetPluginManager();

    const wxLanguageInfo* lang = wxLocale::GetLanguageInfo( Pgm().GetSelectedLanguageIdentifier() );

    std::vector<wxString> args = {
        wxS( "--generate" ),
        wxS( "--lang" ),
        lang->CanonicalName,
        wxS( "--params" )
    };

    kiapi::common::types::WizardParameterList params;

    for( const std::unique_ptr<WIZARD_PARAMETER>& param : aWizard->Info().parameters )
        params.mutable_parameters()->Add( param->Pack() );

    std::string paramsJson;

    if( !google::protobuf::util::MessageToJsonString( params, &paramsJson ).ok() )
        return tl::unexpected( _( "Unexpected error with footprint wizard" ) );

    args.emplace_back( wxString::Format( wxS( "'%s'" ), paramsJson ) );

    wxString out, err;
    int ret = manager.InvokeActionSync( aWizard->Identifier(), args, &out, &err );

    if( ret != 0 )
        return tl::unexpected( wxString::Format( _( "Could not launch footprint wizard '%s'" ), aWizard->Info().meta.name ) );

    kiapi::common::types::WizardGeneratedContent response;

    google::protobuf::util::JsonParseOptions options;
    options.ignore_unknown_fields = true;

    if( !google::protobuf::util::JsonStringToMessage( out.ToStdString(), &response, options ).ok() )
    {
        wxLogTrace( traceApi, wxString::Format( "Could not decode response:\n%s", out ) );
        return tl::unexpected( _( "Unexpected response from footprint wizard" ) );
    }

    if( response.status() != kiapi::common::types::WGS_OK )
        return tl::unexpected( wxString::Format( _( "Footprint wizard error: %s" ), response.error_message() ) );

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( nullptr );

    if( !fp->Deserialize( response.content() ) )
        return tl::unexpected( _( "Unexpected response from footprint wizard" ) );

    return fp.release();
#else
    return tl::unexpected( _( "The KiCad API is disabled" ) );
#endif
}


void WIZARD_META_INFO::FromProto( const kiapi::common::types::WizardMetaInfo& aProto )
{
    identifier = wxString::FromUTF8( aProto.identifier() );
    name = wxString::FromUTF8( aProto.name() );
    description = wxString::FromUTF8( aProto.description() );

    types_generated.clear();

    for( int type : aProto.types_generated() )
        types_generated.insert( static_cast<kiapi::common::types::WizardContentType>( type ) );
}


wxString WIZARD_PARAMETER::ParameterCategoryName( kiapi::common::types::WizardParameterCategory aCategory )
{
    using namespace kiapi::common::types;

    switch( aCategory )
    {
    case WPC_PACKAGE:   return _( "Package" );
    case WPC_PADS:      return _( "Pads" );
    case WPC_3DMODEL:   return _( "3D Model" );
    case WPC_METADATA:  return _( "General" );
    case WPC_RULES:     return _( "Design Rules" );
    case WPC_UNKNOWN:
    default:
        wxCHECK_MSG( false, wxEmptyString, "Unhandled parameter category type!" );
    }
}


std::unique_ptr<WIZARD_PARAMETER> WIZARD_PARAMETER::Create( const kiapi::common::types::WizardParameter& aProto )
{
    std::unique_ptr<WIZARD_PARAMETER> p;

    if( aProto.has_int_() )
    {
        p = std::make_unique<WIZARD_INT_PARAMETER>();
        static_cast<WIZARD_INT_PARAMETER*>( p.get() )->FromProto( aProto.int_() );
    }
    else if( aProto.has_real() )
    {
        p = std::make_unique<WIZARD_REAL_PARAMETER>();
        static_cast<WIZARD_REAL_PARAMETER*>( p.get() )->FromProto( aProto.real() );
    }
    else if( aProto.has_string() )
    {
        p = std::make_unique<WIZARD_STRING_PARAMETER>();
        static_cast<WIZARD_STRING_PARAMETER*>( p.get() )->FromProto( aProto.string() );
    }
    else if( aProto.has_bool_() )
    {
        p = std::make_unique<WIZARD_BOOL_PARAMETER>();
        static_cast<WIZARD_BOOL_PARAMETER*>( p.get() )->FromProto( aProto.bool_() );
    }

    p->identifier = wxString::FromUTF8( aProto.identifier() );
    p->name = wxString::FromUTF8( aProto.name() );
    p->description = wxString::FromUTF8( aProto.description() );
    p->category = aProto.category();
    p->type = aProto.type();

    return p;
}


kiapi::common::types::WizardParameter WIZARD_PARAMETER::Pack( bool aCompact )
{
    kiapi::common::types::WizardParameter packed;

    packed.set_identifier( identifier.ToUTF8() );

    if( !aCompact )
    {
        packed.set_name( name.ToUTF8() );
        packed.set_description( description.ToUTF8() );
        packed.set_category( category );
        packed.set_type( type );
    }

    return packed;
}


kiapi::common::types::WizardParameter WIZARD_INT_PARAMETER::Pack( bool aCompact )
{
    kiapi::common::types::WizardParameter packed = WIZARD_PARAMETER::Pack();

    packed.mutable_int_()->set_value( value );

    if( !aCompact )
    {
        packed.mutable_int_()->set_default_( default_value );

        if( min )
            packed.mutable_int_()->set_min( *min );

        if( max )
            packed.mutable_int_()->set_max( *max );

        if( multiple )
            packed.mutable_int_()->set_multiple( *multiple );
    }

    return packed;
}


kiapi::common::types::WizardParameter WIZARD_REAL_PARAMETER::Pack( bool aCompact )
{
    kiapi::common::types::WizardParameter packed = WIZARD_PARAMETER::Pack();

    packed.mutable_real()->set_value( value );

    if( !aCompact )
    {
        packed.mutable_real()->set_default_( default_value );

        if( min )
            packed.mutable_real()->set_min( *min );

        if( max )
            packed.mutable_real()->set_max( *max );
    }

    return packed;
}


kiapi::common::types::WizardParameter WIZARD_BOOL_PARAMETER::Pack( bool aCompact )
{
    kiapi::common::types::WizardParameter packed = WIZARD_PARAMETER::Pack();

    packed.mutable_bool_()->set_value( value );

    if( !aCompact )
        packed.mutable_bool_()->set_default_( default_value );

    return packed;
}


kiapi::common::types::WizardParameter WIZARD_STRING_PARAMETER::Pack( bool aCompact )
{
    kiapi::common::types::WizardParameter packed = WIZARD_PARAMETER::Pack();

    packed.mutable_string()->set_value( value );

    if( !aCompact )
        packed.mutable_string()->set_default_( default_value );

    return packed;
}


void WIZARD_INT_PARAMETER::FromProto( const kiapi::common::types::WizardIntParameter& aProto )
{
    value = aProto.value();
    default_value = aProto.default_();

    if( aProto.has_min() )
        min = aProto.min();
    else
        min.reset();

    if( aProto.has_max() )
        max = aProto.max();
    else
        max.reset();

    if( aProto.has_multiple() )
        multiple = aProto.multiple();
    else
        multiple.reset();
}


void WIZARD_REAL_PARAMETER::FromProto( const kiapi::common::types::WizardRealParameter& aProto )
{
    value = aProto.value();
    default_value = aProto.default_();

    if( aProto.has_min() )
        min = aProto.min();
    else
        min.reset();

    if( aProto.has_max() )
        max = aProto.max();
    else
        max.reset();
}


void WIZARD_BOOL_PARAMETER::FromProto( const kiapi::common::types::WizardBoolParameter& aProto )
{
    value = aProto.value();
    default_value = aProto.default_();
}


void WIZARD_STRING_PARAMETER::FromProto( const kiapi::common::types::WizardStringParameter& aProto )
{
    value = wxString::FromUTF8( aProto.value() );
    default_value = wxString::FromUTF8( aProto.default_() );

    if( aProto.has_validation_regex() )
        validation_regex = wxString::FromUTF8( aProto.validation_regex() );
    else
        validation_regex.reset();
}


void WIZARD_INFO::FromProto( const kiapi::common::types::WizardInfo& aProto )
{
    meta.FromProto( aProto.meta() );

    parameters.clear();
    parameters.reserve( aProto.parameters_size() );

    for( const kiapi::common::types::WizardParameter& parameter : aProto.parameters() )
        parameters.emplace_back( WIZARD_PARAMETER::Create( parameter ) );
}
