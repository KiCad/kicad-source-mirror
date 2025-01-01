/*
 * Copyright (C) 2016 Mark Roszko <mark.roszko@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SEXPR_H_
#define SEXPR_H_

#include <cstdint>
#include <string>
#include <vector>
#include "sexpr/isexprable.h"
#include "sexpr/sexpr_exception.h"


namespace SEXPR
{
    using std::int32_t;
    using std::int64_t;

    enum class SEXPR_TYPE : char
    {
        SEXPR_TYPE_LIST,
        SEXPR_TYPE_ATOM_INTEGER,
        SEXPR_TYPE_ATOM_DOUBLE,
        SEXPR_TYPE_ATOM_STRING,
        SEXPR_TYPE_ATOM_SYMBOL,
    };

    typedef std::vector< class SEXPR * > SEXPR_VECTOR;

    class SEXPR
    {
    public:
        virtual ~SEXPR() {};
        bool IsList() const { return m_type == SEXPR_TYPE::SEXPR_TYPE_LIST; }
        bool IsSymbol() const { return m_type == SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL; }
        bool IsString() const { return m_type == SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING; }
        bool IsDouble() const { return m_type == SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE; }
        bool IsInteger() const { return m_type == SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER; }
        void AddChild( SEXPR* aChild );
        SEXPR_VECTOR const * GetChildren() const;
        SEXPR * GetChild( size_t aIndex ) const;
        size_t GetNumberOfChildren() const;
        int64_t GetLongInteger() const;
        int32_t GetInteger() const;
        float GetFloat() const;
        double GetDouble() const;
        std::string const & GetString() const;
        std::string const & GetSymbol() const;
        SEXPR_LIST* GetList();
        std::string AsString( size_t aLevel = 0) const;
        size_t GetLineNumber() const { return m_lineNumber; }

    protected:
        SEXPR_TYPE m_type;
        SEXPR( SEXPR_TYPE aType, size_t aLineNumber );
        SEXPR( SEXPR_TYPE aType );
        size_t m_lineNumber;
    };

    struct SEXPR_INTEGER : public SEXPR
    {
        int64_t m_value;

        SEXPR_INTEGER( int64_t aValue ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER ), m_value( aValue ) {};

        SEXPR_INTEGER( int64_t aValue, int aLineNumber ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_INTEGER, aLineNumber ), m_value( aValue ) {};
    };

    struct SEXPR_DOUBLE : public SEXPR
    {
        double m_value;

        SEXPR_DOUBLE( double aValue ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE ), m_value( aValue ) {};

        SEXPR_DOUBLE( double aValue, int aLineNumber ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_DOUBLE, aLineNumber ), m_value( aValue ) {};
    };

    struct SEXPR_STRING : public SEXPR
    {
        std::string m_value;

        SEXPR_STRING( const std::string& aValue ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING ), m_value( aValue ) {};

        SEXPR_STRING( const std::string& aValue, int aLineNumber ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_STRING, aLineNumber ), m_value( aValue ) {};
    };

    struct SEXPR_SYMBOL : public SEXPR
    {
        std::string m_value;

        SEXPR_SYMBOL( const std::string& aValue ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL ), m_value( aValue ) {};

        SEXPR_SYMBOL( const std::string& aValue, int aLineNumber ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_ATOM_SYMBOL, aLineNumber ), m_value( aValue ) {};
    };

    struct _OUT_STRING
    {
        bool _Symbol;
        const std::string& _String;
    };

    inline _OUT_STRING AsSymbol( const std::string& aString )
    {
        struct _OUT_STRING ret = { true, aString };
        return ret;
    }

    inline _OUT_STRING AsString( const std::string& aString )
    {
        struct _OUT_STRING ret = { false, aString };
        return ret;
    }

    struct _IN_STRING
    {
        bool _Symbol;
        std::string& _String;
    };

    inline _IN_STRING AsSymbol( std::string& aString )
    {
        struct _IN_STRING ret = { true, aString };
        return ret;
    }

    inline _IN_STRING AsString( std::string& aString )
    {
        struct _IN_STRING ret = { false, aString };
        return ret;
    }

    class SEXPR_SCAN_ARG
    {
    public:
        SEXPR_SCAN_ARG( int32_t* aValue ) :
            type( Type::INT ) { u.int_value = aValue; }

        SEXPR_SCAN_ARG( int64_t* aValue ) :
            type( Type::LONGINT ) { u.lint_value = aValue; }

        SEXPR_SCAN_ARG( double* aValue ) :
            type( Type::DOUBLE ) { u.dbl_value = aValue; }

        SEXPR_SCAN_ARG( std::string* aValue ) :
            type( Type::STRING ) { u.str_value = aValue; }

        SEXPR_SCAN_ARG( _IN_STRING& aValue ) :
            type( Type::SEXPR_STRING ) { u.sexpr_str = &aValue; }

        SEXPR_SCAN_ARG( const std::string* aValue ) :
            type( Type::STRING_COMP ) { str_value = *aValue; }

        SEXPR_SCAN_ARG( std::string aValue ) :
            type( Type::STRING_COMP ) { str_value = aValue; }

        SEXPR_SCAN_ARG( const char* aValue ) :
            type( Type::STRING_COMP ) { str_value = aValue; }

    private:
        friend class SEXPR_LIST;

        enum class Type : char { INT, DOUBLE, STRING, LONGINT, STRING_COMP, SEXPR_STRING };
        Type type;

        union
        {
            int64_t* lint_value;
            int32_t* int_value;
            double* dbl_value;
            std::string* str_value;
            _IN_STRING* sexpr_str;
        } u;

        std::string str_value;
    };

    class SEXPR_CHILDREN_ARG
    {
    public:
        SEXPR_CHILDREN_ARG( int32_t aValue ) :
            type( Type::INT ) { u.int_value = aValue; }

        SEXPR_CHILDREN_ARG( int64_t aValue ) :
            type( Type::LONGINT ) { u.lint_value = aValue; }

        SEXPR_CHILDREN_ARG( double aValue ) :
            type( Type::DOUBLE ) { u.dbl_value = aValue; }

        SEXPR_CHILDREN_ARG( const std::string& aValue ) :
            type( Type::STRING ) { str_value = aValue; }

        SEXPR_CHILDREN_ARG( const char* aValue ) :
            type( Type::STRING ) { str_value = aValue; }

        SEXPR_CHILDREN_ARG( const _OUT_STRING& aValue ) :
            type( Type::SEXPR_STRING ) { str_value = aValue._String; u.symbol = aValue._Symbol; }

        SEXPR_CHILDREN_ARG( SEXPR* aPointer ) :
            type( Type::SEXPR_ATOM ) { u.sexpr_ptr = aPointer; }

    private:
        friend class SEXPR_LIST;

        enum class Type : char { INT, DOUBLE, STRING, LONGINT, SEXPR_STRING, SEXPR_ATOM };
        Type type;

        union
        {
            int64_t lint_value;
            int32_t int_value;
            double dbl_value;
            SEXPR* sexpr_ptr;
            bool symbol;
        } u;

        std::string str_value;
    };

    class SEXPR_LIST : public SEXPR
    {
    public:
        SEXPR_LIST() : SEXPR( SEXPR_TYPE::SEXPR_TYPE_LIST ), m_inStreamChild( 0 ) {};

        SEXPR_LIST( int aLineNumber ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_LIST, aLineNumber), m_inStreamChild( 0 ) {};

        template <typename... Args>
        SEXPR_LIST( const Args&... args ) :
            SEXPR( SEXPR_TYPE::SEXPR_TYPE_LIST ), m_inStreamChild( 0 )
        {
            AddChildren(args...);
        };

        SEXPR_VECTOR m_children;

        template <typename... Args>
        size_t Scan( const Args&... args )
        {
            SEXPR_SCAN_ARG arg_array[] = { args... };
            return doScan( arg_array, sizeof...( Args ) );
        }

        template <typename... Args>
        void AddChildren( const Args&... args )
        {
            SEXPR_CHILDREN_ARG arg_array[] = { args... };
            doAddChildren( arg_array, sizeof...( Args ) );
        }

        virtual ~SEXPR_LIST();

        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, double value );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, float value );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, int64_t value );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, int32_t value );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, std::string value );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, const _OUT_STRING setting );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, const ISEXPRABLE& obj );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, SEXPR_LIST* list2 );
        friend SEXPR_LIST& operator<< ( SEXPR_LIST& list, SEXPR* obj );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, ISEXPRABLE& obj );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, std::string& str );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, int32_t& inte );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, int64_t& inte );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, float& inte );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, double& inte );
        friend SEXPR_LIST& operator>> ( SEXPR_LIST& input, const _IN_STRING is );

    private:
        int m_inStreamChild;
        size_t doScan( const SEXPR_SCAN_ARG *args, size_t num_args );
        void doAddChildren( const SEXPR_CHILDREN_ARG *args, size_t num_args );
    };

} // namespace SEXPR

#endif
