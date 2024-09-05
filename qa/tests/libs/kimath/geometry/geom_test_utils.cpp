

#include "geom_test_utils.h"


std::ostream& boost_test_print_type( std::ostream& os, const SHAPE_LINE_CHAIN& c )
{
    os << "SHAPE_LINE_CHAIN: " << c.PointCount() << " points: [\n";

    for( int i = 0; i < c.PointCount(); ++i )
    {
        os << "   " << i << ": " << c.CPoint( i ) << "\n";
    }

    os << "]";
    return os;
}

std::string toString( const POINT_TYPE& aType )
{
    switch( aType )
    {
    case PT_NONE: return "PT_NONE";
    case PT_CENTER: return "PT_CENTER";
    case PT_END: return "PT_END";
    case PT_MID: return "PT_MID";
    case PT_QUADRANT: return "PT_QUADRANT";
    case PT_CORNER: return "PT_CORNER";
    case PT_INTERSECTION: return "PT_INTERSECTION";
    default: return "Unknown POINT_TYPE: " + std::to_string( (int) aType );
    }
}

std::ostream& operator<<( std::ostream& os, const TYPED_POINT2I& aPt )
{
    os << "TYPED_POINT2I: " << aPt.m_point << " (" << aPt.m_types << ")";
    return os;
}