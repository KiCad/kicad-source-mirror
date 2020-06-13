#include <cstdio>

#include "class_board.h"

#include "pcb_expr_evaluator.h"

LIBEVAL::VALUE PCB_EXPR_VAR_REF::GetValue( LIBEVAL::UCODE* aUcode ) 
{
    auto ucode = static_cast<const PCB_EXPR_UCODE*>( aUcode );
    auto item  = ucode->GetItem( m_itemIndex );

    auto it = m_matchingTypes.find( TYPE_HASH( *item ) );

    if( it == m_matchingTypes.end() )
    {
        wxString msg;
        msg.Printf("property not found for item of type: 0x%x!\n",  TYPE_HASH( *item ) );
        aUcode->RuntimeError( (const char *) msg.c_str() );
        return LIBEVAL::VALUE( 0.0 );
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

LIBEVAL::VAR_REF* PCB_EXPR_UCODE::createVarRef( LIBEVAL::COMPILER *aCompiler,
        const std::string& var, const std::string& field )
{
    PCB_EXPR_VAR_REF* rv;

    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

    auto classes = propMgr.GetAllClasses();
    auto vref    = new PCB_EXPR_VAR_REF( var == "A" ? 0 : 1 );

    for( auto cls : classes )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );
            if( prop )
            {
                //printf("Field '%s' class %s ptr %p\n", field.c_str(), (const char *) cls.name.c_str(), prop );
                vref->AddAllowedClass( cls.type, prop );
                if( prop->TypeHash() == TYPE_HASH( int ) )
                    vref->SetType( LIBEVAL::VT_NUMERIC );
                else if( prop->TypeHash() == TYPE_HASH( wxString ) )
                    vref->SetType( LIBEVAL::VT_STRING );
                else
                {
                    printf( "Unknown property type\n" );
                }
            }
        }
    }

    return vref;
}


class PCB_UNIT_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
public:
    virtual ~PCB_UNIT_RESOLVER()
    {
    }

    virtual const std::vector<std::string>& GetSupportedUnits() const override
    {
        static const std::vector<std::string> pcbUnits = { "mil", "mm", "in" };

        return pcbUnits;
    }

    virtual double Convert( const std::string aString, int unitId ) const override
    {
        double v = atof( aString.c_str() );
        switch( unitId )
        {
        case 0:
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


PCB_EXPR_COMPILER::PCB_EXPR_COMPILER()
{
    m_unitResolver.reset( new PCB_UNIT_RESOLVER );
}


PCB_EXPR_EVALUATOR::PCB_EXPR_EVALUATOR()
{
    m_error = false;
}

PCB_EXPR_EVALUATOR::~PCB_EXPR_EVALUATOR()
{
}


bool PCB_EXPR_EVALUATOR::Evaluate( const wxString& aExpr )
{
    PCB_EXPR_UCODE ucode;
    if( !m_compiler.Compile( (const char*) aExpr.c_str(), &ucode ) )
        return false;

    auto result = ucode.Run();

    if( result->GetType() == LIBEVAL::VT_NUMERIC )
        m_result = KiROUND( result->AsDouble() );

    return true;
}

wxString PCB_EXPR_EVALUATOR::GetErrorString()
{
    wxString errMsg;
    return errMsg;
}
