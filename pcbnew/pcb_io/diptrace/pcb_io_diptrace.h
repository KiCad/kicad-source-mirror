/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file pcb_io_diptrace.h
 * @brief Pcbnew PCB_IO for DipTrace binary .dip board files.
 */

#ifndef PCB_IO_DIPTRACE_H_
#define PCB_IO_DIPTRACE_H_

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

class PCB_IO_DIPTRACE : public PCB_IO
{
public:
    PCB_IO_DIPTRACE();
    ~PCB_IO_DIPTRACE();

    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "DipTrace PCB files" ), { "dip" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        // No library description for this plugin
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr,
                      PROJECT* aProject = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override
    {
        // No support for libraries
        return 0;
    }
};

#endif    // PCB_IO_DIPTRACE_H_
