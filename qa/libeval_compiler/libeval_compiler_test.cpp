#include <wx/wx.h>
#include <cstdio>

#include "class_board.h"
#include "class_track.h"

#include <pcb_expr_evaluator.h>

#include <io_mgr.h>
#include <kicad_plugin.h>

#include <unordered_set>

#include <profile.h>

bool testEvalExpr( const std::string expr, LIBEVAL::VALUE expectedResult, bool expectError = false, BOARD_ITEM* itemA = nullptr, BOARD_ITEM* itemB = nullptr )
{
    PCB_EXPR_COMPILER compiler;
    PCB_EXPR_UCODE ucode;
    bool ok = true;

    ucode.SetItems( itemA, itemB );

    bool error = !compiler.Compile( expr, &ucode );

    if( error )
    {
        if ( expectError )
        {
            printf("result: OK (expected parse error)\n");
            ok = true;
            return ok;
        } else {
            printf("result: FAIL: %s\n", compiler.GetErrorStatus().Format().c_str() );
            ok = false;
        }
    }

    LIBEVAL::VALUE result;

    if( ok )
    {
        result = *ucode.Run();
        ok = (result == expectedResult);
    }

    if( expectedResult.GetType() == LIBEVAL::VT_NUMERIC )
        printf("result: %s (got %.10f expected: %.10f)\n", ok ? "OK" : "FAIL", result.AsDouble(), expectedResult.AsDouble() );
    else
        printf("result: %s (got '%s' expected: '%s')\n", ok ? "OK" : "FAIL", result.AsString().c_str(), expectedResult.AsString().c_str() );

    if (!ok )
    {
        printf("Offending code dump: \n%s\n", ucode.Dump().c_str() );
    }

    return ok;
}

bool EvaluatePCBExpression( const std::string& aExpr, int& aResult )
{
    PCB_EXPR_COMPILER compiler;
    PCB_EXPR_UCODE ucode;
    if( !compiler.Compile( aExpr, &ucode ) )
        return false;
    
    auto result = ucode.Run();
    return true;
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
    
    printf("netcl1 classname '%s'\n", (const char *) netclass1->GetName().c_str() );
    

    auto net1info = new NETINFO_ITEM( &brd, "net1", 1);
    auto net2info = new NETINFO_ITEM( &brd, "net2", 2);
    
   
    net1info->SetClass( netclass1 );
    net2info->SetClass( netclass2 );

    printf("netX classname '%s'\n", net1info->GetClassName().c_str() );


    printf("net1 class %p %p\n", net1info->GetNetClass(), netclass1.get() );

    TRACK trackA(&brd);
    TRACK trackB(&brd);

    printf("net1 classname '%s'\n", (const char*) net1info->GetClassName().c_str() );

    trackA.SetNet( net1info );
    trackB.SetNet( net2info );

    trackB.SetLayer( F_Cu );

    trackA.SetWidth( Mils2iu( 10 ));
    trackB.SetWidth( Mils2iu( 20 ));

    printf("TrkA %p netclass '%s'\n", &trackA, (const char*) trackA.GetNetClassName().c_str() );
    printf("TrkB %p netclass '%s'\n", &trackB, (const char*) trackB.GetNetClassName().c_str() );

//    testEvalExpr( "A.onlayer(\"F.Cu\") || A.onlayer(\"B.Cu\")", VAL(1.0), false, &trackA, &trackB );
    testEvalExpr( "A.type == \"Pad\" && B.type == \"Pad\" && (A.onLayer(\"F.Cu\"))",VAL(0.0), false, &trackA, &trackB );
        return 0;
    testEvalExpr( "A.Width > B.Width", VAL(0.0), false, &trackA, &trackB );
    testEvalExpr( "A.Width + B.Width", VAL(Mils2iu(10) + Mils2iu(20)), false, &trackA, &trackB );

    testEvalExpr( "A.Netclass", VAL( (const char*) trackA.GetNetClassName().c_str() ), false, &trackA, &trackB );
    testEvalExpr( "(A.Netclass == \"HV\") && (B.netclass == \"otherClass\") && (B.netclass != \"F.Cu\")", VAL( 1.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.Netclass + 1.0", VAL( 1.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.type == \"Track\" && B.type == \"Track\" && A.layer == \"F.Cu\"", VAL(0.0), false, &trackA, &trackB );
    testEvalExpr( "(A.type == \"Track\") && (B.type == \"Track\") && (A.layer == \"F.Cu\")", VAL(0.0), false, &trackA, &trackB );

    return 0;
}
