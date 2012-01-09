#ifndef HASHTABLES_H_
#define HASHTABLES_H_

// Declare some hashtables using a MACRO techique from here:
// http://docs.wxwidgets.org/trunk/classwx_hash_map.html
// This simplifies finding the correct hashtable header file.
// Ideally, std::unordered_map is what we are trying to use here,
// but its header file has been a moving target for some time.
// Let wx figure it out.
#include <wx/hashmap.h>

/**
 * Class PROPERTIES
 * is an associative array consisting of a key and value tuple.
 */
#if 1
    // key:     const char*
    // value:   wxString
    WX_DECLARE_HASH_MAP( char*, wxString, wxStringHash, wxStringEqual, PROPERTIES );
#else
    // key:     wxString
    // value:   wxString
    WX_DECLARE_STRING_HASH_MAP( wxString, PROPERTIES );
#endif


/**
 * Class KEYWORD_MAP
 * is a hashtable consisting of a key and a value tuple.
 * Key is a C string and value is an integer.
 */
//WX_DECLARE_HASH_MAP( char*, int, wxStringHash, wxStringEqual, KEYWORD_MAP );


#endif // HASHTABLES_H_
