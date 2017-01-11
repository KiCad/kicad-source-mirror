/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
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

#include <gal/gal_display_options.h>
#include <wx/config.h>

using namespace KIGFX;

static const wxString GalGLAntialiasingKeyword( wxT( "OpenGLAntialiasingMode" ) );

GAL_DISPLAY_OPTIONS::GAL_DISPLAY_OPTIONS()
    : gl_antialiasing_mode( OPENGL_ANTIALIASING_MODE::NONE )
{}


void GAL_DISPLAY_OPTIONS::ReadConfig( wxConfigBase* aCfg, wxString aBaseName )
{
    aCfg->Read( aBaseName + GalGLAntialiasingKeyword,
        reinterpret_cast<long*>(&gl_antialiasing_mode),
        (long)KIGFX::OPENGL_ANTIALIASING_MODE::NONE );

    NotifyChanged();
}


void GAL_DISPLAY_OPTIONS::WriteConfig( wxConfigBase* aCfg, wxString aBaseName )
{
    aCfg->Write( aBaseName + GalGLAntialiasingKeyword,
                    static_cast<long>(gl_antialiasing_mode) );
}


void GAL_DISPLAY_OPTIONS::NotifyChanged()
{
    Notify( &GAL_DISPLAY_OPTIONS_OBSERVER::OnGalDisplayOptionsChanged, *this );
}
