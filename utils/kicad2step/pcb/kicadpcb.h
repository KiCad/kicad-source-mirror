/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file kicadpcb.h
 * declares the main PCB object
 */

#ifndef KICADPCB_H
#define KICADPCB_H

#include <wx/string.h>
#include <string>
#include <vector>
#include "3d_resolver.h"
#include "base.h"

#ifdef SUPPORTS_IGES
#undef SUPPORTS_IGES
#endif

namespace SEXPR
{
    class SEXPR;
}

class KICADMODULE;
class KICADCURVE;
class PCBMODEL;

class KICADPCB
{
private:
    S3D_RESOLVER m_resolver;
    std::string m_filename;
    PCBMODEL*   m_pcb;
    DOUBLET     m_origin;
    DOUBLET     m_gridOrigin;
    DOUBLET     m_drillOrigin;
    bool        m_useGridOrigin;
    bool        m_useDrillOrigin;
    // set to TRUE if the origin was actually parsed
    bool        m_hasGridOrigin;
    bool        m_hasDrillOrigin;

    // PCB parameters/entities
    double                      m_thickness;
    std::vector< KICADMODULE* > m_modules;
    std::vector< KICADCURVE* >  m_curves;

    bool parsePCB( SEXPR::SEXPR* data );
    bool parseGeneral( SEXPR::SEXPR* data );
    bool parseSetup( SEXPR::SEXPR* data );
    bool parseModule( SEXPR::SEXPR* data );
    bool parseCurve( SEXPR::SEXPR* data, CURVE_TYPE aCurveType );

public:
    KICADPCB();
    virtual ~KICADPCB();

    void SetOrigin( double aXOrigin, double aYOrigin )
    {
        m_origin.x = aXOrigin;
        m_origin.y = aYOrigin;
    }

    void UseGridOrigin( bool aUseOrigin )
    {
        m_useGridOrigin = aUseOrigin;
    }

    void UseDrillOrigin( bool aUseOrigin )
    {
        m_useDrillOrigin = aUseOrigin;
    }

    bool ReadFile( const wxString& aFileName );
    bool ComposePCB( bool aComposeVirtual = true );
    bool WriteSTEP( const wxString& aFileName, bool aOverwrite );
    #ifdef SUPPORTS_IGES
    bool WriteIGES( const wxString& aFileName, bool aOverwrite );
    #endif
};


#endif  // KICADPCB_H
