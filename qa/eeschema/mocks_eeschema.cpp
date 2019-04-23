/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <kiface_i.h>
#include <pgm_base.h>

#include <sch_edit_frame.h>

// The main sheet of the project
SCH_SHEET* g_RootSheet = nullptr;

// a transform matrix, to display components in lib editor
TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, -1 );

static struct IFACE : public KIFACE_I
{
    // Of course all are overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) : KIFACE_I( aName, aType )
    {
    }

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override
    {
        return true;
    }

    void OnKifaceEnd() override
    {
    }

    wxWindow* CreateWindow(
            wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        assert( false );
        return nullptr;
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     *
     * @return void* - and must be cast into the know type.
     */
    void* IfaceOrAddress( int aDataId ) override
    {
        return NULL;
    }
} kiface( "mock_eeschema", KIWAY::FACE_SCH );

static struct PGM_MOCK_EESCHEMA_FRAME : public PGM_BASE
{
    bool OnPgmInit();

    void OnPgmExit()
    {
        Kiway.OnKiwayEnd();

        // Destroy everything in PGM_BASE, especially wxSingleInstanceCheckerImpl
        // earlier than wxApp and earlier than static destruction would.
        PGM_BASE::Destroy();
    }

    void MacOpenFile( const wxString& aFileName ) override
    {
        wxFileName filename( aFileName );

        if( filename.FileExists() )
        {
#if 0
            // this pulls in EDA_DRAW_FRAME type info, which we don't want in
            // the single_top link image.
            KIWAY_PLAYER* frame = dynamic_cast<KIWAY_PLAYER*>( App().GetTopWindow() );
#else
            KIWAY_PLAYER* frame = (KIWAY_PLAYER*) App().GetTopWindow();
#endif

            if( frame )
                frame->OpenProjectFiles( std::vector<wxString>( 1, aFileName ) );
        }
    }
} program;

PGM_BASE& Pgm()
{
    return program;
}


KIFACE_I& Kiface()
{
    return kiface;
}


static COLOR4D s_layerColor[LAYER_ID_COUNT];

COLOR4D GetLayerColor( SCH_LAYER_ID aLayer )
{
    unsigned layer = ( aLayer );
    wxASSERT( layer < arrayDim( s_layerColor ) );
    return s_layerColor[layer];
}

void SetLayerColor( COLOR4D aColor, SCH_LAYER_ID aLayer )
{
    // Do not allow non-background layers to be completely white.
    // This ensures the BW printing recognizes that the colors should be
    // printed black.
    if( aColor == COLOR4D::WHITE && aLayer != LAYER_SCHEMATIC_BACKGROUND )
        aColor.Darken( 0.01 );

    unsigned layer = aLayer;
    wxASSERT( layer < arrayDim( s_layerColor ) );
    s_layerColor[layer] = aColor;
}