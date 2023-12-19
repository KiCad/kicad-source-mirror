/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SOLIDWORKS_PCB_PLUGIN_H
#define KICAD_SOLIDWORKS_PCB_PLUGIN_H

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

class SOLIDWORKS_PCB_PLUGIN : public PCB_IO
{
public:
    const wxString PluginName() const override;

    PLUGIN_FILE_DESC GetBoardFileDesc() const override;

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const STRING_UTF8_MAP* aProperties, PROJECT* aProject = nullptr,
                      PROGRESS_REPORTER* aProgressReporter = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        return 0;
    }

    SOLIDWORKS_PCB_PLUGIN();
    ~SOLIDWORKS_PCB_PLUGIN();

private:
    const STRING_UTF8_MAP* m_props;
    BOARD*                 m_board;
};



#endif //KICAD_SOLIDWORKS_PCB_PLUGIN_H
