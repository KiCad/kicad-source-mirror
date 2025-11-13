# Text Evaluation Parser Tests

This directory contains test suites for the KiCad text evaluation parser functionality.

## Test Files

### `test_text_eval_parser.cpp`
High-level integration tests using the `EXPRESSION_EVALUATOR` wrapper class.

- **Basic Arithmetic**: Addition, subtraction, multiplication, division, modulo, power operations
- **Variable Substitution**: Testing variable storage and retrieval in expressions
- **String Operations**: String concatenation, mixed string/number operations
- **Mathematical Functions**: `abs`, `sqrt`, `pow`, `floor`, `ceil`, `round`, `min`, `max`, `sum`, `avg`
- **String Functions**: `upper`, `lower`, `concat`, `beforefirst`, `beforelast`, `afterfirst`, `afterlast`
- **Formatting Functions**: `format`, `fixed`, `currency`
- **Date/Time Functions**: `today`, `now`, `dateformat`, `weekdayname`
- **Conditional Functions**: `if` statements with boolean logic
- **Random Functions**: `random()` number generation
- **Error Handling**: Syntax errors, runtime errors, undefined variables
- **Complex Expressions**: Nested functions, multi-step calculations
- **Performance Testing**: Large expressions and timing validation

### `test_text_eval_parser_core.cpp`
Low-level unit tests for the core parser components. Tests the internal API including:

- **ValueUtils**: Type conversion, arithmetic operations, string handling
- **Node Creation**: AST node factory methods and structure validation
- **EvaluationVisitor**: Direct AST evaluation with custom variable resolvers
- **Function Evaluation**: Individual function implementations and error cases
- **DocumentProcessor**: Document parsing and processing workflows
- **Error Collection**: Error reporting and message formatting
- **TokenType Utilities**: Token creation and manipulation

### `test_text_eval_parser_datetime.cpp`
Specialized tests for date and time functionality:

- **Date Formatting**: Various output formats (ISO, US, EU, Chinese, Japanese, Korean, long, short)
- **Current Date/Time**: `today()` and `now()` function validation
- **Date Arithmetic**: Adding/subtracting days, date calculations
- **Edge Cases**: Leap years, month boundaries, negative dates
- **Weekday Calculations**: Day-of-week determination and cycling
- **Performance**: Date operation timing validation

### `test_text_eval_parser_integration.cpp`
Integration tests simulating real-world KiCad usage scenarios:

- **Real-World Scenarios**: PCB documentation, title blocks, component summaries
- **Callback Variable Resolution**: Dynamic variable lookup from external sources
- **Thread Safety**: Multi-evaluator state isolation
- **Memory Management**: Large expression handling, resource cleanup
- **Parsing Edge Cases**: Whitespace, special characters, error recovery
- **Performance Testing**: Realistic workload simulation

## Tested Functions

### Mathematical Functions
- `abs(x)` - Absolute value
- `sqrt(x)` - Square root (with negative input validation)
- `pow(x, y)` - Power/exponentiation
- `floor(x)` - Round down to integer
- `ceil(x)` - Round up to integer
- `round(x, [precision])` - Round to specified decimal places
- `min(...)` - Minimum of multiple values
- `max(...)` - Maximum of multiple values
- `sum(...)` - Sum of multiple values
- `avg(...)` - Average of multiple values
- `shunt(r1, r2)` - Parallel resistance calculation: (r1×r2)/(r1+r2)
- `db(ratio)` - Power ratio to decibels: 10×log₁₀(ratio)
- `dbv(ratio)` - Voltage/current ratio to decibels: 20×log₁₀(ratio)
- `fromdb(db)` - Decibels to power ratio: 10^(dB/10)
- `fromdbv(db)` - Decibels to voltage/current ratio: 10^(dB/20)
- `enearest(value, [series])` - Nearest E-series standard value (default: E24)
- `eup(value, [series])` - Next higher E-series standard value (default: E24)
- `edown(value, [series])` - Next lower E-series standard value (default: E24)
  - Series options: "E3", "E6", "E12", "E24", "E48", "E96", "E192"

### String Functions
- `upper(str)` - Convert to uppercase
- `lower(str)` - Convert to lowercase
- `concat(...)` - Concatenate multiple values
- `beforefirst(str, c)` - Substring
- `beforelast(str, c)` - Substring
- `afterfirst(str, c)` - Substring
- `afterlast(str, c)` - Substring
- `format(num, [decimals])` - Format number with specified precision
- `fixed(num, [decimals])` - Fixed decimal formatting
- `currency(amount, [symbol])` - Currency formatting

### Date/Time Functions
- `today()` - Current date as days since epoch
- `now()` - Current timestamp as seconds since epoch
- `dateformat(days, [format])` - Format date string
  - Formats: "ISO", "US", "EU", "Chinese", "Japanese", "Korean", "long", "short"
- `weekdayname(days)` - Get weekday name for date

### Conditional Functions
- `if(condition, true_value, false_value)` - Conditional evaluation

### Utility Functions
- `random()` - Random number between 0 and 1

## Arithmetic Operators

- `+` - Addition (also string concatenation)
- `-` - Subtraction (also unary minus)
- `*` - Multiplication
- `/` - Division (with zero-division error handling)
- `%` - Modulo (with zero-modulo error handling)
- `^` - Exponentiation (right-associative)

## Variable Syntax

Variables are referenced using `${variable_name}` syntax and can be:
- Set statically using `evaluator.SetVariable()`
- Resolved dynamically using callback functions

## Expression Syntax

Calculations are embedded in text using `@{expression}` syntax:
- `"Result: @{2 + 3}"` → `"Result: 5"`
- `"Hello ${name}!"` → `"Hello World!"` (with variable substitution)
- `"Area: @{${width} * ${height}} mm²"` → `"Area: 100 mm²"`

## Error Handling

The parser collects errors for later diagnostics.  However, a string
with multiple expressions may be partially evaluated.  It will return an error for every
expression that was not fully evaluated.