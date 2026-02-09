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
 *
 * This file contains file format knowledge derived from the gEDA/pcb project:
 *
 *   gEDA/gaf  - Copyright (C) 1998-2010 Ales Hvezda
 *               Copyright (C) 1998-2016 gEDA Contributors
 *   Lepton EDA - Copyright (C) 2017-2024 Lepton EDA Contributors
 *
 * Both projects are licensed under the GNU General Public License v2 or later.
 * See https://github.com/lepton-eda/lepton-eda and
 *     https://github.com/rlutz/geda-gaf
 */

/**
 * @file pcb_io_geda.h
 * @brief Geda PCB file plugin definition file.
 */

#ifndef PCB_IO_GEDA_H_
#define PCB_IO_GEDA_H_

#include <map>
#include <string>
#include <vector>

#include <layer_ids.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

class BOARD;
class FOOTPRINT;
class GPCB_FPL_CACHE;
class LINE_READER;
class NETINFO_ITEM;

/**
 * A #PLUGIN derivation for saving and loading Geda PCB files.
 *
 * @note This class is not thread safe, but it is re-entrant multiple times in sequence.
 */
class PCB_IO_GEDA : public PCB_IO
{
public:
    // Board-level file support
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "gEDA / Lepton EDA PCB board file" ), { "pcb" } );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr,
                      PROJECT* aProject = nullptr ) override;

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    // Footprint library support
    const IO_BASE::IO_FILE_DESC GetLibraryFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "gEDA / Lepton EDA PCB footprint file" ), { "fp" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "gEDA / Lepton EDA PCB footprint library directory" ), {}, { "fp" },
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

    ~PCB_IO_GEDA() override;

private:
    void validateCache( const wxString& aLibraryPath, bool checkModified = true );

    const FOOTPRINT* getFootprint( const wxString& aLibraryPath, const wxString& aFootprintName,
                                   const std::map<std::string, UTF8>* aProperties, bool checkModified );

    void init( const std::map<std::string, UTF8>* aProperties );

    friend class GPCB_FPL_CACHE;

    // Board parsing helpers
    PCB_LAYER_ID mapLayer( int aGedaLayer, const wxString& aLayerName ) const;

    void parseVia( wxArrayString& aParameters, double aConvUnit );
    FOOTPRINT* parseElement( wxArrayString& aParameters, LINE_READER* aLineReader,
                             double aConvUnit );
    void parseLayer( wxArrayString& aParameters, LINE_READER* aLineReader, double aConvUnit );
    void parseNetList( LINE_READER* aLineReader );
    void parseParameters( wxArrayString& aParameterList, LINE_READER* aLineReader );
    bool testFlags( const wxString& aFlag, long aMask, const wxChar* aName );

protected:
    wxString               m_error;    ///< for throwing exceptions
    GPCB_FPL_CACHE*        m_cache;    ///< Footprint library cache.
    int                    m_ctl;
    LINE_READER*           m_reader;   ///< no ownership here.
    wxString               m_filename; ///< for saves only, name is in m_reader for loads

    // Board import state
    std::vector<FOOTPRINT*>                     m_cachedFootprints;
    std::map<wxString, NETINFO_ITEM*>           m_netMap;
    int                                         m_numCopperLayers;
};

#endif  // PCB_IO_GEDA_H_
