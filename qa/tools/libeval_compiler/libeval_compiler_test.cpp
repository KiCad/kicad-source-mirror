#include <wx/wx.h>
#include <cstdio>

#include "board.h"
#include "pcb_track.h"

#include <pcbexpr_evaluator.h>

#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>

#include <unordered_set>

#include <core/profile.h>

bool testEvalExpr( const std::string expr, LIBEVAL::VALUE expectedResult, bool expectError = false,
                   BOARD_ITEM* itemA = nullptr, BOARD_ITEM* itemB = nullptr )
{
    PCBEXPR_COMPILER compiler( new PCB_UNIT_RESOLVER() );
    PCBEXPR_UCODE ucode;
    bool ok = true;

    PCBEXPR_CONTEXT  context, preflightContext;

    context.SetItems( itemA, itemB );

    bool error = !compiler.Compile( expr, &ucode, &preflightContext );


    if( error )
    {
        if ( expectError )
        {
            ok = true;
            return ok;
        }
        else
        {
            ok = false;
        }
    }

    LIBEVAL::VALUE result;

    if( ok )
    {
        result = *ucode.Run( &context );
        ok = (result.EqualTo( &expectedResult) );
    }

    return ok;
}


int main( int argc, char *argv[] )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();


    using VAL = LIBEVAL::VALUE;



/*    testEvalExpr( "10mm + 20 mm", VAL(30e6) );
    testEvalExpr( "3*(7+8)", VAL(3*(7+8)) );
    testEvalExpr( "3*7+8", VAL(3*7+8) );
    testEvalExpr( "(3*7)+8", VAL(3*7+8) );
    testEvalExpr( "10mm + 20)", VAL(0), true );
  */

    BOARD brd;

    NETINFO_LIST& netInfo = brd.GetNetInfo();

    NETCLASSPTR netclass1( new NETCLASS("HV") );
    NETCLASSPTR netclass2( new NETCLASS("otherClass" ) );

    auto net1info = new NETINFO_ITEM( &brd, "net1", 1);
    auto net2info = new NETINFO_ITEM( &brd, "net2", 2);

    net1info->SetClass( netclass1 );
    net2info->SetClass( netclass2 );

    PCB_TRACK trackA( &brd );
    PCB_TRACK trackB( &brd );

    trackA.SetNet( net1info );
    trackB.SetNet( net2info );

    trackB.SetLayer( F_Cu );

    trackA.SetWidth( Mils2iu( 10 ) );
    trackB.SetWidth( Mils2iu( 20 ) );

    testEvalExpr( "A.fromTo('U1', 'U3') && A.NetClass == 'DDR3_A' ", VAL(0), false, &trackA, &trackB );

    return 0;

//    testEvalExpr( "A.onlayer('F.Cu') || A.onlayer('B.Cu')", VAL( 1.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.type == 'Pad' && B.type == 'Pad' && (A.existsOnLayer('F.Cu'))", VAL( 0.0 ), false, &trackA, &trackB );
        return 0;
    testEvalExpr( "A.Width > B.Width", VAL( 0.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.Width + B.Width", VAL( Mils2iu(10) + Mils2iu(20) ), false, &trackA, &trackB );

    testEvalExpr( "A.Netclass", VAL( (const char*) trackA.GetNetClassName().c_str() ), false, &trackA, &trackB );
    testEvalExpr( "(A.Netclass == 'HV') && (B.netclass == 'otherClass') && (B.netclass != 'F.Cu')", VAL( 1.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.Netclass + 1.0", VAL( 1.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.type == 'Track' && B.type == 'Track' && A.layer == 'F.Cu'", VAL( 0.0 ), false, &trackA, &trackB );
    testEvalExpr( "(A.type == 'Track') && (B.type == 'Track') && (A.layer == 'F.Cu')", VAL( 0.0 ), false, &trackA, &trackB );

    return 0;
}
