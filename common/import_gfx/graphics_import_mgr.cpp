/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "graphics_import_mgr.h"

#include <eda_item.h>
#include "dxf_import_plugin.h"
#include "svg_import_plugin.h"

#include <wx/regex.h>

std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> GRAPHICS_IMPORT_MGR::GetPlugin( GFX_FILE_T aType ) const
{
    std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> ret;

    switch( aType )
    {
    case DXF: ret = std::make_unique<DXF_IMPORT_PLUGIN>();             break;
    case SVG: ret = std::make_unique<SVG_IMPORT_PLUGIN>();             break;
    default:  throw std::runtime_error( "Unhandled graphics format" ); break;
    }

    return ret;
}


std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> GRAPHICS_IMPORT_MGR::GetPluginByExt(
        const wxString& aExtension ) const
{
    for( GRAPHICS_IMPORT_MGR::GFX_FILE_T fileType : GetImportableFileTypes() )
    {
        std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> plugin = GetPlugin( fileType );
        const std::vector<std::string>&         fileExtensions = plugin->GetFileExtensions();

        if( compareFileExtensions( aExtension.ToStdString(), fileExtensions ) )
            return plugin;
    }

    return {};
}
