/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QA_SCOPED_GIT_BACKEND_H
#define QA_SCOPED_GIT_BACKEND_H

#include <git/git_backend.h>
#include <git/libgit_backend.h>

/// Install a real backend for a test and restore the previous backend afterwards.
class SCOPED_GIT_BACKEND
{
public:
    SCOPED_GIT_BACKEND()
    {
        m_backend = new LIBGIT_BACKEND();
        m_backend->Init();
        SetGitBackend( m_backend );
    }

    ~SCOPED_GIT_BACKEND()
    {
        SetGitBackend( m_previous );
        m_backend->Shutdown();
        delete m_backend;
    }

    SCOPED_GIT_BACKEND( const SCOPED_GIT_BACKEND& ) = delete;
    SCOPED_GIT_BACKEND& operator=( const SCOPED_GIT_BACKEND& ) = delete;

    LIBGIT_BACKEND* Backend() const { return m_backend; }

private:
    GIT_BACKEND*    m_previous = GetGitBackend();
    LIBGIT_BACKEND* m_backend = nullptr;
};

#endif // QA_SCOPED_GIT_BACKEND_H
