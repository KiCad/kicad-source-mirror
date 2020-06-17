#include <cstdio>

#include "class_board.h"
#include "pcb_expr_evaluator.h"


class PCB_EXPR_BUILTIN_FUNCTIONS
{
    public:

        using FPTR = LIBEVAL::UCODE::FUNC_PTR;

        PCB_EXPR_BUILTIN_FUNCTIONS();

        static PCB_EXPR_BUILTIN_FUNCTIONS& Instance() 
        {
            static PCB_EXPR_BUILTIN_FUNCTIONS self;
            return self;
        }

        std::string tolower( const std::string str ) const
        {
            std::string rv;
              std::transform(str.begin(),
                 str.end(),
                 rv.begin(),
                 ::tolower);
                return rv;
        }

        FPTR Get( const std::string &name ) const
        {
            auto it = m_funcs.find( name );

            if( it == m_funcs.end() )
                return nullptr;

            //printf("Cfc2\n");
            //it->second(nullptr, nullptr, nullptr);

            return it->second;
        }

    private:

        std::map<std::string, FPTR> m_funcs;

        static void onLayer( LIBEVAL::UCODE* aUcode, LIBEVAL::UCODE::CONTEXT* aCtx, void *self )
        {
            auto arg = aCtx->Pop();
            auto vref = static_cast<PCB_EXPR_VAR_REF*>( self );
            auto conv = ENUM_MAP<PCB_LAYER_ID>::Instance();
            bool value = vref->GetObject(aUcode)->IsOnLayer( conv.ToEnum( arg->AsString() ) );
            auto rv =  aCtx->AllocValue();
            rv->Set( value ? 1.0 : 0.0 );
            aCtx->Push( rv );
        }
};

PCB_EXPR_BUILTIN_FUNCTIONS::PCB_EXPR_BUILTIN_FUNCTIONS()
{
    m_funcs[ "onlayer" ] = onLayer;
}

BOARD_ITEM* PCB_EXPR_VAR_REF::GetObject( LIBEVAL::UCODE* aUcode ) const
{   
    auto ucode = static_cast<const PCB_EXPR_UCODE*>( aUcode );
    auto item  = ucode->GetItem( m_itemIndex );
    return item;
}

LIBEVAL::VALUE PCB_EXPR_VAR_REF::GetValue( LIBEVAL::UCODE* aUcode ) 
{
    auto item  = GetObject( aUcode );
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
            wxString str;
            if( !m_isEnum )
            {
                //printf("item %p Get string '%s'\n", item, (const char*) it->second->Name().c_str() );
                str = item->Get<wxString>( it->second );
            } else {
                const auto& any = item->Get( it->second );
                any.GetAs<wxString>( &str );
                //printf("item %p get enum: '%s'\n", item , (const char*) str.c_str() );
            }
            return LIBEVAL::VALUE( (const char*) str.c_str() );
        }
    }
}

LIBEVAL::UCODE::FUNC_PTR PCB_EXPR_UCODE::createFuncCall( LIBEVAL::COMPILER* aCompiler, const std::string& name )
{
    auto registry = PCB_EXPR_BUILTIN_FUNCTIONS::Instance();

    //printf("CreateFCall '%s' found %d\n", name.c_str(),  registry.Get(name) != nullptr ? 1 : 0 );

    auto f = registry.Get( name );
    //printf("Cfc3\n");
    //f(nullptr,nullptr,nullptr);
            
    return f;
}

LIBEVAL::VAR_REF* PCB_EXPR_UCODE::createVarRef( LIBEVAL::COMPILER *aCompiler,
        const std::string& var, const std::string& field )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();

    auto classes = propMgr.GetAllClasses();
    auto vref    = new PCB_EXPR_VAR_REF( var == "A" ? 0 : 1 );

    if( field.empty() ) // return reference to base object
        return vref;

    for( auto cls : classes )
    {
        if( propMgr.IsOfType( cls.type, TYPE_HASH( BOARD_ITEM ) ) )
        {
            PROPERTY_BASE* prop = propMgr.GetProperty( cls.type, field );
        
            if( prop )
            {
                //printf("Field '%s' class %s ptr %p haschoices %d typeid %s\n", field.c_str(), (const char *) cls.name.c_str(), prop, !!prop->HasChoices(), typeid(*prop).name() );
                vref->AddAllowedClass( cls.type, prop );
                if( prop->TypeHash() == TYPE_HASH( int ) )
                    vref->SetType( LIBEVAL::VT_NUMERIC );
                else if( prop->TypeHash() == TYPE_HASH( wxString ) )
                    vref->SetType( LIBEVAL::VT_STRING );
                else if ( prop->HasChoices() )
                {   // it's an enum, we treat it as string
                    vref->SetType( LIBEVAL::VT_STRING );
                    vref->SetIsEnum ( true );
                }
                else
                {
                    (void) 0; // should we do anything here?
                    //printf("unmatched type for prop '%s'\n", field.c_str() );
                    //wxString msg;
                    //msg.Printf("Unrecognized type for property '%s'", field.c_str() );
                    //aCompiler->ReportError( (const char*) msg.c_str() );
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
