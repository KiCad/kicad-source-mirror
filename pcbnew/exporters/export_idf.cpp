/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013  Cirilo Bernardo
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <list>
#include <locale_io.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <footprint_library_adapter.h>
#include <idf_parser.h>
#include <pad.h>
#include <pcb_shape.h>
#include <build_version.h>
#include <project_pcb.h>
#include <wx/msgdlg.h>
#include "project.h"
#include "3d_cache/3d_cache.h"
#include "filename_resolver.h"
#include "export_idf.h"


#include <base_units.h>     // to define pcbIUScale.FromMillimeter(x)


// assumed default graphical line thickness: == 0.1mm
#define LINE_WIDTH (pcbIUScale.mmToIU( 0.1 ))


static FILENAME_RESOLVER* resolver;


/**
 * Convert a single Edge_Cuts graphic into IDF segments and append them to @p aLines.
 *
 * Shared between the board outline and footprint cutout emitters so both encode identical
 * geometry into the #BOARD_OUTLINE section.
 */
static void idf_append_shape( PCB_SHAPE* aGraphic, double aScale, double aOffX, double aOffY,
                              std::list<IDF_SEGMENT*>& aLines )
{
    IDF_POINT sp, ep;                   // start and end points from KiCad item

    switch( aGraphic->GetShape() )
    {
    case SHAPE_T::SEGMENT:
    {
        if( aGraphic->GetStart() == aGraphic->GetEnd() )
            break;

        sp.x = aGraphic->GetStart().x * aScale + aOffX;
        sp.y = -aGraphic->GetStart().y * aScale + aOffY;
        ep.x = aGraphic->GetEnd().x * aScale + aOffX;
        ep.y = -aGraphic->GetEnd().y * aScale + aOffY;
        aLines.push_back( new IDF_SEGMENT( sp, ep ) );

        break;
    }

    case SHAPE_T::RECTANGLE:
    {
        if( aGraphic->GetStart() == aGraphic->GetEnd() )
            break;

        // IDF Y is up-positive, so mirror KiCad's down-positive Y like the other shapes do
        double top = -aGraphic->GetStart().y * aScale + aOffY;
        double left = aGraphic->GetStart().x * aScale + aOffX;
        double bottom = -aGraphic->GetEnd().y * aScale + aOffY;
        double right = aGraphic->GetEnd().x * aScale + aOffX;

        IDF_POINT corners[4];
        corners[0] = IDF_POINT( left, top );
        corners[1] = IDF_POINT( right, top );
        corners[2] = IDF_POINT( right, bottom );
        corners[3] = IDF_POINT( left, bottom );

        aLines.push_back( new IDF_SEGMENT( corners[0], corners[1] ) );
        aLines.push_back( new IDF_SEGMENT( corners[1], corners[2] ) );
        aLines.push_back( new IDF_SEGMENT( corners[2], corners[3] ) );
        aLines.push_back( new IDF_SEGMENT( corners[3], corners[0] ) );
        break;
    }

    case SHAPE_T::ARC:
    {
        if( aGraphic->GetCenter() == aGraphic->GetStart() )
            break;

        sp.x = aGraphic->GetCenter().x * aScale + aOffX;
        sp.y = -aGraphic->GetCenter().y * aScale + aOffY;
        ep.x = aGraphic->GetStart().x * aScale + aOffX;
        ep.y = -aGraphic->GetStart().y * aScale + aOffY;
        aLines.push_back( new IDF_SEGMENT( sp, ep, -aGraphic->GetArcAngle().AsDegrees(), true ) );

        break;
    }

    case SHAPE_T::CIRCLE:
    {
        if( aGraphic->GetRadius() == 0 )
            break;

        sp.x = aGraphic->GetCenter().x * aScale + aOffX;
        sp.y = -aGraphic->GetCenter().y * aScale + aOffY;
        ep.x = sp.x - aGraphic->GetRadius() * aScale;
        ep.y = sp.y;

        // Circles must always have an angle of +360 deg. to appease
        // quirky MCAD implementations of IDF.
        aLines.push_back( new IDF_SEGMENT( sp, ep, 360.0, true ) );

        break;
    }

    case SHAPE_T::POLY:
    {
        if( !aGraphic->IsPolyShapeValid() )
            break;

        // Holes within an outline have no IDF representation; each outline is its own loop
        const SHAPE_POLY_SET& polySet = aGraphic->GetPolyShape();

        for( int ii = 0; ii < polySet.OutlineCount(); ++ii )
        {
            const SHAPE_LINE_CHAIN& chain = polySet.COutline( ii );

            for( int jj = 0; jj < chain.PointCount(); ++jj )
            {
                const VECTOR2I& start = chain.CPoint( jj );
                const VECTOR2I& end = chain.CPoint( ( jj + 1 ) % chain.PointCount() );

                if( start == end )
                    continue;

                sp.x = start.x * aScale + aOffX;
                sp.y = -start.y * aScale + aOffY;
                ep.x = end.x * aScale + aOffX;
                ep.y = -end.y * aScale + aOffY;
                aLines.push_back( new IDF_SEGMENT( sp, ep ) );
            }
        }

        break;
    }

    case SHAPE_T::BEZIER:
    {
        aGraphic->RebuildBezierToSegmentsPointsList();

        const std::vector<VECTOR2I>& pts = aGraphic->GetBezierPoints();

        for( size_t ii = 1; ii < pts.size(); ++ii )
        {
            if( pts[ii - 1] == pts[ii] )
                continue;

            sp.x = pts[ii - 1].x * aScale + aOffX;
            sp.y = -pts[ii - 1].y * aScale + aOffY;
            ep.x = pts[ii].x * aScale + aOffX;
            ep.y = -pts[ii].y * aScale + aOffY;
            aLines.push_back( new IDF_SEGMENT( sp, ep ) );
        }

        break;
    }

    default:
        break;
    }
}


/**
 * Retrieve line segment information from the edge layer and compiles the data into a form
 * which can be output as an IDFv3 compliant #BOARD_OUTLINE section.
 */
static void idf_export_outline( BOARD* aPcb, IDF3_BOARD& aIDFBoard )
{
    double scale = aIDFBoard.GetUserScale();
    std::list< IDF_SEGMENT* > lines;    // IDF intermediate form of KiCad graphical item
    IDF_OUTLINE* outline = nullptr;     // graphical items forming an outline or cutout

    // Footprint cutouts are emitted separately by idf_export_footprint(); like board cutouts they
    // belong in the board outline section rather than the Other Outline section.

    double offX, offY;
    aIDFBoard.GetUserOffset( offX, offY );

    // Retrieve segments and arcs from the board
    for( BOARD_ITEM* item : aPcb->Drawings() )
    {
        if( item->Type() != PCB_SHAPE_T || item->GetLayer() != Edge_Cuts )
            continue;

        idf_append_shape( static_cast<PCB_SHAPE*>( item ), scale, offX, offY, lines );
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
    outline = nullptr;

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
        outline = nullptr;
    }

    // an open loop on the final iteration leaves an allocated but empty outline behind
    delete outline;

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

    // Fetch a rectangular bounding box for the board; there is always some uncertainty in the
    // board dimensions computed via ComputeBoundingBox() since this depends on the individual
    // footprint entities.
    BOX2I bbbox = aPcb->GetBoardEdgesBoundingBox();

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
 * Retrieve information from all board footprints, adds drill holes to the DRILLED_HOLES or
 * BOARD_OUTLINE section as appropriate,  Compiles data for the PLACEMENT section and compiles
 * data for the library ELECTRICAL section.
 */
static void idf_export_footprint( BOARD* aPcb, FOOTPRINT* aFootprint, IDF3_BOARD& aIDFBoard,
                                  bool aIncludeUnspecified, bool aIncludeDNP )
{
    // Reference Designator
    std::string crefdes = TO_UTF8( aFootprint->Reference().GetShownText( false ) );

    wxString libraryName = aFootprint->GetFPID().GetLibNickname();
    wxString footprintBasePath = wxEmptyString;

    if( aPcb->GetProject() )
    {
        std::optional<LIBRARY_TABLE_ROW*> fpRow =
                            PROJECT_PCB::FootprintLibAdapter( aPcb->GetProject() )->GetRow( libraryName );
        if( fpRow )
            footprintBasePath = LIBRARY_MANAGER::GetFullURI( *fpRow, true );
    }

    if( crefdes.empty() || !crefdes.compare( "~" ) )
    {
        std::string cvalue = TO_UTF8( aFootprint->Value().GetShownText( false ) );

        // if both the RefDes and Value are empty or set to '~' the board owns the part,
        // otherwise associated parts of the footprint must be marked NOREFDES.
        if( cvalue.empty() || !cvalue.compare( "~" ) )
            crefdes = "BOARD";
        else
            crefdes = "NOREFDES";
    }

    // Export pads
    double  drill, x, y;
    double  scale = aIDFBoard.GetUserScale();
    IDF3::KEY_PLATING kplate;
    std::string pintype;
    std::string tstr;

    double dx, dy;

    aIDFBoard.GetUserOffset( dx, dy );

    // Footprint Edge_Cuts graphics are board cutouts. IDF has no per-component cutout, so they are
    // appended to the board outline section where any loop after the first is treated as a cutout.
    std::list<IDF_SEGMENT*> cutoutLines;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T || item->GetLayer() != Edge_Cuts )
            continue;

        idf_append_shape( static_cast<PCB_SHAPE*>( item ), scale, dx, dy, cutoutLines );
    }

    // GetOutline() consumes at least one segment per call, so this terminates even on open loops.
    while( !cutoutLines.empty() )
    {
        IDF_OUTLINE* cutout = new IDF_OUTLINE;
        IDF3::GetOutline( cutoutLines, *cutout );

        if( cutout->empty() )
        {
            delete cutout;
            continue;
        }

        // ownership transfers only on success
        if( !aIDFBoard.AddBoardOutline( cutout ) )
            delete cutout;
    }

    for( auto pad : aFootprint->Pads() )
    {
        drill = (double) pad->GetDrillSize().x * scale;
        x     = pad->GetPosition().x * scale + dx;
        y     = -pad->GetPosition().y * scale + dy;

        // Export the hole on the edge layer
        if( drill > 0.0 )
        {
            // plating
            if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                kplate = IDF3::NPTH;
            else
                kplate = IDF3::PTH;

            // hole type
            tstr = TO_UTF8( pad->GetNumber() );

            if( tstr.empty() || !tstr.compare( "0" ) || !tstr.compare( "~" )
                || ( kplate == IDF3::NPTH )
                || ( pad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG ) )
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
            if( ( pad->GetDrillShape() == PAD_DRILL_SHAPE::OBLONG )
                && ( pad->GetDrillSize().x != pad->GetDrillSize().y ) )
            {
                // NOTE: IDF does not have direct support for slots;
                // slots are implemented as a board cutout and we
                // cannot represent plating or reference designators

                double dlength = pad->GetDrillSize().y * scale;

                // NOTE: The orientation of footprints and pads have
                // the opposite sense due to KiCad drawing on a
                // screen with a LH coordinate system
                double angle = pad->GetOrientation().AsDegrees();

                // NOTE: Since this code assumes the scenario where
                // GetDrillSize().y is the length but idf_parser.cpp
                // assumes a length along the X axis, the orientation
                // must be shifted +90 deg when GetDrillSize().y is
                // the major axis.

                if( dlength < drill )
                {
                    std::swap( drill, dlength );
                }
                else
                {
                    angle += 90.0;
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

    if( ( !(aFootprint->GetAttributes() & (FP_THROUGH_HOLE|FP_SMD)) ) && !aIncludeUnspecified )
        return;

    if( aFootprint->GetDNPForVariant( aPcb ? aPcb->GetCurrentVariant() : wxString() )
            && !aIncludeDNP )
        return;

    // add any valid models to the library item list
    std::string refdes;

    IDF3_COMPONENT* comp = nullptr;

    auto sM = aFootprint->Models().begin();
    auto eM = aFootprint->Models().end();
    wxFileName idfFile;
    wxString   idfExt;

    while( sM != eM )
    {
        if( !sM->m_Show )
        {
            ++sM;
            continue;
        }

        std::vector<const EMBEDDED_FILES*> embeddedFilesStack;
        embeddedFilesStack.push_back( aFootprint->GetEmbeddedFiles() );
        embeddedFilesStack.push_back( aPcb->GetEmbeddedFiles() );

        idfFile.Assign( resolver->ResolvePath( sM->m_Filename, footprintBasePath, std::move( embeddedFilesStack ) ) );
        idfExt = idfFile.GetExt();

        if( idfExt.Cmp( wxT( "idf" ) ) && idfExt.Cmp( wxT( "IDF" ) ) )
        {
            ++sM;
            continue;
        }

        if( refdes.empty() )
        {
            refdes = TO_UTF8( aFootprint->Reference().GetShownText( false ) );

            // NOREFDES cannot be used or else the software gets confused
            // when writing out the placement data due to conflicting
            // placement and layer specifications; to work around this we
            // create a (hopefully) unique refdes for our exported part.
            if( refdes.empty() || !refdes.compare( "~" ) )
                refdes = aIDFBoard.GetNewRefDes();
        }

        IDF3_COMP_OUTLINE* outline;

        outline = aIDFBoard.GetComponentOutline( idfFile.GetFullPath() );

        if( !outline )
            throw( std::runtime_error( aIDFBoard.GetError() ) );

        double rotz = aFootprint->GetOrientation().AsDegrees();
        double locx = sM->m_Offset.x;  // part offsets are in mm
        double locy = sM->m_Offset.y;
        double locz = sM->m_Offset.z;
        double lrot = sM->m_Rotation.z;

        bool top = ( aFootprint->GetLayer() == B_Cu ) ? false : true;

        if( top )
        {
            locy = -locy;
            RotatePoint( &locx, &locy, aFootprint->GetOrientation() );
            locy = -locy;
        }

        if( !top )
        {
            lrot = -lrot;
            RotatePoint( &locx, &locy, aFootprint->GetOrientation() );
            locy = -locy;

            rotz = 180.0 - rotz;

            if( rotz >= 360.0 )
                while( rotz >= 360.0 ) rotz -= 360.0;

            if( rotz <= -360.0 )
                while( rotz <= -360.0 ) rotz += 360.0;
        }

        if( comp == nullptr )
            comp = aIDFBoard.FindComponent( refdes );

        if( comp == nullptr )
        {
            comp = new IDF3_COMPONENT( &aIDFBoard );

            if( comp == nullptr )
                throw( std::runtime_error( aIDFBoard.GetError() ) );

            comp->SetRefDes( refdes );

            if( top )
            {
                comp->SetPosition( aFootprint->GetPosition().x * scale + dx,
                                   -aFootprint->GetPosition().y * scale + dy,
                                   rotz, IDF3::LYR_TOP );
            }
            else
            {
                comp->SetPosition( aFootprint->GetPosition().x * scale + dx,
                                   -aFootprint->GetPosition().y * scale + dy,
                                   rotz, IDF3::LYR_BOTTOM );
            }

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
                {
                    comp->SetPosition( aFootprint->GetPosition().x * scale + dx,
                                       -aFootprint->GetPosition().y * scale + dy,
                                       rotz, IDF3::LYR_TOP );
                }
                else
                {
                    comp->SetPosition( aFootprint->GetPosition().x * scale + dx,
                                       -aFootprint->GetPosition().y * scale + dy,
                                       rotz, IDF3::LYR_BOTTOM );
                }

                comp->SetPlacement( IDF3::PS_ECAD );

            }
            else
            {
                // check that the retrieved component matches this one
                refX = refX - ( aFootprint->GetPosition().x * scale + dx );
                refY = refY - ( -aFootprint->GetPosition().y * scale + dy );
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
                    ostr << "* X loc: " << ( aFootprint->GetPosition().x * scale + dx);
                    ostr << " vs. " << refX << "\n";
                    ostr << "* Y loc: " << ( -aFootprint->GetPosition().y * scale + dy);
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
        ++sM;
    }
}


/**
 * Generate IDFv3 compliant board (*.emn) and library (*.emp) files representing the user's
 * PCB design.
 *
 * Split out from PCB_EDIT_FRAME::Export_IDF3 so it can be driven headlessly (unit tests, CLI)
 * with an explicitly supplied 3D model resolver and no GUI error reporting.
 */
bool ExportBoardToIDF3( BOARD* aPcb, const wxString& aFullFileName, bool aUseThou, double aXRef,
                        double aYRef, bool aIncludeUnspecified, bool aIncludeDNP,
                        FILENAME_RESOLVER* aResolver, wxString* aErrorMsg )
{
    // idf_export_footprint dereferences the resolver for every 3D model, so a null one
    // must fail up front rather than crash mid-export
    wxCHECK( aResolver, false );

    IDF3_BOARD idfBoard( IDF3::CAD_ELEC );

    // Switch the locale to standard C (needed to print floating point numbers)
    LOCALE_IO toggle;

    resolver = aResolver;

    bool ok = true;
    double scale = pcbIUScale.MM_PER_IU;   // we must scale internal units to mm for IDF
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
    ostr << "KiCad " << TO_UTF8( GetBuildVersion() );
    idfBoard.SetIDFSource( ostr.str() );

    try
    {
        // set up the board reference point
        idfBoard.SetUserOffset( -aXRef, aYRef );

        // Export the board outline
        idf_export_outline( aPcb, idfBoard );

        // Output the drill holes and footprint (library) data.
        for( FOOTPRINT* footprint : aPcb->Footprints() )
            idf_export_footprint( aPcb, footprint, idfBoard, aIncludeUnspecified, aIncludeDNP );

        if( !idfBoard.WriteFile( aFullFileName, idfUnit, false ) )
        {
            if( aErrorMsg )
                *aErrorMsg = From_UTF8( idfBoard.GetError().c_str() );

            ok = false;
        }
    }
    catch( const IO_ERROR& ioe )
    {
        if( aErrorMsg )
            *aErrorMsg = ioe.What();

        ok = false;
    }
    catch( const std::exception& e )
    {
        if( aErrorMsg )
            *aErrorMsg = From_UTF8( e.what() );

        ok = false;
    }

    return ok;
}


bool PCB_EDIT_FRAME::Export_IDF3( BOARD* aPcb, const wxString& aFullFileName,
                                  bool aUseThou, double aXRef, double aYRef,
                                  bool aIncludeUnspecified, bool aIncludeDNP )
{
    FILENAME_RESOLVER* res = PROJECT_PCB::Get3DCacheManager( &Prj() )->GetResolver();
    wxString           errorMsg;

    bool ok = ExportBoardToIDF3( aPcb, aFullFileName, aUseThou, aXRef, aYRef, aIncludeUnspecified,
                                 aIncludeDNP, res, &errorMsg );

    if( !ok )
    {
        wxString msg;
        msg << _( "IDF Export Failed:\n" ) << errorMsg;
        wxMessageBox( msg );
    }

    return ok;
}
