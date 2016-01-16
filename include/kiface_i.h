/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef KIFACE_I_H_
#define KIFACE_I_H_

#include <kiway.h>
#include <bin_mod.h>


/**
 * Class KIFACE_I
 * is a KIFACE (I)mplementation,
 * with some features useful for DSOs which implement a KIFACE.
 * It is abstract, a few functions must be implemented in derivations.
 */
class KIFACE_I : public KIFACE
{
public:

    //-----<KIFACE API>----------------------------------------------------------

        // see base class KIFACE in kiway.h for doxygen docs

    VTBL_ENTRY bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) = 0;
    /*
    {
        typically call start_common() in your overload
        return start_common();
    }
    */

    VTBL_ENTRY void OnKifaceEnd()
    {
        // overload this if you want, end_common() may be handy.
        end_common();
    }

    VTBL_ENTRY  wxWindow* CreateWindow( wxWindow* aParent,
            int aClassId, KIWAY* aKIWAY, int aCtlBits = 0 ) = 0;

    VTBL_ENTRY void* IfaceOrAddress( int aDataId ) = 0;

    //-----</KIFACE API>---------------------------------------------------------

    // The remainder are DSO specific helpers, not part of the KIFACE API

    /**
     * Constructor
     *
     * @param aKifaceName should point to a C string in permanent storage,
     * which contains the name of the DSO.  Examples: "eeschema", "pcbnew", etc.
     * This controls the name of the wxConfigBase established in m_bm,
     * so it should be lowercase.
     */
    KIFACE_I( const char* aKifaceName, KIWAY::FACE_T aId ) :
        m_id( aId ),
        m_bm( aKifaceName ),
        m_start_flags( 0 )
    {
    }

    // ~KIFACE_I();

protected:

    /// Common things to do for a top program module, during OnKifaceStart().
    bool start_common( int aCtlBits );

    /// Common things to do for a top program module, during OnKifaceEnd();
    void end_common();

    // From here down should probably not be in a KIFACE, even though they
    // are DSO specific, they have nothing to do with KIWAY's use of KIFACE,
    // so its a questionable choice to put non KIWAY client code in this class.

public:

    const wxString Name()                               { return wxString::FromUTF8( m_bm.m_name ); }

    wxConfigBase* KifaceSettings() const                { return m_bm.m_config; }

    /**
     * Function StartFlags
     * returns whatever was passed as @a aCtlBits to OnKifaceStart()
     */
    int StartFlags() const                              { return m_start_flags; }

    /**
     * Function IsSingle
     * is this KIFACE_I running under single_top?
     */
    bool IsSingle() const                               { return m_start_flags & KFCTL_STANDALONE; }

    /**
     * Function GetHelpFileName
     * returns just the basename portion of the current help file.
     */
    const wxString& GetHelpFileName() const             { return m_bm.m_help_file; }

    wxFileHistory& GetFileHistory()                     { return m_bm.m_history; }

    /// Only for DSO specific 'non-library' files.
    /// (The library search path is in the PROJECT class.)
    SEARCH_STACK&       KifaceSearch()                  { return m_bm.m_search; }

private:
    KIWAY::FACE_T       m_id;

    BIN_MOD             m_bm;

    int                 m_start_flags;      ///< flags provided in OnKifaceStart()
};


/// Global KIFACE_I "get" accessor.
KIFACE_I& Kiface();

#endif // KIFACE_I_H_
