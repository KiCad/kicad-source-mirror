/*
 * file polygons_defs.h
 * definitions to use boost::polygon in KiCad.
 */

#ifndef _POLYGONS_DEFS_H_
#define _POLYGONS_DEFS_H_

#include "boost/polygon/polygon.hpp"

// Define some types used here from boost::polygon
namespace bpl = boost::polygon;         // bpl = boost polygon library
using namespace bpl::operators;         // +, -, =, ...

typedef int                    coordinate_type;

typedef bpl::polygon_data<int> KPolygon;
typedef std::vector<KPolygon>  KPolygonSet;

typedef bpl::point_data<int>   KPolyPoint;
#endif          // #ifndef _POLYGONS_DEFS_H_
