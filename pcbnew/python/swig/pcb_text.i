

%include pcb_text.h
%include pcb_textbox.h
%include font/text_attributes.h
%{
#include <pcb_text.h>
#include <pcb_textbox.h>
%}


%extend EDA_TEXT
{
    %pythoncode
    %{
    # KiCad 6 API compatibility
    def GetShownText(self):
        r"""GetShownText(EDA_TEXT self) -> wxString"""
        return _pcbnew.EDA_TEXT_GetShownText(self, True, 0)
    %}
}


%extend PCB_TEXT
{
    %pythoncode
    %{
    # KiCad 6 API compatibility
    def GetShownText(self):
        r"""GetShownText(PCB_TEXT self) -> wxString"""
        return _pcbnew.PCB_TEXT_GetShownText(self, True, 0)
    %}
}


%extend PCB_TEXTBOX
{
    %pythoncode
    %{
    # KiCad 6 API compatibility
    def GetShownText(self):
        r"""GetShownText(PCB_TEXTBOX self) -> wxString"""
        return _pcbnew.PCB_TEXTBOX_GetShownText(self, True, 0)
    %}
}
