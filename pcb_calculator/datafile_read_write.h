#ifndef DATAFILE_READ_WRITE_H_
#define DATAFILE_READ_WRITE_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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

#include <wx/wx.h>
#include <pcb_calculator_datafile_lexer.h>
#include <base_struct.h>

class PCB_CALCULATOR_DATAFILE_PARSER;

/**
 * Class PCB_CALCULATOR_DATAFILE
 * handles data to calculate regulators parameters
 */
class PCB_CALCULATOR_DATAFILE
{
    friend class PCB_CALCULATOR_DATAFILE_PARSER;
protected:
     REGULATOR_LIST * m_list;

public:
    PCB_CALCULATOR_DATAFILE( REGULATOR_LIST * aList );

    int         WriteHeader( OUTPUTFORMATTER* aFormatter ) const throw( IO_ERROR );
    void        Format( OUTPUTFORMATTER* aFormatter, int aNestLevel ) const throw( IO_ERROR );
    void        Parse( PCB_CALCULATOR_DATAFILE_PARSER* aParser ) throw( IO_ERROR, PARSE_ERROR );
};


/**
 * Class PCB_CALCULATOR_DATAFILE_PARSER
 * is the parser class for PCB_CALCULATOR_DATAFILE.
 */
class PCB_CALCULATOR_DATAFILE_PARSER : public PCB_CALCULATOR_DATAFILE_LEXER
{
public:
    PCB_CALCULATOR_DATAFILE_PARSER( LINE_READER* aReader );
    PCB_CALCULATOR_DATAFILE_PARSER( char* aLine, wxString aSource );
    LINE_READER* GetReader() { return reader; };
    void Parse( PCB_CALCULATOR_DATAFILE* aDataList ) throw( IO_ERROR, PARSE_ERROR );
    void ParseRegulatorDescr( PCB_CALCULATOR_DATAFILE* aDataList ) throw( IO_ERROR, PARSE_ERROR );
};

#endif // PDATAFILE_READ_WRITE_H_
