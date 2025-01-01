/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2015 Wayne Stambaugh <stambaughw@verizon.net>
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


#include <executable_names.h>

#ifdef __WINDOWS__
const wxString CVPCB_EXE           ( wxS( "cvpcb.exe" ) );
const wxString PCBNEW_EXE          ( wxS( "pcbnew.exe" ) );
const wxString EESCHEMA_EXE        ( wxS( "eeschema.exe" ) );
const wxString GERBVIEW_EXE        ( wxS( "gerbview.exe" ) );
const wxString BITMAPCONVERTER_EXE ( wxS( "bitmap2component.exe" ) );
const wxString PCB_CALCULATOR_EXE  ( wxS( "pcb_calculator.exe" ) );
const wxString PL_EDITOR_EXE       ( wxS( "pl_editor.exe" ) );
#else
const wxString CVPCB_EXE           ( wxS( "cvpcb" ) );
const wxString PCBNEW_EXE          ( wxS( "pcbnew" ) );
const wxString EESCHEMA_EXE        ( wxS( "eeschema" ) );
const wxString GERBVIEW_EXE        ( wxS( "gerbview" ) );
const wxString BITMAPCONVERTER_EXE ( wxS( "bitmap2component" ) );
const wxString PCB_CALCULATOR_EXE  ( wxS( "pcb_calculator" ) );
const wxString PL_EDITOR_EXE       ( wxS( "pl_editor" ) );
#endif
