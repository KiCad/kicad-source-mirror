/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <qa_utils/utility_program.h>

#include "tools/coroutines/coroutine_tools.h"
#include "tools/io_benchmark/io_benchmark.h"

/**
 * List of registered tools.
 *
 * This is a pretty rudimentary way to register, but for a simple purpose,
 * it's effective enough. When you have a new tool, add it to this list.
 */
const static std::vector<KI_TEST::UTILITY_PROGRAM*> known_tools = {
    &coroutine_tool,
    &io_benchmark_tool,
};


int main( int argc, char** argv )
{
    KI_TEST::COMBINED_UTILITY c_util( known_tools );

    return c_util.HandleCommandLine( argc, argv );
}