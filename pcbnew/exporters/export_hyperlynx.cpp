/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
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

#include <cstdio>
#include <vector>

#include <wx/log.h>
#include <wx/filedlg.h>

#include <kiface_base.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <board_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <ki_exception.h>
#include <locale_io.h>
#include <reporter.h>
#include <richio.h>
#include <string_utils.h>
#include <tools/board_editor_control.h>
#include <exporters/board_exporter_base.h>


static double iu2hyp( double iu )
{
    return iu / 1e9 / 0.0254;
}


class HYPERLYNX_EXPORTER;

class HYPERLYNX_PAD_STACK
{
public:
    friend class HYPERLYNX_EXPORTER;

    HYPERLYNX_PAD_STACK( BOARD* aBoard, const PAD* aPad );
    HYPERLYNX_PAD_STACK( BOARD* aBoard, const PCB_VIA* aVia );
    ~HYPERLYNX_PAD_STACK(){};

    bool IsThrough() const
    {
        return m_type == PAD_ATTRIB::NPTH || m_type == PAD_ATTRIB::PTH;
    }

    bool operator==( const HYPERLYNX_PAD_STACK& other ) const
    {
        if( m_shape != other.m_shape )
            return false;

        if( m_type != other.m_type )
            return false;

        if( IsThrough() && other.IsThrough() && m_drill != other.m_drill )
            return false;

        if( m_sx != other.m_sx )
            return false;

        if( m_sy != other.m_sy )
            return false;

        if( m_layers != other.m_layers )
            return false;

        if( m_angle != other.m_angle )
            return false;

        return true;
    }

    void SetId( int id )
    {
        m_id = id;
    }

    int GetId() const
    {
        return m_id;
    }

    bool IsEmpty() const
    {
        LSET outLayers = m_layers & LSET::AllCuMask( m_board->GetCopperLayerCount() );

        return outLayers.none();
    }

private:
    BOARD*      m_board;
    int         m_id;
    int         m_drill;
    PAD_SHAPE   m_shape;
    int         m_sx, m_sy;
    double      m_angle;
    LSET        m_layers;
    PAD_ATTRIB  m_type;
};


class HYPERLYNX_EXPORTER : public BOARD_EXPORTER_BASE
{
public:
    HYPERLYNX_EXPORTER() : m_polyId( 1 )
    {
    }

    ~HYPERLYNX_EXPORTER(){};

    virtual bool Run() override;

private:
    HYPERLYNX_PAD_STACK* addPadStack( HYPERLYNX_PAD_STACK stack )
    {
        for( HYPERLYNX_PAD_STACK* p : m_padStacks )
        {
            if( *p == stack )
                return p;
        }

        stack.SetId( m_padStacks.size() );
        m_padStacks.push_back( new HYPERLYNX_PAD_STACK( stack ) );

        return m_padStacks.back();
    }

    const std::string formatPadShape( const HYPERLYNX_PAD_STACK& aStack )
    {
        int  shapeId = 0;
        char buf[1024];

        switch( aStack.m_shape )
        {
        case PAD_SHAPE::CIRCLE:
        case PAD_SHAPE::OVAL:
            shapeId = 0;
            break;

        case PAD_SHAPE::ROUNDRECT:
            shapeId = 2;
            break;

        case PAD_SHAPE::RECTANGLE:
            shapeId = 1;
            break;

        default:
            if( m_reporter )
            {
                m_reporter->Report( _( "File contains pad shapes that are not supported by the "
                                       "Hyperlynx exporter (supported shapes are oval, rectangle, "
                                       "rounded rectangle, and circle)." ),
                                    RPT_SEVERITY_WARNING );
                m_reporter->Report( _( "They have been exported as oval pads." ),
                                    RPT_SEVERITY_INFO );
            }

            shapeId = 0;
            break;
        }

        snprintf( buf, sizeof( buf ), "%d, %.9f, %.9f, %.1f, M",
                  shapeId,
                  iu2hyp( aStack.m_sx ),
                  iu2hyp( aStack.m_sy ),
                  aStack.m_angle );

        return buf;
    }

    bool generateHeaders();
    bool writeBoardInfo();
    bool writeStackupInfo();
    bool writeDevices();
    bool writePadStacks();
    bool writeNets();
    bool writeNetObjects( const std::vector<BOARD_ITEM*>& aObjects );


    void writeSinglePadStack( HYPERLYNX_PAD_STACK& aStack );

    const std::vector<BOARD_ITEM*> collectNetObjects( int netcode );

private:
    std::vector<HYPERLYNX_PAD_STACK*>           m_padStacks;
    std::map<BOARD_ITEM*, HYPERLYNX_PAD_STACK*> m_padMap;

    std::shared_ptr<FILE_OUTPUTFORMATTER>       m_out;
    int                                         m_polyId;
};


HYPERLYNX_PAD_STACK::HYPERLYNX_PAD_STACK( BOARD* aBoard, const PAD* aPad )
{
    // TODO(JE) padstacks
    m_board = aBoard;
    m_sx    = aPad->GetSize( PADSTACK::ALL_LAYERS ).x;
    m_sy    = aPad->GetSize( PADSTACK::ALL_LAYERS ).y;
    m_angle = 180.0 - aPad->GetOrientation().AsDegrees();

    if( m_angle < 0.0 )
        m_angle += 360.0;

    m_layers = aPad->GetLayerSet();
    m_drill  = aPad->GetDrillSize().x;
    m_shape  = aPad->GetShape( PADSTACK::ALL_LAYERS );
    m_type   = PAD_ATTRIB::PTH;
    m_id     = 0;
}


HYPERLYNX_PAD_STACK::HYPERLYNX_PAD_STACK( BOARD* aBoard, const PCB_VIA* aVia )
{
    m_board  = aBoard;
    // TODO(JE) padstacks
    m_sx = m_sy = aVia->GetWidth( PADSTACK::ALL_LAYERS );
    m_angle  = 0;
    m_layers = aVia->GetLayerSet();
    m_drill  = aVia->GetDrillValue();
    m_shape  = PAD_SHAPE::CIRCLE;
    m_type   = PAD_ATTRIB::PTH;
    m_id     = 0;
}


bool HYPERLYNX_EXPORTER::generateHeaders()
{
    m_out->Print( 0, "{VERSION=2.14}\n" );
    m_out->Print( 0, "{UNITS=ENGLISH LENGTH}\n\n" );
    return true;
}


void HYPERLYNX_EXPORTER::writeSinglePadStack( HYPERLYNX_PAD_STACK& aStack )
{
    LSET layerMask = LSET::AllCuMask( m_board->GetCopperLayerCount() );
    LSET outLayers = aStack.m_layers & layerMask;

    if( outLayers.none() )
        return;

    m_out->Print( 0, "{PADSTACK=%d, %.9f\n", aStack.m_id, iu2hyp( aStack.m_drill ) );

    if( outLayers == layerMask )
    {
        m_out->Print( 1, "(\"MDEF\", %s)\n", formatPadShape( aStack ).c_str() );
    }
    else
    {
        for( PCB_LAYER_ID l : outLayers.Seq() )
        {
            m_out->Print( 1, "(\"%s\", %s)\n",
                          (const char*) m_board->GetLayerName( l ).c_str(),
                          formatPadShape( aStack ).c_str() );
        }
    }

    m_out->Print( 0, "}\n\n" );
}


bool HYPERLYNX_EXPORTER::writeBoardInfo()
{
    SHAPE_POLY_SET outlines;

    m_out->Print( 0, "{BOARD \"%s\"\n", (const char*) m_board->GetFileName().c_str() );

    if( !m_board->GetBoardPolygonOutlines( outlines, false ) )
    {
        wxLogError( _( "Board outline is malformed. Run DRC for a full analysis." ) );
        return false;
    }

    for( int o = 0; o < outlines.OutlineCount(); o++ )
    {
        const SHAPE_LINE_CHAIN& outl = outlines.COutline( o );

        for( int i = 0; i < outl.SegmentCount(); i++ )
        {
            const SEG& s = outl.CSegment( i );
            m_out->Print( 1, "(PERIMETER_SEGMENT X1=%.9f Y1=%.9f X2=%.9f Y2=%.9f)\n",
                          iu2hyp( s.A.x ),
                          iu2hyp( -s.A.y ),
                          iu2hyp( s.B.x ),
                          iu2hyp( -s.B.y ) );
        }
    }

    m_out->Print( 0, "}\n\n" );

    return true;
}


bool HYPERLYNX_EXPORTER::writeStackupInfo()
{
    /* Format:
     * {STACKUP
     * (SIGNAL T=thickness [P=plating_thickness] [C=constant] L=layer_name [M=material_name]) [comment]
     * (DIELECTRIC T=thickness [C=constant] [L=layer_name] [M=material_name]) [comment]
     * }
     * name length is <= 20 chars
     */

    LSEQ layers = m_board->GetDesignSettings().GetEnabledLayers().CuStack();

    // Get the board physical stackup structure
    const BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();

    m_out->Print( 0, "{STACKUP\n" );

    wxString layer_name;    // The last copper layer name used in stackup

    for( BOARD_STACKUP_ITEM* item: stackup.GetList() )
    {
        if( item->GetType() == BS_ITEM_TYPE_COPPER )
        {
            layer_name = m_board->GetLayerName( item->GetBrdLayerId() );
            int plating_thickness = 0;
            double resistivity = 1.724e-8;  // Good for copper
            m_out->Print( 1, "(SIGNAL T=%g P=%g C=%g L=\"%.20s\" M=COPPER)\n",
                          iu2hyp( item->GetThickness( 0 ) ),
                          iu2hyp( plating_thickness ),
                          resistivity,
                          TO_UTF8( layer_name ) );
        }
        else if( item->GetType() == BS_ITEM_TYPE_DIELECTRIC )
        {
            if( item->GetSublayersCount() < 2 )
            {
                m_out->Print( 1, "(DIELECTRIC T=%g C=%g L=\"DE_%.17s\" M=\"%.20s\")\n",
                              iu2hyp( item->GetThickness( 0 ) ),
                              item->GetEpsilonR( 0 ),
                              TO_UTF8( layer_name ),
                              TO_UTF8( item->GetMaterial( 0 ) ) );
            }
            else for( int idx = 0; idx < item->GetSublayersCount(); idx++ )
            {
                m_out->Print( 1, "(DIELECTRIC T=%g C=%g L=\"DE%d_%.16s\" M=\"%.20s\")\n",
                              iu2hyp( item->GetThickness( idx ) ),
                              item->GetEpsilonR( idx ),
                              idx,
                              TO_UTF8( layer_name ),
                              TO_UTF8( item->GetMaterial( idx ) ) );
            }
        }
    }

    m_out->Print( 0, "}\n\n" );

    return true;
}


bool HYPERLYNX_EXPORTER::writeDevices()
{
    m_out->Print( 0, "{DEVICES\n" );

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        wxString ref = footprint->GetReference();
        wxString layerName = m_board->GetLayerName( footprint->GetLayer() );

        if( ref.IsEmpty() )
            ref = wxT( "EMPTY" );

        m_out->Print( 1, "(? REF=\"%s\" L=\"%s\")\n",
                      (const char*) ref.c_str(),
                      (const char*) layerName.c_str() );
    }
    m_out->Print( 0, "}\n\n" );

    return true;
}


bool HYPERLYNX_EXPORTER::writePadStacks()
{
    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            HYPERLYNX_PAD_STACK* ps = addPadStack( HYPERLYNX_PAD_STACK( m_board, pad ) );
            m_padMap[pad] = ps;
        }
    }

    for( PCB_TRACK* track : m_board->Tracks() )
    {
        if( PCB_VIA* via = dyn_cast<PCB_VIA*>( track ) )
        {
            HYPERLYNX_PAD_STACK* ps = addPadStack( HYPERLYNX_PAD_STACK( m_board, via ) );
            m_padMap[via] = ps;
        }
    }

    for( HYPERLYNX_PAD_STACK* pstack : m_padStacks )
        writeSinglePadStack( *pstack );

    return true;
}


bool HYPERLYNX_EXPORTER::writeNetObjects( const std::vector<BOARD_ITEM*>& aObjects )
{
    for( BOARD_ITEM* item : aObjects )
    {
        if( PAD* pad = dyn_cast<PAD*>( item ) )
        {
            auto pstackIter = m_padMap.find( pad );

            if( pstackIter != m_padMap.end() )
            {
                wxString ref = pad->GetParentFootprint()->GetReference();

                if( ref.IsEmpty() )
                    ref = wxT( "EMPTY" );

                wxString padName = pad->GetNumber();

                if( padName.IsEmpty() )
                    padName = wxT( "1" );


                m_out->Print( 1, "(PIN X=%.10f Y=%.10f R=\"%s.%s\" P=%d)\n",
                              iu2hyp( pad->GetPosition().x ),
                              iu2hyp( -pad->GetPosition().y ),
                              (const char*) ref.c_str(),
                              (const char*) padName.c_str(),
                              pstackIter->second->GetId() );
            }
        }
        else if( PCB_VIA* via = dyn_cast<PCB_VIA*>( item ) )
        {
            auto pstackIter = m_padMap.find( via );

            if( pstackIter != m_padMap.end() )
            {
                m_out->Print( 1, "(VIA X=%.10f Y=%.10f P=%d)\n",
                              iu2hyp( via->GetPosition().x ),
                              iu2hyp( -via->GetPosition().y ),
                              pstackIter->second->GetId() );
            }
        }
        else if( PCB_TRACK* track = dyn_cast<PCB_TRACK*>( item ) )
        {
            const wxString layerName = m_board->GetLayerName( track->GetLayer() );

            m_out->Print( 1, "(SEG X1=%.10f Y1=%.10f X2=%.10f Y2=%.10f W=%.10f L=\"%s\")\n",
                          iu2hyp( track->GetStart().x ),
                          iu2hyp( -track->GetStart().y ),
                          iu2hyp( track->GetEnd().x ),
                          iu2hyp( -track->GetEnd().y ),
                          iu2hyp( track->GetWidth() ),
                          (const char*) layerName.c_str() );
        }
        else if( PCB_ARC* arc = dyn_cast<PCB_ARC*>( item ) )
        {
            const wxString layerName = m_board->GetLayerName( arc->GetLayer() );
            VECTOR2I       start = arc->GetStart();
            VECTOR2I       end = arc->GetEnd();

            if( !arc->IsCCW() )
                std::swap( start, end );

            m_out->Print( 1, "(ARC X1=%.10f Y1=%.10f X2=%.10f Y2=%.10f XC=%.10f YC=%.10f R=%.10f W=%.10f L=\"%s\")\n",
                          iu2hyp( start.x ),
                          iu2hyp( -start.y ),
                          iu2hyp( end.x ),
                          iu2hyp( -end.y ),
                          iu2hyp( arc->GetCenter().x ),
                          iu2hyp( -arc->GetCenter().y ),
                          iu2hyp( arc->GetRadius() ),
                          iu2hyp( arc->GetWidth() ),
                          (const char*) layerName.c_str() );
        }
        else if( ZONE* zone = dyn_cast<ZONE*>( item ) )
        {
            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                const wxString layerName   = m_board->GetLayerName( layer );
                SHAPE_POLY_SET fill = zone->GetFilledPolysList( layer )->CloneDropTriangulation();

                fill.Simplify();

                for( int i = 0; i < fill.OutlineCount(); i++ )
                {
                    const SHAPE_LINE_CHAIN& outl = fill.COutline( i );
                    const VECTOR2I          p0 = outl.CPoint( 0 );

                    m_out->Print( 1, "{POLYGON T=POUR L=\"%s\" ID=%d X=%.10f Y=%.10f\n",
                                  (const char*) layerName.c_str(),
                                  m_polyId,
                                  iu2hyp( p0.x ),
                                  iu2hyp( -p0.y ) );

                    for( int v = 0; v < outl.PointCount(); v++ )
                    {
                        m_out->Print( 2, "(LINE X=%.10f Y=%.10f)\n",
                                      iu2hyp( outl.CPoint( v ).x ),
                                      iu2hyp( -outl.CPoint( v ).y ) );
                    }

                    m_out->Print( 2, "(LINE X=%.10f Y=%.10f)\n", iu2hyp( p0.x ), iu2hyp( -p0.y ) );
                    m_out->Print( 1, "}\n" );

                    for( int h = 0; h < fill.HoleCount( i ); h++ )
                    {
                        const SHAPE_LINE_CHAIN& holeShape = fill.CHole( i, h );
                        const VECTOR2I          ph0       = holeShape.CPoint( 0 );

                        m_out->Print( 1, "{POLYVOID ID=%d X=%.10f Y=%.10f\n",
                                      m_polyId,
                                      iu2hyp( ph0.x ),
                                      iu2hyp( -ph0.y ) );

                        for( int v = 0; v < holeShape.PointCount(); v++ )
                        {
                            m_out->Print( 2, "(LINE X=%.10f Y=%.10f)\n",
                                          iu2hyp( holeShape.CPoint( v ).x ),
                                          iu2hyp( -holeShape.CPoint( v ).y ) );
                        }

                        m_out->Print( 2, "(LINE X=%.10f Y=%.10f)\n",
                                      iu2hyp( ph0.x ),
                                      iu2hyp( -ph0.y ) );
                        m_out->Print( 1, "}\n" );
                    }

                    m_polyId++;
                }
            }
        }
    }

    return true;
}


const std::vector<BOARD_ITEM*> HYPERLYNX_EXPORTER::collectNetObjects( int netcode )
{
    std::vector<BOARD_ITEM*> rv;

    auto check =
            [&]( BOARD_CONNECTED_ITEM* item ) -> bool
            {
                if( ( item->GetLayerSet() & LSET::AllCuMask() ).none() )
                    return false;

                if( item->GetNetCode() == netcode || ( netcode < 0 && item->GetNetCode() <= 0 ) )
                    return true;

                return false;
            };

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            if( check( pad ) )
                rv.push_back( pad );
        }
    }

    for( PCB_TRACK* item : m_board->Tracks() )
    {
        if( check( item ) )
            rv.push_back( item );
    }

    for( ZONE* zone : m_board->Zones() )
    {
        if( check( zone ) )
            rv.push_back( zone );
    }

    return rv;
}


bool HYPERLYNX_EXPORTER::writeNets()
{
    m_polyId = 1;

    for( const NETINFO_ITEM* netInfo : m_board->GetNetInfo() )
    {
        int  netcode = netInfo->GetNetCode();
        bool isNullNet = netInfo->GetNetCode() <= 0 || netInfo->GetNetname().IsEmpty();

        if( isNullNet )
            continue;

        const std::vector<BOARD_ITEM*> netObjects = collectNetObjects( netcode );

        if( netObjects.size() )
        {
            m_out->Print( 0, "{NET=\"%s\"\n", (const char*) netInfo->GetNetname().c_str() );
            writeNetObjects( netObjects );
            m_out->Print( 0, "}\n\n" );
        }
    }

    const std::vector<BOARD_ITEM*> nullNetObjects = collectNetObjects( -1 );

    int idx = 0;

    for( BOARD_ITEM* item : nullNetObjects )
    {
        m_out->Print( 0, "{NET=\"EmptyNet%d\"\n", idx );
        writeNetObjects( { item } );
        m_out->Print( 0, "}\n\n" );
        idx++;
    }

    return true;
}


bool HYPERLYNX_EXPORTER::Run()
{
    LOCALE_IO toggle; // toggles on, then off, the C locale.

    try
    {
        m_out.reset( new FILE_OUTPUTFORMATTER( m_outputFilePath.GetFullPath() ) );

        generateHeaders();
        writeBoardInfo();
        writeStackupInfo();
        writeDevices();
        writePadStacks();
        writeNets();
    }
    catch( IO_ERROR& )
    {
        return false;
    }

    return true;
}


int BOARD_EDITOR_CONTROL::ExportHyperlynx( const TOOL_EVENT& aEvent )
{
    wxString    wildcard =  wxT( "*.hyp" );
    BOARD*      board = m_frame->GetBoard();
    wxFileName  fn = board->GetFileName();

    fn.SetExt( wxT("hyp") );

    wxFileDialog dlg( m_frame, _( "Export Hyperlynx Layout" ), fn.GetPath(), fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    fn = dlg.GetPath();

    // always enforce filename extension, user may not have entered it.
    fn.SetExt( wxT( "hyp" ) );

    HYPERLYNX_EXPORTER exporter;
    exporter.SetBoard( board );
    exporter.SetOutputFilename( fn );
    exporter.Run();

    return 0;
}
