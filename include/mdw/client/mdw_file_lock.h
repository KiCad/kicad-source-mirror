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

/**
 * @file mdw_file_lock.h
 * Edits of a small JSON file that several KiCad processes share.
 */

#ifndef MDW_FILE_LOCK_H
#define MDW_FILE_LOCK_H

#include <kicommon.h>

#include <functional>

#include <wx/string.h>


/**
 * Run aLocked holding the lock beside aFile ("<aFile>.lock"), so a read, change and atomic write of aFile
 * cannot interleave with another process's. Another process holds it only for one rewrite, so this waits up to
 * five seconds and a filesystem that cannot answer is used as is.
 *
 * @return false when another process held the lock throughout, in which case aLocked did not run
 */
KICOMMON_API bool MdwWithFileLock( const wxString& aFile, const std::function<void()>& aLocked );

#endif // MDW_FILE_LOCK_H
