/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2020 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcb_io_geda.cpp
 * @brief Geda PCB file plugin definition file.
 */

#ifndef PCB_IO_GEDA_H_
#define PCB_IO_GEDA_H_

#include <string>

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

class GPCB_FPL_CACHE;
class LINE_READER;

/**
 * A #PLUGIN derivation for saving and loading Geda PCB files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 * @note Currently only reading GPCB footprint files is implemented.
 */
class PCB_IO_GEDA : public PCB_IO
{
public:
    const IO_BASE::IO_FILE_DESC GetLibraryFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "gEDA PCB footprint file" ), { "fp" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "gEDA PCB footprint library directory" ), {}, { "fp" },
                                      false );
    }

    FOOTPRINT* ImportFootprint( const wxString& aFootprintPath, wxString& aFootprintNameOut,
                                const std::map<std::string, UTF8>* aProperties ) override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts,
                             const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool  aKeepUUID = false,
                              const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
                          const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    bool DeleteLibrary( const wxString& aLibraryPath,
                        const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool IsLibraryWritable( const wxString& aLibraryPath ) override;

    //-----</PLUGIN API>--------------------------------------------------------

    PCB_IO_GEDA();

    PCB_IO_GEDA( int aControlFlags );

    ~PCB_IO_GEDA();

private:
    void validateCache( const wxString& aLibraryPath, bool checkModified = true );

    const FOOTPRINT* getFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                                   const std::map<std::string, UTF8>* aProperties, bool checkModified );

    void init( const std::map<std::string, UTF8>* aProperties );

    friend class GPCB_FPL_CACHE;

protected:
    wxString               m_error;    ///< for throwing exceptions
    GPCB_FPL_CACHE*        m_cache;    ///< Footprint library cache.
    int                    m_ctl;
    LINE_READER*           m_reader;   ///< no ownership here.
    wxString               m_filename; ///< for saves only, name is in m_reader for loads
};

#endif  // PCB_IO_GEDA_H_
