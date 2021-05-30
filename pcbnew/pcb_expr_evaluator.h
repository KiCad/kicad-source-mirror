/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef __PCB_EXPR_EVALUATOR_H
#define __PCB_EXPR_EVALUATOR_H

#include <unordered_map>

#include <property.h>
#include <property_mgr.h>

#include <libeval_compiler/libeval_compiler.h>


class BOARD_ITEM;

class PCB_EXPR_VAR_REF;

class PCB_EXPR_UCODE final : public LIBEVAL::UCODE
{
public:
    PCB_EXPR_UCODE() {};
    virtual ~PCB_EXPR_UCODE() {};

    virtual std::unique_ptr<LIBEVAL::VAR_REF> CreateVarRef( const wxString& aVar, const wxString& aField ) override;
    virtual LIBEVAL::FUNC_CALL_REF CreateFuncCall( const wxString& aName ) override;
};


class PCB_EXPR_CONTEXT : public LIBEVAL::CONTEXT
{
public:
    PCB_EXPR_CONTEXT( PCB_LAYER_ID aLayer = UNDEFINED_LAYER ) :
            m_layer( aLayer )
    {
        m_items[0] = nullptr;
        m_items[1] = nullptr;
    }

    void SetItems( BOARD_ITEM* a, BOARD_ITEM* b = nullptr )
    {
        m_items[0] = a;
        m_items[1] = b;
    }

    BOARD_ITEM* GetItem( int index ) const
    {
        return m_items[index];
    }

    PCB_LAYER_ID GetLayer() const
    {
        return m_layer;
    }

private:
    BOARD_ITEM*  m_items[2];
    PCB_LAYER_ID m_layer;
};


class PCB_EXPR_VAR_REF : public LIBEVAL::VAR_REF
{
public:
    PCB_EXPR_VAR_REF( int aItemIndex ) :
        m_itemIndex( aItemIndex ),
        m_type( LIBEVAL::VT_UNDEFINED ),
        m_isEnum( false )
    {
        //printf("*** CreateVarRef %p %d\n", this, aItemIndex );
    }

    ~PCB_EXPR_VAR_REF() {};

    void SetIsEnum( bool s ) { m_isEnum = s; }
    bool IsEnum() const { return m_isEnum; }

    void SetType( LIBEVAL::VAR_TYPE_T type ) { m_type = type; }
    LIBEVAL::VAR_TYPE_T GetType() const override { return m_type; }

    void AddAllowedClass( TYPE_ID type_hash, PROPERTY_BASE* prop )
    {
        m_matchingTypes[type_hash] = prop;
    }

    virtual LIBEVAL::VALUE GetValue( LIBEVAL::CONTEXT* aCtx ) override;

    BOARD_ITEM* GetObject( const LIBEVAL::CONTEXT* aCtx ) const;

private:
    std::unordered_map<TYPE_ID, PROPERTY_BASE*> m_matchingTypes;
    int                                         m_itemIndex;
    LIBEVAL::VAR_TYPE_T                         m_type;
    bool                                        m_isEnum;
};


// "Object code" version of a netclass reference (for performance).
class PCB_EXPR_NETCLASS_REF : public PCB_EXPR_VAR_REF
{
public:
    PCB_EXPR_NETCLASS_REF( int aItemIndex ) :
        PCB_EXPR_VAR_REF( aItemIndex )
    {
        SetType( LIBEVAL::VT_STRING );
        //printf("*** CreateVarRef %p %d\n", this, aItemIndex );
    }

    LIBEVAL::VALUE GetValue( LIBEVAL::CONTEXT* aCtx ) override;
};


// "Object code" version of a netname reference (for performance).
class PCB_EXPR_NETNAME_REF : public PCB_EXPR_VAR_REF
{
public:
    PCB_EXPR_NETNAME_REF( int aItemIndex ) :
        PCB_EXPR_VAR_REF( aItemIndex )
    {
        SetType( LIBEVAL::VT_STRING );
        //printf("*** CreateVarRef %p %d\n", this, aItemIndex );
    }

    LIBEVAL::VALUE GetValue( LIBEVAL::CONTEXT* aCtx ) override;
};


class PCB_EXPR_BUILTIN_FUNCTIONS
{
public:
    PCB_EXPR_BUILTIN_FUNCTIONS();

    static PCB_EXPR_BUILTIN_FUNCTIONS& Instance()
    {
        static PCB_EXPR_BUILTIN_FUNCTIONS self;
        return self;
    }

    LIBEVAL::FUNC_CALL_REF Get( const wxString &name )
    {
        return m_funcs[ name ];
    }

    const wxArrayString GetSignatures() const
    {
        return m_funcSigs;
    }

    void RegisterFunc( const wxString& funcSignature, LIBEVAL::FUNC_CALL_REF funcPtr )
    {
        wxString funcName = funcSignature.BeforeFirst( '(' );
        m_funcs[std::string( funcName.Lower() )] = std::move( funcPtr );
        m_funcSigs.Add( funcSignature );
    }

    void RegisterAllFunctions();

private:
    std::map<wxString, LIBEVAL::FUNC_CALL_REF> m_funcs;

    wxArrayString m_funcSigs;
};


class PCB_EXPR_COMPILER : public LIBEVAL::COMPILER
{
public:
    PCB_EXPR_COMPILER();
};


class PCB_EXPR_EVALUATOR
{
public:
    PCB_EXPR_EVALUATOR( );
    ~PCB_EXPR_EVALUATOR();

    bool Evaluate( const wxString& aExpr );
    int  Result() const { return m_result; }

    void SetErrorCallback( std::function<void( const wxString& aMessage, int aOffset )> aCallback )
    {
        m_compiler.SetErrorCallback( aCallback );
    }

    bool IsErrorPending() const { return m_errorStatus.pendingError; }
    const LIBEVAL::ERROR_STATUS& GetError() const { return m_errorStatus; }

private:
    int  m_result;

    PCB_EXPR_COMPILER     m_compiler;
    PCB_EXPR_UCODE        m_ucode;
    LIBEVAL::ERROR_STATUS m_errorStatus;
};

#endif
