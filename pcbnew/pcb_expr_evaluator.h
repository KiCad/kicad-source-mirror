#ifndef __PCB_EXPR_EVALUATOR_H
#define __PCB_EXPR_EVALUATOR_H

#include <unordered_map>

#include <property.h>
#include <property_mgr.h>

#include <libeval_compiler/libeval_compiler.h>


class BOARD_ITEM;

class PCB_EXPR_VAR_REF;

class PCB_EXPR_UCODE : public LIBEVAL::UCODE
{
public:
    virtual LIBEVAL::VAR_REF* createVarRef( LIBEVAL::COMPILER *aCompiler,
            const std::string& var, const std::string& field ) override;

    void SetItems( BOARD_ITEM* a, BOARD_ITEM* b = nullptr )
    {
        m_items[0] = a;
        m_items[1] = b;
    }

    BOARD_ITEM* GetItem( int index ) const
    {
        return m_items[index];
    }

private:
    BOARD_ITEM* m_items[2];
};


class PCB_EXPR_VAR_REF : public LIBEVAL::VAR_REF
{
public:
    PCB_EXPR_VAR_REF( int aItemIndex ) : m_itemIndex( aItemIndex )
    {
        //printf("*** createVarRef %p %d\n", this, aItemIndex );
    }

    void SetType( LIBEVAL::VAR_TYPE_T type )
    {
        m_type = type;
    }

    void AddAllowedClass( TYPE_ID type_hash, PROPERTY_BASE* prop )
    {
        m_matchingTypes[type_hash] = prop;
    }

    virtual LIBEVAL::VAR_TYPE_T GetType( LIBEVAL::UCODE* aUcode ) override
    {
        return m_type;
    }

    virtual LIBEVAL::VALUE GetValue( LIBEVAL::UCODE* aUcode ) override;

private:
    std::unordered_map<TYPE_ID, PROPERTY_BASE*> m_matchingTypes;
    int                                         m_itemIndex;
    LIBEVAL::VAR_TYPE_T                         m_type;
};


class PCB_EXPR_COMPILER : public LIBEVAL::COMPILER
{
public:
    PCB_EXPR_COMPILER();
};


class PCB_EXPR_EVALUATOR
{
public:
    PCB_EXPR_EVALUATOR();
    ~PCB_EXPR_EVALUATOR();

    bool Evaluate( const wxString& aExpr );
    int  Result() const
    {
        return m_result;
    }
    wxString GetErrorString();

private:
    bool m_error;
    int  m_result;

    PCB_EXPR_COMPILER m_compiler;
    PCB_EXPR_UCODE    m_ucode;
};

#endif
