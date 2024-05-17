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
    bool isJunction;
    bool hasExplicitJunctionDot;
    bool hasBusEntry;
};

/**
 * Check a tree of items for a confluence at a given point and work out what kind of junction
 * it is, if any.
*/
POINT_INFO AnalyzePoint( const EE_RTREE& aItem, const VECTOR2I& aPosition, bool aBreakCrossings );

} // namespace JUNCTION_HELPERS