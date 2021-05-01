/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <eda_item.h>
#include "graphics_importer.h"
#include "graphics_import_plugin.h"

GRAPHICS_IMPORTER::GRAPHICS_IMPORTER()
{
    m_millimeterToIu = 1.0;
    m_lineWidth = DEFAULT_LINE_WIDTH_DFX;
    m_scale = 1.0;
    m_originalWidth = 0.0;
    m_originalHeight = 0.0;
}


bool GRAPHICS_IMPORTER::Load( const wxString &aFileName )
{
    m_items.clear();

    if( !m_plugin )
    {
        wxASSERT_MSG( false, "Plugin must be set before load." );
        return false;
    }

    m_plugin->SetImporter( this );

    return m_plugin->Load( aFileName );
}

bool GRAPHICS_IMPORTER::Import( double aScale )
{
    if( !m_plugin )
    {
        wxASSERT_MSG( false, "Plugin must be set before import." );
        return false;
    }

    SetScale( aScale );

    m_plugin->SetImporter( this );

    bool success = false;

    try
    {
        success = m_plugin->Import();
    }
    catch( const std::bad_alloc& )
    {
        // Memory exhaustion
        // TODO report back an error message
        return false;
    }

    return success;
}
