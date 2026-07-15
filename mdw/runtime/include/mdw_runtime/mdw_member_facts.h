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

/**
 * @file mdw_member_facts.h
 * @brief The one rule that weighs a placed member's content against the releases of its item.
 *
 * CheckPlacedParts replies and library listings both use it, so the two never classify a part differently.
 * It depends on the standard library and the protocol's placed part status only.
 */

#ifndef MDW_MEMBER_FACTS_H
#define MDW_MEMBER_FACTS_H

#include <api/mdw/backend_revisions.pb.h>

#include <string>
#include <vector>


/// What one release of an item says about a placed part
struct MDW_RELEASE_VIEW
{
    std::string digest;         ///< The released content's digest, empty when no placed part may match it
    bool        usable = true;  ///< Its state may be used in designs
    bool        published = true;
    bool        forNewDesigns = true; ///< Its state may be placed anew, not only kept where it is
};


struct MDW_MEMBER_VERDICT
{
    std::vector<kiapi::mdw::PlacedPartStatus> statuses;
    int                                       matched = -1; ///< The index of the release the part matches, or -1
};


/// Weigh a part placed with aDigest against aReleases, oldest first. An empty aDigest knows nothing of the part
inline MDW_MEMBER_VERDICT MdwWeighMember( const std::vector<MDW_RELEASE_VIEW>& aReleases, const std::string& aDigest )
{
    MDW_MEMBER_VERDICT verdict;

    if( aReleases.empty() )
    {
        verdict.statuses.push_back( kiapi::mdw::PPS_NEVER_RELEASED );
        return verdict;
    }

    if( aDigest.empty() )
    {
        verdict.statuses.push_back( kiapi::mdw::PPS_UNKNOWN );
        return verdict;
    }

    for( int i = static_cast<int>( aReleases.size() ) - 1; i >= 0; --i )
    {
        if( aReleases[i].digest == aDigest )
        {
            verdict.matched = i;
            break;
        }
    }

    if( verdict.matched < 0 )
    {
        verdict.statuses.push_back( kiapi::mdw::PPS_DIFFERS );
        return verdict;
    }

    const MDW_RELEASE_VIEW& matched = aReleases[verdict.matched];

    if( verdict.matched != static_cast<int>( aReleases.size() ) - 1 )
        verdict.statuses.push_back( kiapi::mdw::PPS_OUTDATED );

    if( !matched.usable )
        verdict.statuses.push_back( kiapi::mdw::PPS_NOT_USABLE );

    if( !matched.published )
        verdict.statuses.push_back( kiapi::mdw::PPS_UNPUBLISHED );

    if( matched.usable && !matched.forNewDesigns )
        verdict.statuses.push_back( kiapi::mdw::PPS_NOT_FOR_NEW_DESIGNS );

    if( verdict.statuses.empty() )
        verdict.statuses.push_back( kiapi::mdw::PPS_RELEASED );

    return verdict;
}

#endif // MDW_MEMBER_FACTS_H
