import re, sys
from pathlib import Path

COPYRIGHT = """/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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

"""

class MockGen:
    def __init__(self, path):
        self.members = []
        self.path = path
        with open(self.path) as f:
            self.lines = f.readlines()

    def __parseHeader(self):
        # group 1: retval and function
        # group 2: arguments
        # group 3: other keywords (const, override...)        
        regex = re.compile("virtual\s+([^\(]+)\(([^\)]*)\)(.*)")

        for line in self.lines:
            virtualFcn = regex.search(line)
            if virtualFcn:
                function = {
                    'name':virtualFcn[1],
                    'args':virtualFcn[2],
                    'extra':virtualFcn[3],
                    }
                self.members.append(function)

    def __generateMockDirective(self, virtualMember):
        if "const" in virtualMember['extra']:
            mockDirective = str("MOCK_CONST_METHOD( ")
        else:
            mockDirective = str("MOCK_METHOD( ")

        fcnName = virtualMember['name'].split()[-1]
        arguments = virtualMember['args'].split(',')
        arity = 0 if len(arguments) == 1 and not arguments[0] else len(arguments)
        mockDirective += fcnName + ", " + str(arity) + ", "

        fcnRetval = virtualMember['name'].strip().removesuffix(fcnName).strip()
        mockDirective += fcnRetval + "("

        if arity != 0:
            for i, arg in enumerate(arguments):
                mockDirective += " " if i == 0 else ", "
                mockDirective += arg.strip()
            mockDirective += " "

        return mockDirective + ") );\n"

    def generateMock(self):
        self.__parseHeader()

        with open('qa/mocks/out.txt', 'w') as output:
            output.write(COPYRIGHT)
            for virtualMember in self.members:
                directive = self.__generateMockDirective(virtualMember)
                output.write(directive)

if __name__ == "__main__":
    header = Path(sys.argv[1])
    if header.exists() and len(sys.argv) == 2:
        mockGen = MockGen(header)
        mockGen.generateMock()
    else:
        print("This script generates mocks for Turtle mock framework")
        print(f"Usage: {sys.argv[0]} path/to/header.hpp")
