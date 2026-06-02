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
 * @file pcb_io_diptrace.cpp
 * @brief Pcbnew PCB_IO for DipTrace binary .dip board files.
 */

#include <diptrace/pcb_io_diptrace.h>
#include <diptrace/diptrace_pcb_parser.h>

#include <board.h>
#include <ki_exception.h>
#include <progress_reporter.h>

#include <wx/string.h>
#include <wx/wfstream.h>
#include <wx/file.h>
#include <wx/filename.h>


PCB_IO_DIPTRACE::PCB_IO_DIPTRACE() : PCB_IO( wxS( "DipTrace" ) )
{
}


PCB_IO_DIPTRACE::~PCB_IO_DIPTRACE()
{
}


bool PCB_IO_DIPTRACE::CanReadBoard( const wxString& aFileName ) const
{
    if( !PCB_IO::CanReadBoard( aFileName ) )
        return false;

    // DipTrace .dip files start with: 0x07 "DTBOARD" or legacy 0x0B "DTBOARDx.yy".
    // This is a binary format, so we read the file header directly rather
    // than using IO_UTILS::fileStartsWithPrefix (which is for text files).
    wxFileInputStream stream( aFileName );

    if( !stream.IsOk() || stream.GetLength() < 8 )
        return false;

    uint8_t header[12] = {};
    stream.Read( header, sizeof( header ) );

    if( stream.LastRead() < 8 )
        return false;

    if( header[0] != 0x07 && header[0] != 0x0B )
        return false;

    return ( header[1] == 'D' && header[2] == 'T' && header[3] == 'B'
             && header[4] == 'O' && header[5] == 'A' && header[6] == 'R'
             && header[7] == 'D' );
}


BOARD* PCB_IO_DIPTRACE::LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                                    const std::map<std::string, UTF8>* aProperties,
                                    PROJECT* aProject )
{
    m_props = aProperties;

    m_board = aAppendToMe ? aAppendToMe : new BOARD();

    // Give the filename to the board if it's new
    if( !aAppendToMe )
        m_board->SetFileName( aFileName );

    if( m_progressReporter )
    {
        m_progressReporter->Report( wxString::Format( _( "Loading %s..." ), aFileName ) );

        if( !m_progressReporter->KeepRefreshing() )
            THROW_IO_ERROR( _( "File import canceled by user." ) );
    }

    DIPTRACE::PCB_PARSER parser( aFileName, m_board );
    parser.Parse();

    // Emit a sidecar .kicad_dru for per-zone DipTrace properties KiCad cannot
    // store natively. Skip when appending so an existing project's rules survive.
    if( !aAppendToMe )
    {
        wxString rules = parser.GenerateDesignRules();

        if( !rules.IsEmpty() )
        {
            wxFileName drcPath( aFileName );
            drcPath.SetExt( wxT( "kicad_dru" ) );

            wxFile drcFile( drcPath.GetFullPath(), wxFile::write );

            if( drcFile.IsOpened() )
                drcFile.Write( rules );
        }
    }

    return m_board;
}
