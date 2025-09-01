/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <kicommon.h>
#include <wx/string.h>
#include <unordered_map>
#include <string>
#include <memory>
#include <variant>
#include <vector>
#include <functional>

// Include EDA units support
#include <eda_units.h>

// Include the parser types
#include <text_eval/text_eval_types.h>

// Forward declaration for NUMERIC_EVALUATOR compatibility
class NUMERIC_EVALUATOR_COMPAT;

/**
 * @brief High-level wrapper for evaluating mathematical and string expressions in wxString format
 *
 * This class provides a simple interface for evaluating expressions containing @{} syntax
 * within wxString objects. It supports both map-based variable lookup and flexible
 * callback-based variable resolution for dynamic data access.
 *
 * The evaluator can work in two modes:
 * 1. Static variable mode: Variables are stored internally and looked up from memory
 * 2. Callback mode: Variables are resolved dynamically using a user-provided function
 *
 * Example usage:
 * @code
 * // Static variable mode
 * EXPRESSION_EVALUATOR evaluator;
 * evaluator.SetVariable("price", 99.99);
 * evaluator.SetVariable("product", "Widget");
 * evaluator.SetVariable("qty", 3);
 *
 * wxString input = "Product: @{upper(${product})} - Total: @{currency(${price} * ${qty})}";
 * wxString result = evaluator.Evaluate(input);
 * // Result: "Product: WIDGET - Total: $299.97"
 *
 * // Callback mode
 * auto callback = [](const std::string& varName) -> calc_parser::Result<calc_parser::Value> {
 *     if (varName == "current_time") {
 *         return calc_parser::MakeValue<calc_parser::Value>(getCurrentTimestamp());
 *     }
 *     return calc_parser::MakeError<calc_parser::Value>("Variable not found: " + varName);
 * };
 * EXPRESSION_EVALUATOR callbackEvaluator(callback);
 * wxString result2 = callbackEvaluator.Evaluate("Current time: @{${current_time}}");
 * @endcode
 */
class KICOMMON_API EXPRESSION_EVALUATOR
{
public:
    // Callback function type for dynamic variable resolution
    using VariableCallback = std::function<calc_parser::Result<calc_parser::Value>(const std::string& aVariableName)>;

private:
    std::unordered_map<std::string, calc_parser::Value> m_variables;
    mutable std::unique_ptr<calc_parser::ERROR_COLLECTOR> m_lastErrors;
    bool m_clearVariablesOnEvaluate;
    VariableCallback m_customCallback;
    bool m_useCustomCallback;
    EDA_UNITS m_defaultUnits;  // Default units for calculations

public:
    /**
     * @brief Construct a new Expression Evaluator in static variable mode
     * @param aClearVariablesOnEvaluate If true, variables are cleared after each evaluation
     */
    explicit EXPRESSION_EVALUATOR( bool aClearVariablesOnEvaluate = false );

    /**
     * @brief Construct with default units support
     * @param aUnits Default units for parsing and evaluating expressions
     * @param aClearVariablesOnEvaluate If true, variables are cleared after each evaluation
     */
    explicit EXPRESSION_EVALUATOR( EDA_UNITS aUnits, bool aClearVariablesOnEvaluate = false );

    /**
     * @brief Construct with custom variable resolver callback
     * @param aVariableCallback Custom function for variable resolution
     * @param aClearVariablesOnEvaluate If true, local variables are cleared after evaluation
     */
    explicit EXPRESSION_EVALUATOR( VariableCallback aVariableCallback,
                                 bool aClearVariablesOnEvaluate = false );

    /**
     * @brief Construct with units and custom variable resolver callback
     * @param aUnits Default units for parsing and evaluating expressions
     * @param aVariableCallback Custom function for variable resolution
     * @param aClearVariablesOnEvaluate If true, local variables are cleared after evaluation
     */
    explicit EXPRESSION_EVALUATOR( EDA_UNITS aUnits, VariableCallback aVariableCallback,
                                 bool aClearVariablesOnEvaluate = false );

    /**
     * @brief Destructor
     */
    ~EXPRESSION_EVALUATOR();

    // Copy and move operations
    EXPRESSION_EVALUATOR( const EXPRESSION_EVALUATOR& aOther );
    EXPRESSION_EVALUATOR& operator=( const EXPRESSION_EVALUATOR& aOther );
    EXPRESSION_EVALUATOR( EXPRESSION_EVALUATOR&& aOther ) noexcept;
    EXPRESSION_EVALUATOR& operator=( EXPRESSION_EVALUATOR&& aOther ) noexcept;

    /**
     * @brief Set a custom variable resolver callback
     * @param aCallback Function to call for variable resolution
     *
     * When set, this callback takes precedence over stored variables.
     * The callback receives variable names and should return Result<Value>.
     * Set to nullptr or call ClearVariableCallback() to disable callback mode.
     */
    void SetVariableCallback( VariableCallback aCallback );

    /**
     * @brief Clear the custom variable resolver callback
     *
     * After calling this, the evaluator will use stored variables only.
     */
    void ClearVariableCallback();

    /**
     * @brief Check if a custom variable callback is set
     * @return true if custom callback is active
     */
    bool HasVariableCallback() const;

    /**
     * @brief Set the default units for expressions
     * @param aUnits The units to use as default (mm, mil, inch, etc.)
     *
     * When expressions contain numeric values with unit suffixes (e.g., "1mm", "25mil"),
     * they will be converted to the default units for calculation.
     */
    void SetDefaultUnits( EDA_UNITS aUnits );

    /**
     * @brief Get the current default units
     * @return Current default units
     */
    EDA_UNITS GetDefaultUnits() const;

    /**
     * @brief Set a numeric variable for use in expressions
     * @param aName Variable name (used as ${name} in expressions)
     * @param aValue Numeric value
     *
     * This has no effect when using callback mode, unless the callback
     * chooses to fall back to stored variables.
     */
    void SetVariable( const wxString& aName, double aValue );

    /**
     * @brief Set a string variable for use in expressions
     * @param aName Variable name (used as ${name} in expressions)
     * @param aValue String value
     *
     * This has no effect when using callback mode, unless the callback
     * chooses to fall back to stored variables.
     */
    void SetVariable( const wxString& aName, const wxString& aValue );

    /**
     * @brief Set a variable using std::string (convenience overload)
     * @param aName Variable name
     * @param aValue String value
     */
    void SetVariable( const std::string& aName, const std::string& aValue );

    /**
     * @brief Remove a variable from the evaluator
     * @param aName Variable name to remove
     * @return true if variable was found and removed, false otherwise
     */
    bool RemoveVariable( const wxString& aName );

    /**
     * @brief Clear all stored variables
     *
     * This does not affect callback-based variable resolution.
     */
    void ClearVariables();

    /**
     * @brief Check if a variable exists in stored variables
     * @param aName Variable name to check
     * @return true if variable exists in stored variables, false otherwise
     *
     * Note: This only checks stored variables, not callback-resolved variables.
     */
    bool HasVariable( const wxString& aName ) const;

    /**
     * @brief Get the current value of a stored variable
     * @param aName Variable name
     * @return Variable value as wxString, or empty string if not found
     *
     * Note: This only returns stored variables, not callback-resolved variables.
     */
    wxString GetVariable( const wxString& aName ) const;

    /**
     * @brief Get all stored variable names currently defined
     * @return Vector of variable names
     *
     * Note: This only returns stored variables, not callback-available variables.
     */
    std::vector<wxString> GetVariableNames() const;

    /**
     * @brief Set multiple variables at once from a map
     * @param aVariables Map of variable names to numeric values
     */
    void SetVariables( const std::unordered_map<wxString, double>& aVariables );

    /**
     * @brief Set multiple string variables at once from a map
     * @param aVariables Map of variable names to string values
     */
    void SetVariables( const std::unordered_map<wxString, wxString>& aVariables );

    /**
     * @brief Main evaluation function - processes input string and evaluates all @{} expressions
     * @param aInput Input string potentially containing @{} expressions
     * @return Fully evaluated string with all expressions replaced by their values
     *
     * Variables are resolved using the callback (if set) or stored variables.
     */
    wxString Evaluate( const wxString& aInput );

    /**
     * @brief Evaluate with additional temporary variables (doesn't modify stored variables)
     * @param aInput Input string to evaluate
     * @param aTempVariables Temporary numeric variables for this evaluation only
     * @return Evaluated string
     *
     * Temporary variables have lower priority than callback resolution but higher
     * priority than stored variables.
     */
    wxString Evaluate( const wxString& aInput,
                      const std::unordered_map<wxString, double>& aTempVariables );

    /**
     * @brief Evaluate with mixed temporary variables
     * @param aInput Input string to evaluate
     * @param aTempNumericVars Temporary numeric variables
     * @param aTempStringVars Temporary string variables
     * @return Evaluated string
     *
     * Priority order: callback > temp string vars > temp numeric vars > stored variables
     */
    wxString Evaluate( const wxString& aInput,
                      const std::unordered_map<wxString, double>& aTempNumericVars,
                      const std::unordered_map<wxString, wxString>& aTempStringVars );

    /**
     * @brief Check if the last evaluation had errors
     * @return true if errors occurred during last evaluation
     */
    bool HasErrors() const;

    /**
     * @brief Get count of errors from the last evaluation
     * @return Number of errors that occurred
     */
    size_t GetErrorCount() const;

    /**
     * @brief Get detailed error information from the last evaluation
     * @return Error summary as wxString, empty if no errors
     */
    wxString GetErrorSummary() const;

    /**
     * @brief Get individual error messages from the last evaluation
     * @return Vector of error messages
     */
    std::vector<wxString> GetErrors() const;

    /**
     * @brief Clear any stored error information
     */
    void ClearErrors();

    /**
     * @brief Enable or disable automatic variable clearing after evaluation
     * @param aEnable If true, stored variables are cleared after each Evaluate() call
     *
     * Note: This only affects stored variables, not callback behavior.
     */
    void SetClearVariablesOnEvaluate( bool aEnable );

    /**
     * @brief Check if automatic variable clearing is enabled
     * @return true if variables are cleared after each evaluation
     */
    bool GetClearVariablesOnEvaluate() const;

    /**
     * @brief Test if an expression can be parsed without evaluating it
     * @param aExpression Single expression to test (without @{} wrapper)
     * @return true if expression is syntactically valid
     *
     * This creates a temporary evaluator to test syntax only.
     */
    bool TestExpression( const wxString& aExpression );

    /**
     * @brief Count the number of @{} expressions in input string
     * @param aInput Input string to analyze
     * @return Number of @{} expression blocks found
     */
    size_t CountExpressions( const wxString& aInput ) const;

    /**
     * @brief Extract all @{} expressions from input without evaluating
     * @param aInput Input string to analyze
     * @return Vector of expression strings (content between @{} markers)
     */
    std::vector<wxString> ExtractExpressions( const wxString& aInput ) const;

private:
    /**
     * @brief Convert wxString to std::string using UTF-8 encoding
     * @param aWxStr wxString to convert
     * @return Converted std::string
     */
    std::string wxStringToStdString( const wxString& aWxStr ) const;

    /**
     * @brief Convert std::string to wxString using UTF-8 encoding
     * @param aStdStr std::string to convert
     * @return Converted wxString
     */
    wxString stdStringToWxString( const std::string& aStdStr ) const;

    /**
     * @brief Create a callback function that combines all variable sources
     * @param aTempNumericVars Temporary numeric variables (optional)
     * @param aTempStringVars Temporary string variables (optional)
     * @return Combined callback for parser
     */
    VariableCallback createCombinedCallback(
        const std::unordered_map<wxString, double>* aTempNumericVars = nullptr,
        const std::unordered_map<wxString, wxString>* aTempStringVars = nullptr ) const;

    /**
     * @brief Parse and evaluate the input string using the expression parser
     * @param aInput Input string in std::string format
     * @param aVariableCallback Callback function to use for variable resolution
     * @return Pair of (result_string, had_errors)
     */
    std::pair<std::string, bool> evaluateWithParser(
        const std::string& aInput,
        VariableCallback aVariableCallback );

    /**
     * @brief Parse and evaluate with partial error recovery - malformed expressions left unchanged
     * @param aInput Input string in std::string format
     * @param aVariableCallback Callback function to use for variable resolution
     * @return Pair of (result_string, had_errors)
     */
    std::pair<std::string, bool> evaluateWithPartialErrorRecovery(
        const std::string& aInput,
        VariableCallback aVariableCallback );

    /**
     * @brief Full parser evaluation (original behavior) - fails completely on any error
     * @param aInput Input string in std::string format
     * @param aVariableCallback Callback function to use for variable resolution
     * @return Pair of (result_string, had_errors)
     */
    std::pair<std::string, bool> evaluateWithFullParser(
        const std::string& aInput,
        VariableCallback aVariableCallback );

    /**
     * @brief Expand ${variable} patterns that are outside @{} expressions
     * @param aInput Input string to process
     * @param aTempNumericVars Temporary numeric variables
     * @param aTempStringVars Temporary string variables
     * @return String with ${variable} patterns outside expressions expanded
     */
    wxString expandVariablesOutsideExpressions(
        const wxString& aInput,
        const std::unordered_map<wxString, double>& aTempNumericVars,
        const std::unordered_map<wxString, wxString>& aTempStringVars ) const;
};

/**
 * @brief NUMERIC_EVALUATOR compatible wrapper around EXPRESSION_EVALUATOR
 *
 * This class provides a drop-in replacement for NUMERIC_EVALUATOR that uses
 * the new EXPRESSION_EVALUATOR backend. It maintains the same API to allow
 * seamless migration of existing code.
 *
 * The key difference is that expressions are automatically wrapped in @{...}
 * syntax before evaluation.
 *
 * Example usage:
 * @code
 * // Old NUMERIC_EVALUATOR code:
 * NUMERIC_EVALUATOR eval(EDA_UNITS::MM);
 * eval.Process("1 + 2");
 * wxString result = eval.Result(); // "3"
 *
 * // New compatible code:
 * NUMERIC_EVALUATOR_COMPAT eval(EDA_UNITS::MM);
 * eval.Process("1 + 2");
 * wxString result = eval.Result(); // "3"
 * @endcode
 */
class KICOMMON_API NUMERIC_EVALUATOR_COMPAT
{
private:
    EXPRESSION_EVALUATOR m_evaluator;
    wxString m_lastInput;
    wxString m_lastResult;
    bool m_lastValid;

public:
    /**
     * @brief Constructor with default units
     * @param aUnits Default units for the evaluator
     */
    explicit NUMERIC_EVALUATOR_COMPAT( EDA_UNITS aUnits );

    /**
     * @brief Destructor
     */
    ~NUMERIC_EVALUATOR_COMPAT();

    /**
     * @brief Clear parser state but retain variables
     *
     * Resets the parser state for processing a new expression.
     * User-defined variables are retained.
     */
    void Clear();

    /**
     * @brief Set default units for evaluation
     * @param aUnits The default units to use
     */
    void SetDefaultUnits( EDA_UNITS aUnits );

    /**
     * @brief Handle locale changes (for decimal separator)
     *
     * This is a no-op in the EXPRESSION_EVALUATOR implementation
     * since it handles locale properly internally.
     */
    void LocaleChanged();

    /**
     * @brief Check if the last evaluation was successful
     * @return True if last Process() call was successful
     */
    bool IsValid() const;

    /**
     * @brief Get the result of the last evaluation
     * @return Result string, or empty if invalid
     */
    wxString Result() const;

    /**
     * @brief Process and evaluate an expression
     * @param aString Expression to evaluate
     * @return True if evaluation was successful
     */
    bool Process( const wxString& aString );

    /**
     * @brief Get the original input text
     * @return The last input string passed to Process()
     */
    wxString OriginalText() const;

    /**
     * @brief Set a variable value
     * @param aString Variable name
     * @param aValue Variable value
     */
    void SetVar( const wxString& aString, double aValue );

    /**
     * @brief Get a variable value
     * @param aString Variable name
     * @return Variable value, or 0.0 if not defined
     */
    double GetVar( const wxString& aString );

    /**
     * @brief Remove a single variable
     * @param aString Variable name to remove
     */
    void RemoveVar( const wxString& aString );

    /**
     * @brief Remove all variables
     */
    void ClearVar();
};