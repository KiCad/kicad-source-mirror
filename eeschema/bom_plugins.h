/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef BOM_GENERATOR_HANDLERS_H
#define BOM_GENERATOR_HANDLERS_H

#include <wx/arrstr.h>
#include <wx/file.h>
#include <wx/filename.h>

#include <memory>

extern const wxChar BOM_TRACE[];

/**
 * Bill of material output generator.
 *
 * A Material output generator is an external application called by Eeschema to create
 * a BOM from our intermediate XML netlist.  A generator can be a script or an executable
 * that can read the intermediate XML netlist file and generates a output (the BOM file)
 */
class BOM_GENERATOR_HANDLER
{
public:
    /**
     * @param aFile is path to the plugin file.
     */
    BOM_GENERATOR_HANDLER( const wxString& aFile );

    /**
     * Return true if the plugin is ready to work, i.e. if the plugin file is found and readable.
     */
    bool IsOk() { return m_isOk; }

    /**
     * Return true if a file name matches a recognized plugin format.
     *
     * @param aFile is path to the plugin file.
     */
    static bool IsValidGenerator( const wxString& aFile );

    /**
     * Return plugin description stored in the plugin header file (if available).
     */
    const wxString& GetInfo() const
    {
        return m_info;
    }

    /**
     * Return the file name of the plugin.
     */
    const wxFileName& GetFile() const
    {
        return m_file;
    }

    wxString GetStoredPath() const { return m_storedPath; }

    /**
     * Returns the calculated path to the plugin: if the path is already absolute and exists,
     * just return it.  Otherwise if the path is just a filename, look for that file in the user
     * and system plugin directories and return the first one found. If neither is found, just
     * return m_file.
     *
     * @return the full path to the plugin
     */
    wxFileName FindFilePath() const;

    /**
     * Return the customisable plugin name.
     */
    const wxString& GetName() const
    {
        return m_name;
    }

    /**
     * Set the customisable plugin name.
     *
     * @param aName is the new name.
     */
    void SetName( const wxString& aName )
    {
        m_name = aName;
    }

    /**
     * Return the command to execute the plugin.
     */
    const wxString& GetCommand() const
    {
        return m_cmd;
    }

    /**
     * Set the command to execute the plugin.
     */
    void SetCommand( const wxString& aCommand )
    {
        m_cmd = aCommand;
    }

    /**
     * Accessor to array of options.
     */
    wxArrayString& Options()
    {
        return m_options;
    }

protected:
    /**
     * Read the plugin file header.
     *
     * @param aEndSection is a string marking end of the header.
     */
    wxString readHeader( const wxString& aEndSection );

    /**
     * Extracts the output BOM file's extension, including the '.', from the
     * plugin file header. If the output extension cannot be determined from
     * the plugin header, returns wxEmptyString.
     * @param aHeader is the plugin file's header, as returned by readHeader()
     **/
    static wxString getOutputExtension( const wxString& aHeader );

    ///< true if the plugin is working (i.e. if the plugin file exists and was read
    bool m_isOk;

    ///< Path to the plugin
    wxFileName m_file;

    ///< Path to the plugin stored in config (can be absolute or just a filename)
    const wxString m_storedPath;

    ///< User customisable name
    wxString m_name;

    ///< Command to execute the plugin
    wxString m_cmd;

    ///< Description of the plugin (normally from the plugin header)
    wxString m_info;

    ///< Plugin specific options
    wxArrayString m_options;
};

#endif /* BOM_GENERATOR_HANDLERS_H */
