/**
 * @file pcbnew/netlist_reader_kicad.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
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
#include <netlist_lexer.h>  // netlist_lexer is common to Eeschema and Pcbnew
#include <netlist_reader.h>

using namespace NL_T;

/**
 * Class PCB_PLOT_PARAMS_PARSER
 * is the parser class for PCB_PLOT_PARAMS.
 */
class NETLIST_READER_KICAD_PARSER : public NETLIST_LEXER
{
private:
    T token;
    NETLIST_READER * netlist_reader;

public:
    NETLIST_READER_KICAD_PARSER( FILE_LINE_READER* aReader, NETLIST_READER *aNetlistReader );

    /**
     * Function Parse
     * parse the full netlist
     */
    void Parse( BOARD * aBrd ) throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function ParseComp
     * parse the comp description like
     * (comp (ref P1)
     * (value DB25FEMELLE)
     * (footprint DB25FC)
     * (libsource (lib conn) (part DB25))
     * (sheetpath (names /) (tstamps /))
     * (tstamp 3256759C))
     */
    COMPONENT_INFO* ParseComp() throw( IO_ERROR, PARSE_ERROR );


    /**
     * Function ParseKicadLibpartList
     * Read the section "libparts" like:
     * (libparts
     *   (libpart (lib device) (part C)
     *     (description "Condensateur non polarise")
     *     (footprints
     *       (fp SM*)
     *       (fp C?)
     *       (fp C1-1))
     *     (fields
     *       (field (name Reference) C)
     *       (field (name Value) C))
     *     (pins
     *       (pin (num 1) (name ~) (type passive))
     *       (pin (num 2) (name ~) (type passive))))
     *
     *  And add the strings giving the footprint filter (subsection footprints)
     *  of the corresponding module info
     *  <p>This section is used by CvPcb, and is not useful in Pcbnew,
     *  therefore it it not always read </p>
     */
    void ParseKicadLibpartList() throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function ParseNet
     * Parses a section like
     * (net (code 20) (name /PC-A0)
     *  (node (ref BUS1) (pin 62))
     *  (node (ref U3) (pin 3))
     *  (node (ref U9) (pin M6)))
     *
     * and set the corresponfings pads netnames
     */
    void ParseNet( BOARD * aBrd ) throw( IO_ERROR, PARSE_ERROR );

    /**
     * Function SkipCurrent
     * Skip the current token level, i.e
     * search for the RIGHT parenthesis which closes the current description
     */
    void SkipCurrent() throw( IO_ERROR, PARSE_ERROR );

    // Useful for debug only:
    const char* getTokenName( T aTok )
    {
        return NETLIST_LEXER::TokenName( aTok );
    }
};


bool NETLIST_READER::ReadKicadNetList( FILE* aFile )
{
    BOARD * brd = m_pcbframe ? m_pcbframe->GetBoard() : NULL;

        // netlineReader dtor will close aFile
    FILE_LINE_READER netlineReader( aFile, m_netlistFullName );
    NETLIST_READER_KICAD_PARSER netlist_parser( &netlineReader, this );

    try
    {
        netlist_parser.Parse( brd );
    }
    catch( IO_ERROR& ioe )
    {
        ioe.errorText += '\n';
        ioe.errorText += _("Netlist error.");

        wxMessageBox( ioe.errorText );
        return false;
    }

    return true;
}



// NETLIST_READER_KICAD_PARSER
NETLIST_READER_KICAD_PARSER::NETLIST_READER_KICAD_PARSER( FILE_LINE_READER* aReader,
                                                          NETLIST_READER *aNetlistReader ) :
    NETLIST_LEXER( aReader )
{
    netlist_reader = aNetlistReader;
}

/**
 * Function SkipCurrent
 * Skip the current token level, i.e
 * search for the RIGHT parenthesis which closes the current description
 */
void NETLIST_READER_KICAD_PARSER::SkipCurrent() throw( IO_ERROR, PARSE_ERROR )
{
    int curr_level = 0;
    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            curr_level--;
        if( token == T_RIGHT )
        {
            curr_level++;
            if( curr_level > 0 )
                return;
        }
    }
}


void NETLIST_READER_KICAD_PARSER::Parse( BOARD * aBrd )
    throw( IO_ERROR, PARSE_ERROR )
{
    wxString text;
    while( ( token = NextTok() ) != T_EOF )
    {
        if( token == T_LEFT )
            token = NextTok();
        if( token == T_components )
        {
            // The section comp starts here.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();
                if( token == T_comp )
                {
                    // A comp section if found. Read it
                    COMPONENT_INFO* cmp_info = ParseComp();
                    netlist_reader->AddModuleInfo( cmp_info );
                }
            }
            if( netlist_reader->BuildModuleListOnlyOpt() )
                return; // at this point, the module list is read and built.
            // Load new footprints
            netlist_reader->InitializeModules();
            netlist_reader->TestFootprintsMatchingAndExchange();
        }

        if( token == T_nets )
        {
            // The section nets starts here.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();
                if( token == T_net )
                {
                    // A net section if found. Read it
                    ParseNet( aBrd );
                }
            }
        }

        if( token == T_libparts && netlist_reader->ReadLibpartSectionOpt() )
        {
            // The section libparts starts here.
            while( ( token = NextTok() ) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();
                if( token == T_libpart )
                {
                    // A libpart section if found. Read it
                    ParseKicadLibpartList();
                }
            }
        }
    }
}

void NETLIST_READER_KICAD_PARSER::ParseNet( BOARD * aBrd )
    throw( IO_ERROR, PARSE_ERROR )
{
    /* Parses a section like
     * (net (code 20) (name /PC-A0)
     *  (node (ref BUS1) (pin 62))
     *  (node (ref U3) (pin 3))
     *  (node (ref U9) (pin M6)))
     */

    wxString code;
    wxString name;
    wxString cmpref;
    wxString pin;
    D_PAD * pad = NULL;
    int nodecount = 0;
    // The token net was read, so the next data is (code <number>)
    while( (token = NextTok()) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();
        switch( token )
        {
        case T_code:
            NeedSYMBOLorNUMBER();
            code = FROM_UTF8( CurText() );
            NeedRIGHT();
        break;

        case T_name:
            NeedSYMBOLorNUMBER();
            name = FROM_UTF8( CurText() );
            NeedRIGHT();
            if( name.IsEmpty() )      // Give a dummy net name like N-000109
                name = wxT("N-00000") + code;
            break;

        case T_node:
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();
                switch( token )
                {
                case T_ref:
                    NeedSYMBOLorNUMBER();
                    cmpref = FROM_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                case T_pin:
                    NeedSYMBOLorNUMBER();
                    pin = FROM_UTF8( CurText() );
                    NeedRIGHT();
                    break;

                default:
                    SkipCurrent();
                    break;
                }
            }
            pad = netlist_reader->SetPadNetName( cmpref, pin, name );
            nodecount++;
            break;

        default:
            SkipCurrent();
            break;
        }
    }

    // When there is only one item in net, clear pad netname
    if( nodecount < 2 && pad )
        pad->SetNetname( wxEmptyString );
}


COMPONENT_INFO* NETLIST_READER_KICAD_PARSER::ParseComp()
    throw( IO_ERROR, PARSE_ERROR )
{
   /* Parses a section like
     * (comp (ref P1)
     * (value DB25FEMELLE)
     * (footprint DB25FC)
     * (libsource (lib conn) (part DB25))
     * (sheetpath (names /) (tstamps /))
     * (tstamp 3256759C))
     *
     * other fields (unused) are skipped
     * A component need a reference, value, foorprint name and a full time stamp
     * The full time stamp is the sheetpath time stamp + the component time stamp
     */
    wxString ref;
    wxString value;
    wxString footprint;
    wxString libpart;
    wxString pathtimestamp, timestamp;
    // The token comp was read, so the next data is (ref P1)

    while( (token = NextTok()) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();
        switch( token )
        {
        case T_ref:
            NeedSYMBOLorNUMBER();
            ref = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_value:
            NeedSYMBOLorNUMBER();
            value = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_footprint:
            NeedSYMBOLorNUMBER();
            footprint = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        case T_libsource:
            // Read libsource
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();
                if( token == T_part )
                {
                    NeedSYMBOLorNUMBER();
                    libpart = FROM_UTF8( CurText() );
                    NeedRIGHT();
                }
                else
                    SkipCurrent();
            }
            break;

        case T_sheetpath:
            while( ( token = NextTok() ) != T_tstamps );
            NeedSYMBOLorNUMBER();
            pathtimestamp = FROM_UTF8( CurText() );
            NeedRIGHT();
            NeedRIGHT();
            break;

        case T_tstamp:
            NeedSYMBOLorNUMBER();
            timestamp = FROM_UTF8( CurText() );
            NeedRIGHT();
            break;

        default:
            // Skip not used data (i.e all other tokens)
            SkipCurrent();
            break;
        }
    }
    pathtimestamp += timestamp;
    COMPONENT_INFO* cmp_info = new COMPONENT_INFO( footprint, ref, value, pathtimestamp );
    cmp_info->m_Libpart = libpart;

    return cmp_info;
}

/* Read the section "libparts" like:
 * (libparts
 *   (libpart (lib device) (part C)
 *     (description "Condensateur non polarise")
 *     (footprints
 *       (fp SM*)
 *       (fp C?)
 *       (fp C1-1))
 *     (fields
 *       (field (name Reference) C)
 *       (field (name Value) C))
 *     (pins
 *       (pin (num 1) (name ~) (type passive))
 *       (pin (num 2) (name ~) (type passive))))
 *
 *  And add the strings giving the footprint filter (subsection footprints)
 *  of the corresponding module info
 */
void NETLIST_READER_KICAD_PARSER::ParseKicadLibpartList() throw( IO_ERROR, PARSE_ERROR )
{
   /* Parses a section like
     *   (libpart (lib device) (part C)
     *     (description "Condensateur non polarise")
     *     (footprints
     *       (fp SM*)
     *       (fp C?)
     *       (fp C1-1))
     *     (fields
     *       (field (name Reference) C)
     *       (field (name Value) C))
     *     (pins
     *       (pin (num 1) (name ~) (type passive))
     *       (pin (num 2) (name ~) (type passive))))
     *
     * Currently footprints section/fp are read and data stored
     * other fields (unused) are skipped
     */
    wxString device;
    wxString filter;
    LIPBART_INFO* libpart_info = NULL;

    // The last token read was libpart, so read the next token
    while( (token = NextTok()) != T_RIGHT )
    {
        if( token == T_LEFT )
            token = NextTok();
        switch( token )
        {
        case T_part:
            NeedSYMBOLorNUMBER();
            device = FROM_UTF8( CurText() );
            NeedRIGHT();
            libpart_info = new LIPBART_INFO( device );
            netlist_reader->AddLibpartInfo( libpart_info );
            break;

        case T_footprints:
            // Ensure "(part C)" was already read
            if( libpart_info == NULL )
                Expecting( T_part );
            // Read all fp elements (footprint filter item)
            while( (token = NextTok()) != T_RIGHT )
            {
                if( token == T_LEFT )
                    token = NextTok();
                if( token != T_fp )
                    Expecting( T_fp );
                NeedSYMBOLorNUMBER();
                filter = FROM_UTF8( CurText() );
                NeedRIGHT();
                libpart_info->m_FootprintFilter.Add( filter );
            }
            break;

        default:
            // Skip not used data (i.e all other tokens)
            SkipCurrent();
            break;
        }
    }
}
