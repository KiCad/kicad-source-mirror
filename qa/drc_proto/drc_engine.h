/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DRC_ENGINE_H
#define DRC_ENGINE_H

#include <board_commit.h>
#include <class_board.h>
#include <class_marker_pcb.h>
#include <class_track.h>
#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <memory>
#include <tools/pcb_tool_base.h>
#include <vector>


#include <drc_proto/drc_rule.h>

class BOARD_DESIGN_SETTINGS;
class PCB_EDIT_FRAME;
class DIALOG_DRC;
class BOARD_ITEM;
class BOARD;
class D_PAD;
class ZONE_CONTAINER;
class TRACK;
class MARKER_PCB;
class NETCLASS;
class EDA_TEXT;
class DRAWSEGMENT;
class NETLIST;
class PROGRESS_REPORTER;
class REPORTER;

//#ifdef DEBUG
#define drc_dbg(level, fmt, ...) \
    wxLogTrace( "drc_proto", fmt, __VA_ARGS__ );
//#else
//#define drc_dbg(level, fmt, ...)
//#endif

namespace test
{

class DRC_RULE_CONDITION;
class DRC_ITEM;
class DRC_RULE;
class DRC_TEST_PROVIDER;
class DRC_CONSTRAINT;

enum DRC_CONSTRAINT_QUERY_T
{
    DRCCQ_LARGEST_MINIMUM = 0
    // fixme: more to come I guess...
};

/// DRC error codes:
enum PCB_DRC_CODE
{
    DRCE_FIRST = 1,
    DRCE_UNCONNECTED_ITEMS = DRCE_FIRST,    ///< items are unconnected
    DRCE_SHORTING_ITEMS,                    ///< items shorting two nets but not a net-tie
    DRCE_ALLOWED_ITEMS,                     ///< a disallowed item has been used
    DRCE_CLEARANCE,                         ///< items are too close together
    DRCE_TRACKS_CROSSING,                   ///< tracks are crossing
    DRCE_COPPER_EDGE_CLEARANCE,             ///< a copper item is too close to the board edge
    DRCE_ZONES_INTERSECT,                   ///< copper area outlines intersect
    DRCE_ZONE_HAS_EMPTY_NET,                ///< copper area has a net but no pads in nets, which is suspicious
    DRCE_DANGLING_VIA,                      ///< via which isn't connected to anything
    DRCE_DANGLING_TRACK,                    ///< track with at least one end not connected to anything
    DRCE_HOLE_CLEARANCE,           ///< overlapping drilled holes break drill bits
    DRCE_TRACK_WIDTH,                       ///< Track width is too small or too large
    DRCE_TOO_SMALL_VIA,                     ///< Too small via size
    DRCE_VIA_ANNULUS,                       ///< Via size and drill leave annulus too small or too large
    DRCE_TOO_SMALL_DRILL,                   ///< Too small via or pad drill
    DRCE_VIA_HOLE_BIGGER,                   ///< via's hole is bigger than its diameter
    DRCE_PADSTACK,                          ///< something is wrong with a pad or via stackup
    DRCE_TOO_SMALL_MICROVIA,                ///< Too small micro via size
    DRCE_TOO_SMALL_MICROVIA_DRILL,          ///< Too small micro via drill
    DRCE_KEEPOUT,                           ///< A disallowed object is inside a keepout
    DRCE_OVERLAPPING_FOOTPRINTS,            ///< footprint courtyards overlap
    DRCE_MISSING_COURTYARD,                 ///< footprint has no courtyard defined
    DRCE_MALFORMED_COURTYARD,               ///< footprint has a courtyard but malformed
                                            ///< (not convertible to a closed polygon with holes)
    DRCE_PTH_IN_COURTYARD,
    DRCE_NPTH_IN_COURTYARD,
    DRCE_DISABLED_LAYER_ITEM,               ///< item on a disabled layer
    DRCE_INVALID_OUTLINE,                   ///< invalid board outline
    DRCE_MISSING_FOOTPRINT,                 ///< footprint not found for netlist item
    DRCE_DUPLICATE_FOOTPRINT,               ///< more than one footprints found for netlist item
    DRCE_EXTRA_FOOTPRINT,                   ///< netlist item not found for footprint

    DRCE_UNRESOLVED_VARIABLE,

    DRCE_LAST = DRCE_UNRESOLVED_VARIABLE
};

class DRC_REPORT
{
public:
    struct ENTRY
    {
        std::shared_ptr<DRC_ITEM> m_item;
        ::MARKER_PCB* m_marker;
    };

    typedef std::vector<ENTRY> ENTRIES;

    DRC_REPORT() {};
    ~DRC_REPORT();

    void AddItem( std::shared_ptr<DRC_ITEM> aItem, ::MARKER_PCB *aMarker = nullptr)
    {
        ENTRY ent;
        ent.m_item = aItem;
        ent.m_marker = aMarker;
        m_entries.push_back(ent);
    }

    const ENTRIES& GetReportEntries() const { return m_entries; };

private:
    ENTRIES m_entries;
};

/**
 * Design Rule Checker object that performs all the DRC tests.  The output of
 * the checking goes to the BOARD file in the form of two MARKER lists.  Those
 * two lists are displayable in the drc dialog box.  And they can optionally
 * be sent to a text file on disk.
 * This class is given access to the windows and the BOARD
 * that it needs via its constructor or public access functions.
 */
class DRC_ENGINE
{

public:
    DRC_ENGINE( BOARD* aBoard, BOARD_DESIGN_SETTINGS* aSettings );
    ~DRC_ENGINE();

    void SetProgressReporter( PROGRESS_REPORTER* aProgRep )
    {
        m_progressReporter = aProgRep;
    }

    void SetLogReporter( REPORTER* aReporter )
    {
        m_reporter = aReporter;
    }

    bool LoadRules( wxFileName aPath );
    void RunTests();

    void SetErrorLimit( int aLimit );

    BOARD_DESIGN_SETTINGS* GetDesignSettings() const
    {
        return m_designSettings;
    }

    BOARD* GetBoard() const
    {
        return m_board;
    }

    const DRC_CONSTRAINT& EvalRulesForItems(
            DRC_CONSTRAINT_TYPE_T ruleID,  BOARD_ITEM* a, BOARD_ITEM* b = nullptr, PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

    std::vector<DRC_CONSTRAINT> QueryConstraintsById( test::DRC_CONSTRAINT_TYPE_T ruleID );

    bool HasCorrectRulesForId( test::DRC_CONSTRAINT_TYPE_T ruleID );

    EDA_UNITS UserUnits() const
    {
        return EDA_UNITS::MILLIMETRES;
    }

    bool CompileRules();

    void Report( std::shared_ptr<DRC_ITEM> aItem, ::MARKER_PCB *Marker );
    void ReportProgress( double aProgress );
    void ReportStage ( const wxString& aStageName, int index, int total );
    void ReportAux( const wxString& aStr );

    std::shared_ptr<DRC_REPORT> GetReport() const { return m_drcReport; }

    bool QueryWorstConstraint( DRC_CONSTRAINT_TYPE_T aRuleId, test::DRC_CONSTRAINT& aConstraint, DRC_CONSTRAINT_QUERY_T aQueryType );


private:


    void freeCompiledRules();

    struct RULE_WITH_CONDITIONS
    {
        std::vector<test::DRC_RULE_CONDITION*> conditions;
        test::DRC_RULE*                        rule;
        std::vector<test::DRC_CONSTRAINT>      constraints;
    };

    struct RULE_SET
    {
        std::vector<RULE_WITH_CONDITIONS*> sortedRules;
        DRC_TEST_PROVIDER* provider;
    };

    typedef std::unordered_map<test::DRC_CONSTRAINT_TYPE_T, RULE_SET*> RULE_MAP;


    void inferImplicitRules();
    void loadTestProviders();

    BOARD_DESIGN_SETTINGS* m_designSettings;
    BOARD*                 m_board;

    std::shared_ptr<DRC_REPORT> m_drcReport;

    std::vector<DRC_RULE_CONDITION*> m_ruleConditions;
    std::vector<DRC_RULE*>           m_rules;
    std::vector<DRC_TEST_PROVIDER*>  m_testProviders;
    std::unordered_map<EDA_ITEM*, RULE_SET*> m_implicitRules;
    RULE_MAP m_ruleMap;
    REPORTER* m_reporter;
    PROGRESS_REPORTER* m_progressReporter;

    // condition -> rule -> provider
};

}; // namespace test

#endif // DRC_H
