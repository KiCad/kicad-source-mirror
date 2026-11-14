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

#pragma once

#include <kicommon.h>
#include <vector>

/**
 * Interface implemented by a GAL canvas that wants to participate in cross-window cursor
 * ownership broadcasting. See #EDA_CURSOR_BROADCASTER.
 */
class KICOMMON_API EDA_CURSOR_OWNER
{
public:
    virtual ~EDA_CURSOR_OWNER() = default;

    /**
     * @return true if this owner currently believes the pointer is inside its canvas.
     */
    virtual bool IsCursorInCanvas() const = 0;

    /**
     * Give up canvas ownership, because the pointer has been shown to be somewhere else.
     */
    virtual void HideCursorInCanvas() = 0;
};

/**
 * Process-wide registry of every live #EDA_CURSOR_OWNER: on a genuine enter, tells every
 * other owner the pointer left them.
 *
 * Must live in shared `kicommon`, not static `common` -- static duplicates one copy per
 * module (kicad, eeschema.kiface, pcbnew.kiface, ...), breaking the process-wide part.
 */
class KICOMMON_API EDA_CURSOR_BROADCASTER
{
public:
    static EDA_CURSOR_BROADCASTER& Instance();

    void Register( EDA_CURSOR_OWNER* aOwner );
    void Unregister( EDA_CURSOR_OWNER* aOwner );

    /// Hide the cursor in every registered owner other than \a aEnteredOwner.
    void NotifyCursorEntered( EDA_CURSOR_OWNER* aEnteredOwner );

private:
    EDA_CURSOR_BROADCASTER() = default;

    std::vector<EDA_CURSOR_OWNER*> m_owners;
};
