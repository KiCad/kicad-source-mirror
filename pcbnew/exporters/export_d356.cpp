/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011-2013 Lorenzo Marcantonio <l.marcantonio@logossrl.com>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <kiface_i.h>
#include <wxPcbStruct.h>
#include <trigo.h>
#include <build_version.h>
#include <macros.h>

#include <pcbnew.h>

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_edge_mod.h>
#include <vector>
#include <cctype>

/* Structure for holding the D-356 record fields.
 * Useful because 356A (when implemented) must be sorted before outputting it */
struct D356_RECORD
{
    bool       smd;
    bool       hole;
    wxString   netname;
    wxString   refdes;
    wxString   pin;
    bool       midpoint;
    int        drill;
    bool       mechanical;
    int        access;      // Access 0 is 'both sides'
    int        soldermask;
    // All these in PCB units, will be output in decimils
    int        x_location;
    int        y_location;
    int        x_size;
    int        y_size;
    int        rotation;
};

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
    for( LAYER_NUM layer = In1_Cu; layer < B_Cu; ++layer )
    {
        if( aLayerMask[layer] )
            return layer + 1;
    }

    // This shouldn't happen
    return -1;
}

/* Convert and clamp a size from IU to decimils */
static int iu_to_d356(int iu, int clamp)
{
    int val = KiROUND( iu / IU_PER_DECIMILS );
    if( val > clamp ) return clamp;
    if( val < -clamp ) return -clamp;
    return val;
}

/* Extract the D356 record from the modules (pads) */
static void build_pad_testpoints( BOARD *aPcb,
    std::vector <D356_RECORD>& aRecords )
{
    wxPoint origin = aPcb->GetAuxOrigin();

    for( MODULE *module = aPcb->m_Modules;
        module; module = module->Next() )
    {
        for( D_PAD *pad = module->Pads();  pad; pad = pad->Next() )
        {
            D356_RECORD rk;
            rk.access = compute_pad_access_code( aPcb, pad->GetLayerSet() );

            // It could be a mask only pad, we only handle pads with copper here
            if( rk.access != -1 )
            {
                rk.netname = pad->GetNetname();
                rk.refdes = module->GetReference();
                pad->StringPadName( rk.pin );
                rk.midpoint = false; // XXX MAYBE need to be computed (how?)
                const wxSize& drill = pad->GetDrillSize();
                rk.drill = std::min( drill.x, drill.y );
                rk.hole = (rk.drill != 0);
                rk.smd = pad->GetAttribute() == PAD_SMD;
                rk.mechanical = (pad->GetAttribute() == PAD_HOLE_NOT_PLATED);
                rk.x_location = pad->GetPosition().x - origin.x;
                rk.y_location = origin.y - pad->GetPosition().y;
                rk.x_size = pad->GetSize().x;

                // Rule: round pads have y = 0
                if( pad->GetShape() == PAD_CIRCLE )
                    rk.y_size = 0;
                else
                    rk.y_size = pad->GetSize().y;

                rk.rotation = -KiROUND( pad->GetOrientation() ) / 10;
                if( rk.rotation < 0 ) rk.rotation += 360;

                // the value indicates which sides are *not* accessible
                rk.soldermask = 3;
                if( pad->GetLayerSet()[F_Mask] )
                    rk.soldermask &= ~1;
                if( pad->GetLayerSet()[B_Mask] )
                    rk.soldermask &= ~2;

                aRecords.push_back( rk );
            }
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
    return bottom_layer + 1; // XXX is this correct?
}

/* Extract the D356 record from the vias */
static void build_via_testpoints( BOARD *aPcb,
    std::vector <D356_RECORD>& aRecords )
{
    wxPoint origin = aPcb->GetAuxOrigin();

    // Enumerate all the track segments and keep the vias
    for( TRACK *track = aPcb->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            VIA *via = (VIA*) track;
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

            LAYER_ID top_layer, bottom_layer;

            via->LayerPair( &top_layer, &bottom_layer );

            rk.access = via_access_code( aPcb, top_layer, bottom_layer );
            rk.x_location = via->GetPosition().x - origin.x;
            rk.y_location = origin.y - via->GetPosition().y;
            rk.x_size = via->GetWidth();
            rk.y_size = 0; // Round so height = 0
            rk.rotation = 0;
            rk.soldermask = 3; // XXX always tented?

            aRecords.push_back( rk );
        }
    }
}

/* Add a new netname to the d356 canonicalized list */
static const wxString intern_new_d356_netname( const wxString &aNetname,
        std::map<wxString, wxString> &aMap, std::set<wxString> &aSet )
{
    wxString canon;
    for (wxString::const_iterator i = aNetname.begin();
            i != aNetname.end(); ++i)
    {
        // Rule: we can only use the standard ASCII, control excluded
        char ch = *i;
        if( ch > 126 || !std::isgraph( ch ) )
            ch = '?';
        canon += ch;
    }

    // Rule: only uppercase (unofficial, but known to give problems
    // otherwise)
    canon.MakeUpper();

    // Rule: maximum length is 14 characters, otherwise we keep the tail
    if( canon.size() > 14 )
    {
        canon = canon.Right( 14 );
    }

    // Check if it's still unique
    if( aSet.count( canon ) )
    {
        // Nope, need to uniquify it, trim it more and add a number
        wxString base( canon );
        if( base.size() > 10 )
        {
            base = base.Right( 10 );
        }

        int ctr = 0;
        do
        {
            ++ctr;
            canon = base;
            canon << '#' << ctr;
        } while ( aSet.count( canon ) );
    }

    // Register it
    aMap[aNetname] = canon;
    aSet.insert( canon );
    return canon;
}

/* Write all the accumuled data to the file in D356 format */
static void write_D356_records( std::vector <D356_RECORD> &aRecords,
                                FILE *fout )
{
    // Sanified and shorted network names and set of short names
    std::map<wxString, wxString> d356_net_map;
    std::set<wxString> d356_net_set;

    for (unsigned i = 0; i < aRecords.size(); i++)
    {
        D356_RECORD &rk = aRecords[i];

        // Try to sanify the network name (there are limits on this), if
        // not already done. Also 'empty' net are marked as N/C, as
        // specified.
        wxString d356_net( wxT("N/C") );
        if( !rk.netname.empty() )
        {
            d356_net = d356_net_map[rk.netname];

            if( d356_net.empty() )
                d356_net = intern_new_d356_netname( rk.netname, d356_net_map,
                                                    d356_net_set );
        }

        // Choose the best record type
        int rktype;
        if( rk.smd )
            rktype = 327;
        else
        {
            if( rk.mechanical )
                rktype = 367;
            else
                rktype = 317;
        }

        // Operation code, signal and component
        fprintf( fout, "%03d%-14.14s   %-6.6s%c%-4.4s%c",
                 rktype, TO_UTF8(d356_net),
                 TO_UTF8(rk.refdes),
                 rk.pin.empty()?' ':'-',
                 TO_UTF8(rk.pin),
                 rk.midpoint?'M':' ' );

        // Hole definition
        if( rk.hole )
        {
            fprintf( fout, "D%04d%c",
                     iu_to_d356( rk.drill, 9999 ),
                     rk.mechanical ? 'U':'P' );
        }
        else
            fprintf( fout, "      " );

        // Test point access
        fprintf( fout, "A%02dX%+07dY%+07dX%04dY%04dR%03d",
                rk.access,
                iu_to_d356( rk.x_location, 999999 ),
                iu_to_d356( rk.y_location, 999999 ),
                iu_to_d356( rk.x_size, 9999 ),
                iu_to_d356( rk.y_size, 9999 ),
                rk.rotation );

        // Soldermask
        fprintf( fout, "S%d\n", rk.soldermask );
    }
}

/* Driver function: processing starts here */
void PCB_EDIT_FRAME::GenD356File( wxCommandEvent& aEvent )
{
    wxFileName fn = GetBoard()->GetFileName();
    wxString   msg, ext, wildcard;
    FILE       *file;

    ext = wxT( "d356" );
    wildcard = _( "IPC-D-356 Test Files (.d356)|*.d356" );
    fn.SetExt( ext );

    wxFileDialog dlg( this, _( "Export D-356 Test File" ), wxGetCwd(),
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( ( file = wxFopen( dlg.GetPath(), wxT( "wt" ) ) ) == NULL )
    {
        msg = _( "Unable to create " ) + dlg.GetPath();
        DisplayError( this, msg ); return;
    }

    LOCALE_IO       toggle;     // Switch the locale to standard C

    // This will contain everything needed for the 356 file
    std::vector <D356_RECORD> d356_records;
    BOARD* pcb = GetBoard();

    build_via_testpoints( pcb, d356_records );

    build_pad_testpoints( pcb, d356_records );

    // Code 00 AFAIK is ASCII, CUST 0 is decimils/degrees
    // CUST 1 would be metric but gerbtool simply ignores it!
    fprintf( file, "P  CODE 00\n" );
    fprintf( file, "P  UNITS CUST 0\n" );
    fprintf( file, "P  DIM   N\n" );
    write_D356_records( d356_records, file );
    fprintf( file, "999\n" );

    fclose( file );
}

