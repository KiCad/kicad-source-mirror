#ifndef GAL_DISPLAY_OPTIONS_H__
#define GAL_DISPLAY_OPTIONS_H__

#include <observable.h>

class wxConfigBase;
class wxString;

namespace KIGFX {

    class GAL_DISPLAY_OPTIONS;

    enum class OPENGL_ANTIALIASING_MODE : long {
        NONE = 0,
        SUBSAMPLE_HIGH = 1,
        SUBSAMPLE_ULTRA = 2,
        SUPERSAMPLING_X2 = 3,
        SUPERSAMPLING_X4 = 4
    };

    class GAL_DISPLAY_OPTIONS_OBSERVER
    {
    public:
        virtual void OnGalDisplayOptionsChanged(const GAL_DISPLAY_OPTIONS&) = 0;
    };

    class GAL_DISPLAY_OPTIONS
        : public UTIL::OBSERVABLE< GAL_DISPLAY_OPTIONS_OBSERVER >
    {
    public:
        GAL_DISPLAY_OPTIONS();

        OPENGL_ANTIALIASING_MODE gl_antialiasing_mode;

        void ReadConfig ( wxConfigBase* aCfg, wxString aBaseName );
        void WriteConfig( wxConfigBase* aCfg, wxString aBaseName );

        void NotifyChanged();
    };

}

#endif

