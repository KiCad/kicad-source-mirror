/**
 * @file export_idf.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013  Cirilo Bernardo
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


#include <list>
#include <wxPcbStruct.h>
#include <macros.h>
#include <pcbnew.h>
#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <idf.h>
#include <3d_struct.h>

// assumed default graphical line thickness: 10000 IU == 0.1mm
#define LINE_WIDTH (100000)

/**
 * Function idf_export_outline
 * retrieves line segment information from the edge layer and compiles
 * the data into a form which can be output as an IDFv3 compliant
 * BOARD_OUTLINE section.
 */
static void idf_export_outline( BOARD* aPcb, IDF_BOARD& aIDFBoard )
{
    double scale = aIDFBoard.GetScale();

    DRAWSEGMENT* graphic;               // KiCad graphical item
    IDF_POINT sp, ep;                   // start and end points from KiCad item

    std::list< IDF_SEGMENT* > lines;    // IDF intermediate form of KiCad graphical item
    IDF_OUTLINE outline;                // graphical items forming an outline or cutout

    // NOTE: IMPLEMENTATION
    // If/when component cutouts are allowed, we must implement them separately. Cutouts
    // must be added to the board outline section and not to the Other Outline section.
    // The module cutouts should be handled via the idf_export_module() routine.

    double offX, offY;
    aIDFBoard.GetOffset( offX, offY );

    // Retrieve segments and arcs from the board
    for( BOARD_ITEM* item = aPcb->m_Drawings; item; item = item->Next() )
    {
        if( item->Type() != PCB_LINE_T || item->GetLayer() != EDGE_N )
            continue;

        graphic = (DRAWSEGMENT*) item;

        switch( graphic->GetShape() )
        {
        case S_SEGMENT:
            {
                sp.x    = graphic->GetStart().x * scale + offX;
                sp.y    = -graphic->GetStart().y * scale + offY;
                ep.x    = graphic->GetEnd().x * scale + offX;
                ep.y    = -graphic->GetEnd().y * scale + offY;
                IDF_SEGMENT* seg = new IDF_SEGMENT( sp, ep );

                if( seg )
                    lines.push_back( seg );
            }
            break;

        case S_ARC:
            {
                sp.x = graphic->GetCenter().x * scale + offX;
                sp.y = -graphic->GetCenter().y * scale + offY;
                ep.x = graphic->GetArcStart().x * scale + offX;
                ep.y = -graphic->GetArcStart().y * scale + offY;
                IDF_SEGMENT* seg = new IDF_SEGMENT( sp, ep, -graphic->GetAngle() / 10.0, true );

                if( seg )
                    lines.push_back( seg );
            }
            break;

        case S_CIRCLE:
            {
                sp.x = graphic->GetCenter().x * scale + offX;
                sp.y = -graphic->GetCenter().y * scale + offY;
                ep.x = sp.x - graphic->GetRadius() * scale;
                ep.y = sp.y;
                // Circles must always have an angle of +360 deg. to appease
                // quirky MCAD implementations of IDF.
                IDF_SEGMENT* seg = new IDF_SEGMENT( sp, ep, 360.0, true );

                if( seg )
                    lines.push_back( seg );
            }
            break;

        default:
            break;
        }
    }

    // if there is no outline then use the bounding box
    if( lines.empty() )
    {
        goto UseBoundingBox;
    }

    // get the board outline and write it out
    // note: we do not use a try/catch block here since we intend
    // to simply ignore unclosed loops and continue processing
    // until we're out of segments to process
    IDF3::GetOutline( lines, outline );

    if( outline.empty() )
        goto UseBoundingBox;

    aIDFBoard.AddOutline( outline );

    // get all cutouts and write them out
    while( !lines.empty() )
    {
        IDF3::GetOutline( lines, outline );

        if( outline.empty() )
            continue;

        aIDFBoard.AddOutline( outline );
    }

    return;

UseBoundingBox:

    // clean up if necessary
    while( !lines.empty() )
    {
        delete lines.front();
        lines.pop_front();
    }

    outline.Clear();

    // fetch a rectangular bounding box for the board;
    // there is always some uncertainty in the board dimensions
    // computed via ComputeBoundingBox() since this depends on the
    // individual module entities.
    EDA_RECT bbbox = aPcb->ComputeBoundingBox( true );

    // convert to mm and compensate for an assumed LINE_WIDTH line thickness
    double  x   = ( bbbox.GetOrigin().x + LINE_WIDTH / 2 ) * scale + offX;
    double  y   = ( bbbox.GetOrigin().y + LINE_WIDTH / 2 ) * scale + offY;
    double  dx  = ( bbbox.GetSize().x - LINE_WIDTH ) * scale;
    double  dy  = ( bbbox.GetSize().y - LINE_WIDTH ) * scale;

    double px[4], py[4];
    px[0]   = x;
    py[0]   = y;

    px[1]   = x;
    py[1]   = y + dy;

    px[2]   = x + dx;
    py[2]   = y + dy;

    px[3]   = x + dx;
    py[3]   = y;

    IDF_POINT p1, p2;

    p1.x    = px[3];
    p1.y    = py[3];
    p2.x    = px[0];
    p2.y    = py[0];

    outline.push( new IDF_SEGMENT( p1, p2 ) );

    for( int i = 1; i < 4; ++i )
    {
        p1.x    = px[i - 1];
        p1.y    = py[i - 1];
        p2.x    = px[i];
        p2.y    = py[i];
        
        outline.push( new IDF_SEGMENT( p1, p2 ) );
    }

    aIDFBoard.AddOutline( outline );
}


/**
 * Function idf_export_module
 * retrieves information from all board modules, adds drill holes to
 * the DRILLED_HOLES or BOARD_OUTLINE section as appropriate,
 * compiles data for the PLACEMENT section and compiles data for
 * the library ELECTRICAL section.
 */
static void idf_export_module( BOARD* aPcb, MODULE* aModule,
        IDF_BOARD& aIDFBoard )
{
    // Reference Designator
    std::string crefdes = TO_UTF8( aModule->GetReference() );

    if( crefdes.empty() || !crefdes.compare( "~" ) )
    {
        std::string cvalue = TO_UTF8( aModule->GetValue() );

        // if both the RefDes and Value are empty or set to '~' the board owns the part,
        // otherwise associated parts of the module must be marked NOREFDES.
        if( cvalue.empty() || !cvalue.compare( "~" ) )
            crefdes = "BOARD";
        else
            crefdes = "NOREFDES";
    }

    // TODO: If module cutouts are supported we must add code here
    // for( EDA_ITEM* item = aModule->GraphicalItems();  item != NULL;  item = item->Next() )
    // {
    // if( ( item->Type() != PCB_MODULE_EDGE_T )
    // || (item->GetLayer() != EDGE_N ) ) continue;
    // code to export cutouts
    // }

    // Export pads
    double  drill, x, y;
    double  scale = aIDFBoard.GetScale();
    IDF3::KEY_PLATING kplate;
    std::string pintype;
    std::string tstr;

    double dx, dy;

    aIDFBoard.GetOffset( dx, dy );

    for( D_PAD* pad = aModule->Pads(); pad; pad = pad->Next() )
    {
        drill = (double) pad->GetDrillSize().x * scale;
        x     = pad->GetPosition().x * scale + dx;
        y     = -pad->GetPosition().y * scale + dy;

        // Export the hole on the edge layer
        if( drill > 0.0 )
        {
            // plating
            if( pad->GetAttribute() == PAD_HOLE_NOT_PLATED )
                kplate = IDF3::NPTH;
            else
                kplate = IDF3::PTH;

            // hole type
            tstr = TO_UTF8( pad->GetPadName() );

            if( tstr.empty() || !tstr.compare( "0" ) || !tstr.compare( "~" )
                || ( kplate == IDF3::NPTH ) || ( pad->GetDrillShape() == PAD_OVAL ) )
                pintype = "MTG";
            else
                pintype = "PIN";

            // fields:
            // 1. hole dia. : float
            // 2. X coord : float
            // 3. Y coord : float
            // 4. plating : PTH | NPTH
            // 5. Assoc. part : BOARD | NOREFDES | PANEL | {"refdes"}
            // 6. type : PIN | VIA | MTG | TOOL | { "other" }
            // 7. owner : MCAD | ECAD | UNOWNED
            if( ( pad->GetDrillShape() == PAD_OVAL )
                && ( pad->GetDrillSize().x != pad->GetDrillSize().y ) )
            {
                // NOTE: IDF does not have direct support for slots;
                // slots are implemented as a board cutout and we
                // cannot represent plating or reference designators

                double dlength = pad->GetDrillSize().y * scale;

                // NOTE: The orientation of modules and pads have
                // the opposite sense due to KiCad drawing on a
                // screen with a LH coordinate system
                double angle = pad->GetOrientation() / 10.0;

                if( dlength < drill )
                {
                    std::swap( drill, dlength );
                    angle += M_PI2;
                }

                // NOTE: KiCad measures a slot's length from end to end
                // rather than between the centers of the arcs
                dlength -= drill;

                aIDFBoard.AddSlot( drill, dlength, angle, x, y );
            }
            else
            {
                aIDFBoard.AddDrill( drill, x, y, kplate, crefdes, pintype, IDF3::ECAD );
            }
        }
    }

    // add any valid models to the library item list
    std::string refdes;

    for( S3D_MASTER* modfile = aModule->Models(); modfile != 0; modfile = modfile->Next() )
    {
        if( !modfile->Is3DType( S3D_MASTER::FILE3D_IDF ) )
            continue;

        double rotz = modfile->m_MatRotation.z + aModule->GetOrientation()/10.0;
        double locx = modfile->m_MatPosition.x;
        double locy = modfile->m_MatPosition.y;
        double locz = modfile->m_MatPosition.z;

        bool top = ( aModule->GetLayer() == LAYER_N_BACK ) ? false : true;

        refdes = TO_UTF8( aModule->GetReference() );

        if( top )
        {
            locy = -locy;
            RotatePoint( &locx, &locy, aModule->GetOrientation() );
            locy = -locy;
        }
        if( !top )
        {
            RotatePoint( &locx, &locy, aModule->GetOrientation() );
            locy = -locy;

            rotz = 180.0 - rotz;

            if( rotz >= 360.0 )
                while( rotz >= 360.0 ) rotz -= 360.0;

            if( rotz <= -360.0 )
                while( rotz <= -360.0 ) rotz += 360.0;
        }

        locx += aModule->GetPosition().x * scale + dx;
        locy += -aModule->GetPosition().y * scale + dy;

        aIDFBoard.PlaceComponent(modfile->GetShape3DName(), refdes, locx, locy, locz, rotz, top);
    }

    return;
}


/**
 * Function Export_IDF3
 * generates IDFv3 compliant board (*.emn) and library (*.emp)
 * files representing the user's PCB design.
 */
bool Export_IDF3( BOARD* aPcb, const wxString& aFullFileName, double aUseThou )
{
    IDF_BOARD idfBoard;

    SetLocaleTo_C_standard();

    idfBoard.Setup( aPcb->GetFileName(), aFullFileName, aUseThou,
            aPcb->GetDesignSettings().GetBoardThickness() );

    // set up the global offsets
    EDA_RECT bbox = aPcb->ComputeBoundingBox( true );
    idfBoard.SetOffset( -bbox.Centre().x * idfBoard.GetScale(),
                         bbox.Centre().y * idfBoard.GetScale() );

    // Export the board outline
    idf_export_outline( aPcb, idfBoard );

    // Output the drill holes and module (library) data.
    for( MODULE* module = aPcb->m_Modules; module != 0; module = module->Next() )
        idf_export_module( aPcb, module, idfBoard );

    idfBoard.Finish();

    SetLocaleTo_Default();

    return true;
}
