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

#ifndef GRAPHICS_IMPORT_PLUGIN_H
#define GRAPHICS_IMPORT_PLUGIN_H

#include <wx/arrstr.h>

class GRAPHICS_IMPORTER;

/**
 * @brief Interface for vector graphics import plugins.
 */
class GRAPHICS_IMPORT_PLUGIN
{
public:
    virtual ~GRAPHICS_IMPORT_PLUGIN()
    {
    }

    /**
     * @brief Sets the receiver of the imported shapes.
     */
    void SetImporter( GRAPHICS_IMPORTER* aImporter )
    {
        m_importer = aImporter;
    }

    /**
     * @breif Returns the plugin name.
     *
     * This string will be used as the description in the file dialog.
     */
    virtual const wxString GetName() const = 0;

    /**
     * @brief Returns a string array of the file extensions handled by this plugin.
     */
    virtual const wxArrayString GetFileExtensions() const = 0;

    /**
     * @brief Returns a list of wildcards that contains the file extensions
     * handled by this plugin, separated with a coma.
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
                ret += ", ";

            ret += "*." + extension;
        }

        return ret;
    }

    /**
     * @brief Loads file for import.
     *
     * It is necessary to have the GRAPHICS_IMPORTER object set before.
     */
    virtual bool Load( const wxString& aFileName ) = 0;

    /**
     * @brief Return image height from original imported file.
     *
     * @return Original Image height in mm.
     */
    virtual double GetImageHeight() const = 0;

    /**
     * @brief Return image width from original imported file.
     *
     * @return Original Image width in mm.
     */
    virtual double GetImageWidth() const = 0;

    /**
     * @brief Actually imports the file.
     *
     * It is necessary to have loaded the file beforehand.
     */
    virtual bool Import() = 0;

    /**
     * @brief collect warning and error messages after loading/importing.
     * @return the list of messages in one string. Each message ends by '\n'
     */
    const virtual std::string& GetMessages() const = 0;



protected:
    ///> Importer used to create objects representing the imported shapes.
    GRAPHICS_IMPORTER* m_importer;
};


#endif /* GRAPHICS_IMPORT_PLUGIN_H */

