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

#define TESTMODE 0

#include <libeval/numeric_evaluator.h>

#if !TESTMODE
#include <common.h>
#else
#include <unistd.h>
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>


/* The (generated) lemon parser is written in C.
 * In order to keep its symbol from the global namespace include the parser code with
 * a C++ namespace.
 */
namespace numEval
{

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include "grammar.c"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

} /* namespace numEval */

NumericEvaluator :: NumericEvaluator() : pClParser(0)
{
   struct lconv* lc = localeconv();
   cClDecSep = *lc->decimal_point;

   bClTextInputStorage = true;

   bClError = false;
   bClParseFinished = false;

   init();
}

NumericEvaluator :: ~NumericEvaluator()
{
   numEval::ParseFree(pClParser, free);

   // Allow explicit call to destructor
   pClParser = nullptr;

   clear();
}

void
NumericEvaluator :: init()
{
   if (pClParser == nullptr)
      pClParser = numEval::ParseAlloc(malloc);

   //numEval::ParseTrace(stdout, "lib");

#if TESTMODE
   eClUnitDefault = Unit::Metric;
#else
   switch (g_UserUnit)
   {
   case INCHES      : eClUnitDefault = Unit::Inch;   break;
   case MILLIMETRES : eClUnitDefault = Unit::Metric; break;
   default:           eClUnitDefault = Unit::Metric; break;
   }
#endif
}

void
NumericEvaluator :: clear(const void* pObj)
{
   free(clToken.token);
   clToken.token = nullptr;
   clToken.input = nullptr;
   bClError = true;

   if (bClTextInputStorage && pObj) clObjMap.erase(pObj);
}

void
NumericEvaluator :: parse(int token, numEval::TokenType value)
{
   numEval::Parse(pClParser, token, value, this);
}

void
NumericEvaluator :: parseError(const char* s)
{
   bClError = true;
}

void
NumericEvaluator :: parseOk()
{
   bClParseFinished = true;
}

void
NumericEvaluator :: parseSetResult(double val)
{
   snprintf(clToken.token, clToken.OutLen, "%.10g", val);
}

const char*
NumericEvaluator :: textInput(const void* pObj) const
{
   auto it = clObjMap.find(pObj);
   if (it != clObjMap.end()) return it->second.c_str();

   return nullptr;
}

bool
NumericEvaluator :: process(const char* s)
{
   /* Process new string.
    * Feed parser token after token until end of input.
    */

   newString(s);

   if (pClParser == nullptr) init();
   bClError = false;
   bClParseFinished = false;

   Token tok;
   do {
      tok = getToken();
      parse(tok.token, tok.value);
      if (bClParseFinished || tok.token == ENDS) {
         // Reset parser by passing zero as token ID, value is ignored.
         numEval::Parse(pClParser, 0, tok.value, this);
         break;
      }
     //usleep(200000);
   } while (tok.token);

   return !bClError;
}

bool
NumericEvaluator :: process(const char* s, const void* pObj)
{
   if (bClTextInputStorage) // Store input string for (text entry) pObj.
      clObjMap[pObj] = s;
   return process(s);
}

void
NumericEvaluator :: newString(const char* s)
{
   clear();
   auto len = strlen(s);
   clToken.token = reinterpret_cast<decltype(clToken.token)>(malloc(TokenStat::OutLen+1));
   strcpy(clToken.token, "0");
   clToken.inputLen = len;
   clToken.pos = 0;
   clToken.input = s;
   bClParseFinished = false;
}

NumericEvaluator::Token
NumericEvaluator :: getToken()
{
   Token retval;
   size_t idx;

   retval.token = ENDS;
   retval.value.dValue = 0;

   if (clToken.token == nullptr) return retval;
   if (clToken.input == nullptr) return retval;
   if (clToken.pos >= clToken.inputLen) return retval;

   auto isDecSep = [&](char ch) -> bool {
      if (ch == cClDecSep) return true;
      if (cClDecSep == ',' && ch == '.') return true;
      return false;
   };

   // Lambda: get value as string, store into clToken.token and update current index.
   auto extractNumber = [&]() {
      short sepCount = 0;
      idx = 0;
      auto ch = clToken.input[clToken.pos];
      do {
         if (ch == isDecSep(ch) && sepCount) break;
         clToken.token[idx++] = ch;
         if (isDecSep(ch)) sepCount++;
         ch = clToken.input[++clToken.pos];
      } while (isdigit(ch) || isDecSep(ch));
      clToken.token[idx] = 0;

      // Ensure that the systems decimal separator is used
      for (int i = strlen(clToken.token); i; i--) if (isDecSep(clToken.token[i-1])) clToken.token[i-1] = cClDecSep;
   };

   /* Lamda: Get unit for current token. Returns Unit::Invalid if token is not a unit.
    * '"', "in", "th", "mi", "mil" or "mm"
    */
   auto checkUnit = [this]() -> Unit {
      const int sizeLeft = clToken.inputLen - clToken.pos;
      Unit convertFrom = Unit::Invalid;
      char unit[2] = { 0, 0 };
      for (int i = 0; i < sizeLeft && i < int(sizeof(unit)/sizeof(unit[0])); i++) unit[i] = tolower(clToken.input[clToken.pos+i]);
      auto tokcmp = [sizeLeft, unit](const char* s, int len) -> int {
         if (len > sizeLeft) return 0;
         if (!strncmp(unit, s, len)) return len;
         return 0;
      };
      int size = 0;
      if ((size = tokcmp("\"", 1)))       convertFrom = Unit::Inch;
      else if ((size = tokcmp("in",  2))) convertFrom = Unit::Inch;
      else if ((size = tokcmp("mi",  2))) convertFrom = Unit::Mil;
      else if ((size = tokcmp("th",  2))) convertFrom = Unit::Mil;
      else if ((size = tokcmp("mm",  2))) convertFrom = Unit::Metric;
      clToken.pos += size;

      if (size) {
         while (clToken.pos < clToken.inputLen && isalnum(clToken.input[clToken.pos])) clToken.pos++;
      }

      return convertFrom;
   };

   // Start processing of first/next token: Remove whitespace
   char ch;
   for (;;) {
      ch = clToken.input[clToken.pos];
      if (ch == ' ') {
         clToken.pos++;
      }
      else break;
   }

   Unit convertFrom;

   if (ch == 0) {
      /* End of input */
   }
   else if (isdigit(ch) || isDecSep(ch)) { // VALUE
      extractNumber();
      retval.token = VALUE;
      retval.value.dValue = atof(clToken.token);
   }
   else if ((convertFrom = checkUnit()) != Unit::Invalid) { // UNIT
      /* Units are appended to a VALUE.
       * Determine factor to default unit if unit for value is given.
       * Example: Default is mm, unit is inch: factor is 25.4
       * The factor is assigned to the terminal UNIT. The actual
       * conversion is done within a parser action.
       */
      retval.token = UNIT;
      if (eClUnitDefault == Unit::Metric)
      {
         switch (convertFrom) {
            case Unit::Inch    : retval.value.dValue = 25.4;        break;
            case Unit::Mil     : retval.value.dValue = 25.4/1000.0; break;
            case Unit::Metric  : retval.value.dValue = 1.0;         break;
            case Unit::Invalid : break;
         }
      }
      else if (eClUnitDefault == Unit::Inch)
      {
         switch (convertFrom) {
            case Unit::Inch    : retval.value.dValue =  1.0;         break;
            case Unit::Mil     : retval.value.dValue =  1.0/1000.0;  break;
            case Unit::Metric  : retval.value.dValue =  1.0/25.4;    break;
            case Unit::Invalid : break;
         }
      }
   }
   else if (isalpha(ch)) { // VAR
      const char* cptr = &clToken.input[clToken.pos];
      cptr++;
      while (isalnum(*cptr)) cptr++;
      retval.token = VAR;
      size_t bytesToCopy = cptr - &clToken.input[clToken.pos];
      if (bytesToCopy >= sizeof(retval.value.text)) bytesToCopy = sizeof(retval.value.text)-1;
      strncpy(retval.value.text, &clToken.input[clToken.pos], bytesToCopy);
      retval.value.text[bytesToCopy] = 0;
      clToken.pos += cptr - &clToken.input[clToken.pos];
   }
   else { // Single char tokens
      switch (ch) {
         case '+' : retval.token = PLUS;  break;
         case '-' : retval.token = MINUS; break;
         case '*' : retval.token = MULT;  break;
         case '/' : retval.token = DIVIDE; break;
         case '(' : retval.token = PARENL; break;
         case ')' : retval.token = PARENR; break;
         case '=' : retval.token = ASSIGN; break;
         case ';' : retval.token = SEMCOL; break;
         default: bClError = true; break;   /* invalid character */
      }
      clToken.pos++;
   }

   return retval;
}

void
NumericEvaluator :: setVar(const std::string& s, double value)
{
   clVarMap[s] = value;
}

double
NumericEvaluator :: getVar(const std::string& s)
{
   auto result = clVarMap.find(s);
   if (result != clVarMap.end()) return result->second;
   return 0.0;
}
