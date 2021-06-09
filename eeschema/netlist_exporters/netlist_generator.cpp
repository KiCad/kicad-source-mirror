/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <common.h>     // for ProcessExecute
#include <kicad_string.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <reporter.h>
#include <confirm.h>
#include <kiway.h>
#include <erc.h>

#include <netlist.h>
#include <netlist_exporter_base.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_pspice.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_xml.h>


bool SCH_EDIT_FRAME::WriteNetListFile( int aFormat, const wxString& aFullFileName,
                                       unsigned aNetlistOptions, REPORTER* aReporter )
{
    // Ensure all power symbols have a valid reference
    Schematic().GetSheets().AnnotatePowerSymbols();

    // Ensure the netlist data is up to date:
    RecalculateConnections( NO_CLEANUP );

    if( !ReadyToNetlist( _( "Exporting netlist requires a fully annotated schematic." ) ) )
        return false;

    bool res = true;
    bool executeCommandLine = false;

    wxString    fileName = aFullFileName;

    NETLIST_EXPORTER_BASE *helper;

    SCHEMATIC* sch = &Schematic();

    switch( aFormat )
    {
    case NET_TYPE_PCBNEW:
        helper = new NETLIST_EXPORTER_KICAD( sch );
        break;

    case NET_TYPE_ORCADPCB2:
        helper = new NETLIST_EXPORTER_ORCADPCB2( sch );
        break;

    case NET_TYPE_CADSTAR:
        helper = new NETLIST_EXPORTER_CADSTAR( sch );
        break;

    case NET_TYPE_SPICE:
        helper = new NETLIST_EXPORTER_PSPICE( sch );
        break;

    default:
    {
        wxFileName  tmpFile = fileName;
        tmpFile.SetExt( GENERIC_INTERMEDIATE_NETLIST_EXT );
        fileName = tmpFile.GetFullPath();

        helper = new NETLIST_EXPORTER_XML( sch );
        executeCommandLine = true;
    }
        break;
    }

    res = helper->WriteNetlist( fileName, aNetlistOptions );
    delete helper;

    // If user provided a plugin command line, execute it.
    if( executeCommandLine && res && !m_netListerCommand.IsEmpty() )
    {
        wxString prj_dir = Prj().GetProjectPath();

        // strip trailing '/'
        prj_dir = prj_dir.SubString( 0, prj_dir.Len() - 2 );

        // build full command line from user's format string.
        // For instance, "xsltproc -o %O /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl %I"
        // becomes, after the user selects /tmp/s1.net as the output file from the file dialog:
        // "xsltproc -o /tmp/s1.net /usr/local/lib/kicad/plugins/netlist_form_pads-pcb.xsl /tmp/s1.xml"
        wxString commandLine = NETLIST_EXPORTER_BASE::MakeCommandLine( m_netListerCommand,
                                                                       fileName, aFullFileName,
                                                                       prj_dir );

        if( aReporter )
        {
            wxArrayString output, errors;
            int diag = wxExecute( commandLine, output, errors, m_exec_flags );

            wxString msg;

            msg << _( "Run command:" ) << wxT( "\n" ) << commandLine << wxT( "\n\n" );
            aReporter->ReportHead( msg, RPT_SEVERITY_ACTION );

            if( diag != 0 )
            {
                msg.Printf( _( "Command error. Return code %d." ), diag );
                aReporter->ReportTail( msg, RPT_SEVERITY_ERROR );
            }
            else
            {
                aReporter->ReportTail( _( "Success." ), RPT_SEVERITY_INFO );
            }

            if( output.GetCount() )
            {
                msg.Empty();
                msg << wxT( "\n" ) << _( "Info messages:" ) << wxT( "\n" );
                aReporter->Report( msg, RPT_SEVERITY_INFO );

                for( unsigned ii = 0; ii < output.GetCount(); ii++ )
                    aReporter->Report( output[ii] + wxT( "\n" ), RPT_SEVERITY_INFO );
            }

            if( errors.GetCount() )
            {
                msg.Empty();
                msg << wxT("\n") << _( "Error messages:" ) << wxT( "\n" );
                aReporter->Report( msg, RPT_SEVERITY_INFO );

                for( unsigned ii = 0; ii < errors.GetCount(); ii++ )
                    aReporter->Report( errors[ii] + wxT( "\n" ), RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            ProcessExecute( commandLine, m_exec_flags );
        }

        DefaultExecFlags(); // Reset flags to default after executing
    }

    return res;
}


bool SCH_EDIT_FRAME::ReadyToNetlist( const wxString& aAnnotateMessage )
{
    // Ensure all power symbols have a valid reference
    Schematic().GetSheets().AnnotatePowerSymbols();

    // Components must be annotated
    if( CheckAnnotate( []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* ) {} ) )
    {
        // Schematic must be annotated: call Annotate dialog and tell the user why.
        ModalAnnotate( aAnnotateMessage );

        if( CheckAnnotate( []( ERCE_T, const wxString&, SCH_REFERENCE*, SCH_REFERENCE* ) {} ) )
            return false;
    }

    // Test duplicate sheet names:
    ERC_TESTER erc( &Schematic() );

    if( erc.TestDuplicateSheetNames( false ) > 0 )
    {
        if( !IsOK( this, _( "Error: duplicate sheet names. Continue?" ) ) )
            return false;
    }

    return true;
}


void SCH_EDIT_FRAME::sendNetlistToCvpcb()
{
    std::string packet;

    {
        NETLIST_EXPORTER_KICAD exporter( &Schematic() );
        STRING_FORMATTER       formatter;

        // @todo : trim GNL_ALL down to minimum for CVPCB
        exporter.Format( &formatter, GNL_ALL );

        packet = formatter.GetString();  // an abbreviated "kicad" (s-expr) netlist

        // NETLIST_EXPORTER_KICAD must go out of scope so it can clean up things like the
        // current sheet setting before sending expressmail
    }

    Kiway().ExpressMail( FRAME_CVPCB, MAIL_EESCHEMA_NETLIST, packet, this );
}

