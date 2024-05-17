#include <math/vector2d.h>
#include <sch_rtree.h>

namespace JUNCTION_HELPERS
{

/**
 * A selection of information about a point in the schematic that might
 * be eligible for turning into a junction.
*/
struct POINT_INFO
{
    ///< True if the point has 3+ wires and/or 3+ buses meeting there
    bool isJunction;
    ///< True if there is already junction dot at the point
    bool hasExplicitJunctionDot;
    ///< True if there is a bus entry at the point (either end)
    bool hasBusEntry;
    ///< True if there is a bus entry at the point and it connects to more than one wire
    bool hasBusEntryToMultipleWires;
};

/**
 * Check a tree of items for a confluence at a given point and work out what kind of junction
 * it is, if any.
*/
POINT_INFO AnalyzePoint( const EE_RTREE& aItem, const VECTOR2I& aPosition, bool aBreakCrossings );

} // namespace JUNCTION_HELPERS