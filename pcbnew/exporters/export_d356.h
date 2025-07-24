/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Jon Evans <jon@craftyjon.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/string.h>

class BOARD;
class wxWindow;


/* Structure for holding the D-356 record fields.
 * Useful because 356A (when implemented) must be sorted before outputting it */
struct D356_RECORD
{
    bool       smd;
    bool       hole;
    wxString   netname;
    wxString   refdes;
    wxString   pin;
    bool       midpoint;
    int        drill;
    bool       mechanical;
    int        access;      // Access 0 is 'both sides'
    int        soldermask;
    // All these in PCB units, will be output in decimils
    int        x_location;
    int        y_location;
    int        x_size;
    int        y_size;
    int        rotation;
};


/**
 * Wrapper to expose an API for writing IPC-D356 files
 */
class IPC356D_WRITER
{
public:
    /**
     * Constructs an IPC-356D file writer
     * @param aPcb is the board to extract a netlist from
     * @param aParent will be used as the parent for any warning dialogs
     */
    IPC356D_WRITER( BOARD* aPcb ) :
            m_pcb( aPcb ),
            m_doNotExportUnconnectedPads( false )
    {}

    virtual ~IPC356D_WRITER() {}

    /**
     * Generates and writes the netlist to a given path
     * @param aFilename is the full path and name of the output file
     * @return true on success
     */
    bool Write( const wxString& aFilename );

    /**
     * Sets whether unconnected pads should be exported
     * @param aDoNotExportUnconnectedPads if true, unconnected pads will not be exported
     */
    void SetDoNotExportUnconnectedPads( bool aDoNotExportUnconnectedPads )
    {
        m_doNotExportUnconnectedPads = aDoNotExportUnconnectedPads;
    }

private:
    BOARD* m_pcb;

    /// Writes a list of records to the given output stream
    void write_D356_records( std::vector<D356_RECORD> &aRecords, FILE* aFile );

    void build_pad_testpoints( BOARD *aPcb, std::vector <D356_RECORD>& aRecords );

    bool m_doNotExportUnconnectedPads;
};
