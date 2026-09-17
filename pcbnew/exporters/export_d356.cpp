/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2013 Lorenzo Marcantonio <l.marcantonio@logossrl.com>
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

/**
 * @file export_d356.cpp
 * @brief Export IPC-D-356 test format
 */
#include "export_d356.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <vector>

#include <wx/checkbox.h>
#include <wx/filedlg.h>
#include <wx/filedlgcustomize.h>
#include <wx/msgdlg.h>
#include <kiplatform/ui.h>

#include <confirm.h>
#include <gestfich.h>
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <build_version.h>
#include <macros.h>
#include <wildcards_and_files_ext.h>
#include <locale_io.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <layer_range.h>
#include <pad.h>
#include <pcb_track.h>
#include <string_utils.h>

#include <math/util.h>      // for KiROUND
#include <tools/board_editor_control.h>


// D-356 numbers copper layers physically from 1 (front) to the copper layer count (back)
static int physical_layer( BOARD* aPcb, PCB_LAYER_ID aLayer )
{
    if( aLayer == B_Cu )
        return aPcb->GetCopperLayerCount();

    return static_cast<int>( CopperLayerToOrdinal( aLayer ) ) + 1;
}


// Compute the access code for a pad. Returns -1 if there is no copper
static int compute_pad_access_code( BOARD *aPcb, LSET aLayerMask )
{
    // Non-copper is not interesting here
    aLayerMask &= LSET::AllCuMask();

    if( !aLayerMask.any() )
        return -1;

    // Traditional TH pad
    if( aLayerMask[F_Cu] && aLayerMask[B_Cu] )
        return 0;

    // Front SMD pad
    if( aLayerMask[F_Cu] )
        return 1;

    // Back SMD pad
    if( aLayerMask[B_Cu] )
        return aPcb->GetCopperLayerCount();

    // OK, we have an inner-layer only pad (and I have no idea about
    // what could be used for); anyway, find the first copper layer
    // it's on
    for( PCB_LAYER_ID layer : LAYER_RANGE( In1_Cu, B_Cu, aPcb->GetCopperLayerCount() ) )
    {
        if( aLayerMask[layer] )
            return physical_layer( aPcb, layer );
    }

    // This shouldn't happen
    return -1;
}

/* Convert and clamp a size from IU to decimils */
static int iu_to_d356(int iu, int clamp)
{
    int val = KiROUND( static_cast<double>( iu ) / ( pcbIUScale.IU_PER_MILS / 10.0 ) );
    if( val > clamp ) return clamp;
    if( val < -clamp ) return -clamp;
    return val;
}

/* Extract the D356 record from the footprints (pads) */
void IPC356D_WRITER::build_pad_testpoints( BOARD *aPcb, std::vector <D356_RECORD>& aRecords )
{
    VECTOR2I origin = aPcb->GetDesignSettings().GetAuxOrigin();

    for( FOOTPRINT* footprint : aPcb->Footprints() )
    {
        for( PAD* pad : footprint->Pads() )
        {
            D356_RECORD rk;
            rk.access = compute_pad_access_code( aPcb, pad->GetLayerSet() );

            // It could be a mask only pad, we only handle pads with copper here
            if( rk.access == -1 )
                continue;

            if( m_doNotExportUnconnectedPads && pad->GetNetCode() == NETINFO_LIST::UNCONNECTED )
                continue;

            rk.netname = pad->GetNetname();
            rk.pin = pad->GetNumber();
            rk.refdes = footprint->GetReference();
            // A named pad is a component terminal (end-net point); an unnamed copper
            // feature is only reachable mid-net, so it is a midpoint per IPC-D-356A
            rk.midpoint = rk.pin.IsEmpty();
            const VECTOR2I& drill = pad->GetDrillSize();
            rk.drill = std::min( drill.x, drill.y );
            rk.hole = (rk.drill != 0);
            rk.smd = pad->GetAttribute() == PAD_ATTRIB::SMD
                        || pad->GetAttribute() == PAD_ATTRIB::CONN;
            rk.mechanical = ( pad->GetAttribute() == PAD_ATTRIB::NPTH );
            rk.x_location = pad->GetPosition().x - origin.x;
            rk.y_location = origin.y - pad->GetPosition().y;

            PCB_LAYER_ID accessLayer = footprint->IsFlipped() ? B_Cu : F_Cu;
            rk.x_size = pad->GetSize( accessLayer ).x;

            // Rule: round pads have y = 0
            if( pad->GetShape( accessLayer ) == PAD_SHAPE::CIRCLE )
                rk.y_size = 0;
            else
                rk.y_size = pad->GetSize( accessLayer ).y;

            rk.rotation = - pad->GetOrientation().AsDegrees();

            if( rk.rotation < 0 )
                rk.rotation += 360;

            // the value indicates which sides are *not* accessible
            rk.soldermask = 3;

            if( pad->GetLayerSet()[F_Mask] )
                rk.soldermask &= ~1;

            if( pad->GetLayerSet()[B_Mask] )
                rk.soldermask &= ~2;

            aRecords.push_back( std::move( rk ) );
        }
    }
}

/* Compute the access code for a via. In D-356 layers are numbered from 1 up,
   where '1' is the 'primary side' (usually the component side);
   '0' means 'both sides', and other layers follows in an unspecified order */
static int via_access_code( BOARD *aPcb, int top_layer, int bottom_layer )
{
    // Easy case for through vias: top_layer is component, bottom_layer is
    // solder, access code is 0
    if( (top_layer == F_Cu) && (bottom_layer == B_Cu) )
        return 0;

    // Blind via, reachable from front
    if( top_layer == F_Cu )
        return 1;

    // Blind via, reachable from bottom
    if( bottom_layer == B_Cu )
        return aPcb->GetCopperLayerCount();

    // It's a buried via, accessible from some inner layer
    // (maybe could be used for testing before laminating? no idea)
    return physical_layer( aPcb, static_cast<PCB_LAYER_ID>( top_layer ) );
}

/* Extract the D356 record from the vias */
static void build_via_testpoints( BOARD *aPcb, std::vector <D356_RECORD>& aRecords )
{
    VECTOR2I origin = aPcb->GetDesignSettings().GetAuxOrigin();

    // Enumerate all the track segments and keep the vias
    for( auto track : aPcb->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA *via = static_cast<PCB_VIA*>( track );
            NETINFO_ITEM *net = track->GetNet();

            D356_RECORD rk;
            rk.smd = false;
            rk.hole = true;
            if( net )
                rk.netname = net->GetNetname();
            else
                rk.netname = wxEmptyString;
            rk.refdes = wxT("VIA");
            rk.pin = wxT("");
            rk.midpoint = true; // Vias are always midpoints
            rk.drill = via->GetDrillValue();
            rk.mechanical = false;

            PCB_LAYER_ID top_layer, bottom_layer;

            via->LayerPair( &top_layer, &bottom_layer );

            rk.access = via_access_code( aPcb, top_layer, bottom_layer );

            if( rk.access != 0 )
            {
                rk.start_layer = physical_layer( aPcb, top_layer );
                rk.end_layer = physical_layer( aPcb, bottom_layer );
            }

            rk.x_location = via->GetPosition().x - origin.x;
            rk.y_location = origin.y - via->GetPosition().y;

            // The record has a single size for vias. A blind via's 027 record describes its one
            // surface pad; otherwise take the smaller of the front and back
            if( rk.access == 1 )
                rk.x_size = via->GetWidth( F_Cu );
            else if( rk.access == aPcb->GetCopperLayerCount() )
                rk.x_size = via->GetWidth( B_Cu );
            else if( via->Padstack().Mode() != PADSTACK::MODE::NORMAL )
                rk.x_size = std::min( via->GetWidth( F_Cu ), via->GetWidth( B_Cu ) );
            else
                rk.x_size = via->GetWidth( PADSTACK::ALL_LAYERS );

            rk.y_size = 0; // Round so height = 0
            rk.rotation = 0;

            // the value indicates which sides are *not* accessible
            rk.soldermask = 0;

            if( via->IsTented( F_Mask ) )
                rk.soldermask |= 1;

            if( via->IsTented( B_Mask ) )
                rk.soldermask |= 2;

            aRecords.push_back( rk );
        }
    }
}

// Rule: we can only use the standard ASCII, control excluded
// Case folding is ASCII only since wxString::Upper() follows the locale (Turkish i)
static wxString sanitize_d356_name( const wxString& aNetname, bool aUpper )
{
    wxString out;

    for( wxUniChar ch : aNetname )
    {
        if( ch > 126 || !std::isgraph( static_cast<unsigned char>( ch ) ) )
            ch = '?';
        else if( aUpper && ch >= 'a' && ch <= 'z' )
            ch = static_cast<char>( ch.GetValue() - 'a' + 'A' );

        out += ch;
    }

    return out;
}


// NNAME aliases fill columns 9-13, so base 36 keeps them to five characters
static wxString d356_alias( int aIndex )
{
    static const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    wxString          alias( wxT( "M" ) );

    for( int div = 36 * 36 * 36; div > 0; div /= 36 )
        alias += digits[( aIndex / div ) % 36];

    return alias;
}


/* Write all the accumuled data to the file in D356 format */
void IPC356D_WRITER::write_D356_records( std::vector <D356_RECORD> &aRecords, FILE* aFile )
{
    const wxString nc( wxT( "N/C" ) );

    // Source net name to the signal name written in columns 4-17
    std::map<wxString, wxString> d356_net_map;
    std::set<wxString>           d356_net_set = { nc };
    std::map<wxString, int>      canonCount;

    // Rule: only uppercase (unofficial, but known to give problems otherwise)
    for( const D356_RECORD& rk : aRecords )
    {
        if( !rk.netname.empty() && d356_net_map.emplace( rk.netname, wxEmptyString ).second )
            canonCount[sanitize_d356_name( rk.netname, true )]++;
    }

    // Every candidate is reserved so no alias can shadow a real net
    for( const auto& [candidate, count] : canonCount )
        d356_net_set.insert( candidate );

    // Every member of a collision is aliased, otherwise its NNAME would name the net that kept it
    for( auto& [netname, canon] : d356_net_map )
    {
        wxString candidate = sanitize_d356_name( netname, true );

        if( candidate.size() <= 14 && candidate != nc && canonCount[candidate] == 1 )
            canon = candidate;
    }

    int aliasIndex = 0;

    for( auto& [netname, canon] : d356_net_map )
    {
        if( !canon.empty() )
            continue;

        do
        {
            canon = d356_alias( aliasIndex++ );
        } while( !d356_net_set.insert( canon ).second );

        // Columns 15-72 hold the source name, keeping the tail since hierarchical prefixes repeat
        fprintf( aFile, "P  NNAME%-5.5s %-66.66s\n", TO_UTF8( canon ),
                 TO_UTF8( sanitize_d356_name( netname, false ).Right( 58 ) ) );
    }

    auto signalName =
            [&]( const D356_RECORD& aRecord ) -> const wxString&
            {
                return aRecord.netname.empty() ? nc : d356_net_map.at( aRecord.netname );
            };

    // 356A wants the netlist sorted by net; stable keeps each net's vias ahead of its pads
    std::stable_sort( aRecords.begin(), aRecords.end(),
                      [&]( const D356_RECORD& aLhs, const D356_RECORD& aRhs )
                      {
                          return signalName( aLhs ) < signalName( aRhs );
                      } );

    for( D356_RECORD& rk : aRecords )
    {
        const wxString& d356_net = signalName( rk );
        const bool blindOrBuried = rk.start_layer != 0;

        // Choose the best record type
        int rktype;

        if( rk.smd )
            rktype = 327;
        else if( rk.mechanical )
            rktype = 367;
        else if( blindOrBuried )
            rktype = 307;
        else
            rktype = 317;

        auto writeComponent =
                [&]( int aOpCode, bool aMidpoint )
                {
                    fprintf( aFile, "%03d%-14.14s   %-6.6s%c%-4.4s%c", aOpCode, TO_UTF8( d356_net ),
                             TO_UTF8( rk.refdes ), rk.pin.empty() ? ' ' : '-', TO_UTF8( rk.pin ),
                             aMidpoint ? 'M' : ' ' );
                };

        auto writeAccessAndSize =
                [&]()
                {
                    fprintf( aFile, "A%02dX%+07dY%+07dX%04dY%04dR%03d", rk.access,
                             iu_to_d356( rk.x_location, 999999 ), iu_to_d356( rk.y_location, 999999 ),
                             iu_to_d356( rk.x_size, 9999 ), iu_to_d356( rk.y_size, 9999 ), rk.rotation );
                };

        writeComponent( rktype, rk.midpoint );

        // Hole definition
        if( rk.hole )
        {
            fprintf( aFile, "D%04d%c",
                     iu_to_d356( rk.drill, 9999 ),
                     rk.mechanical ? 'U':'P' );
        }
        else
        {
            fprintf( aFile, "      " );
        }

        // A 307 record omits the surface pad size, which goes in its 027 continuation instead
        if( blindOrBuried )
        {
            fprintf( aFile, "A%02dX%+07dY%+07d%14s", rk.access, iu_to_d356( rk.x_location, 999999 ),
                     iu_to_d356( rk.y_location, 999999 ), "" );
        }
        else
        {
            writeAccessAndSize();
        }

        // Soldermask lives in columns 73-74 with column 72 left blank
        fprintf( aFile, " S%d", rk.soldermask );

        // Every test record is a full 80 columns
        if( blindOrBuried )
            fprintf( aFile, "L%02dL%02d\n", rk.start_layer, rk.end_layer );
        else
            fprintf( aFile, "%6s\n", "" );

        // Blind vias reach a surface, so the 027 continuation describes that pad
        if( blindOrBuried
            && ( rk.start_layer == 1 || rk.end_layer == m_pcb->GetCopperLayerCount() ) )
        {
            // The midpoint flag belongs to the parent record only
            writeComponent( 27, false );
            fprintf( aFile, "      " );
            writeAccessAndSize();
            fprintf( aFile, " S%d%6s\n", rk.soldermask, "" );
        }
    }
}


bool IPC356D_WRITER::Write( const wxString& aFilename )
{
    FILE*     file = nullptr;
    LOCALE_IO toggle; // Switch the locale to standard C

    if( ( file = wxFopen( aFilename, wxT( "wt" ) ) ) == nullptr )
    {
        return false;
    }

    // This will contain everything needed for the 356 file
    std::vector<D356_RECORD> d356_records;

    build_via_testpoints( m_pcb, d356_records );

    build_pad_testpoints( m_pcb, d356_records );

    // Code 00 AFAIK is ASCII, CUST 0 is decimils/degrees
    // CUST 1 would be metric but gerbtool simply ignores it!
    // Every record is 80 columns and parameter values must end by column 72
    auto writeRecord =
            [&]( const std::string& aRecord )
            {
                fprintf( file, "%-80.80s\n", aRecord.substr( 0, 72 ).c_str() );
            };

    // Parameter values start in column 10 and JOB must lead the header
    writeRecord( "P  JOB   " + std::string( TO_UTF8( wxFileName( m_pcb->GetFileName() ).GetName() ) ) );
    writeRecord( "P  CODE  00" );
    writeRecord( "P  UNITS CUST 0" );
    writeRecord( "P  arrayDim   N" );
    writeRecord( "P  VER   IPC-D-356A" );
    writeRecord( "P  IMAGE PRIMARY" );
    write_D356_records( d356_records, file );
    writeRecord( "999" );

    fclose( file );

    return true;
}




// Subclass for wxFileDialogCustomizeHook to add the checkbox
class D365_CUSTOMIZE_HOOK : public wxFileDialogCustomizeHook
{
public:

public:
    D365_CUSTOMIZE_HOOK( bool aDoNotExportUnconnectedPads = false ) :
            m_doNotExportUnconnectedPads( aDoNotExportUnconnectedPads ),
            m_cb(nullptr)
    {};

    virtual void AddCustomControls( wxFileDialogCustomize& customizer ) override
    {
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

        m_cb = customizer.AddCheckBox( _( "Do not export unconnected pads" ) );
        m_cb->SetValue( m_doNotExportUnconnectedPads );
    }

    virtual void TransferDataFromCustomControls() override
    {
        m_doNotExportUnconnectedPads = m_cb->GetValue();
    }

    bool GetDoNotExportUnconnectedPads() const { return m_doNotExportUnconnectedPads; }
private:

    bool m_doNotExportUnconnectedPads;
    wxFileDialogCheckBox* m_cb;

    wxDECLARE_NO_COPY_CLASS( D365_CUSTOMIZE_HOOK );
};


int BOARD_EDITOR_CONTROL::GenD356File( const TOOL_EVENT& aEvent )
{
    wxFileName  fn = m_frame->GetBoard()->GetFileName();
    wxString    ext, wildcard;

    ext = FILEEXT::IpcD356FileExtension;
    wildcard = FILEEXT::IpcD356FileWildcard();
    fn.SetExt( ext );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( m_frame, _( "Generate IPC-D-356 netlist file" ), pro_dir, fn.GetFullName(),
                      wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    D365_CUSTOMIZE_HOOK customizeHook( m_frame->GetPcbNewSettings()->m_ExportD356.doNotExportUnconnectedPads );
    dlg.SetCustomizeHook( customizeHook );

    KIPLATFORM::UI::AllowNetworkFileSystems( &dlg );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    IPC356D_WRITER writer( m_frame->GetBoard() );
    bool doNotExportUnconnectedPads = customizeHook.GetDoNotExportUnconnectedPads();
    writer.SetDoNotExportUnconnectedPads( doNotExportUnconnectedPads );
    m_frame->GetPcbNewSettings()->m_ExportD356.doNotExportUnconnectedPads = doNotExportUnconnectedPads;

    if( writer.Write( dlg.GetPath() ) )
    {
        DisplayInfoMessage( m_frame, wxString::Format( _( "IPC-D-356 netlist file created:\n'%s'." ),
                                        dlg.GetPath() ) );
    }
    else
    {
        DisplayError( m_frame, wxString::Format( _( "Failed to create file '%s'." ),
                                                 dlg.GetPath() ) );
    }

    return 0;
}
