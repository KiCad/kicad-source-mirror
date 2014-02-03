
#include <wx/app.h>
#include <wx/frame.h>
#include <kiway.h>


// test static initialization, and translation in the DSO:
wxString GlobalTitle = _( "Some Translatable Window Title Text" );

/// Implement a KIFACE, and create a static instance of one.
static struct SCH_FACE : public KIFACE
{
    wxWindow* CreateWindow( int aClassId, KIWAY* aKIWAY, int aCtlBits = 0 )
    {
        switch( aClassId )
        {
        // for now, I have no class:
        default:
            return new wxFrame( NULL, 0, GlobalTitle, wxPoint( 0, 0 ), wxSize( 600, 400 ) );
        }
    }

    /**
     * Function IfaceOrAddress
     * return a pointer to the requested object.  The safest way to use this
     * is to retrieve a pointer to a static instance of an interface, similar to
     * how the KIFACE interface is exported.  But if you know what you are doing
     * use it to retrieve anything you want.
     *
     * @param aDataId identifies which object you want the address of.
     *
     * @return void* - and must be cast into the known type.
     */
    void* IfaceOrAddress( int aDataId )
    {
        return NULL;
    }

} kiface_impl;


static wxApp* app;

extern "C" KIFACE* KIFACE_GETTER( int* aKIFACEversion, int aKIWAYversion, wxApp* aProcess );

MY_API( KIFACE* ) KIFACE_GETTER( int* aKIFACEversion, int aKIWAYversion, wxApp* aProcess )
{
    // record the app's address.
    app = aProcess;

    // return a pointer to the KIFACE implementation.
    return &kiface_impl;
}


wxApp& wxGetApp()
{
    return *app;
}

