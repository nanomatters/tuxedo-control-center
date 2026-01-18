# TUXEDO Control Center - Coding Guidelines

## Indentation
- Use **2 spaces** for indentation
- Never use tabs

## Braces
- Opening braces `{` should always be on the **next line** (Allman style)
- **Single statements must not be wrapped in braces**
- Only use braces when you have multiple statements or for clarity in complex nested structures

Example:
```cpp
// Good: single statement without braces
if ( condition )
  return false;

// Good: multiple statements with braces
if ( condition )
{
  statement1;
  statement2;
}

// Bad: single statement with braces
if ( condition )
{
  return false;
}
```

## Parentheses and Spacing
- Always add **spaces before and after parentheses** in conditionals and function calls
- Format: `if ( condition )`, `while ( condition )`, `func( param )`
- Add **spaces inside template angle brackets**: `std::vector< std::string >`, `std::unique_ptr< DeviceInterface >`
- Add **spaces inside array/vector access brackets**: `array[ index ]`, `vector[ i ]`

Example:
```cpp
if ( not isAvailable() )
  return false;

int result = ioctl( m_fileHandle, request );
if ( result >= 0 )
{
  // process result
}

std::vector< std::string > names;
int value = array[ index ];
```

## Operators
- Use **`not`** instead of `!` for logical negation
- Use **`and`** instead of `&&`
- Use **`or`** instead of `||`

Example:
```cpp
if ( not condition )
if ( condition and other )
if ( condition or other )
```

## C++17 If-Statements with Initializer
- **Prefer C++17 if-with-initializer** when a variable is only used in the condition and body
- Move variable declaration into the if-statement using the syntax: `if ( init-statement; condition )`
- This limits variable scope and makes code more concise

Example:
```cpp
// Good: C++17 if-with-initializer
if ( int64_t value = readSysFsInteger( path, -1 ); value >= 0 )
  processValue( value );

if ( std::string result = executeCommand( cmd ); not result.empty() )
  parseResult( result );

// Bad: separate declaration
int64_t value = readSysFsInteger( path, -1 );
if ( value >= 0 )
  processValue( value );

std::string result = executeCommand( cmd );
if ( not result.empty() )
  parseResult( result );
```

## Control Flow Spacing
- **Always add a blank line before control flow statements** (`if`, `while`, `for`, `switch`) when preceded by other statements
- This improves readability by visually separating logic blocks
- Exception: No blank line needed after opening brace or before closing brace

Example:
```cpp
// Good: blank line before if/while/for
int result = performCalculation();
std::string data = fetchData();

if ( result > 0 )
  processResult( result );

while ( hasMoreData() )
  processNext();

// Bad: no blank line before control flow
int result = performCalculation();
if ( result > 0 )
  processResult( result );
```

## Return Statements
- Always add an **empty line after a `return` statement** if additional statements follow
- This improves code readability and separates control flow from logic

Example:
```cpp
bool ioctlCall(unsigned long request)
{
  if ( not isAvailable() )
    return false;

  int result = ioctl(m_fileHandle, request);
  return result >= 0;
}
```

## Naming Conventions

### Methods and Functions
- Use **camelCase** with **lowercase first letter**
- Public methods: `isAvailable()`, `ioctlCall()`
- Private methods: `openDevice()`, `closeDevice()`

### Member Variables
- Private member variables: prefix with `m_`
- Example: `m_fileHandle`, `m_buffer`

### Class Names
- Use **PascalCase** (capitalize first letter)
- Example: `class IO`, `class DeviceInterface`

## Comments and Documentation
- Use **Doxygen-style comments** for public APIs
- Document parameters with `@param`
- Document return values with `@return`
- **Avoid obvious comments** that simply restate what the code does
- **Use lowercase** in comments unless referring to type names, class names, or proper nouns
- Comments should explain **why**, not **what** (the code already shows what)

Example:
```cpp
/**
 * @brief Brief description of the function
 * @param paramName Description of parameter
 * @return Description of return value
 */

// Good: explains why, uses lowercase
if ( result < 0 )
{
  // negative values indicate hardware failure on this chipset
  return false;
}

// Bad: obvious comment, uses capital letter
if ( result < 0 )
{
  // Return false if result is negative
  return false;
}

// Good: refers to type name (DeviceInterface)
// initialize DeviceInterface before accessing hardware
auto device = std::make_unique< DeviceInterface >();
```

## Constexpr and Const Correctness
- **Use `constexpr` for functions** that can be evaluated at compile time
- Functions that return constant values should be `constexpr`
- **String parameters and references** that are not modified must be `const`
- Apply `const` to all read-only parameters

Example:
```cpp
// Good: constexpr for compile-time evaluable function
[[nodiscard]] constexpr int getDefaultValue() const noexcept
{
  return -1;
}

// Good: const string parameters
void processData( const std::string &input, const std::string &config )
{
  // use input and config without modification
}

// Bad: missing constexpr
[[nodiscard]] int getDefaultValue() const noexcept
{
  return -1;
}

// Bad: non-const string parameter
void processData( std::string &input )
{
  // only reads input, doesn't modify it
}
```

## Includes
- System headers in angle brackets: `#include <vector>`
- Local headers in quotes: `#include "local.h"`
- Group includes logically (system, then local)

## Line Length
- Keep lines reasonably readable, preferably under 100 characters
- Break long statements across multiple lines with proper indentation

## Memory Management
- Prefer `std::make_unique` over raw `new`/`delete`
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) exclusively
- Use **Null Object pattern** for safe fallback behavior instead of null pointers

Example:
```cpp
// Good: Smart pointers with unique_ptr
std::unique_ptr< DeviceInterface > device = std::make_unique< ClevoDevice >( io );

// Good: Null Object pattern instead of null checks
std::unique_ptr< DeviceInterface > m_activeDevice;

// Avoid: Raw pointers
DeviceInterface* device = new ClevoDevice(io); // DON'T DO THIS
```

## Static Factory Methods
- Use **static factory methods** for object identification and creation
- Pattern: `static bool CanIdentify(IO &io)` for checking object capabilities
- Keep identification logic separate from instantiation

Example:
```cpp
class ClevoDevice : public DeviceInterface
{
public:
  static bool CanIdentify( IO &io )
  {
    int result;
    int ret = io.ioctlCall( R_HWCHECK_CL, result );
    return ret and result == 1;
  }

  virtual bool Identify( bool &identified ) { /* instance method */ }
};

// Usage
if ( ClevoDevice::CanIdentify( io ) )
{
  return std::make_unique< ClevoDevice >( io );
}
```

## Classes and Methods
- Place class declaration opening brace on next line
- Place method implementation opening brace on next line
- Constructor initializer lists on separate line before opening brace
- **Single-statement methods** can be formatted inline: `{ statement; return value; }`
- **Multi-statement methods** use Allman style with opening brace on next line

Example:
```cpp
class MyClass
{
public:
  explicit MyClass( const std::string &param )
    : m_member( param )
  {
    // constructor body
  }

  // Single-statement method (inline)
  bool IsValid() { return m_member.empty() == false; }

  // Multi-statement method (Allman style)
  bool Initialize()
  {
    if ( not isAvailable() )
      return false;

    m_initialized = true;
    return true;
  }

private:
  std::string m_member;
  bool m_initialized = false;
};
```

## Summary
These guidelines ensure consistent, readable, and maintainable code throughout the TUXEDO Control Center project. All code contributions should follow these standards.

## Quick Reference Checklist
- ✓ 2-space indentation (never tabs)
- ✓ Allman-style braces (opening brace on next line)
- ✓ Single statements must not use braces
- ✓ Spaces around parentheses: `if ( condition )`, `func( param )`
- ✓ Spaces inside template brackets: `std::vector< T >`
- ✓ Spaces inside array access: `array[ index ]`
- ✓ Logical operators: `and`, `or`, `not` (not `&&`, `||`, `!`)
- ✓ C++17 if-with-initializer: `if ( auto x = foo(); condition )`
- ✓ Use `constexpr` for compile-time evaluable functions
- ✓ String parameters must be `const` if not modified
- ✓ Member variables prefixed: `m_variableName`
- ✓ Classes and methods: PascalCase, camelCase respectively
- ✓ Smart pointers: `std::unique_ptr`, `std::make_unique`
- ✓ Static factory methods: `CanIdentify()` for object identification
- ✓ Null Object pattern for safe fallbacks
- ✓ Empty line after return statements (when more code follows)
- ✓ Blank line before control flow statements (if/while/for/switch)
- ✓ Single-statement methods can be inline
- ✓ Multi-statement methods use Allman style
- ✓ Avoid obvious comments; use lowercase except for type/class names
- ✓ Comments explain why, not what
