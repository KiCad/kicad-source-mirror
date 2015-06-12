/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <string>
#include <cassert>

#include <geometry/shape.h>
#include <geometry/shape_file_io.h>

SHAPE_FILE_IO::SHAPE_FILE_IO( const std::string& aFilename, bool aAppend )
{
    m_groupActive = false;
    m_file = fopen ( aFilename.c_str(), aAppend ? "ab" : "wb" );

    // fixme: exceptions
}

SHAPE_FILE_IO::~SHAPE_FILE_IO()
{
    if(!m_file)
        return;

    if (m_groupActive)
        fprintf(m_file,"endgroup\n");

    fclose(m_file);
}

SHAPE *SHAPE_FILE_IO::Read()
{
    assert(false);
    return NULL;
}

void SHAPE_FILE_IO::BeginGroup( const std::string aName )
{
    if(!m_file)
        return;
    fprintf(m_file, "group %s\n", aName.c_str());
    m_groupActive = true;
}

void SHAPE_FILE_IO::EndGroup()
{
    if(!m_file || !m_groupActive)
        return;

    fprintf(m_file, "endgroup\n");
    m_groupActive = false;
}

void SHAPE_FILE_IO::Write( const SHAPE *aShape, const std::string aName )
{
    printf("write %p f %p\n", aShape, m_file );

    if(!m_file)
        return;

    if(!m_groupActive)
        fprintf(m_file,"group default\n");

    std::string sh = aShape->Format();

    fprintf(m_file,"shape %d %s %s\n", aShape->Type(), aName.c_str(), sh.c_str() );
    fflush(m_file);
}