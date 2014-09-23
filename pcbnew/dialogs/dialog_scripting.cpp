/**
 * @file dialog_scripting.cpp
 */


#include <Python.h>
#include <fctsys.h>
#include <pcbnew.h>
#include <wxPcbStruct.h>
#include <dialog_scripting.h>


DIALOG_SCRIPTING::DIALOG_SCRIPTING( wxWindow* parent )
    : DIALOG_SCRIPTING_BASE( parent )
{
    SetFocus();

}



void DIALOG_SCRIPTING::OnRunButtonClick( wxCommandEvent& event )
{
	wxCharBuffer buffer = m_txScript->GetValue().ToUTF8();
        PyRun_SimpleString(buffer.data());
}


