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

#pragma once

#include <fstream>
#include <algorithm>
#include <initializer_list>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <wx/image.h>
#include <wx/imagpng.h>
#include <wx/mstream.h>

#include <boost/test/unit_test.hpp>
#include <compatibility_report.h>
#include <downgrade_scan.h>
#include <downgrade_target.h>

namespace KI_TEST
{

inline const DOWNGRADE_TARGET& RequireDowngradeTarget( const wxString& aId )
{
    const DOWNGRADE_TARGET* target = FindDowngradeTarget( aId );

    if( !target )
        throw std::runtime_error( "Required downgrade test target is missing: " + aId.ToStdString() );

    return *target;
}


struct EXPECTED_DOWNGRADE_FEATURE
{
    wxString         m_name;
    int              m_introducedIn;
    DOWNGRADE_BUCKET m_bucket;

    /// Mirrors BOARD_RULE::m_keepUnderOmitPolicy. Declared here rather than read from the
    /// production table, so the expectation stays independent of the code under test.
    bool m_keepUnderOmitPolicy = false;
};


inline void CheckTransformCoverage( const COMPATIBILITY_REPORT& aReport, int aTargetVersion,
                                    std::initializer_list<EXPECTED_DOWNGRADE_FEATURE> aFeatures,
                                    bool                                              aDropInsteadOfApproximate )
{
    BOOST_REQUIRE( !aReport.IsBlocked() );
    BOOST_REQUIRE( aReport.IsLossy() );
    size_t expectedRows = 0;

    for( const EXPECTED_DOWNGRADE_FEATURE& feature : aFeatures )
    {
        BOOST_TEST_CONTEXT( feature.m_name.ToStdString() )
        {
            const auto entry = std::find_if( aReport.Entries().begin(), aReport.Entries().end(),
                                             [&]( const COMPAT_ENTRY& aEntry )
                                             {
                                                 return aEntry.m_feature == feature.m_name;
                                             } );

            if( aTargetVersion >= feature.m_introducedIn )
            {
                BOOST_CHECK( entry == aReport.Entries().end() );
                continue;
            }

            ++expectedRows;
            BOOST_REQUIRE_MESSAGE( entry != aReport.Entries().end(), "Transform fixture no longer exercises feature" );
            BOOST_CHECK_GT( entry->m_count, 0 );
            const DOWNGRADE_BUCKET expectedBucket = aDropInsteadOfApproximate
                                                            && feature.m_bucket == DOWNGRADE_BUCKET::LOWER
                                                            && !feature.m_keepUnderOmitPolicy
                                                    ? DOWNGRADE_BUCKET::DROP
                                                    : feature.m_bucket;
            BOOST_CHECK( entry->m_bucket == expectedBucket );
        }
    }

    BOOST_CHECK_EQUAL( aReport.Entries().size(), expectedRows );
}


/// A tiny PNG whose embedded resolution gives a fractional pixels/cm, so the legacy
/// truncating PPI differs from the corrected one and scale migrations become observable.
inline wxMemoryBuffer MakePngWithFractionalPixelsPerCm()
{
    if( !wxImage::FindHandler( wxBITMAP_TYPE_PNG ) )
        wxImage::AddHandler( new wxPNGHandler );

    wxImage img( 4, 4 );
    img.SetOption( wxIMAGE_OPTION_RESOLUTIONX, 96 );
    img.SetOption( wxIMAGE_OPTION_RESOLUTIONY, 96 );
    img.SetOption( wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES );

    wxMemoryOutputStream out;
    img.SaveFile( out, wxBITMAP_TYPE_PNG );

    wxMemoryBuffer buf;
    buf.SetBufSize( out.GetSize() );
    out.CopyTo( buf.GetData(), out.GetSize() );
    buf.SetDataLen( out.GetSize() );

    return buf;
}


/// Read a release keyword file from qa/data/downgrade, one token per line.
inline std::set<std::string> LoadTokenSet( const std::string& aFileName )
{
    std::set<std::string> tokens;
    std::ifstream         file( aFileName );
    std::string           line;

    while( std::getline( file, line ) )
    {
        while( !line.empty() && ( line.back() == '\r' || line.back() == ' ' ) )
            line.pop_back();

        if( !line.empty() )
            tokens.insert( line );
    }

    return tokens;
}


/// Every node head in the serialized text that is not in the known or harmless sets.
/// Quoted strings are stripped first, matching the production gate.
inline std::vector<std::string> UnknownSexprHeads( const std::string& aContent, const std::set<std::string>& aKnown,
                                                   const std::set<std::string>& aHarmless = {} )
{
    std::vector<std::string> unknown;

    for( const std::string& head : ExtractSexprNodeHeads( StripSexprStrings( aContent ) ) )
    {
        if( !aKnown.count( head ) && !aHarmless.count( head ) )
            unknown.push_back( head );
    }

    return unknown;
}

} // namespace KI_TEST
