/////////////////////////////////////////////////////////////////////////////
// Name:        polygon_test_point_inside.h
/////////////////////////////////////////////////////////////////////////////

#ifndef __WXWINDOWS__
// define here wxPoint if we want to compile outside wxWidgets
class wxPoint
{
public:
    int x, y;
};
#endif
class CPOLYGONS_LIST;

/**
 * Function TestPointInsidePolygon
 * test if a point is inside or outside a polygon.
 * @param aPolysList: the list of polygons
 * @param aIdxstart: the starting point of a given polygon in m_FilledPolysList.
 * @param aIdxend: the ending point of the polygon in m_FilledPolysList.
 * @param aRefx, aRefy: the point coordinate to test
 * @return true if the point is inside, false for outside
 */
bool TestPointInsidePolygon( const CPOLYGONS_LIST& aPolysList,
                             int             aIdxstart,
                             int             aIdxend,
                             int             aRefx,
                             int             aRefy);
/**
 * Function TestPointInsidePolygon (overlaid)
 * same as previous, but mainly use wxPoint
 * @param aPolysList: the list of polygons
 * @param aCount: corners count in aPolysList.
 * @param aRefPoint: the point coordinate to test
 * @return true if the point is inside, false for outside
 */
bool TestPointInsidePolygon( const wxPoint* aPolysList,
                             int      aCount,
                             const wxPoint  &aRefPoint );
