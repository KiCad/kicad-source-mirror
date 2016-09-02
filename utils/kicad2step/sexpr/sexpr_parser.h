/*
 * Copyright (C) 2016 Mark Roszko <mark.roszko@gmail.com>
 * Copyright (C) 2016 QiEDA Developers
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

#ifndef SEXPR_PARSER_H_
#define SEXPR_PARSER_H_

#include "sexpr/sexpr.h"
#include <string>
#include <vector>


namespace SEXPR
{
    class PARSER
    {
    public:
        PARSER();
        ~PARSER();
        SEXPR* Parse(const std::string &aString);
        SEXPR* ParseFromFile(const std::string &filename);
        static std::string GetFileContents(const std::string &filename);
    private:
        SEXPR* parseString(const std::string& aString, std::string::const_iterator& it);
        static const std::string whitespaceCharacters;
        int m_lineNumber;
        int m_lineOffset;
    };
}

#endif
