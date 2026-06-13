/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <diff_merge/merge_validators.h>

#include <algorithm>
#include <map>


namespace KICAD_DIFF
{

bool VALIDATION_REPORT::HasErrors() const
{
    return std::any_of( failures.begin(), failures.end(),
                        []( const VALIDATION_FAILURE& aF )
                        {
                            return aF.severity == RPT_SEVERITY_ERROR;
                        } );
}


void VALIDATION_REPORT::Merge( VALIDATION_REPORT&& aOther )
{
    failures.reserve( failures.size() + aOther.failures.size() );

    for( VALIDATION_FAILURE& f : aOther.failures )
        failures.push_back( std::move( f ) );

    aOther.failures.clear();
}


VALIDATION_REPORT CheckRefdesUniqueness( const std::vector<REFDES_ENTRY>& aEntries )
{
    VALIDATION_REPORT report;

    // Group entries by refdes. Empty refdeses are unassigned components and
    // don't collide.
    std::map<wxString, std::vector<KIID_PATH>> byRefdes;

    for( const REFDES_ENTRY& e : aEntries )
    {
        if( e.refdes.IsEmpty() )
            continue;

        // Skip refdeses with a trailing '?' — those are also unassigned
        // (placeholder before annotation).
        if( e.refdes.EndsWith( wxS( "?" ) ) )
            continue;

        byRefdes[e.refdes].push_back( e.id );
    }

    for( const auto& [refdes, ids] : byRefdes )
    {
        if( ids.size() < 2 )
            continue;

        VALIDATION_FAILURE f;
        f.severity     = RPT_SEVERITY_ERROR;
        f.validator    = wxS( "RefdesUniqueness" );
        f.message      = wxString::Format( wxS( "Reference designator '%s' is used %zu times" ),
                                           refdes, ids.size() );
        f.relatedItems = ids;
        report.failures.push_back( std::move( f ) );
    }

    return report;
}


VALIDATION_REPORT CheckConnectivityRebuildFlag( bool aPlanRequiredRebuild,
                                                bool aApplierReportedRebuild )
{
    VALIDATION_REPORT report;

    if( aPlanRequiredRebuild && !aApplierReportedRebuild )
    {
        VALIDATION_FAILURE f;
        f.severity  = RPT_SEVERITY_ERROR;
        f.validator = wxS( "ConnectivityRebuild" );
        f.message   = wxS( "Plan required a connectivity rebuild but the applier did not "
                           "report performing one — connectivity data may be stale." );
        report.failures.push_back( std::move( f ) );
    }

    return report;
}


VALIDATION_REPORT CheckSchemaVersions( int aAncestorVersion, int aOursVersion,
                                       int aTheirsVersion )
{
    VALIDATION_REPORT report;

    // Versions are encoded as YYYYMMDD ints in KiCad serialization. Any two
    // sides differing in their major epoch (>5y span) is rejected; closer
    // spans surface as WARNING so the user can intervene.
    constexpr int MAJOR_EPOCH_DELTA = 50000;   // ~5 years in YYYYMMDD encoding

    int maxV = std::max( { aAncestorVersion, aOursVersion, aTheirsVersion } );
    int minV = std::min( { aAncestorVersion, aOursVersion, aTheirsVersion } );

    if( maxV - minV >= MAJOR_EPOCH_DELTA )
    {
        VALIDATION_FAILURE f;
        f.severity  = RPT_SEVERITY_ERROR;
        f.validator = wxS( "SchemaVersion" );
        f.message   = wxString::Format(
                wxS( "Schema version spread between merge inputs is too large "
                     "(min=%d, max=%d). Refusing to merge." ),
                minV, maxV );
        report.failures.push_back( std::move( f ) );
    }
    else if( maxV != minV )
    {
        VALIDATION_FAILURE f;
        f.severity  = RPT_SEVERITY_WARNING;
        f.validator = wxS( "SchemaVersion" );
        f.message   = wxString::Format(
                wxS( "Merge inputs use different schema versions "
                     "(ancestor=%d, ours=%d, theirs=%d)." ),
                aAncestorVersion, aOursVersion, aTheirsVersion );
        report.failures.push_back( std::move( f ) );
    }

    return report;
}

} // namespace KICAD_DIFF
