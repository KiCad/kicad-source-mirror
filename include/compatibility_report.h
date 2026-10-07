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

#include <vector>
#include <wx/string.h>
#include <wx/intl.h>

#include <reporter.h>

/// What happens to a feature when we export to an older KiCad.
enum class DOWNGRADE_BUCKET
{
    KEEP,  ///< Exists in the target, written unchanged
    LOWER, ///< Approximated down to a simpler equivalent
    DROP,  ///< No equivalent, left out
    BLOCK  ///< Cannot be represented at all, export to this target is not possible
};


/// One line in the compatibility report.
struct COMPAT_ENTRY
{
    DOWNGRADE_BUCKET m_bucket;
    wxString         m_feature; ///< e.g. "Net chains"
    wxString         m_detail;  ///< optional note, e.g. the electrical consequence
    int              m_count;
};


/// What an export to a given target will keep, approximate, drop, or block.
///
/// The classifier fills this in a dry run before anything is written, so the user
/// sees the cost up front.
class COMPATIBILITY_REPORT
{
public:
    void Add( DOWNGRADE_BUCKET aBucket, const wxString& aFeature, const wxString& aDetail = wxEmptyString,
              int aCount = 1 )
    {
        // Merge repeats, so classifying many sheets or files yields one row per feature.
        for( COMPAT_ENTRY& entry : m_entries )
        {
            if( entry.m_bucket == aBucket && entry.m_feature == aFeature )
            {
                entry.m_count += aCount;

                if( entry.m_detail.IsEmpty() )
                    entry.m_detail = aDetail;

                return;
            }
        }

        m_entries.push_back( { aBucket, aFeature, aDetail, aCount } );
    }

    void Merge( const COMPATIBILITY_REPORT& aOther )
    {
        for( const COMPAT_ENTRY& entry : aOther.m_entries )
            Add( entry.m_bucket, entry.m_feature, entry.m_detail, entry.m_count );
    }

    const std::vector<COMPAT_ENTRY>& Entries() const { return m_entries; }

    /// True if anything is approximated or dropped. This is what triggers the lossy warning.
    bool IsLossy() const { return Has( DOWNGRADE_BUCKET::LOWER ) || Has( DOWNGRADE_BUCKET::DROP ); }

    /// True if some feature makes the target impossible. The user cannot export anyway.
    bool IsBlocked() const { return Has( DOWNGRADE_BUCKET::BLOCK ); }

    int Count( DOWNGRADE_BUCKET aBucket ) const
    {
        int total = 0;

        for( const COMPAT_ENTRY& entry : m_entries )
        {
            if( entry.m_bucket == aBucket )
                total += entry.m_count;
        }

        return total;
    }

    void Print( REPORTER* aReporter ) const
    {
        for( const COMPAT_ENTRY& entry : m_entries )
        {
            wxString label;

            switch( entry.m_bucket )
            {
            case DOWNGRADE_BUCKET::KEEP: label = _( "keep" ); break;
            case DOWNGRADE_BUCKET::LOWER: label = _( "approximate" ); break;
            case DOWNGRADE_BUCKET::DROP: label = _( "drop" ); break;
            case DOWNGRADE_BUCKET::BLOCK: label = _( "block" ); break;
            }

            aReporter->Report( wxString::Format( wxT( "  [%s] %s (%d): %s\n" ), label, entry.m_feature, entry.m_count,
                                                 entry.m_detail ),
                               RPT_SEVERITY_INFO );
        }
    }

private:
    bool Has( DOWNGRADE_BUCKET aBucket ) const
    {
        for( const COMPAT_ENTRY& entry : m_entries )
        {
            if( entry.m_bucket == aBucket )
                return true;
        }

        return false;
    }

    std::vector<COMPAT_ENTRY> m_entries;
};
