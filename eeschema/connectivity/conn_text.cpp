/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conn_text.h"

namespace SCH_CONNECTIVITY
{
INPUT_TEXT_SCOPE::INPUT_TEXT_SCOPE() :
        m_previous( s_active )
{
    if( !TEXT_ENVIRONMENT::Current() )
    {
        m_owned.emplace();
        m_scope.emplace( *m_owned );
    }

    s_active = true;
}

INPUT_TEXT_SCOPE::INPUT_TEXT_SCOPE( TEXT_ENVIRONMENT& aEnvironment ) :
        m_previous( s_active ),
        m_scope( std::in_place, aEnvironment )
{
    s_active = true;
}

INPUT_TEXT_SCOPE::~INPUT_TEXT_SCOPE()
{
    s_active = m_previous;
}
} // namespace SCH_CONNECTIVITY
