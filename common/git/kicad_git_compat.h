/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_GIT_COMPAT_H_
#define KICAD_GIT_COMPAT_H_

#include <git2.h>

// GIT_BUF_INIT was added in libgit2 1.4
#ifndef GIT_BUF_INIT
#define GIT_BUF_INIT { NULL, 0, 0 }
#endif

#if LIBGIT2_VER_MAJOR > 1 || ( LIBGIT2_VER_MAJOR == 1 && LIBGIT2_VER_MINOR >= 8 )
#include <git2/sys/errors.h>
#endif

#endif // KICAD_GIT_COMPAT_H_