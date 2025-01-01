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

#ifndef SEXPR_EXCEPTION_H_
#define SEXPR_EXCEPTION_H_

#include <exception>
#include <string>

namespace SEXPR
{
    class PARSE_EXCEPTION : public std::exception
    {
    public:
        PARSE_EXCEPTION( const std::string& aMessage ) : msg( aMessage ) {}
        const char* what() const noexcept override { return msg.c_str(); }
        virtual ~PARSE_EXCEPTION() noexcept {}

    private:
        std::string msg;
    };

    class INVALID_TYPE_EXCEPTION : public std::exception
    {
    public:
        INVALID_TYPE_EXCEPTION( const std::string& aMessage ) : msg( aMessage ) {}
        const char* what() const noexcept override { return msg.c_str(); }
        virtual ~INVALID_TYPE_EXCEPTION() noexcept {}

    private:
        std::string msg;
    };
}
#endif
