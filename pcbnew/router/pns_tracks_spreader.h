/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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


#ifndef __PNS_TRACKS_SPREADER_H
#define __PNS_TRACKS_SPREADER_H

#include <math/vector2d.h>

#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_line.h"
#include "pns_placement_algo.h"
#include <unordered_set>

namespace PNS
{

class ROUTER;
class SHOVE;
class OPTIMIZER;

class TRACKS_SPREADER : public PLACEMENT_ALGO
{
public:
    TRACKS_SPREADER( ROUTER* aRouter );
    virtual ~TRACKS_SPREADER();

    /// @copydoc PLACEMENT_ALGO::Start()
    virtual bool Start( const VECTOR2I& aP, ITEM* aStartItem ) override;

    /// @copydoc PLACEMENT_ALGO::Move()
    virtual bool Move( const VECTOR2I& aP, ITEM* aEndItem ) override;

    /// @copydoc PLACEMENT_ALGO::FixRoute()
    virtual bool FixRoute( const VECTOR2I& aP, ITEM* aEndItem, bool aForceFinish = false ) override;

    /// @copydoc PLACEMENT_ALGO::CommitPlacement()
    bool CommitPlacement() override;

    /// @copydoc PLACEMENT_ALGO::AbortPlacement()
    bool AbortPlacement() override;

    /// @copydoc PLACEMENT_ALGO::HasPlacedAnything()
    bool HasPlacedAnything() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentNode()
    NODE* CurrentNode( bool aLoopsRemoved = false ) const override;

    /// @copydoc PLACEMENT_ALGO::Traces()
    const ITEM_SET Traces() override;

    /// @copydoc PLACEMENT_ALGO::CurrentStart()
    const VECTOR2I& CurrentStart() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentEnd()
    const VECTOR2I& CurrentEnd() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentNets()
    const std::vector<NET_HANDLE> CurrentNets() const override;

    /// @copydoc PLACEMENT_ALGO::CurrentLayer()
    int CurrentLayer() const override;

    void            SetSpreadableItems( const ITEM_SET& aAuxItems );
    const ITEM_SET& GetSpreadableItems() const { return m_auxItems; }


private:
    struct SPREADABLE_LINE
    {
        LINE     originLine;
        LINE     spreadableLine;
        LINE     forwardLine;
        LINE     backwardLine;
        LINE     optimizedLine;
        VECTOR2I p_start, p_end;
    };

    static std::optional<LINE> breakAndReassemble( NODE* aNode, LINE* aOrigLine, const VECTOR2I& aP1,
                                                   const VECTOR2I& aP2 );
    bool                       prepareSpreadableLines();
    void                       determineWalkOrder();
    const std::vector<SEG>     findNoOptSegments( const LINE&                             aClippedHead,
                                                  const TRACKS_SPREADER::SPREADABLE_LINE& aEntry ) const;


    LINKED_ITEM* m_startItem;
    VECTOR2I     m_startAnchor, m_endAnchor;
    ITEM_SET     m_auxItems;
    NODE*        m_currentNode;
    NODE*        m_preSpreadNode;

    LINE                                   m_rootLine;
    std::vector<SPREADABLE_LINE>           m_lines;
    std::unordered_set<const LINKED_ITEM*> m_spreadableItems;
};

} // namespace PNS

#endif // __PNS_TRACKS_SPREADER_H
