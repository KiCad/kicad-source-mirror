/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2017-2020 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <class_drawsegment.h>
#include <tool/tool_manager.h>
#include <tools/pcb_tool_base.h>
#include <netlist_reader/pcb_netlist.h>
#include <drc/drc.h>
#include <drc/drc_rule_parser.h>
#include <dialogs/panel_setup_rules.h>
#include <project.h>
#include <reporter.h>


DRC::DRC() :
        PCB_TOOL_BASE( "pcbnew.legacyDRCTool" ),
        m_editFrame( nullptr )
{
}


DRC::~DRC()
{
}


void DRC::Reset( RESET_REASON aReason )
{
    m_editFrame = getEditFrame<PCB_EDIT_FRAME>();
}


// JEY TODO: make DRC_TOOL's DRC_ENGINE be long-lived so it can be used for BOARD_CONNECTED_ITEM's
// GetClearance() and the retire this.

bool DRC::LoadRules()
{
    wxString   rulesFilepath = m_editFrame->Prj().AbsolutePath( "drc-rules" );
    wxFileName rulesFile( rulesFilepath );

    if( rulesFile.FileExists() )
    {
        m_rules.clear();

        FILE* fp = wxFopen( rulesFilepath, wxT( "rt" ) );

        if( fp )
        {
            try
            {
                DRC_RULES_PARSER parser( m_editFrame->GetBoard(), fp, rulesFilepath );
                parser.Parse( m_rules, &NULL_REPORTER::GetInstance() );
            }
            catch( PARSE_ERROR& pe )
            {
                // Don't leave possibly malformed stuff around for us to trip over
                m_rules.clear();

                wxSafeYield( m_editFrame );
                m_editFrame->ShowBoardSetupDialog( _( "Rules" ), pe.What(), ID_RULES_EDITOR,
                                                   pe.lineNumber, pe.byteIndex );

                return false;
            }
        }
    }

    std::reverse( std::begin( m_rules ), std::end( m_rules ) );

    BOARD_DESIGN_SETTINGS& bds = m_editFrame->GetBoard()->GetDesignSettings();
    bds.m_DRCRules = m_rules;

    return true;
}


