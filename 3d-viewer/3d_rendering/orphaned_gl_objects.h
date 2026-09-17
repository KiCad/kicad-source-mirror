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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ORPHANED_GL_OBJECTS_H
#define ORPHANED_GL_OBJECTS_H

/**
 * Mark a scope whose GL objects have lost the context that owns them.
 *
 * Deleting a GL object requires its own context to be current.  A canvas whose native window
 * is already gone cannot make its context current, and every 3D canvas owns an unshared
 * context, so a delete issued then would be executed against whichever context is current and
 * would destroy an unrelated object carrying the same name.  Names are released when the
 * owning context is destroyed, so skipping the deletes leaks nothing.
 *
 * Only the thread holding the context issues GL commands, so no locking is needed here.
 */
class ORPHANED_GL_OBJECTS
{
public:
    ORPHANED_GL_OBJECTS() :
            m_wasOrphaned( s_orphaned )
    {
        s_orphaned = true;
    }

    ~ORPHANED_GL_OBJECTS()
    {
        s_orphaned = m_wasOrphaned;
    }

    /**
     * @return True when GL objects must be released without deleting them.
     */
    static bool Active()
    {
        return s_orphaned;
    }

private:
    bool               m_wasOrphaned;
    static inline bool s_orphaned = false;
};

#endif // ORPHANED_GL_OBJECTS_H
