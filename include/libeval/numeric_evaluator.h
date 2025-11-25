/*
 * This file is part of KiCad, a free EDA CAD application.
 * Derived from libeval, a simple math expression evaluator.
 *
 * Copyright (C) 2017 Michael Geselbracht, mgeselbracht3@gmail.com
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
As long as no units are used the default unit is not relevant.
Supported units are millimeters (mm), Mil (mil) and inch (")

  eval.process("1\"");
  printf("1\" = %s\n", eval.result());
  eval.process("12.7 - 0.1\" - 50mil");
  printf("12.7 - 0.1\" - 50mil = %s\n", eval.result());
*/

#ifndef NUMERIC_EVALUATOR_H_
#define NUMERIC_EVALUATOR_H_

#include <cstddef>
#include <map>
#include <string>

#include <eda_units.h>

// This namespace is used for the lemon parser
namespace numEval
{
    struct TokenType
    {
        union
        {
            double dValue;
            int    iValue;
        };

        bool valid;
        char text[32];
    };

} // namespace numEval

class KICOMMON_API NUMERIC_EVALUATOR
{
    enum class Unit
    {
        Invalid,
        UM,
        MM,
        CM,
        Inch,
        Mil,
        Degrees,
        SI,
        Femtoseconds,
        Picoseconds,
        PsPerInch,
        PsPerCm,
        PsPerMm
    };

public:
    NUMERIC_EVALUATOR( EDA_UNITS aUnits );
    ~NUMERIC_EVALUATOR();

    /* clear() should be invoked by the client if a new input string is to be processed. It
     * will reset the parser. User defined variables are retained.
     */
    void Clear();

    void SetDefaultUnits( EDA_UNITS aUnits );

    void LocaleChanged();

    /* Used by the lemon parser */
    void parseError(const char* s);
    void parseOk();
    void parseSetResult(double);

    /* Check if previous invocation of process() was successful */
    inline bool IsValid() const { return !m_parseError; }

    /* Result of string processing. Undefined if !isValid() */
    inline wxString Result() const { return wxString::FromUTF8( m_token.token ); }

    /* Evaluate input string.
     * Result can be retrieved by result().
     * Returns true if input string could be evaluated, otherwise false.
     */
    bool Process( const wxString& aString );

    /* Retrieve the original text before evaluation. */
    wxString OriginalText() const;

    /* Add/set variable with value */
    void SetVar( const wxString& aString, double aValue );

    /* Get value of variable. Returns 0.0 if not defined. */
    double GetVar( const wxString& aString );

    /* Remove single variable */
    void RemoveVar( const wxString& aString ) { m_varMap.erase( aString ); }

    /* Remove all variables */
    void ClearVar() { m_varMap.clear(); }

    static bool IsOldSchoolDecimalSeparator( wxUniChar ch, double* siScaler );
    static bool isOldSchoolDecimalSeparator( char ch, double* siScaler );
    static bool IsDecimalSeparator( char ch, char localeSeparator, bool allowInfixNotation );
    static bool IsDigit( char ch );

protected:
    /* Token type used by the tokenizer */
    struct Token
    {
        int token;
        numEval::TokenType value;
    };

    /* Begin processing of a new input string */
    void newString( const wxString& aString );

    /* Tokenizer: Next token/value taken from input string. */
    Token getToken();

    /* Used by processing loop */
    void parse( int token, numEval::TokenType value );

private:
    void* m_parser; // the current lemon parser state machine

    /* Token state for input string. */
    struct TokenStat
    {
        TokenStat() :
            input( nullptr ),
            token( nullptr ),
            inputLen( 0 ),
            outputLen( 0 ),
            pos( 0 )
        {};

        const char* input;      // current input string ("var=4")
        char*       token;      // output token ("var", type:VAR; "4", type:VALUE)
        size_t      inputLen;   // strlen(input)
        size_t      outputLen;  // At least 64, up to input length
        size_t      pos;        // current index
    }    m_token;

    char m_localeDecimalSeparator;

    /* Parse progress. Set by parser actions. */
    bool m_parseError;
    bool m_parseFinished;

    Unit m_defaultUnits;      // Default unit for values

    wxString m_originalText;

    std::map<wxString, double> m_varMap;
};


#endif /* NUMERIC_EVALUATOR_H_ */
