/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <io/altium/altium_project_variants.h>

#include <boost/uuid/name_generator_sha1.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <wx/fileconf.h>
#include <wx/log.h>


KIID AltiumUniqueIdToKiid( const wxString& aUniqueId )
{
    // Fixed namespace so a given id always maps to the same KIID. Must never change or
    // previously stamped footprint paths would stop matching.
    static const boost::uuids::uuid s_namespace =
            boost::uuids::string_generator()( "6f9619ff-8b86-d011-b42d-00cf4fc964ff" );

    boost::uuids::name_generator_sha1 generator( s_namespace );
    boost::uuids::uuid                uuid = generator( aUniqueId.utf8_string() );

    return KIID( wxString::FromUTF8( boost::uuids::to_string( uuid ) ) );
}


static ALTIUM_VARIANT_ENTRY ParseVariationString( const wxString& aValue )
{
    ALTIUM_VARIANT_ENTRY entry;

    for( const wxString& token : wxSplit( aValue, '|' ) )
    {
        int eqPos = token.Find( '=' );

        if( eqPos == wxNOT_FOUND )
            continue;

        wxString key = token.Left( eqPos ).Trim().Trim( false );
        wxString val = token.Mid( eqPos + 1 ).Trim().Trim( false );

        if( key.CmpNoCase( wxS( "Designator" ) ) == 0 )
        {
            entry.designator = val;
        }
        else if( key.CmpNoCase( wxS( "UniqueId" ) ) == 0 )
        {
            // The target is a backslash-delimited path; only the final segment is the
            // component's own unique id, which is what symbol and footprint records store.
            int sep = val.Find( '\\', true );

            if( sep != wxNOT_FOUND )
                entry.uniqueId = val.Mid( sep + 1 );
            else
                entry.uniqueId = val;
        }
        else if( key.CmpNoCase( wxS( "Kind" ) ) == 0 )
        {
            long k = 0;
            val.ToLong( &k );
            entry.kind = static_cast<int>( k );
        }
        else if( key.CmpNoCase( wxS( "AlternatePart" ) ) == 0 )
        {
            if( val.empty() )
                continue;

            // AlternatePart contains sub-fields in the same pipe-delimited format, but since
            // it appears at the end of the line, the remaining tokens are its sub-fields.
            // We've already split on '|', so we just keep accumulating from here.
            // However, wxSplit already split everything. The AlternatePart sub-fields are the
            // remaining tokens after this one. We handle this below after the loop.
        }
    }

    // Parse AlternatePart sub-fields. In the raw line, AlternatePart= is followed by
    // pipe-separated key=value pairs that are part of the alternate part definition.
    // Since we already split on '|', find the AlternatePart token and treat everything
    // after it as sub-fields.
    bool inAlternatePart = false;

    for( const wxString& token : wxSplit( aValue, '|' ) )
    {
        int eqPos = token.Find( '=' );

        if( eqPos == wxNOT_FOUND )
            continue;

        wxString key = token.Left( eqPos ).Trim().Trim( false );
        wxString val = token.Mid( eqPos + 1 ).Trim().Trim( false );

        if( key.CmpNoCase( wxS( "AlternatePart" ) ) == 0 )
        {
            inAlternatePart = true;

            // The value after AlternatePart= might itself be the first sub-field value
            // In practice it's typically empty for Kind=1, or a lib reference for Kind=0
            if( !val.empty() )
                entry.alternateFields[wxS( "LibReference" )] = val;

            continue;
        }

        if( inAlternatePart && !val.empty() )
            entry.alternateFields[key] = val;
    }

    return entry;
}


std::map<wxString, wxString> ParseAltiumProjectParameters( const wxString& aPrjPcbPath )
{
    std::map<wxString, wxString> parameters;

    wxFileConfig config( wxEmptyString, wxEmptyString, wxEmptyString, aPrjPcbPath,
                         wxCONFIG_USE_NO_ESCAPE_CHARACTERS );

    wxString groupname;
    long     groupid;

    for( bool more = config.GetFirstGroup( groupname, groupid ); more;
         more = config.GetNextGroup( groupname, groupid ) )
    {
        if( !groupname.StartsWith( wxS( "Parameter" ) ) )
            continue;

        // Only numbered [ParameterN] sections hold the project parameters; reject look-alikes
        // such as [ParameterEngine] by requiring a trailing integer.
        wxString numStr = groupname.Mid( 9 );
        long     num;

        if( !numStr.ToLong( &num ) )
            continue;

        wxString name = config.Read( groupname + wxS( "/Name" ), wxEmptyString );

        if( name.empty() )
            continue;

        wxString value = config.Read( groupname + wxS( "/Value" ), wxEmptyString );

        // KiCad variable references are matched case-insensitively by resolving to upper case,
        // which is what AltiumPcbSpecialStringsToKiCadStrings emits, so key on the upper-cased
        // name to keep ${PCB_REVISION} and friends resolvable.
        name.UpperCase();

        parameters[name] = value;
    }

    return parameters;
}


std::vector<ALTIUM_PROJECT_VARIANT> ParseAltiumProjectVariants( const wxString& aPrjPcbPath )
{
    std::vector<ALTIUM_PROJECT_VARIANT> variants;

    wxFileConfig config( wxEmptyString, wxEmptyString, wxEmptyString, aPrjPcbPath,
                         wxCONFIG_USE_NO_ESCAPE_CHARACTERS );

    wxString groupname;
    long     groupid;

    for( bool more = config.GetFirstGroup( groupname, groupid ); more;
         more = config.GetNextGroup( groupname, groupid ) )
    {
        if( !groupname.StartsWith( wxS( "ProjectVariant" ) ) )
            continue;

        wxString numStr = groupname.Mid( 14 );
        long     num;

        if( !numStr.ToLong( &num ) )
            continue;

        ALTIUM_PROJECT_VARIANT pv;

        pv.description = config.Read( groupname + wxS( "/Description" ), wxEmptyString );
        pv.name = pv.description;

        if( pv.name.empty() )
            pv.name = groupname;

        long variationCount = 0;
        config.Read( groupname + wxS( "/VariationCount" ), &variationCount, 0 );

        for( long vi = 1; vi <= variationCount; ++vi )
        {
            wxString key = wxString::Format( wxS( "%s/Variation%ld" ), groupname, vi );
            wxString value = config.Read( key, wxEmptyString );

            if( value.empty() )
                continue;

            ALTIUM_VARIANT_ENTRY entry = ParseVariationString( value );

            if( !entry.designator.empty() )
                pv.variations.push_back( std::move( entry ) );
        }

        if( !pv.variations.empty() )
            variants.push_back( std::move( pv ) );
    }

    return variants;
}
