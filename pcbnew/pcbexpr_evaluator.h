/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef PCBEXPR_EVALUATOR_H
#define PCBEXPR_EVALUATOR_H

#include <set>
#include <map>
#include <unordered_map>
#include <core/typeinfo.h>

#include <layer_ids.h>

#include <properties/property.h>
#include <properties/property_mgr.h>

#include <libeval_compiler/libeval_compiler.h>

class BOARD;
class BOARD_ITEM;

class PCBEXPR_VAR_REF;


// A navigation step applied to a variable reference before its property or method is resolved.
// "A.Parent.getField('x')" resolves item A, then steps to its parent before calling getField().
enum class PCBEXPR_NAV_STEP
{
    PARENT
};

enum class PCBEXPR_PROPERTY_KIND
{
    UNSUPPORTED,
    INT,
    OPTIONAL_INT,
    UNSIGNED,
    LONG_LONG,
    DOUBLE,
    OPTIONAL_DOUBLE,
    BOOL,
    STRING,
    ENUM,
    ANGLE,
    COLOR
};

class PCBEXPR_UCODE final : public LIBEVAL::UCODE
{
public:
    PCBEXPR_UCODE() {};
    virtual ~PCBEXPR_UCODE() {};

    virtual std::unique_ptr<LIBEVAL::VAR_REF> CreateVarRef( const wxString& aVar,
                                                            const wxString& aField ) override;
    virtual LIBEVAL::FUNC_CALL_REF CreateFuncCall( const wxString& aName ) override;

    bool HasGeometryDependentFunctions() const { return m_hasGeometryDependentFunctions; }
    bool RequiresPairItems() const { return m_requiresPairItems; }

private:
    bool m_hasGeometryDependentFunctions = false;
    bool m_requiresPairItems = false;
};


class PCBEXPR_CONTEXT : public LIBEVAL::CONTEXT
{
public:
    PCBEXPR_CONTEXT( int aConstraint = 0, PCB_LAYER_ID aLayer = F_Cu ) :
            m_constraint( aConstraint ),
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

    void SetConstraint( int aConstraint ) { m_constraint = aConstraint; }
    void SetLayer( PCB_LAYER_ID aLayer ) { m_layer = aLayer; }

    /// Rewind for reuse on the next evaluation (see LIBEVAL::CONTEXT::Reset()).
    void Reset() override
    {
        LIBEVAL::CONTEXT::Reset();
        m_constraint = 0;
        m_layer = F_Cu;
        m_items[0] = nullptr;
        m_items[1] = nullptr;
        m_typeOverrides.clear();
    }

    void SetTypeOverride( const BOARD_ITEM* aItem, KICAD_T aType ) { m_typeOverrides[aItem] = aType; }

    KICAD_T GetEffectiveType( const BOARD_ITEM* aItem ) const;

    BOARD* GetBoard() const;

    int GetConstraint() const              { return m_constraint; }
    BOARD_ITEM* GetItem( int index ) const { return m_items[index]; }
    PCB_LAYER_ID GetLayer() const          { return m_layer; }

private:
    int                                  m_constraint;
    BOARD_ITEM*                          m_items[2];
    PCB_LAYER_ID                         m_layer;
    std::map<const BOARD_ITEM*, KICAD_T> m_typeOverrides;
};


class PCBEXPR_VAR_REF : public LIBEVAL::VAR_REF
{
public:
    PCBEXPR_VAR_REF( int aItemIndex ) :
            m_itemIndex( aItemIndex ),
            m_type( LIBEVAL::VT_UNDEFINED )
    {
    }

    ~PCBEXPR_VAR_REF() {};

    static PCBEXPR_PROPERTY_KIND ClassifyProperty( const PROPERTY_BASE* aProperty );
    static LIBEVAL::VAR_TYPE_T   ExpressionType( PCBEXPR_PROPERTY_KIND aKind );

    void SetType( LIBEVAL::VAR_TYPE_T type ) { m_type = type; }
    LIBEVAL::VAR_TYPE_T GetType() const override { return m_type; }

    void AddAllowedClass( TYPE_ID aTypeHash, PROPERTY_BASE* aProperty, PCBEXPR_PROPERTY_KIND aKind )
    {
        m_matchingTypes[aTypeHash] = { aProperty, aKind };
    }

    // Navigation steps walked from the base item before the property/method is resolved.
    // An empty chain (the default) resolves the base item directly, as before.
    void SetNavigation( std::vector<PCBEXPR_NAV_STEP> aNavigation )
    {
        m_navigation = std::move( aNavigation );
    }

    LIBEVAL::VALUE* GetValue( LIBEVAL::CONTEXT* aCtx ) override;

    BOARD_ITEM* GetObject( const LIBEVAL::CONTEXT* aCtx ) const;

private:
    struct MATCHED_PROPERTY
    {
        PROPERTY_BASE*        property;
        PCBEXPR_PROPERTY_KIND kind;
    };

    std::unordered_map<TYPE_ID, MATCHED_PROPERTY> m_matchingTypes;
    int                                           m_itemIndex;
    LIBEVAL::VAR_TYPE_T                           m_type;
    std::vector<PCBEXPR_NAV_STEP>                 m_navigation;
};


// "Object code" version of a netclass reference (for performance).
class PCBEXPR_NETCLASS_REF : public PCBEXPR_VAR_REF
{
public:
    PCBEXPR_NETCLASS_REF( int aItemIndex ) :
            PCBEXPR_VAR_REF( aItemIndex )
    {
        SetType( LIBEVAL::VT_STRING );
    }

    LIBEVAL::VALUE* GetValue( LIBEVAL::CONTEXT* aCtx ) override;
};


// "Object code" version of a component class reference (for performance).
class PCBEXPR_COMPONENT_CLASS_REF : public PCBEXPR_VAR_REF
{
public:
    PCBEXPR_COMPONENT_CLASS_REF( int aItemIndex ) : PCBEXPR_VAR_REF( aItemIndex )
    {
        SetType( LIBEVAL::VT_STRING );
    }

    LIBEVAL::VALUE* GetValue( LIBEVAL::CONTEXT* aCtx ) override;
};


// "Object code" version of a netname reference (for performance).
class PCBEXPR_NETNAME_REF : public PCBEXPR_VAR_REF
{
public:
    PCBEXPR_NETNAME_REF( int aItemIndex ) :
            PCBEXPR_VAR_REF( aItemIndex )
    {
        SetType( LIBEVAL::VT_STRING );
    }

    LIBEVAL::VALUE* GetValue( LIBEVAL::CONTEXT* aCtx ) override;
};


class PCBEXPR_TYPE_REF : public PCBEXPR_VAR_REF
{
public:
    PCBEXPR_TYPE_REF( int aItemIndex ) :
            PCBEXPR_VAR_REF( aItemIndex )
    {
        SetType( LIBEVAL::VT_STRING );
    }

    LIBEVAL::VALUE* GetValue( LIBEVAL::CONTEXT* aCtx ) override;
};


class PCBEXPR_BUILTIN_FUNCTIONS
{
public:
    PCBEXPR_BUILTIN_FUNCTIONS();

    static PCBEXPR_BUILTIN_FUNCTIONS& Instance()
    {
        static PCBEXPR_BUILTIN_FUNCTIONS self;
        return self;
    }

    LIBEVAL::FUNC_CALL_REF Get( const wxString& name )
    {
        return m_funcs[ name ];
    }

    bool IsGeometryDependent( const wxString& name ) const
    {
        return m_geometryDependentFuncs.count( name ) > 0;
    }

    const wxArrayString GetSignatures() const
    {
        return m_funcSigs;
    }

    void RegisterFunc( const wxString& funcSignature, LIBEVAL::FUNC_CALL_REF funcPtr,
                       bool aIsGeometryDependent = false )
    {
        wxString funcName = funcSignature.BeforeFirst( '(' );
        wxString lower = funcName.Lower();
        m_funcs[std::string( lower )] = std::move( funcPtr );
        m_funcSigs.Add( funcSignature );

        if( aIsGeometryDependent )
            m_geometryDependentFuncs.insert( lower );
    }

    void RegisterAllFunctions();

private:
    std::map<wxString, LIBEVAL::FUNC_CALL_REF> m_funcs;
    std::set<wxString>                         m_geometryDependentFuncs;

    wxArrayString m_funcSigs;
};


class PCBEXPR_UNIT_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
public:
    const std::vector<wxString>& GetSupportedUnits() const override;
    const std::vector<EDA_UNITS>& GetSupportedUnitsTypes() const override;

    wxString GetSupportedUnitsMessage() const override;

    double Convert( const wxString& aString, int unitId ) const override;
};


class PCBEXPR_UNITLESS_RESOLVER : public LIBEVAL::UNIT_RESOLVER
{
public:
    const std::vector<wxString>& GetSupportedUnits() const override;
    const std::vector<EDA_UNITS>& GetSupportedUnitsTypes() const override;

    double Convert( const wxString& aString, int unitId ) const override;
};


class PCBEXPR_COMPILER : public LIBEVAL::COMPILER
{
public:
    PCBEXPR_COMPILER( LIBEVAL::UNIT_RESOLVER* aUnitResolver );
};


class PCBEXPR_EVALUATOR
{
public:
    PCBEXPR_EVALUATOR( LIBEVAL::UNIT_RESOLVER* aUnitResolver );
    ~PCBEXPR_EVALUATOR();

    bool Evaluate( const wxString& aExpr );
    int  Result() const { return m_result; }
    EDA_UNITS Units() const { return m_units; }

    void SetErrorCallback( std::function<void( const wxString& aMessage, int aOffset )> aCallback )
    {
        m_compiler.SetErrorCallback( std::move( aCallback ) );
    }

    bool IsErrorPending() const { return m_errorStatus.pendingError; }
    const LIBEVAL::ERROR_STATUS& GetError() const { return m_errorStatus; }

private:
    int  m_result;
    EDA_UNITS m_units;

    PCBEXPR_COMPILER      m_compiler;
    PCBEXPR_UCODE         m_ucode;
    LIBEVAL::ERROR_STATUS m_errorStatus;
};

#endif
