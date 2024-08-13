

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
