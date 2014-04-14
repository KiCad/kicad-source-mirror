#ifndef PGM_KICAD_H_
#define PGM_KICAD_H_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <pgm_base.h>
#include <bin_mod.h>


/**
 * Class PGM_KICAD
 * extends PGM_BASE to bring in FileHistory() and PdfBrowser() which were moved
 * from EDA_APP into KIFACE_I.  KIFACE_I is not applicable in the project manager
 * since it is not a KIFACE.  This header is in the kicad directory since nobody
 * else needs to know about it.
 */
class PGM_KICAD : public PGM_BASE
{
public:
    PGM_KICAD() :
        m_bm( "kicad" )     // indicates a "$HOME/.kicad wxConfig like" config file.
    {}

    ~PGM_KICAD()
    {
        destroy();
    }

    bool OnPgmInit( wxApp* aWxApp );                // overload PGM_BASE virtual
    void OnPgmExit();                               // overload PGM_BASE virtual
    void MacOpenFile( const wxString& aFileName );  // overload PGM_BASE virtual

    wxFileHistory&  GetFileHistory()            { return m_bm.m_history; }

    wxConfigBase*   PgmSettings()               { return m_bm.m_config; }

    SEARCH_STACK&   SysSearch()                 { return m_bm.m_search; }

    wxString        GetHelpFileName()           { return m_bm.m_help_file; }

protected:

    // The PGM_* classes can have difficulties at termination if they
    // are not destroyed soon enough.  Relying on a static destructor can be
    // too late for contained objects like wxSingleInstanceChecker.
    void destroy();

    BIN_MOD         m_bm;
};


extern PGM_KICAD&  Pgm();

#endif  // PGM_KICAD_H_
