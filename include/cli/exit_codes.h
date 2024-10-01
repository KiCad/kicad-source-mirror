/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CLI_EXIT_CODES_H
#define CLI_EXIT_CODES_H

namespace CLI
{
    namespace EXIT_CODES
    {
        static const int AVOID_CLOSING = -1;
        static const int SUCCESS = 0;
        static const int OK = 0;
        static const int ERR_ARGS = 1;
        static const int ERR_UNKNOWN = 2;
        static const int ERR_INVALID_INPUT_FILE = 3;
        static const int ERR_INVALID_OUTPUT_CONFLICT = 4;
        ///< Rules check violation count was greater than 0
        static const int ERR_RC_VIOLATIONS = 5;
        static const int ERR_JOBS_RUN_FAILED = 6;
    };
}

#endif