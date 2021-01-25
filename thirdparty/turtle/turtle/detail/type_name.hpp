// http://turtle.sourceforge.net
//
// Copyright Mathieu Champlon 2009
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef MOCK_TYPE_NAME_HPP_INCLUDED
#define MOCK_TYPE_NAME_HPP_INCLUDED

#include "../config.hpp"
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/test/utils/basic_cstring/io.hpp>
#include <boost/version.hpp>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <typeinfo>
#ifdef __GNUC__
#    include <cstdlib>
#    include <cxxabi.h>
#endif

namespace mock { namespace detail {
    class type_name
    {
    public:
        explicit type_name(const std::type_info& info) : info_(&info) {}
        friend std::ostream& operator<<(std::ostream& s, const type_name& t)
        {
            t.serialize(s, *t.info_);
            return s;
        }

    private:
        static void serialize(std::ostream& s, const std::type_info& info)
        {
            const char* name = info.name();
#ifdef __GNUC__
            int status = 0;
            struct Deleter
            {
                void operator()(const void* p) { std::free(const_cast<void*>(p)); }
            };
            std::unique_ptr<const char, Deleter> demangled(abi::__cxa_demangle(name, 0, 0, &status));
            if(!status && demangled)
                name = demangled.get();
#endif
            serialize(s, name);
        }

        typedef std::string::size_type size_type;

        static void serialize(std::ostream& s, std::string name)
        {
            const size_type nm = rfind(name, ':') + 1;
            const size_type tpl = name.find('<', nm);
            s << clean(name.substr(nm, tpl - nm));
            if(tpl == std::string::npos)
                return;
            s << '<';
            list(s, name.substr(tpl + 1, name.rfind('>') - tpl - 1));
            s << '>';
        }
        static void list(std::ostream& s, const std::string& name)
        {
            const size_type comma = rfind(name, ',');
            if(comma != std::string::npos)
            {
                list(s, name.substr(0, comma));
                s << ", ";
            }
            serialize(s, name.substr(comma + 1));
        }
        static std::string clean(std::string name)
        {
            boost::algorithm::trim(name);
            boost::algorithm::erase_all(name, "class ");
            boost::algorithm::erase_all(name, "struct ");
            boost::algorithm::erase_all(name, "__ptr64");
            boost::algorithm::replace_all(name, " &", "&");
            boost::algorithm::replace_all(name, "& ", "&");
            boost::algorithm::replace_all(name, " *", "*");
            boost::algorithm::replace_all(name, "* ", "*");
            return name;
        }
        static size_type rfind(const std::string& name, char c)
        {
            size_type count = 0;
            for(size_type i = name.size() - 1; i > 0; --i)
            {
                if(name[i] == '>')
                    ++count;
                else if(name[i] == '<')
                    --count;
                if(name[i] == c && count == 0)
                    return i;
            }
            return std::string::npos;
        }

        const std::type_info* info_;
    };
    template<typename T>
    type_name make_type_name()
    {
        return type_name(typeid(T));
    }
    template<typename T>
    type_name make_type_name(const T&)
    {
        return type_name(typeid(T));
    }
}} // namespace mock::detail

#endif // MOCK_TYPE_NAME_HPP_INCLUDED
