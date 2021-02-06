#include <common.h> // ProcessExecute
#include <iterator> // __gnu_cxx::__normal_iterator
#include <macros.h> // FROM_UTF8
#include <memory> // std::allocator
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/process.h> // wxProcess

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_macros(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// FROM_UTF8(const char *) file:macros.h line:110
	M("").def("FROM_UTF8", (class wxString (*)(const char *)) &FROM_UTF8, "Convert a UTF8 encoded C string to a wxString for all wxWidgets build modes.\n\nC++: FROM_UTF8(const char *) --> class wxString", pybind11::arg("cstring"));

	// ProcessExecute(const class wxString &, int, class wxProcess *) file:common.h line:65
	M("").def("ProcessExecute", [](const class wxString & a0) -> int { return ProcessExecute(a0); }, "", pybind11::arg("aCommandLine"));
	M("").def("ProcessExecute", [](const class wxString & a0, int const & a1) -> int { return ProcessExecute(a0, a1); }, "", pybind11::arg("aCommandLine"), pybind11::arg("aFlags"));
	M("").def("ProcessExecute", (int (*)(const class wxString &, int, class wxProcess *)) &ProcessExecute, "Run a command in a child process.\n\n \n The process and any arguments to it all in a single string.\n \n\n The same args as allowed for wxExecute()\n \n\n wxProcess implementing OnTerminate to be run when the\n                   child process finishes\n \n\n pid of process, 0 in case of error (like return values of wxExecute()).\n\nC++: ProcessExecute(const class wxString &, int, class wxProcess *) --> int", pybind11::arg("aCommandLine"), pybind11::arg("aFlags"), pybind11::arg("callback"));

}
