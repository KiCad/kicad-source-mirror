#include <wx/wx.h>
#include <cstdio>

#include "class_board.h"
#include "class_track.h"
#include "libeval_compiler.h"


#include <io_mgr.h>
#include <kicad_plugin.h>

#include <unordered_set>

#include <profile.h>

class PCB_EXPR_VAR_REF;

class PCB_EXPR_UCODE : public LIBEVAL::UCODE
{
public:

    virtual VAR_REF *createVarRef( const std::string &var, const std::string &field ) override;
    
    void SetItems(  BOARD_ITEM *a,  BOARD_ITEM* b )
    {
        m_items[0] = a;
        m_items[1] = b;
    }

     BOARD_ITEM *GetItem( int index ) const 
    {
        return m_items[index];
    }

private:
    BOARD_ITEM *m_items[2];
};


class PCB_EXPR_VAR_REF : public LIBEVAL::UCODE::VAR_REF
{
public:
    PCB_EXPR_VAR_REF ( int aItemIndex )
        : m_itemIndex(aItemIndex)
    {
        //printf("*** createVarRef %p %d\n", this, aItemIndex );
    }

    void SetType( LIBEVAL::VAR_TYPE_T type )
    {
        m_type = type;
    }

    void AddAllowedClass ( TYPE_ID type_hash, PROPERTY_BASE *prop )
    {
        m_matchingTypes[type_hash] = prop;
    }

    virtual LIBEVAL::VAR_TYPE_T GetType( const LIBEVAL::UCODE* aUcode ) const override
    {
        return m_type;
    }

    virtual LIBEVAL::VALUE GetValue( const LIBEVAL::UCODE* aUcode ) const override
    {
        auto ucode = static_cast<const PCB_EXPR_UCODE*> (aUcode);
        auto item = ucode->GetItem( m_itemIndex );

        auto it = m_matchingTypes.find( TYPE_HASH( *item ) );

        if( it == m_matchingTypes.end() )
        {
            printf("Null field!\n");
            return LIBEVAL::VALUE(0.0);
        }
        else
        {
            if( m_type == LIBEVAL::VT_NUMERIC )
                return LIBEVAL::VALUE( (double) item->Get<int>( it->second ) );
            else
            {
                wxString str = item->Get<wxString>( it->second );
                //printf("item %p GetStr '%s'\n", item, (const char*) str.c_str());
                return LIBEVAL::VALUE( (const char*) str.c_str() );
            }
        }
    }

private:

    std::unordered_map<TYPE_ID, PROPERTY_BASE*> m_matchingTypes;
    int m_itemIndex;
    LIBEVAL::VAR_TYPE_T m_type;
};

LIBEVAL::UCODE::VAR_REF *PCB_EXPR_UCODE::createVarRef( const std::string &var, const std::string &field )
{
    PCB_EXPR_VAR_REF *rv;

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

    auto classes = propMgr.GetAllClasses();
    auto vref = new PCB_EXPR_VAR_REF( var == "A" ? 0 : 1 );

    for ( auto cls : classes )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );
            if( prop )
            {
                //printf("Field '%s' class %s ptr %p\n", field.c_str(), (const char *) cls.name.c_str(), prop );
                vref->AddAllowedClass( cls.type, prop );
                if ( prop->TypeHash() == TYPE_HASH(int) )
                    vref->SetType( LIBEVAL::VT_NUMERIC );
                else if ( prop->TypeHash() == TYPE_HASH(wxString) )
                    vref->SetType( LIBEVAL::VT_STRING );
                else {
                    printf("Unknown property type\n");
                }
            }
        }
    }

    return vref;
}


BOARD* loadBoard( const std::string& filename )
{
    PLUGIN::RELEASER pi( new PCB_IO );
    BOARD* brd = nullptr;

    try
    {
        brd = pi->Load( wxString( filename.c_str() ), NULL, NULL );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ),
                ioe.Problem() );

        printf( "%s\n", (const char*) msg.mb_str() );
        return nullptr;
    }

    return brd;
}


class PCB_UNIT_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
    public:
        virtual ~PCB_UNIT_RESOLVER()
        {
        }

        virtual const std::vector<std::string>& GetSupportedUnits() const override
        {
            static const std::vector<std::string> pcbUnits = {"mil", "mm", "in"};

            return pcbUnits;
        }

        virtual double Convert( const std::string aString, int unitId ) const override
        {
            double v = atof(aString.c_str());
            switch(unitId)
            {
                case 0 :
                    return Mils2iu( v );
                case 1:
                    return Millimeter2iu( v );
                case 2:
                    return Mils2iu( v * 1000.0 );
                default:
                    return v;
            }
        };
};


class PCB_EXPR_COMPILER : public LIBEVAL::COMPILER
{
    public:
        PCB_EXPR_COMPILER()
        {
            m_unitResolver.reset( new PCB_UNIT_RESOLVER );
        }
};

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
            printf("result: FAIL (unexpected parse error)\n");
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

class PCB_EXPR_EVALUATOR 
{
public:
    PCB_EXPR_EVALUATOR();
    ~PCB_EXPR_EVALUATOR();

    bool Evaluate( const wxString& aExpr );
    int Result() const { return m_result; }
    wxString GetErrorString();

private:
    bool m_error;
    int m_result;

    PCB_EXPR_COMPILER m_compiler;
    PCB_EXPR_UCODE m_ucode;
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

    trackA.SetWidth( Mils2iu( 10 ));
    trackB.SetWidth( Mils2iu( 20 ));

    printf("TrkA %p netclass '%s'\n", &trackA, (const char*) trackA.GetNetClassName().c_str() );
    printf("TrkB %p netclass '%s'\n", &trackB, (const char*) trackB.GetNetClassName().c_str() );

    testEvalExpr( "A.Width > B.Width", VAL(0.0), false, &trackA, &trackB );
    testEvalExpr( "A.Width + B.Width", VAL(Mils2iu(10) + Mils2iu(20)), false, &trackA, &trackB );

    testEvalExpr( "A.Netclass", VAL( (const char*) trackA.GetNetClassName().c_str() ), false, &trackA, &trackB );
    testEvalExpr( "A.Netclass == \"HV\" && B.netclass == \"otherClass\"", VAL( 1.0 ), false, &trackA, &trackB );
    testEvalExpr( "A.Netclass + 1.0", VAL( 1.0 ), false, &trackA, &trackB );
    
    return 0;
}
