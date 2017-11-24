/*
    This file is part of libeval, a simple math expression evaluator

    Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <assert.h>

#include "numeric_evaluator.h"

int main()
{
   NumericEvaluator eval;

   eval.process("2.54mm+50mil");
   if (eval.isValid()) printf("%s\n", eval.result());

   eval.process("x=1; y=5;");
   if (eval.isValid()) printf("%s\n", eval.result());
   eval.process("x+y");
   if (eval.isValid()) printf("%s\n", eval.result());

   eval.setVar("posx", -3.14152);
   bool retval = eval.process("posx");
   assert(retval == eval.isValid());
   if (eval.isValid()) printf("%s\n", eval.result());

   eval.process("x=1; y=2");
   eval.setVar("z", 3);
   eval.process("x+y+z");
   printf("x+y+z=%s\n", eval.result());

   eval.process("1\"");
   printf("1\" = %s\n", eval.result());
   eval.process("12.7 - 0.1\" - 50mil");
   printf("12.7 - 0.1\" - 50mil = %s\n", eval.result());
}

