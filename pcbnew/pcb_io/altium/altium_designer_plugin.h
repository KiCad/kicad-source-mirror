/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ALTIUM_DESIGNER_PLUGIN_H_
#define ALTIUM_DESIGNER_PLUGIN_H_

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

#include <map>
#include <memory>

class ALTIUM_COMPOUND_FILE;

class ALTIUM_DESIGNER_PLUGIN : public PCB_IO
{
public:
    // -----<PUBLIC PCB_IO API>--------------------------------------------------

    const wxString PluginName() const override { return wxT( "Altium Designer" ); }

    PLUGIN_FILE_DESC GetBoardFileDesc() const override
    {
        return PLUGIN_FILE_DESC( _HKI( "Altium Designer PCB files" ), { "PcbDoc" } );
    }

    PLUGIN_FILE_DESC GetFootprintLibDesc() const override
    {
        return PLUGIN_FILE_DESC( _HKI( "Altium PCB Library or Integrated Library" ),
                                 { "PcbLib", "IntLib" } );
    }

    PLUGIN_FILE_DESC GetFootprintFileDesc() const override { return GetFootprintLibDesc(); }

    bool CanReadBoard( const wxString& aFileName ) const override;
    bool CanReadFootprintLib( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const STRING_UTF8_MAP* aProperties, PROJECT* aProject = nullptr,
                      PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool aBestEfforts, const STRING_UTF8_MAP* aProperties = nullptr ) override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool              aKeepUUID = false,
                              const STRING_UTF8_MAP* aProperties = nullptr ) override;

    //bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName, const PROPERTIES* aProperties = nullptr );

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override { return false; }

    // -----</PUBLIC PCB_IO API>-------------------------------------------------

    ALTIUM_DESIGNER_PLUGIN();
    ~ALTIUM_DESIGNER_PLUGIN();

    static bool checkFileHeader( const wxString& aFileName );

private:
    const STRING_UTF8_MAP* m_props;
    BOARD*                 m_board;

    std::map<wxString, std::vector<std::unique_ptr<ALTIUM_COMPOUND_FILE>>> m_fplibFiles;

    void loadAltiumLibrary( const wxString& aLibraryPath );
};

#endif // ALTIUM_DESIGNER_PLUGIN_H_
