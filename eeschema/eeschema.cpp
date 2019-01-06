/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2017 KiCad Developers, see change_log.txt for contributors.
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
 * @file eeschema.cpp
 * @brief the main file
 */

#include <fctsys.h>
#include <pgm_base.h>
#include <kiface_i.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <eda_dde.h>
#include <sch_edit_frame.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <eda_text.h>

#include <general.h>
#include <class_libentry.h>
#include <hotkeys.h>
#include <transform.h>
#include <wildcards_and_files_ext.h>
#include <symbol_lib_table.h>
#include <dialogs/dialog_global_sym_lib_table_config.h>
#include <dialogs/panel_sym_lib_table.h>

#include <kiway.h>
#include <sim/sim_plot_frame.h>

// The main sheet of the project
SCH_SHEET*  g_RootSheet = NULL;

// a transform matrix, to display components in lib editor
TRANSFORM DefaultTransform = TRANSFORM( 1, 0, 0, -1 );


namespace SCH {

static struct IFACE : public KIFACE_I
{
    // Of course all are virtual overloads, implementations of the KIFACE.

    IFACE( const char* aName, KIWAY::FACE_T aType ) :
        KIFACE_I( aName, aType )
    {}

    bool OnKifaceStart( PGM_BASE* aProgram, int aCtlBits ) override;

    void OnKifaceEnd() override;

    wxWindow* CreateWindow( wxWindow* aParent, int aClassId, KIWAY* aKiway, int aCtlBits = 0 ) override
    {
        switch( aClassId )
        {
        case FRAME_SCH:
            {
                SCH_EDIT_FRAME* frame = new SCH_EDIT_FRAME( aKiway, aParent );

                if( Kiface().IsSingle() )
                {
                    // only run this under single_top, not under a project manager.
                    frame->CreateServer( KICAD_SCH_PORT_SERVICE_NUMBER );
                }
                return frame;
            }
            break;

        case FRAME_SCH_LIB_EDITOR:
            {
                LIB_EDIT_FRAME* frame = new LIB_EDIT_FRAME( aKiway, aParent );
                return frame;
            }
            break;

#ifdef KICAD_SPICE
        case FRAME_SIMULATOR:
            {
                SIM_PLOT_FRAME* frame = new SIM_PLOT_FRAME( aKiway, aParent );
                return frame;
            }
            break;
#endif /* KICAD_SPICE */

        case FRAME_SCH_VIEWER:
        case FRAME_SCH_VIEWER_MODAL:
            {
                LIB_VIEW_FRAME* frame = new LIB_VIEW_FRAME( aKiway, aParent, FRAME_T( aClassId ) );
                return frame;
            }
            break;

        case DIALOG_SCH_LIBRARY_TABLE:
            InvokeSchEditSymbolLibTable( aKiway, aParent );

            // Dialog has completed; nothing to return.
            return nullptr;

        default:
            return NULL;
        }
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

} kiface( "eeschema", KIWAY::FACE_SCH );

} // namespace

using namespace SCH;

static PGM_BASE* process;


KIFACE_I& Kiface() { return kiface; }


// KIFACE_GETTER's actual spelling is a substitution macro found in kiway.h.
// KIFACE_GETTER will not have name mangling due to declaration in kiway.h.
MY_API( KIFACE* ) KIFACE_GETTER(  int* aKIFACEversion, int aKiwayVersion, PGM_BASE* aProgram )
{
    process = aProgram;
    return &kiface;
}


PGM_BASE& Pgm()
{
    wxASSERT( process );    // KIFACE_GETTER has already been called.
    return *process;
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


static PARAM_CFG_ARRAY& cfg_params()
{
    static PARAM_CFG_ARRAY ca;

    if( !ca.size() )
    {
        // These are KIFACE specific, they need to be loaded once when the
        // eeschema KIFACE comes in.

#define CLR(x, y, z)\
    ca.push_back( new PARAM_CFG_SETCOLOR( true, wxT( x ),\
                                          &s_layerColor[( y )], z ) );

        CLR( "Color4DWireEx",             LAYER_WIRE,                 COLOR4D( GREEN ) )
        CLR( "Color4DBusEx",              LAYER_BUS,                  COLOR4D( BLUE ) )
        CLR( "Color4DConnEx",             LAYER_JUNCTION,             COLOR4D( GREEN ) )
        CLR( "Color4DLLabelEx",           LAYER_LOCLABEL,             COLOR4D( BLACK ) )
        CLR( "Color4DHLabelEx",           LAYER_HIERLABEL,            COLOR4D( BROWN ) )
        CLR( "Color4DGLabelEx",           LAYER_GLOBLABEL,            COLOR4D( RED ) )
        CLR( "Color4DPinNumEx",           LAYER_PINNUM,               COLOR4D( RED ) )
        CLR( "Color4DPinNameEx",          LAYER_PINNAM,               COLOR4D( CYAN ) )
        CLR( "Color4DFieldEx",            LAYER_FIELDS,               COLOR4D( MAGENTA ) )
        CLR( "Color4DReferenceEx",        LAYER_REFERENCEPART,        COLOR4D( CYAN ) )
        CLR( "Color4DValueEx",            LAYER_VALUEPART,            COLOR4D( CYAN ) )
        CLR( "Color4DNoteEx",             LAYER_NOTES,                COLOR4D( LIGHTBLUE ) )
        CLR( "Color4DBodyEx",             LAYER_DEVICE,               COLOR4D( RED ) )
        CLR( "Color4DBodyBgEx",           LAYER_DEVICE_BACKGROUND,    COLOR4D( LIGHTYELLOW ) )
        CLR( "Color4DNetNameEx",          LAYER_NETNAM,               COLOR4D( DARKGRAY ) )
        CLR( "Color4DPinEx",              LAYER_PIN,                  COLOR4D( RED ) )
        CLR( "Color4DSheetEx",            LAYER_SHEET,                COLOR4D( MAGENTA ) )
        CLR( "Color4DSheetFileNameEx",    LAYER_SHEETFILENAME,        COLOR4D( BROWN ) )
        CLR( "Color4DSheetNameEx",        LAYER_SHEETNAME,            COLOR4D( CYAN ) )
        CLR( "Color4DSheetLabelEx",       LAYER_SHEETLABEL,           COLOR4D( BROWN ) )
        CLR( "Color4DNoConnectEx",        LAYER_NOCONNECT,            COLOR4D( BLUE ) )
        CLR( "Color4DErcWEx",             LAYER_ERC_WARN,             COLOR4D( GREEN ).WithAlpha(0.8 ) )
        CLR( "Color4DErcEEx",             LAYER_ERC_ERR,              COLOR4D( RED ).WithAlpha(0.8 ) )
        CLR( "Color4DGridEx",             LAYER_SCHEMATIC_GRID,       COLOR4D( DARKGRAY ) )
        CLR( "Color4DBgCanvasEx",         LAYER_SCHEMATIC_BACKGROUND, COLOR4D( WHITE ) )
        CLR( "Color4DCursorEx",           LAYER_SCHEMATIC_CURSOR,     COLOR4D( BLACK ) )
        CLR( "Color4DBrightenedEx",       LAYER_BRIGHTENED,           COLOR4D( PUREMAGENTA ) )
        CLR( "Color4DHiddenEx",           LAYER_HIDDEN,               COLOR4D( LIGHTGRAY ) )
        CLR( "Color4DWorksheetEx",        LAYER_WORKSHEET,            COLOR4D( RED ) )
    }

    return ca;
}


bool IFACE::OnKifaceStart( PGM_BASE* aProgram, int aCtlBits )
{
    // This is process level, not project level, initialization of the DSO.

    // Do nothing in here pertinent to a project!

    start_common( aCtlBits );

    // Give a default colour for all layers
    // (actual color will be initialized by config)
    for( SCH_LAYER_ID ii = SCH_LAYER_ID_START; ii < SCH_LAYER_ID_END; ++ii )
        SetLayerColor( COLOR4D( DARKGRAY ), ii );

    SetLayerColor( COLOR4D::WHITE, LAYER_SCHEMATIC_BACKGROUND );
    SetLayerColor( COLOR4D::BLACK, LAYER_SCHEMATIC_CURSOR );

    // Must be called before creating the main frame in order to
    // display the real hotkeys in menus or tool tips
    ReadHotkeyConfig( SCH_EDIT_FRAME_NAME, g_Eeschema_Hokeys_Descr );

    wxConfigLoadSetups( KifaceSettings(), cfg_params() );

    wxFileName fn = SYMBOL_LIB_TABLE::GetGlobalTableFileName();

    if( !fn.FileExists() )
    {
        DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG fpDialog( NULL );

        fpDialog.ShowModal();
    }
    else
    {
        try
        {
            // The global table is not related to a specific project.  All projects
            // will use the same global table.  So the KIFACE::OnKifaceStart() contract
            // of avoiding anything project specific is not violated here.
            if( !SYMBOL_LIB_TABLE::LoadGlobalTable( SYMBOL_LIB_TABLE::GetGlobalLibTable() ) )
                return false;
        }
        catch( const IO_ERROR& ioe )
        {
            // if we are here, a incorrect global symbol library table was found.
            // Incorrect global symbol library table is not a fatal error:
            // the user just has to edit the (partially) loaded table.
            wxString msg = _(
                "An error occurred attempting to load the global symbol library table.\n"
                "Please edit this global symbol library table in Preferences menu."
                );

            DisplayErrorMessage( NULL, msg, ioe.What() );
        }
    }

    return true;
}


void IFACE::OnKifaceEnd()
{
    wxConfigSaveSetups( KifaceSettings(), cfg_params() );
    end_common();
}
