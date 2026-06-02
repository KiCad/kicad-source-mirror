/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef TEST_DIPTRACE_BENCHMARKS_FIXTURE_H
#define TEST_DIPTRACE_BENCHMARKS_FIXTURE_H

/**
 * @file test_diptrace_benchmarks_fixture.h
 * Shared fixture and includes for the DipTrace PCB benchmark test suite.
 */

#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcbnew/pcb_io/diptrace/pcb_io_diptrace.h>
#include <pcbnew/pcb_io/diptrace/diptrace_pcb_parser.h>
#include <pcbnew/drc/drc_rule_parser.h>
#include <pcbnew/drc/drc_rule.h>

#include <reporter.h>
#include <board.h>
#include <ki_exception.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_shape.h>
#include <zone.h>
#include <netinfo.h>
#include <layer_ids.h>
#include <base_units.h>
#include <geometry/shape_poly_set.h>
#include <wx/log.h>
#include <wx/xml/xml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


struct DIPTRACE_BENCHMARK_FIXTURE
{
    DIPTRACE_BENCHMARK_FIXTURE() {}

    PCB_IO_DIPTRACE m_plugin;

    std::string GetTestDataDir() { return KI_TEST::GetPcbnewTestDataDir() + "plugins/diptrace/"; }

    std::unique_ptr<BOARD> LoadBoard( const std::string& aFileName )
    {
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
        m_plugin.LoadBoard( GetTestDataDir() + aFileName, board.get() );
        return board;
    }

    std::unique_ptr<BOARD> LoadBoardFromPath( const std::string& aPath )
    {
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
        m_plugin.LoadBoard( aPath, board.get() );
        return board;
    }
};

#endif // TEST_DIPTRACE_BENCHMARKS_FIXTURE_H
