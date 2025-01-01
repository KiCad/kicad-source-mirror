/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KIFACE_BASE_H
#define KIFACE_BASE_H

#include <kiway.h>
#include <bin_mod.h>
#include <tool/action_manager.h>


/**
 * A #KIFACE implementation.
 *
 * This has useful for DSOs which implement a #KIFACE.  It is abstract so a few functions
 * must be implemented in derivations.
 */
class KIFACE_BASE : public KIFACE
{
public:
    /**
     * Typically #start_common() is called from here.
     */
    virtual bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits, KIWAY* aKiway ) override = 0;

    virtual void OnKifaceEnd() override
    {
        // overload this if you want, end_common() may be handy.
        end_common();
    }

    virtual  wxWindow* CreateKiWindow( wxWindow* aParent, int aClassId, KIWAY* aKIWAY,
                                       int aCtlBits = 0 ) override = 0;

    virtual void Reset() override{};

    virtual void* IfaceOrAddress( int aDataId ) override = 0;

    /**
     * @param aKifaceName should point to a C string in permanent storage which contains the
     *                    name of the DSO.  Examples: "eeschema", "pcbnew", etc.  This controls
     *                    the name of the wxConfigBase established in m_bm, so it should be
     *                    lowercase.
     * @param aId is the type of DSO ( #FACE_SCH, #FACE_PCB, #FACE_CVPCB, #FACE_GERBVIEW,
     *            #FACE_PL_EDITOR, #FACE_PCB_CALCULATOR, #FACE_BMP2CMP)
     */
    KIFACE_BASE( const char* aKifaceName, KIWAY::FACE_T aId ) :
        m_start_flags( 0 ),
        m_id( aId ),
        m_bm( aKifaceName )
    {
    }

    // ~KIFACE_BASE();

protected:

    /// Common things to do for a top program module, during OnKifaceStart().
    bool start_common( int aCtlBits );

    /// Common things to do for a top program module, during OnKifaceEnd();
    void end_common();

    // From here down should probably not be in a KIFACE, even though they
    // are DSO specific, they have nothing to do with KIWAY's use of KIFACE,
    // so its a questionable choice to put non KIWAY client code in this class.

public:

    const wxString Name()
    {
        return wxString::FromUTF8( m_bm.m_name );
    }

    APP_SETTINGS_BASE* KifaceSettings() const         { return m_bm.m_config; }

    void InitSettings( APP_SETTINGS_BASE* aSettings ) { m_bm.InitSettings( aSettings ); }

    /**
     * Return whatever was passed as @a aCtlBits to OnKifaceStart().
     */
    int StartFlags() const                            { return m_start_flags; }

    /**
     * Is this KIFACE running under single_top?
     */
    bool IsSingle() const                             { return m_start_flags & KFCTL_STANDALONE; }

    /**
     * Return just the basename portion of the current help file.
     */
    const wxString& GetHelpFileName() const           { return m_bm.m_help_file; }

    /// Only for DSO specific 'non-library' files.
    /// (The library search path is in the PROJECT class.)
    SEARCH_STACK&       KifaceSearch()                { return m_bm.m_search; }

    void GetActions( std::vector<TOOL_ACTION*>& aActions ) const override
    {
        for( TOOL_ACTION* action : ACTION_MANAGER::GetActionList() )
            aActions.push_back( action );
    }

protected:
    int m_start_flags;      ///< flags provided in OnKifaceStart()

private:
    KIWAY::FACE_T m_id;
    BIN_MOD       m_bm;
};


/// Global KIFACE_BASE "get" accessor.
KIFACE_BASE& Kiface();

#endif // KIFACE_BASE_H
