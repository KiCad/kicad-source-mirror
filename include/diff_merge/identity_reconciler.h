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
 */

#ifndef KICAD_DIFF_IDENTITY_RECONCILER_H
#define KICAD_DIFF_IDENTITY_RECONCILER_H

#include <kicommon.h>
#include <kiid.h>
#include <math/vector2d.h>
#include <math/box2.h>

#include <wx/string.h>

#include <map>
#include <set>
#include <string>
#include <vector>


namespace KICAD_DIFF
{

/**
 * Descriptor used by the identity reconciler to compare items across two documents.
 *
 * Per-document-type differs produce a vector of descriptors. The reconciler is
 * type-agnostic: it sees only this struct.
 *
 * `type` is the KiCad item class name. Cross-type matches are never produced.
 * `keyProps` is a stable set of identifying properties (lib id, refdes, footprint
 * name, net code, etc.) the differ decides are strong identity signals for similarity
 * fallback. Order is irrelevant — pairs are compared as a set.
 */
struct KICOMMON_API ITEM_DESCRIPTOR
{
    KIID_PATH id;
    wxString  type;
    VECTOR2I  position{ 0, 0 };
    BOX2I     bbox;
    std::vector<std::pair<wxString, std::string>> keyProps;
};


/**
 * Maps every item in document A to either a peer in document B or to "only-in-A",
 * and vice versa. Also flags duplicate KIID_PATHs within either side.
 *
 * For the merge engine, the same reconciler is run on both (ancestor, ours) and
 * (ancestor, theirs); the union of the two outputs is what the merge plan walks.
 */
struct KICOMMON_API RECONCILIATION
{
    std::map<KIID_PATH, KIID_PATH> aToB;        // id-in-A -> id-in-B for matched items
    std::map<KIID_PATH, KIID_PATH> bToA;        // inverse for quick lookup
    std::set<KIID_PATH>            aOnly;       // items in A with no B match
    std::set<KIID_PATH>            bOnly;       // items in B with no A match
    std::vector<KIID_PATH>         duplicatesA; // KIID_PATH appearing 2+ times in A
    std::vector<KIID_PATH>         duplicatesB;
    std::size_t                    similarityMatches = 0;  // count of matches made via fallback
};


/**
 * Reconciles item identity across two snapshots of the same document.
 *
 * Primary key is `KIID_PATH`. Items with the same KIID_PATH on both sides are
 * matched directly. Items that exist only on one side are then candidates for
 * similarity matching — useful when UUIDs churn from imports, copy-paste, or
 * third-party tools rewriting identifiers.
 *
 * Similarity score is 0..1 and combines:
 *   - position equality                (weight 0.40)
 *   - bbox equality                    (weight 0.20)
 *   - keyProp matches                  (weight up to 0.40, prorated)
 *
 * Items with score >= threshold are matched; ties prefer the closest position.
 * A given B item is matched at most once (greedy by best score; deterministic order).
 */
class KICOMMON_API IDENTITY_RECONCILER
{
public:
    struct CONFIG
    {
        double       similarityThreshold = 0.85;
        bool         enableSimilarity    = true;
        bool         detectDuplicates    = true;
        unsigned int positionTolerance   = 0;   // nm; 0 = strict equality
    };

    IDENTITY_RECONCILER() = default;
    explicit IDENTITY_RECONCILER( const CONFIG& aConfig ) : m_config( aConfig ) {}

    const CONFIG& GetConfig() const { return m_config; }
    void          SetConfig( const CONFIG& aConfig ) { m_config = aConfig; }

    RECONCILIATION Reconcile( const std::vector<ITEM_DESCRIPTOR>& aA,
                              const std::vector<ITEM_DESCRIPTOR>& aB ) const;

    /**
     * Compute the similarity score between two items of the same type.
     *
     * Exposed publicly so per-document differs can tune the threshold against
     * representative fixtures.
     */
    double ScoreSimilarity( const ITEM_DESCRIPTOR& aA, const ITEM_DESCRIPTOR& aB ) const;

private:
    CONFIG m_config;
};

} // namespace KICAD_DIFF

#endif // KICAD_DIFF_IDENTITY_RECONCILER_H
