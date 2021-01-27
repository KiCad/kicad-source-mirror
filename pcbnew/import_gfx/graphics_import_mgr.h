/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef GRAPHICS_IMPORT_MGR_H
#define GRAPHICS_IMPORT_MGR_H

#include <memory>
#include <vector>

class GRAPHICS_IMPORT_PLUGIN;
class wxString;

/**
 * Manage vector graphics importers.
 */
class GRAPHICS_IMPORT_MGR
{
public:
    ///< List of handled file types.
    enum GFX_FILE_T
    {
        DXF,
        SVG
    };

    using TYPE_LIST = std::vector<GFX_FILE_T>;

    /**
     * Construct an import plugin manager, with a specified list of filetypes
     * that are not permitted (can be used for when some file type support
     * is not available due to config or other reasons)
     */
    GRAPHICS_IMPORT_MGR( const TYPE_LIST& aBlacklist );

    ///< Vector containing all GFX_FILE_T values that can be imported.
    TYPE_LIST GetImportableFileTypes() const
    {
        return m_importableTypes;
    }

    ///< Returns a plugin that handles a specific file extension.
    std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> GetPluginByExt( const wxString& aExtension ) const;

    ///< Returns a plugin instance for a specific file type.
    std::unique_ptr<GRAPHICS_IMPORT_PLUGIN> GetPlugin( GFX_FILE_T aType ) const;

private:
    TYPE_LIST m_importableTypes;
};

#endif /* GRAPHICS_IMPORT_MGR_H */
