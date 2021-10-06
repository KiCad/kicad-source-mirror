/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef QA_EESCHEMA_EESCHEMA_TEST_UTILS__H
#define QA_EESCHEMA_EESCHEMA_TEST_UTILS__H

#include <wx/filename.h>

namespace KI_TEST
{

/**
 * Get the configured location of Eeschema test data.
 *
 * By default, this is the test data in the source tree, but can be overridden
 * by the KICAD_TEST_EESCHEMA_DATA_DIR environment variable.
 *
 * @return a filename referring to the test data dir to use.
 */
wxFileName GetEeschemaTestDataDir();

} // namespace KI_TEST

#endif // QA_EESCHEMA_EESCHEMA_TEST_UTILS__H