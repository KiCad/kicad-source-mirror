/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <config_params.h>
#include <kiface_i.h>
#include <project.h>

#include <cvpcb_mainframe.h>


PARAM_CFG_ARRAY& CVPCB_MAINFRAME::GetProjectFileParameters()
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_BASE( GROUP_PCB_LIBS, PARAM_COMMAND_ERASE ) );

    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST(
        wxT( "EquName" ), &m_EquFilesNames, GROUP_CVP_EQU ) );

    return m_projectFileParams;
}


void CVPCB_MAINFRAME::LoadProjectFile()
{
    PROJECT&    prj = Prj();

    m_ModuleLibNames.Clear();
    m_EquFilesNames.Clear();

    prj.ConfigLoad( Kiface().KifaceSearch(), GROUP_CVP, GetProjectFileParameters() );
}


void CVPCB_MAINFRAME::SaveProjectFile()
{
    PROJECT&    prj = Prj();
    wxFileName  fn = prj.GetProjectFullName();

    if( !IsWritable( fn ) )
    {
        wxMessageBox( _( "Project file \"%s\" is not writable" ), fn.GetFullPath() );
        return;
    }

    wxString pro_name = fn.GetFullPath();

    prj.ConfigSave( Kiface().KifaceSearch(), GROUP_CVP, GetProjectFileParameters(), pro_name );
}

