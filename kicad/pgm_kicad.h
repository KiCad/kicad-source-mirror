/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef PGM_KICAD_H_
#define PGM_KICAD_H_

#include <pgm_base.h>
#include <bin_mod.h>

#ifdef KICAD_IPC_API
#include <api/api_handler_common.h>
#endif

/**
 * PGM_KICAD
 * extends PGM_BASE to bring in FileHistory() and PdfBrowser() which were moved from EDA_APP
 * into KIFACE_BASE.  KIFACE_BASE is not applicable in the project manager since it is not a
 * KIFACE.  This header is in the kicad directory since nobody else needs to know about it.
 */
class PGM_KICAD : public PGM_BASE
{
public:
    PGM_KICAD() :
        m_bm( "kicad" )     // indicates a "$HOME/.kicad wxConfig like" config file.
    {}

    ~PGM_KICAD() throw()
    {
        Destroy();
    }

    bool OnPgmInit();
    void OnPgmExit();
    int OnPgmRun();

    void MacOpenFile( const wxString& aFileName ) override;

    APP_SETTINGS_BASE* PgmSettings()       { return m_bm.m_config; }

    SEARCH_STACK&      SysSearch()         { return m_bm.m_search; }

    wxString           GetHelpFileName()   { return m_bm.m_help_file; }

    // The PGM_* classes can have difficulties at termination if they
    // are not destroyed soon enough.  Relying on a static destructor can be
    // too late for contained objects like wxSingleInstanceChecker.
    void Destroy();

protected:

    BIN_MOD         m_bm;

#ifdef KICAD_IPC_API
    // In PGM_SINGLE_TOP because PGM_BASE is in kicommon, and this can't be in the DLL
    // because it depends on things like EDA_TEXT and EDA_SHAPE that aren't in the DLL
    std::unique_ptr<API_HANDLER_COMMON> m_api_common_handler;
#endif
};


extern PGM_KICAD& PgmTop();


#endif  // PGM_KICAD_H_
