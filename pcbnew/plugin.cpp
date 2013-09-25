/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 KiCad Developers, see change_log.txt for contributors.
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

#include <io_mgr.h>

#define FMT_UNIMPLEMENTED   _( "Plugin '%s' does not implement the '%s' function." )
#define FMT_NOTFOUND        _( "Plugin type '%s' is not found." )


BOARD* PLUGIN::Load( const wxString& aFileName, BOARD* aAppendToMe, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData(), __FUNCTION__ ) );
}


void PLUGIN::Save( const wxString& aFileName, BOARD* aBoard, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData(), __FUNCTION__ ) );
}


wxArrayString PLUGIN::FootprintEnumerate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


MODULE* PLUGIN::FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
                                    const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


void PLUGIN::FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


bool PLUGIN::FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}


bool PLUGIN::IsFootprintLibWritable( const wxString& aLibraryPath )
{
    // not pure virtual so that plugins only have to implement subset of the PLUGIN interface.
    THROW_IO_ERROR( wxString::Format( FMT_UNIMPLEMENTED, PluginName().GetData() , __FUNCTION__ ) );
}

