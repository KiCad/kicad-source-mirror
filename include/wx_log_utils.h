/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <wx/log.h>


/**
 * A scoped application of a wxLog target.
 *
 * While this class lives, all wxLog messages are sent to the given target.
 * When this class is destroyed, the previous target is restored.
 */
class SCOPED_WXLOG_TARGET
{
public:
    /**
     * @param aTarget the wxLog target to use for the lifetime of this object
     *                (null is allowed, and means no logging)
     */
    SCOPED_WXLOG_TARGET( wxLog* aTarget ) :
            m_logger( aTarget ),
            m_previous( wxLog::SetActiveTarget( m_logger ) )
    {
    }

    ~SCOPED_WXLOG_TARGET() { wxLog::SetActiveTarget( m_previous ); }

private:
    wxLog* m_logger;
    wxLog* m_previous;
};