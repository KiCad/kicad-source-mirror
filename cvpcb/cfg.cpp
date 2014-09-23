/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jean-pierre.charras
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file cfg.cpp
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <fp_lib_table.h>
#include <id.h>
#include <common.h>
#include <gestfich.h>
#include <config_params.h>
#include <wildcards_and_files_ext.h>
#include <fp_lib_table.h>
#include <confirm.h>

#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <class_DisplayFootprintsFrame.h>


PARAM_CFG_ARRAY& CVPCB_MAINFRAME::GetProjectFileParameters()
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_BASE( GROUP_PCB_LIBS, PARAM_COMMAND_ERASE ) );

    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST(
        wxT( "EquName" ), &m_AliasLibNames, GROUP_CVP_EQU ) );

    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING(
        wxT( "NetIExt" ), &m_NetlistFileExtension ) );

    return m_projectFileParams;
}


void CVPCB_MAINFRAME::LoadProjectFile()
{
    PROJECT&    prj = Prj();

    m_ModuleLibNames.Clear();
    m_AliasLibNames.Clear();

    // was: Pgm().ReadProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters(), false );
    prj.ConfigLoad( Kiface().KifaceSearch(), GROUP_CVP, GetProjectFileParameters() );

    if( m_NetlistFileExtension.IsEmpty() )
        m_NetlistFileExtension = wxT( "net" );
}


void CVPCB_MAINFRAME::SaveProjectFile( wxCommandEvent& aEvent )
{
    PROJECT&    prj = Prj();
    wxFileName  fn = prj.AbsolutePath( m_NetlistFileName.GetFullPath() );

    fn.SetExt( ProjectFileExtension );

    if( aEvent.GetId() == ID_SAVE_PROJECT_AS || !m_NetlistFileName.IsOk() )
    {
        wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                          wxEmptyString, ProjectFileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();

        if( !fn.HasExt() )
            fn.SetExt( ProjectFileExtension );
    }

    if( !IsWritable( fn ) )
        return;

    wxString pro_name = fn.GetFullPath();

    prj.ConfigSave( Kiface().KifaceSearch(), GROUP_CVP, GetProjectFileParameters(), pro_name );
}

