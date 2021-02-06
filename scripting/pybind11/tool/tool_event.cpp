#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <tool/actions.h> // ACTIONS
#include <tool/actions.h> // EVENTS
#include <tool/tool_action.h> // TOOL_ACTION
#include <tool/tool_event.h> // TOOL_ACTIONS
#include <tool/tool_event.h> // TOOL_ACTION_SCOPE
#include <tool/tool_event.h> // TOOL_EVENT
#include <tool/tool_event.h> // TOOL_EVENT_CATEGORY
#include <tool/tool_event.h> // TOOL_EVENT_LIST
#include <tools/ee_actions.h> // EE_ACTIONS
#include <tools/ee_actions.h> // mapCoords

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_tool_tool_event(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // TOOL_EVENT_LIST file:tool/tool_event.h line:575
		pybind11::class_<TOOL_EVENT_LIST, std::shared_ptr<TOOL_EVENT_LIST>> cl(M(""), "TOOL_EVENT_LIST", "A list of TOOL_EVENTs, with overloaded || operators allowing for concatenating TOOL_EVENTs\n with little code.");
		cl.def( pybind11::init( [](){ return new TOOL_EVENT_LIST(); } ) );
		cl.def( pybind11::init<const class TOOL_EVENT &>(), pybind11::arg("aSingleEvent") );

		cl.def( pybind11::init( [](TOOL_EVENT_LIST const &o){ return new TOOL_EVENT_LIST(o); } ) );
		cl.def("Matches", (int (TOOL_EVENT_LIST::*)(const class TOOL_EVENT &) const) &TOOL_EVENT_LIST::Matches, "C++: TOOL_EVENT_LIST::Matches(const class TOOL_EVENT &) const --> int", pybind11::arg("aEvent"));
		cl.def("Add", (void (TOOL_EVENT_LIST::*)(const class TOOL_EVENT &)) &TOOL_EVENT_LIST::Add, "Add a tool event to the list.\n\n \n is the tool event to be added.\n\nC++: TOOL_EVENT_LIST::Add(const class TOOL_EVENT &) --> void", pybind11::arg("aEvent"));
		cl.def("begin", (int (TOOL_EVENT_LIST::*)()) &TOOL_EVENT_LIST::begin, "C++: TOOL_EVENT_LIST::begin() --> int");
		cl.def("end", (int (TOOL_EVENT_LIST::*)()) &TOOL_EVENT_LIST::end, "C++: TOOL_EVENT_LIST::end() --> int");
		cl.def("cbegin", (int (TOOL_EVENT_LIST::*)() const) &TOOL_EVENT_LIST::cbegin, "C++: TOOL_EVENT_LIST::cbegin() const --> int");
		cl.def("cend", (int (TOOL_EVENT_LIST::*)() const) &TOOL_EVENT_LIST::cend, "C++: TOOL_EVENT_LIST::cend() const --> int");
		cl.def("size", (int (TOOL_EVENT_LIST::*)() const) &TOOL_EVENT_LIST::size, "C++: TOOL_EVENT_LIST::size() const --> int");
		cl.def("clear", (void (TOOL_EVENT_LIST::*)()) &TOOL_EVENT_LIST::clear, "C++: TOOL_EVENT_LIST::clear() --> void");
		cl.def("assign", (class TOOL_EVENT_LIST & (TOOL_EVENT_LIST::*)(const class TOOL_EVENT_LIST &)) &TOOL_EVENT_LIST::operator=, "C++: TOOL_EVENT_LIST::operator=(const class TOOL_EVENT_LIST &) --> class TOOL_EVENT_LIST &", pybind11::return_value_policy::automatic, pybind11::arg("aEventList"));
		cl.def("assign", (class TOOL_EVENT_LIST & (TOOL_EVENT_LIST::*)(const class TOOL_EVENT &)) &TOOL_EVENT_LIST::operator=, "C++: TOOL_EVENT_LIST::operator=(const class TOOL_EVENT &) --> class TOOL_EVENT_LIST &", pybind11::return_value_policy::automatic, pybind11::arg("aEvent"));
	}
	{ // TOOL_ACTION file:tool/tool_action.h line:49
		pybind11::class_<TOOL_ACTION, std::shared_ptr<TOOL_ACTION>> cl(M(""), "TOOL_ACTION", "Represent a single user action.\n\n For instance:\n - changing layer to top by pressing PgUp\n - running the DRC from the menu\n and so on, and so forth....\n\n Action class groups all necessary properties of an action, including explanation,\n icons, hotkeys, menu items, etc.");
		cl.def("__eq__", (bool (TOOL_ACTION::*)(const class TOOL_ACTION &) const) &TOOL_ACTION::operator==, "C++: TOOL_ACTION::operator==(const class TOOL_ACTION &) const --> bool", pybind11::arg("aRhs"));
		cl.def("__ne__", (bool (TOOL_ACTION::*)(const class TOOL_ACTION &) const) &TOOL_ACTION::operator!=, "C++: TOOL_ACTION::operator!=(const class TOOL_ACTION &) const --> bool", pybind11::arg("aRhs"));
		cl.def("GetDefaultHotKey", (int (TOOL_ACTION::*)() const) &TOOL_ACTION::GetDefaultHotKey, "Return the default hotkey (if any) for the action.\n\nC++: TOOL_ACTION::GetDefaultHotKey() const --> int");
		cl.def("GetHotKey", (int (TOOL_ACTION::*)() const) &TOOL_ACTION::GetHotKey, "Return the hotkey keycode which initiates the action.\n\nC++: TOOL_ACTION::GetHotKey() const --> int");
		cl.def("SetHotKey", (void (TOOL_ACTION::*)(int)) &TOOL_ACTION::SetHotKey, "C++: TOOL_ACTION::SetHotKey(int) --> void", pybind11::arg("aKeycode"));
		cl.def("GetId", (int (TOOL_ACTION::*)() const) &TOOL_ACTION::GetId, "Return the unique id of the TOOL_ACTION object.\n\n It is valid only after registering the TOOL_ACTION by #ACTION_MANAGER.\n\n \n The unique identification number. If the number is negative, then it is not valid.\n\nC++: TOOL_ACTION::GetId() const --> int");
		cl.def("GetUIId", (int (TOOL_ACTION::*)() const) &TOOL_ACTION::GetUIId, "C++: TOOL_ACTION::GetUIId() const --> int");
		cl.def_static("GetBaseUIId", (int (*)()) &TOOL_ACTION::GetBaseUIId, "C++: TOOL_ACTION::GetBaseUIId() --> int");
		cl.def("MakeEvent", (class TOOL_EVENT (TOOL_ACTION::*)() const) &TOOL_ACTION::MakeEvent, "Return the event associated with the action (i.e. the event that will be sent after\n activating the action).\n\nC++: TOOL_ACTION::MakeEvent() const --> class TOOL_EVENT");
		cl.def("GetLabel", (class wxString (TOOL_ACTION::*)() const) &TOOL_ACTION::GetLabel, "C++: TOOL_ACTION::GetLabel() const --> class wxString");
		cl.def("GetMenuItem", (class wxString (TOOL_ACTION::*)() const) &TOOL_ACTION::GetMenuItem, "C++: TOOL_ACTION::GetMenuItem() const --> class wxString");
		cl.def("GetDescription", [](TOOL_ACTION const &o) -> wxString { return o.GetDescription(); }, "");
		cl.def("GetDescription", (class wxString (TOOL_ACTION::*)(bool) const) &TOOL_ACTION::GetDescription, "C++: TOOL_ACTION::GetDescription(bool) const --> class wxString", pybind11::arg("aIncludeHotkey"));
		cl.def("GetScope", (enum TOOL_ACTION_SCOPE (TOOL_ACTION::*)() const) &TOOL_ACTION::GetScope, "C++: TOOL_ACTION::GetScope() const --> enum TOOL_ACTION_SCOPE");
		cl.def("GetParam", (void * (TOOL_ACTION::*)() const) &TOOL_ACTION::GetParam, "C++: TOOL_ACTION::GetParam() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("IsActivation", (bool (TOOL_ACTION::*)() const) &TOOL_ACTION::IsActivation, "Return true if the action is intended to activate a tool.\n\nC++: TOOL_ACTION::IsActivation() const --> bool");
		cl.def("IsNotification", (bool (TOOL_ACTION::*)() const) &TOOL_ACTION::IsNotification, "Return true if the action is a notification.\n\nC++: TOOL_ACTION::IsNotification() const --> bool");
	}
	{ // ACTIONS file:tool/actions.h line:43
		pybind11::class_<ACTIONS, std::shared_ptr<ACTIONS>> cl(M(""), "ACTIONS", "Gather all the actions that are shared by tools.\n\n The instance of a subclass of ACTIONS is created inside of #ACTION_MANAGER object that\n registers the actions.");
		cl.def( pybind11::init( [](){ return new ACTIONS(); } ) );
		cl.def( pybind11::init( [](ACTIONS const &o){ return new ACTIONS(o); } ) );

		pybind11::enum_<ACTIONS::CURSOR_EVENT_TYPE>(cl, "CURSOR_EVENT_TYPE", pybind11::arithmetic(), "")
			.value("CURSOR_NONE", ACTIONS::CURSOR_NONE)
			.value("CURSOR_UP", ACTIONS::CURSOR_UP)
			.value("CURSOR_DOWN", ACTIONS::CURSOR_DOWN)
			.value("CURSOR_LEFT", ACTIONS::CURSOR_LEFT)
			.value("CURSOR_RIGHT", ACTIONS::CURSOR_RIGHT)
			.value("CURSOR_CLICK", ACTIONS::CURSOR_CLICK)
			.value("CURSOR_DBL_CLICK", ACTIONS::CURSOR_DBL_CLICK)
			.value("CURSOR_RIGHT_CLICK", ACTIONS::CURSOR_RIGHT_CLICK)
			.value("CURSOR_FAST_MOVE", ACTIONS::CURSOR_FAST_MOVE)
			.export_values();


		pybind11::enum_<ACTIONS::REMOVE_FLAGS>(cl, "REMOVE_FLAGS", "")
			.value("NORMAL", ACTIONS::REMOVE_FLAGS::NORMAL)
			.value("ALT", ACTIONS::REMOVE_FLAGS::ALT)
			.value("CUT", ACTIONS::REMOVE_FLAGS::CUT);

		cl.def("TranslateLegacyId", (int (ACTIONS::*)(int)) &ACTIONS::TranslateLegacyId, "Translate legacy tool ids to the corresponding TOOL_ACTION name.\n\n \n is legacy tool id to be translated.\n \n\n std::string is name of the corresponding TOOL_ACTION. It may be empty, if there is\n         no corresponding TOOL_ACTION.\n\nC++: ACTIONS::TranslateLegacyId(int) --> int", pybind11::arg("aId"));
		cl.def("assign", (class ACTIONS & (ACTIONS::*)(const class ACTIONS &)) &ACTIONS::operator=, "C++: ACTIONS::operator=(const class ACTIONS &) --> class ACTIONS &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // EVENTS file:tool/actions.h line:206
		pybind11::class_<EVENTS, std::shared_ptr<EVENTS>> cl(M(""), "EVENTS", "Gather all the events that are shared by tools.");
		cl.def( pybind11::init( [](){ return new EVENTS(); } ) );
	}
	{ // EE_ACTIONS file:tools/ee_actions.h line:39
		pybind11::class_<EE_ACTIONS, std::shared_ptr<EE_ACTIONS>, ACTIONS> cl(M(""), "EE_ACTIONS", "Gather all the actions that are shared by tools. The instance of SCH_ACTIONS is created\n inside of ACTION_MANAGER object that registers the actions.");
		cl.def( pybind11::init( [](){ return new EE_ACTIONS(); } ) );
		cl.def( pybind11::init( [](EE_ACTIONS const &o){ return new EE_ACTIONS(o); } ) );
		cl.def("TranslateLegacyId", (int (EE_ACTIONS::*)(int)) &EE_ACTIONS::TranslateLegacyId, "C++: EE_ACTIONS::TranslateLegacyId(int) --> int", pybind11::arg("aId"));
		cl.def("assign", (class EE_ACTIONS & (EE_ACTIONS::*)(const class EE_ACTIONS &)) &EE_ACTIONS::operator=, "C++: EE_ACTIONS::operator=(const class EE_ACTIONS &) --> class EE_ACTIONS &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// mapCoords(const class wxPoint &) file:tools/ee_actions.h line:229
	M("").def("mapCoords", (int (*)(const class wxPoint &)) &mapCoords, "C++: mapCoords(const class wxPoint &) --> int", pybind11::arg("aCoord"));

	// mapCoords(const int &) file:tools/ee_actions.h line:234
	M("").def("mapCoords", (class wxPoint (*)(const int &)) &mapCoords, "C++: mapCoords(const int &) --> class wxPoint", pybind11::arg("aCoord"));

	// mapCoords(const int, const int) file:tools/ee_actions.h line:239
	M("").def("mapCoords", (class wxPoint (*)(const int, const int)) &mapCoords, "C++: mapCoords(const int, const int) --> class wxPoint", pybind11::arg("x"), pybind11::arg("y"));

}
