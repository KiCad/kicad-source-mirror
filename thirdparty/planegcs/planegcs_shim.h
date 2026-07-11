/*
 * KiCad vendoring shim for FreeCAD's planegcs constraint solver.
 *
 * planegcs (src/Mod/Sketcher/App/planegcs) is otherwise standalone, but its
 * sources pull in four FreeCAD-only headers. Rather than carry FreeCAD's build
 * tree, we redirect those includes to this single shim (see README.kicad.md for
 * the exact patch applied to the upstream sources):
 *
 *   <Base/Console.h>            -> Base::Console() / Base::TimeElapsed below
 *   <FCConfig.h>                -> deleted (no FCConfig symbol is referenced)
 *   "../../SketcherGlobal.h"    -> SketcherExport macro below
 *   <boost_graph_adjacency_list.hpp> -> <boost/graph/adjacency_list.hpp> (real boost)
 *
 * The only FreeCAD symbols planegcs actually uses are Base::Console().log()/
 * .warning() and Base::TimeElapsed. For the KiCad integration these route to
 * wxLogTrace and a std::chrono timer respectively; this spike build keeps them
 * trivial so the measured compile cost reflects planegcs + Eigen, not logging.
 */

#ifndef KICAD_PLANEGCS_SHIM_H
#define KICAD_PLANEGCS_SHIM_H

#include <chrono>

#include <boost/graph/adjacency_list.hpp>

#ifndef SketcherExport
#define SketcherExport
#endif

namespace Base
{
class ConsoleSink
{
public:
    void log( const char*, ... ) {}
    void warning( const char*, ... ) {}
};

inline ConsoleSink& Console()
{
    static ConsoleSink sink;
    return sink;
}

class TimeElapsed
{
public:
    TimeElapsed() : m_t( std::chrono::steady_clock::now() ) {}

    static double diffTimeF( const TimeElapsed& aStart, const TimeElapsed& aEnd )
    {
        return std::chrono::duration<double>( aEnd.m_t - aStart.m_t ).count();
    }

private:
    std::chrono::steady_clock::time_point m_t;
};
} // namespace Base

#endif // KICAD_PLANEGCS_SHIM_H
