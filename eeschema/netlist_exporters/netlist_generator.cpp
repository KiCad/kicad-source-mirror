/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <advanced_config.h>
#include <common.h>     // for ProcessExecute
#include <string_utils.h>
#include <gestfich.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <reporter.h>
#include <confirm.h>
#include <kiway.h>
#include <erc/erc.h>
#include <richio.h>

#include <netlist.h>
#include <netlist_exporter_base.h>
#include <netlist_exporter_orcadpcb2.h>
#include <netlist_exporter_cadstar.h>
#include <netlist_exporter_spice.h>
#include <netlist_exporter_spice_model.h>
#include <netlist_exporter_kicad.h>
#include <netlist_exporter_allegro.h>
#include <netlist_exporter_pads.h>
#include <netlist_exporter_xml.h>


bool SCH_EDIT_FRAME::WriteNetListFile( int aFormat, const wxString& aFullFileName,
                                       unsigned aNetlistOptions, REPORTER* aReporter )
{
    // Ensure all power symbols have a valid reference
    Schematic().Hierarchy().AnnotatePowerSymbols();

    if( !ReadyToNetlist( _( "Exporting netlist requires a fully annotated schematic." ) ) )
        return false;

    // If we are using the new connectivity, make sure that we do a full-rebuild
    if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
        RecalculateConnections( nullptr, GLOBAL_CLEANUP );

    bool res = true;
    bool executeCommandLine = false;

    wxString    fileName = aFullFileName;

    NETLIST_EXPORTER_BASE *helper = nullptr;

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
        helper = new NETLIST_EXPORTER_SPICE( sch );
        break;

    case NET_TYPE_SPICE_MODEL:
        helper = new NETLIST_EXPORTER_SPICE_MODEL( sch );
        break;

    case NET_TYPE_ALLEGRO:
        helper = new NETLIST_EXPORTER_ALLEGRO( sch );
        break;

    case NET_TYPE_PADS:
        helper = new NETLIST_EXPORTER_PADS( sch );
        break;

    case NET_TYPE_BOM:
        // When generating the BOM, we have a bare filename so don't strip
        // the extension or you might string a '.' from the middle of the filename
        fileName += wxT( "." GENERIC_INTERMEDIATE_NETLIST_EXT );

        helper = new NETLIST_EXPORTER_XML( sch );
        executeCommandLine = true;
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

    NULL_REPORTER devnull;

    if( aReporter )
        res = helper->WriteNetlist( fileName, aNetlistOptions, *aReporter );
    else
        res = helper->WriteNetlist( fileName, aNetlistOptions, devnull );

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
            int           diag = wxExecute( commandLine, output, errors, m_exec_flags );
            wxString      msg;

            aReporter->ReportHead( commandLine, RPT_SEVERITY_ACTION );

            if( diag != 0 )
            {
                msg.Printf( _( "Command error. Return code %d." ), diag );
                res = false;
                aReporter->ReportTail( msg, RPT_SEVERITY_ERROR );
            }
            else
            {
                aReporter->ReportTail( _( "Done." ), RPT_SEVERITY_INFO );
            }

            if( output.GetCount() )
            {
                for( unsigned ii = 0; ii < output.GetCount(); ii++ )
                    aReporter->Report( output[ii], RPT_SEVERITY_INFO );
            }

            if( errors.GetCount() )
            {
                for( unsigned ii = 0; ii < errors.GetCount(); ii++ )
                    aReporter->Report( errors[ii], RPT_SEVERITY_ERROR );
            }
        }
        else
        {
            int diag = wxExecute( commandLine, m_exec_flags );
            if( diag != 0 )
                res = false;
        }

        DefaultExecFlags(); // Reset flags to default after executing
    }

    return res;
}


bool SCH_EDIT_FRAME::ReadyToNetlist( const wxString& aAnnotateMessage )
{
    // Ensure all power symbols have a valid reference
    Schematic().Hierarchy().AnnotatePowerSymbols();

    // Symbols must be annotated
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
        exporter.Format( &formatter, GNL_ALL | GNL_OPT_KICAD );

        packet = formatter.GetString();  // an abbreviated "kicad" (s-expr) netlist

        // NETLIST_EXPORTER_KICAD must go out of scope so it can clean up things like the
        // current sheet setting before sending expressmail
    }

    Kiway().ExpressMail( FRAME_CVPCB, MAIL_EESCHEMA_NETLIST, packet, this );
}

