/*
 * This program source code file is part of kicad2mcad
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2016-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KICAD2STEP_H
#define KICAD2STEP_H

#include <wx/string.h>
#include <import_export.h>

class PANEL_KICAD2STEP;

class APIEXPORT KICAD2MCAD_PRMS // A small class to handle parameters of conversion
{
public:
    KICAD2MCAD_PRMS();

    ///< Return file extension for the selected output format
    wxString getOutputExt() const;

#ifdef SUPPORTS_IGES
    bool m_fmtIGES;
#endif
    bool     m_overwrite;
    bool     m_useGridOrigin;
    bool     m_useDrillOrigin;
    bool     m_includeVirtual;
    bool     m_substModels;
    wxString m_filename;
    wxString m_outputFile;
    double   m_xOrigin;
    double   m_yOrigin;
    double   m_minDistance;
};

class APIEXPORT KICAD2STEP
{
public:
    KICAD2STEP( KICAD2MCAD_PRMS aParams );

    int Run();
    void ReportMessage( const wxString& aMessage );

private:
    KICAD2MCAD_PRMS m_params;
    PANEL_KICAD2STEP* m_panel;
};

#endif