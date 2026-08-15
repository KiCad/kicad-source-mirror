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

#include <table_io.h>

#include <clipboard.h>
#include <eda_base_frame.h>
#include <io/csv.h>
#include <kiplatform/ui.h>
#include <richio.h>
#include <wildcards_and_files_ext.h>

#include <wx/filedlg.h>
#include <wx/intl.h>
#include <wx/wfstream.h>


std::optional<std::vector<std::vector<wxString>>> ReadTableFromFileOrClipboard( EDA_BASE_FRAME& aFrame, bool aFromFile )
{
    wxString path;

    if( aFromFile )
    {
        wxFileDialog dlg( &aFrame, _( "Select data file" ), "", "", FILEEXT::CsvTsvFileWildcard(),
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

        if( dlg.ShowModal() == wxID_CANCEL )
            return std::nullopt;

        path = dlg.GetPath();
    }

    std::vector<std::vector<wxString>> table;
    bool                               ok = false;

    if( !path.IsEmpty() )
        ok = AutoDecodeCSV( SafeReadFile( path, "r" ), table );
    else
        ok = GetTabularDataFromClipboard( table );

    if( !ok )
        return std::nullopt;

    return table;
}


void WriteTableToFileOrClipboard( const wxString& aToFile, const std::vector<std::vector<wxString>>& aTable )
{
    if( !aToFile.IsEmpty() )
    {
        wxFileOutputStream os( aToFile );
        CSV_WRITER         writer( os );
        writer.WriteLines( aTable );
    }
    else
    {
        SaveTabularDataToClipboard( aTable );
    }
}
