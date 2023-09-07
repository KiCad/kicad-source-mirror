/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef PCB_EASYEDA_PLUGIN_H_
#define PCB_EASYEDA_PLUGIN_H_

#include <io_mgr.h>
#include <footprint.h>


class EASYEDA_PLUGIN : public PLUGIN
{
public:
    const wxString PluginName() const override
    {
        return wxS( "EasyEDA (JLCEDA) Standard" );
    }

    PLUGIN_FILE_DESC GetBoardFileDesc() const override
    {
        return PLUGIN_FILE_DESC( _HKI( "EasyEDA (JLCEDA) Std files" ), { "json", "zip" } );
    }

    PLUGIN_FILE_DESC GetFootprintFileDesc() const override { return GetBoardFileDesc(); }

    PLUGIN_FILE_DESC GetFootprintLibDesc() const override { return GetBoardFileDesc(); }

    bool CanReadBoard( const wxString& aFileName ) const override;

    bool CanReadFootprint( const wxString& aFileName ) const override;

    bool CanReadFootprintLib( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const STRING_UTF8_MAP* aProperties = nullptr, PROJECT* aProject = nullptr,
                      PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
                             bool                   aBestEfforts,
                             const STRING_UTF8_MAP* aProperties = nullptr ) override;

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

    FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                              bool                   aKeepUUID = false,
                              const STRING_UTF8_MAP* aProperties = nullptr ) override;

    bool IsFootprintLibWritable( const wxString& aLibraryPath ) override { return false; }

    EASYEDA_PLUGIN();
    ~EASYEDA_PLUGIN();

private:
    const STRING_UTF8_MAP* m_props;
    BOARD*                 m_board;

    std::map<wxString, std::unique_ptr<FOOTPRINT>> m_loadedFootprints;
};


#endif // PCB_EASYEDA_PLUGIN_H_
