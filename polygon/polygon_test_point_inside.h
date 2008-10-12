/////////////////////////////////////////////////////////////////////////////
// Name:        polygon_test_point_inside.h
/////////////////////////////////////////////////////////////////////////////

using namespace std;

/** Function TestPointInsidePolygon
 * test if a point is inside or outside a polygon.
 * @param aPolysList: the list of polygons
 * @param istart: the starting point of a given polygon in m_FilledPolysList.
 * @param iend: the ending point of the polygon in m_FilledPolysList.
 * @param refx, refy: the point coordinate to test
 * @return true if the point is inside, false for outside
 */
bool TestPointInsidePolygon( std::vector <CPolyPt> aPolysList,
                             int                   istart,
                             int                   iend,
                             int                    refx,
                             int                    refy);
