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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_IO_TARGET3001_H_
#define PCB_IO_TARGET3001_H_

#include <map>
#include <memory>

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>


/**
 * Imports TARGET 3001! project files (.T3001).  A project holds schematic and board together; this
 * plugin reads the board part.
 */
class PCB_IO_TARGET3001 : public PCB_IO
{
public:
    PCB_IO_TARGET3001();

    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "TARGET 3001! project files" ), { "T3001" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( wxEmptyString, {} );
    }

    bool CanReadBoard( const wxString& aFileName ) const override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override { return 0; }

    wxString GetImportedDesignRules() const override { return m_importedDesignRules; }

    std::vector<FOOTPRINT*> GetImportedCachedLibraryFootprints() override;

protected:
    void loadBoard( const wxString& aFileName, BOARD& aBoard, bool aIsNewLoad,
                    const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

private:
    wxString                                            m_importedDesignRules;
    std::map<wxString, std::unique_ptr<FOOTPRINT>>      m_libraryFootprints;   ///< by package name
};

#endif // PCB_IO_TARGET3001_H_
