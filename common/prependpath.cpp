
#include <macros.h>
#include <fctsys.h>
#include <wx/filename.h>



#if !wxCHECK_VERSION( 2, 9, 0 )

// implement missing wx2.8 function until >= wx3.0 pervades.
static wxString wxJoin(const wxArrayString& arr, const wxChar sep,
                const wxChar escape = '\\')
{
    size_t count = arr.size();
    if ( count == 0 )
        return wxEmptyString;

    wxString str;

    // pre-allocate memory using the estimation of the average length of the
    // strings in the given array: this is very imprecise, of course, but
    // better than nothing
    str.reserve(count*(arr[0].length() + arr[count-1].length()) / 2);

    if ( escape == wxT('\0') )
    {
        // escaping is disabled:
        for ( size_t i = 0; i < count; i++ )
        {
            if ( i )
                str += sep;
            str += arr[i];
        }
    }
    else // use escape character
    {
        for ( size_t n = 0; n < count; n++ )
        {
            if ( n )
                str += sep;

            for ( wxString::const_iterator i = arr[n].begin(),
                                         end = arr[n].end();
                  i != end;
                  ++i )
            {
                const wxChar ch = *i;
                if ( ch == sep )
                    str += escape;      // escape this separator
                str += ch;
            }
        }
    }

    str.Shrink(); // release extra memory if we allocated too much
    return str;
}
#endif


/// Put aPriorityPath in front of all paths in the value of aEnvVar.
const wxString PrePendPath( const wxString& aEnvVar, const wxString& aPriorityPath )
{
    wxPathList  paths;

    paths.AddEnvList( aEnvVar );
    paths.Insert( aPriorityPath, 0 );

    return wxJoin( paths, wxPATH_SEP[0] );
}
