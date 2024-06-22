/*
* This program source code file is part of KiCad, a free EDA CAD application.
*
* Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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
* You should have received a copy of the GNU General Public License along
* with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SCH_IO_FUSION_H_
#define SCH_IO_FUSION_H_

#include <sch_io/sch_io.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/filename.h>
#include "sch_sheet_path.h"

class SCH_SHEET;
class SCHEMATIC;

/**
* A #SCH_IO derivation for loading Autodesk Fusion schematic files.
*
* As with all #SCH_IO objects there are no UI dependencies i.e. windowing calls allowed.
*/
class SCH_IO_FUSION : public SCH_IO
{
public:
   SCH_IO_FUSION();
   ~SCH_IO_FUSION();

   const IO_BASE::IO_FILE_DESC GetSchematicFileDesc() const override
   {
       return IO_BASE::IO_FILE_DESC( _HKI( "Autodesk Fusion schematic files" ), { "fsch", "f3z" } );
   }

   const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override
   {
       return IO_BASE::IO_FILE_DESC( wxEmptyString, { } );
   }

   bool CanReadSchematicFile( const wxString& aFileName ) const override;
   bool CanReadLibrary( const wxString& aFileName ) const override;

   int GetModifyHash() const override;

   SCH_SHEET* LoadSchematicFile( const wxString& aFileName, SCHEMATIC* aSchematic,
                                 SCH_SHEET*             aAppendToMe = nullptr,
                                 const STRING_UTF8_MAP* aProperties = nullptr ) override;

private:
    SCH_SHEET*  m_rootSheet;      ///< The root sheet of the schematic being loaded
    SCH_SHEET_PATH  m_sheetPath;  ///< The current sheet path of the schematic being loaded.
    wxString    m_version;        ///< Eagle file version.
    wxFileName  m_filename;
    SCHEMATIC*  m_schematic;      ///< Passed to Load(), the schematic object being loaded
};

#endif  // SCH_IO_FUSION_H_
