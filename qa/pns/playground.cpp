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

int playground_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );
    Pgm().App().SetTopWindow( frame );      // wxApp gets a face.
    frame->Show();

    auto overlay = frame->GetPanel()->DebugOverlay();
    overlay->SetIsFill(false);
    overlay->SetLineWidth(10000);

    auto hull = SHAPE_LINE_CHAIN( { VECTOR2I( 66280505, 107710033), VECTOR2I( 65914967, 107344495), VECTOR2I( 65914967, 106827549), VECTOR2I( 66280505, 106462011), VECTOR2I( 74810033, 106462009), VECTOR2I( 75175571, 106827547), VECTOR2I( 75175571, 107344493), VECTOR2I( 74810033, 107710031)}, true ); 
    auto path = SHAPE_LINE_CHAIN( { VECTOR2I( 143928480, 109445996), VECTOR2I( 111066480, 109445996), VECTOR2I( 106254391, 104633907), VECTOR2I( 105909001, 104633907), VECTOR2I( 105775094, 104500000), VECTOR2I( 76250000, 104500000), VECTOR2I( 74287991, 106462009), VECTOR2I( 66280505, 106462011), VECTOR2I( 66012989, 106462011)}, false );

    BOX2D bb ( path.BBox().GetPosition(), path.BBox().GetSize() );

    frame->GetPanel()->GetView()->SetViewport(bb);

    overlay->SetStrokeColor( WHITE );
    overlay->SetLineWidth( 100000.0 );
    overlay->Polyline( path );
    overlay->SetStrokeColor( RED );
    overlay->Polyline( hull );

    overlay = nullptr;

    return 0;
}

static bool registered = UTILITY_REGISTRY::Register( {
        "playground",
        "Geometry/drawing playground",
        playground_main_func,
} );
