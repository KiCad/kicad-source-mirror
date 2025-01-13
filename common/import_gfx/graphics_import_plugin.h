/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GRAPHICS_IMPORT_PLUGIN_H
#define GRAPHICS_IMPORT_PLUGIN_H

#include <math/box2.h>
#include <wildcards_and_files_ext.h>
#include <wx/arrstr.h>

class GRAPHICS_IMPORTER;

/**
 * Interface for vector graphics import plugins.
 */
class GRAPHICS_IMPORT_PLUGIN
{
public:
    virtual ~GRAPHICS_IMPORT_PLUGIN() { }

    /**
     * Set the receiver of the imported shapes.
     */
    virtual void SetImporter( GRAPHICS_IMPORTER* aImporter ) { m_importer = aImporter; }

    /**
     * Return the plugin name.
     *
     * This string will be used as the description in the file dialog.
     */
    virtual const wxString GetName() const = 0;

    /**
     * Return a vector of the file extensions handled by this plugin.
     */
    virtual const std::vector<std::string> GetFileExtensions() const = 0;

    /**
     * Return a list of wildcards that contains the file extensions
     * handled by this plugin, separated with a semi-colon.
     */
    wxString GetWildcards() const
    {
        wxString ret;
        bool first = true;

        for( const auto& extension : GetFileExtensions() )
        {
            if( first )
                first = false;
            else
                ret += wxT( ";" );

            ret += wxT( "*." ) + formatWildcardExt( extension );
        }

        return ret;
    }

    /**
     * Load file for import.
     *
     * It is necessary to have the GRAPHICS_IMPORTER object set before.
     */
    virtual bool Load( const wxString& aFileName ) = 0;

    /**
     * Set memory buffer with content for import.
     *
     * It is necessary to have the GRAPHICS_IMPORTER object set before.
     */
    virtual bool LoadFromMemory( const wxMemoryBuffer& aMemBuffer ) = 0;

    /**
     * Return image height from original imported file.
     *
     * @return Original Image height in mm.
     */
    virtual double GetImageHeight() const = 0;

    /**
     * Return image width from original imported file.
     *
     * @return Original Image width in mm.
     */
    virtual double GetImageWidth() const = 0;

    /**
     * Return image bounding box from original imported file.
     *
     * @return Image bounding box.
     */
    virtual BOX2D GetImageBBox() const = 0;

    /**
     * Actually imports the file.
     *
     * It is necessary to have loaded the file beforehand.
     */
    virtual bool Import() = 0;

    virtual void SetLineWidthMM( double aLineWidth ) {}

    /**
     * Collect warning and error messages after loading/importing.
     *
     * @return the list of messages in one string. Each message ends by '\n'
     */
    const virtual wxString& GetMessages() const = 0;

    virtual void ReportMsg( const wxString& aMessage ) = 0;

protected:
    /// Importer used to create objects representing the imported shapes.
    GRAPHICS_IMPORTER* m_importer;
};


#endif /* GRAPHICS_IMPORT_PLUGIN_H */
