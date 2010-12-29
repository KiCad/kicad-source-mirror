
#ifndef UTF8_H_
#define UTF8_H_

#include <string>
#include <deque>

/**
 * @ingroup string_types
 * @{
 */


/**
 * Type STRING
 * holds a sequence of 8 bit bytes that represent a sequence
 * of variable multi-byte international characters, with unspecified encoding.
 */
typedef std::string STRING;


/**
 * Type STRING_UTF8
 * holds a UTF8 encoded sequence of 8 bit bytes that represent a sequence
 * of variable multi-byte international characters.  UTF8 is the chosen encoding
 * for all Kicad data files so that they can be transported from one nation to another
 * without ambiguity. Data files are those where Kicad controls the content.
 * This is not the same thing as filenames, which are not file content.
 * Filenames may be encoded on disk using an encoding chosen by the host operating
 * system.  Nonetheless, Kicad data file _content_ is always UTF8 encoded, regardless
 * of host operating system.
 * STRING_UTF is UTF8 encoded, by definition.
 */
typedef STRING      STRING_UTF8;


/**
 * Type STRINGS_UTF8
 * is an "array like" list of STRING_UTF8s
 */
typedef std::deque<STRING>  STRINGS_UTF8;

typedef std::deque<STRING>  STRINGS;


/** @} string_types */


// @todo this does not belong here
#ifndef D
#ifdef DEBUG
#define D(x)        x
#else
#define D(x)        // nothing
#endif
#endif

#endif  // UTF8_H_
