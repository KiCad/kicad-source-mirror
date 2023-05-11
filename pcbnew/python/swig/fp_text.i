
%include fp_text.h
%include fp_textbox.h
%{
#include <fp_text.h>
#include <fp_textbox.h>
%}

%extend FP_TEXT
{
    %pythoncode
    %{
    # KiCad 6 API compatibility
    def GetShownText(self):
        r"""GetShownText(FP_TEXT self) -> wxString"""
        return _pcbnew.FP_TEXT_GetShownText(self, True, 0)
    %}
}


%extend FP_TEXTBOX
{
    %pythoncode
    %{
    # KiCad 6 API compatibility
    def GetShownText(self):
        r"""GetShownText(FP_TEXTBOX self) -> wxString"""
        return _pcbnew.FP_TEXTBOX_GetShownText(self, True, 0)
    %}
}
