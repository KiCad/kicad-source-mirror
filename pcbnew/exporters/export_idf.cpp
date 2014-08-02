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
#include <idf_parser.h>
#include <3d_struct.h>
#include <build_version.h>

// assumed default graphical line thickness: 10000 IU == 0.1mm
#define LINE_WIDTH (100000)

/**
 * Function idf_export_outline
 * retrieves line segment information from the edge layer and compiles
 * the data into a form which can be output as an IDFv3 compliant
 * BOARD_OUTLINE section.
 */
static void idf_export_outline( BOARD* aPcb, IDF3_BOARD& aIDFBoard )
{
    double scale = aIDFBoard.GetUserScale();

    DRAWSEGMENT* graphic;               // KiCad graphical item
    IDF_POINT sp, ep;                   // start and end points from KiCad item

    std::list< IDF_SEGMENT* > lines;    // IDF intermediate form of KiCad graphical item
    IDF_OUTLINE* outline = NULL;        // graphical items forming an outline or cutout

    // NOTE: IMPLEMENTATION
    // If/when component cutouts are allowed, we must implement them separately. Cutouts
    // must be added to the board outline section and not to the Other Outline section.
    // The module cutouts should be handled via the idf_export_module() routine.

    double offX, offY;
    aIDFBoard.GetUserOffset( offX, offY );

    // Retrieve segments and arcs from the board
    for( BOARD_ITEM* item = aPcb->m_Drawings; item; item = item->Next() )
    {
        if( item->Type() != PCB_LINE_T || item->GetLayer() != Edge_Cuts )
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
    outline = new IDF_OUTLINE;
    IDF3::GetOutline( lines, *outline );

    if( outline->empty() )
        goto UseBoundingBox;

    aIDFBoard.AddBoardOutline( outline );
    outline = NULL;

    // get all cutouts and write them out
    while( !lines.empty() )
    {
        if( !outline )
            outline = new IDF_OUTLINE;

        IDF3::GetOutline( lines, *outline );

        if( outline->empty() )
        {
            outline->Clear();
            continue;
        }

        aIDFBoard.AddBoardOutline( outline );
        outline = NULL;
    }

    return;

UseBoundingBox:

    // clean up if necessary
    while( !lines.empty() )
    {
        delete lines.front();
        lines.pop_front();
    }

    if( outline )
        outline->Clear();
    else
        outline = new IDF_OUTLINE;

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

    outline->push( new IDF_SEGMENT( p1, p2 ) );

    for( int i = 1; i < 4; ++i )
    {
        p1.x    = px[i - 1];
        p1.y    = py[i - 1];
        p2.x    = px[i];
        p2.y    = py[i];

        outline->push( new IDF_SEGMENT( p1, p2 ) );
    }

    aIDFBoard.AddBoardOutline( outline );
}


/**
 * Function idf_export_module
 * retrieves information from all board modules, adds drill holes to
 * the DRILLED_HOLES or BOARD_OUTLINE section as appropriate,
 * compiles data for the PLACEMENT section and compiles data for
 * the library ELECTRICAL section.
 */
static void idf_export_module( BOARD* aPcb, MODULE* aModule,
        IDF3_BOARD& aIDFBoard )
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
    // || (item->GetLayer() != Edge_Cuts ) ) continue;
    // code to export cutouts
    // }

    // Export pads
    double  drill, x, y;
    double  scale = aIDFBoard.GetUserScale();
    IDF3::KEY_PLATING kplate;
    std::string pintype;
    std::string tstr;

    double dx, dy;

    aIDFBoard.GetUserOffset( dx, dy );

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
                || ( kplate == IDF3::NPTH )
                ||( pad->GetDrillShape() == PAD_DRILL_OBLONG ) )
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
            if( ( pad->GetDrillShape() == PAD_DRILL_OBLONG )
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
                IDF_DRILL_DATA *dp = new IDF_DRILL_DATA( drill, x, y, kplate, crefdes,
                                                         pintype, IDF3::ECAD );

                if( !aIDFBoard.AddDrill( dp ) )
                {
                    delete dp;

                    std::ostringstream ostr;
                    ostr << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__;
                    ostr << "(): could not add drill";

                    throw std::runtime_error( ostr.str() );
                }
            }
        }
    }

    // add any valid models to the library item list
    std::string refdes;

    IDF3_COMPONENT* comp = NULL;

    for( S3D_MASTER* modfile = aModule->Models(); modfile != 0; modfile = modfile->Next() )
    {
        if( !modfile->Is3DType( S3D_MASTER::FILE3D_IDF ) )
            continue;

        if( refdes.empty() )
        {
            refdes = TO_UTF8( aModule->GetReference() );

            // NOREFDES cannot be used or else the software gets confused
            // when writing out the placement data due to conflicting
            // placement and layer specifications; to work around this we
            // create a (hopefully) unique refdes for our exported part.
            if( refdes.empty() || !refdes.compare( "~" ) )
                refdes = aIDFBoard.GetNewRefDes();
        }

        IDF3_COMP_OUTLINE* outline;

        outline = aIDFBoard.GetComponentOutline( modfile->GetShape3DName() );

        if( !outline )
            throw( std::runtime_error( aIDFBoard.GetError() ) );

        double rotz = aModule->GetOrientation()/10.0;
        double locx = modfile->m_MatPosition.x;
        double locy = modfile->m_MatPosition.y;
        double locz = modfile->m_MatPosition.z;
        double lrot = modfile->m_MatRotation.z;

        bool top = ( aModule->GetLayer() == B_Cu ) ? false : true;

        if( top )
        {
            rotz += modfile->m_MatRotation.z;
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

        if( comp == NULL )
            comp = aIDFBoard.FindComponent( refdes );

        if( comp == NULL )
        {
            comp = new IDF3_COMPONENT( &aIDFBoard );

            if( comp == NULL )
                throw( std::runtime_error( aIDFBoard.GetError() ) );

            comp->SetRefDes( refdes );

            if( top )
                comp->SetPosition( aModule->GetPosition().x * scale + dx,
                                   -aModule->GetPosition().y * scale + dy,
                                   rotz, IDF3::LYR_TOP );
            else
                comp->SetPosition( aModule->GetPosition().x * scale + dx,
                                   -aModule->GetPosition().y * scale + dy,
                                   rotz, IDF3::LYR_BOTTOM );

            comp->SetPlacement( IDF3::PS_ECAD );

            aIDFBoard.AddComponent( comp );
        }
        else
        {
            double refX, refY, refA;
            IDF3::IDF_LAYER side;

            if( ! comp->GetPosition( refX, refY, refA, side ) )
            {
                // place the item
                if( top )
                    comp->SetPosition( aModule->GetPosition().x * scale + dx,
                                       -aModule->GetPosition().y * scale + dy,
                                       rotz, IDF3::LYR_TOP );
                    else
                        comp->SetPosition( aModule->GetPosition().x * scale + dx,
                                           -aModule->GetPosition().y * scale + dy,
                                           rotz, IDF3::LYR_BOTTOM );
            }
            else
            {
                // check that the retrieved component matches this one
                refX = refX - ( aModule->GetPosition().x * scale + dx );
                refY = refY - ( -aModule->GetPosition().y * scale + dy );
                refA = refA - rotz;
                refA *= refA;
                refX *= refX;
                refY *= refY;
                refX += refY;

                // conditions: same side, X,Y coordinates within 10 microns,
                // angle within 0.01 degree
                if( ( top && side == IDF3::LYR_BOTTOM ) || ( !top && side == IDF3::LYR_TOP )
                    || ( refA > 0.0001 ) || ( refX > 0.0001 ) )
                {
                    comp->GetPosition( refX, refY, refA, side );

                    std::ostringstream ostr;
                    ostr << "* " << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ << "():\n";
                    ostr << "* conflicting Reference Designator '" << refdes << "'\n";
                    ostr << "* X loc: " << (aModule->GetPosition().x * scale + dx);
                    ostr << " vs. " << refX << "\n";
                    ostr << "* Y loc: " << (-aModule->GetPosition().y * scale + dy);
                    ostr << " vs. " << refY << "\n";
                    ostr << "* angle: " << rotz;
                    ostr << " vs. " << refA << "\n";

                    if( top )
                        ostr << "* TOP vs. ";
                    else
                        ostr << "* BOTTOM vs. ";

                    if( side == IDF3::LYR_TOP )
                        ostr << "TOP";
                    else
                        ostr << "BOTTOM";

                    throw( std::runtime_error( ostr.str() ) );
                }
            }
        }


        // create the local data ...
        IDF3_COMP_OUTLINE_DATA* data = new IDF3_COMP_OUTLINE_DATA( comp, outline );

        data->SetOffsets( locx, locy, locz, lrot );
        comp->AddOutlineData( data );
    }

    return;
}


/**
 * Function Export_IDF3
 * generates IDFv3 compliant board (*.emn) and library (*.emp)
 * files representing the user's PCB design.
 */
bool Export_IDF3( BOARD* aPcb, const wxString& aFullFileName, bool aUseThou )
{
    IDF3_BOARD idfBoard( IDF3::CAD_ELEC );

    SetLocaleTo_C_standard();

    bool ok = true;
    double scale = 1e-6;    // we must scale internal units to mm for IDF
    IDF3::IDF_UNIT idfUnit;

    if( aUseThou )
    {
        idfUnit = IDF3::UNIT_THOU;
        idfBoard.SetUserPrecision( 1 );
    }
    else
    {
        idfUnit = IDF3::UNIT_MM;
        idfBoard.SetUserPrecision( 5 );
    }

    wxFileName brdName = aPcb->GetFileName();

    idfBoard.SetUserScale( scale );
    idfBoard.SetBoardThickness( aPcb->GetDesignSettings().GetBoardThickness() * scale );
    idfBoard.SetBoardName( TO_UTF8( brdName.GetFullName() ) );
    idfBoard.SetBoardVersion( 0 );
    idfBoard.SetLibraryVersion( 0 );

    std::ostringstream ostr;
    ostr << "Created by KiCad " << TO_UTF8( GetBuildVersion() );
    idfBoard.SetIDFSource( ostr.str() );

    try
    {
        // set up the global offsets
        EDA_RECT bbox = aPcb->ComputeBoundingBox( true );
        idfBoard.SetUserOffset( -bbox.Centre().x * scale,
                            bbox.Centre().y * scale );

        // Export the board outline
        idf_export_outline( aPcb, idfBoard );

        // Output the drill holes and module (library) data.
        for( MODULE* module = aPcb->m_Modules; module != 0; module = module->Next() )
            idf_export_module( aPcb, module, idfBoard );

        if( !idfBoard.WriteFile( aFullFileName, idfUnit, false ) )
        {
            wxString msg;
            msg << _( "IDF Export Failed:\n" ) << FROM_UTF8( idfBoard.GetError().c_str() );
            wxMessageBox( msg );

            ok = false;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;
        msg << _( "IDF Export Failed:\n" ) << ioe.errorText;
        wxMessageBox( msg );

        ok = false;
    }
    catch( const std::exception& e )
    {
        wxString msg;
        msg << _( "IDF Export Failed:\n" ) << FROM_UTF8( e.what() );
        wxMessageBox( msg );
        ok = false;
    }

    SetLocaleTo_Default();

    return ok;
}
