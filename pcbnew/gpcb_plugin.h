/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2017 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file gpcb_plugin.cpp
 * @brief Geda PCB file plugin definition file.
 */

#ifndef _GPCB_PLUGIN_H_
#define _GPCB_PLUGIN_H_

#include <io_mgr.h>
#include <string>


class GPCB_FPL_CACHE;


/**
 * Class GPCB_PLUGIN
 * is a PLUGIN derivation for saving and loading Geda PCB files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 * @note Currently only reading GPCB footprint files is implemented.
 */
class GPCB_PLUGIN : public PLUGIN
{
    friend class GPCB_FPL_CACHE;

public:

    //-----<PLUGIN API>---------------------------------------------------------

    const wxString PluginName() const override
    {
        return wxT( "Geda PCB" );
    }

    const wxString GetFileExtension() const override
    {
        return wxT( "fp" );
    }

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = NULL) override;

    MODULE* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
            const PROPERTIES* aProperties = NULL ) override;

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
            const PROPERTIES* aProperties = NULL ) override;

    bool FootprintLibDelete( const wxString& aLibraryPath,
                             const PROPERTIES* aProperties = NULL ) override;

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override;

    //-----</PLUGIN API>--------------------------------------------------------

    GPCB_PLUGIN();

    GPCB_PLUGIN( int aControlFlags );

    ~GPCB_PLUGIN();

protected:

    wxString          m_error;        ///< for throwing exceptions
    const PROPERTIES* m_props;        ///< passed via Save() or Load(), no ownership, may be NULL.
    GPCB_FPL_CACHE*   m_cache;        ///< Footprint library cache.
    int               m_ctl;
    LINE_READER*      m_reader;       ///< no ownership here.
    wxString          m_filename;     ///< for saves only, name is in m_reader for loads

private:
    /// we only cache one footprint library for now, this determines which one.
    void cacheLib( const wxString& aLibraryPath, const wxString& aFootprintName = wxEmptyString );

    void init( const PROPERTIES* aProperties );
};

#endif  // _GPCB_PLUGIN_H_
