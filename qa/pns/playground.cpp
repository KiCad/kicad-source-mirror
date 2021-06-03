/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers.
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


// WARNING - this Tom's crappy PNS hack tool code. Please don't complain about its quality
// (unless you want to improve it).

#include <pgm_base.h>
#include <qa_utils/utility_registry.h>

#include "pns_log_viewer_frame.h"
#include "label_manager.h"

namespace PNS {
    extern SHAPE_LINE_CHAIN g_pnew, g_hnew;
};

int playground_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );
    Pgm().App().SetTopWindow( frame );      // wxApp gets a face.
    frame->Show();

    auto overlay = frame->GetOverlay();

    
    overlay->SetIsFill(false);
    overlay->SetLineWidth(10000);

    //auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 111435489, 74692234), VECTOR2I( 111812234, 74315489), VECTOR2I( 112507766, 74315489), VECTOR2I( 112884511, 74692234), VECTOR2I( 112884511, 75387766), VECTOR2I( 112507766, 75764511), VECTOR2I( 111812234, 75764511), VECTOR2I( 111435489, 75387766)}, true );; 
    //auto path =  SHAPE_LINE_CHAIN( { VECTOR2I( 112609520, 74417243), VECTOR2I( 112609520, 73774520), VECTOR2I( 112670046, 73774520), VECTOR2I( 112999520, 73445046), VECTOR2I( 112999520, 72054954), VECTOR2I( 112670046, 71725480), VECTOR2I( 111654954, 71725480), VECTOR2I( 111325480, 72054954), VECTOR2I( 111325480, 73445046), VECTOR2I( 111654954, 73774520), VECTOR2I( 111710480, 73774520), VECTOR2I( 111710480, 75226197), VECTOR2I( 111973803, 75489520), VECTOR2I( 112346197, 75489520), VECTOR2I( 112609520, 75226197), VECTOR2I( 112609520, 74417243)}, false );;

// hull-cw-1
    //auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 111435489, 74692234), VECTOR2I( 111812234, 74315489), VECTOR2I( 112507766, 74315489), VECTOR2I( 112884511, 74692234), VECTOR2I( 112884511, 75387766), VECTOR2I( 112507766, 75764511), VECTOR2I( 111812234, 75764511), VECTOR2I( 111435489, 75387766)}, true );; 
// path-cw-1
    //auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 112609520, 74417243), VECTOR2I( 112609520, 75226197), VECTOR2I( 112346197, 75489520), VECTOR2I( 111973803, 75489520), VECTOR2I( 111710480, 75226197), VECTOR2I( 111710480, 72566303), VECTOR2I( 111973803, 72302980), VECTOR2I( 112346197, 72302980), VECTOR2I( 112609520, 72566303), VECTOR2I( 112609520, 74417243)}, false );; 
// hull
    auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 106035489, 85253452), VECTOR2I( 106313452, 84975489), VECTOR2I( 106706548, 84975489), VECTOR2I( 106984511, 85253452), VECTOR2I( 106984511, 85976548), VECTOR2I( 106706548, 86254511), VECTOR2I( 106313452, 86254511), VECTOR2I( 106035489, 85976548)}, true );; 
// path
    auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 101092000, 85246500), VECTOR2I( 101971211, 86125711), VECTOR2I( 106380778, 86125711), VECTOR2I( 106509572, 86254505)}, false );; 

    SHAPE_LINE_CHAIN path_w;

    BOX2D bb ( path.BBox().GetPosition(), path.BBox().GetSize() );

    LABEL_MANAGER labelMgr( frame->GetPanel()->GetGAL() );

    frame->GetPanel()->GetView()->SetViewport(bb);

    PNS::LINE l;
    l.SetShape( path );
    l.Walkaround( hull, path_w, true );

    overlay->SetStrokeColor( WHITE );
    overlay->SetLineWidth( 10000.0 );
    overlay->AnnotatedPolyline( PNS::g_pnew, "path", true );
    overlay->SetStrokeColor( RED );
    overlay->AnnotatedPolyline( PNS::g_hnew, "hull", true );
    overlay->SetLineWidth( 20000.0 );
    overlay->SetStrokeColor( YELLOW );
    overlay->AnnotatedPolyline( path_w, "walk", "true" );
    overlay->DrawAnnotations();

    overlay = nullptr;

    return 0;
}

static bool registered = UTILITY_REGISTRY::Register( {
        "playground",
        "Geometry/drawing playground",
        playground_main_func,
} );
