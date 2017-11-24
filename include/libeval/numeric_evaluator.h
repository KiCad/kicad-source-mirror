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

/*
An evaluator object is used to replace an input string that represents
a mathematical expression by its result.

Example: Consider the input "3+4". The result of this expression is "7".
The NumericEvaluator can be used like this:

  NumericEvaluator eval;
  eval.process("3+4");
  printf("3+4", eval.result());

The same example with error checking. Please note that even a valid input string may result
in an empty output string or "NaN".

  NumericEvaluator eval;
  bool ret = eval.process("3+4");
  assert(ret == eval.isValid()); // isValid() reflects return value of process().
  if (eval.isValid()) printf("3+4=%s\n", eval.result());

Using variables
Expressions can refer to variables if they were defined by previous expressions.
A variable can be defined by an expression or by the setVar() method.
Expressions that define/set variables do not have a result.

  eval.process("x=1; y=2"); // Result is NaN
  eval.setVar("z", 3);
  eval.process("x+y+z");
  printf("x+y+z=%s\n", eval.result());

Input string storage
An evaluator object can store and retrieve the original input string using a pointer
as key. This can be used to restore the input string of a text entry field.

  eval.process("25.4-0.7", &eval);
  printf("%s = %s\n", eval.textInput(&eval), eval.result());

Unit conversion
The evaluator uses a default unit and constants can be specified with a unit.
As long as no units are used the default unit is not relevant. The default
unit is taken from the global (Kicad) variable g_UserUnit.
Supported units are millimeters (mm), Mil (mil) and inch (")

  eval.process("1\"");
  printf("1\" = %s\n", eval.result());
  eval.process("12.7 - 0.1\" - 50mil");
  printf("12.7 - 0.1\" - 50mil = %s\n", eval.result());
*/

#ifndef NUMERIC_EVALUATOR_H_
#define NUMERIC_EVALUATOR_H_

#include "grammar.h"
#include <stddef.h>
#include <string>
#include <string>
#include <map>

// This namespace is used for the lemon parser
namespace numEval
{

struct TokenType
{
   union {
      double dValue;
      int    iValue;
   };
   bool valid;
   char text[32];
};

} // namespace numEval

class NumericEvaluator {
   enum class Unit { Invalid, Metric, Inch, Mil };

public:
   NumericEvaluator();
   ~NumericEvaluator();

   /* Initialization and destruction. init() is invoked be the constructor and should not be needed
    * by the user.
    * clear() should be invoked by the user if a new input string is to be processed. It will reset
    * the parser. User defined variables are retained.
    */
   void init();
   void clear();

   /* Set the decimal separator for the input string. Defaults to '.' */
   void setDecimalSeparator(char sep);

   /* Enable or disable support for input string storage.
    * If enabled the input string is saved if process(const char*, const void*) is used.
    */
   void enableTextInputStorage(bool w) { bClTextInputStorage = w; }

   /* Used by the lemon parser */
   void parse(int token, numEval::TokenType value);
   void parseError(const char* s);
   void parseOk();
   void parseSetResult(double);

   /* Check if previous invokation of process() was successful */
   inline bool isValid() const { return !bClError; }

   /* Result of string processing. Undefined if !isValid() */
   inline const char* result() const { return clToken.token; }

   /* Evaluate input string.
    * Result can be retrieved by result().
    * Returns true if input string could be evaluated, otherwise false.
    */
   bool process(const char* s);

   /* Like process(const char*) but also stores input string in a std:map with key pObj. */
   bool process(const char* s, const void* pObj);

   /* Retrieve old input string with key pObj. */
   const char* textInput(const void* pObj) const;

   /* Add/set variable with value */
   void setVar(const std::string&, double value);

   /* Get value of variable. Returns 0.0 if not defined. */
   double getVar(const std::string&);

   /* Remove single variable */
   void removeVar(const std::string& s) { clVarMap.erase(s); }

   /* Remove all variables */
   void clearVar() { clVarMap.clear(); }

protected:
   /* Token type used by the tokenizer */
   struct Token {
      int token;
      numEval::TokenType value;
   };

   /* Begin processing of a new input string */
   void newString(const char* s);

   /* Tokenizer: Next token/value taken from input string. */
   Token getToken();

private:
   void* pClParser; // the current lemon parser state machine

   /* Token state for input string. */
   struct TokenStat {
      enum { OutLen=32 };
      TokenStat() : input(0), token(0), inputLen(0) { /* empty */ }
      const char* input; // current input string ("var=4")
      char* token;       // output token ("var", type:VAR; "4", type:VALUE)
      size_t inputLen;   // strlen(input)
      size_t pos;        // current index
   } clToken;

   char cClDecSep;       // decimal separator ('.')

   /* Parse progress. Set by parser actions. */
   bool bClError;
   bool bClParseFinished;

   bool bClTextInputStorage; // Enable input string storage used by process(const char*, const void*)

   Unit eClUnitDefault;      // Default unit for values

   std::map<const void*, std::string> clObjMap; // Map pointer to text entry -> (original) input string
   std::map<std::string, double> clVarMap;
};


#endif /* NUMERIC_EVALUATOR_H_ */
