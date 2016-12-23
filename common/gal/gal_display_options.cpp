#include <gal/gal_display_options.h>
#include <wx/config.h>


namespace KIGFX {

    static const wxString GalGLAntialiasingKeyword( wxT( "OpenGLAntialiasingMode" ) );

    GAL_DISPLAY_OPTIONS::GAL_DISPLAY_OPTIONS()
        : gl_antialiasing_mode( OPENGL_ANTIALIASING_MODE::NONE )
    {}

    void GAL_DISPLAY_OPTIONS::ReadConfig( wxConfigBase* aCfg, wxString aBaseName )
    {
        aCfg->Read( aBaseName + GalGLAntialiasingKeyword,
            reinterpret_cast<long*>(&gl_antialiasing_mode),
            (long)KIGFX::OPENGL_ANTIALIASING_MODE::NONE );

        NotifyChanged();
    }

    void GAL_DISPLAY_OPTIONS::WriteConfig( wxConfigBase* aCfg, wxString aBaseName )
    {
        aCfg->Write( aBaseName + GalGLAntialiasingKeyword,
                     static_cast<long>(gl_antialiasing_mode) );
    }

    void GAL_DISPLAY_OPTIONS::NotifyChanged()
    {
        Notify( &GAL_DISPLAY_OPTIONS_OBSERVER::OnGalDisplayOptionsChanged, *this );
    }

}
