#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_49(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxTimeSpan file: line:1178
		pybind11::class_<wxTimeSpan, std::shared_ptr<wxTimeSpan>> cl(M(""), "wxTimeSpan", "");
		cl.def( pybind11::init( [](){ return new wxTimeSpan(); } ) );
		cl.def( pybind11::init( [](long const & a0){ return new wxTimeSpan(a0); } ), "doc" , pybind11::arg("hours"));
		cl.def( pybind11::init( [](long const & a0, long const & a1){ return new wxTimeSpan(a0, a1); } ), "doc" , pybind11::arg("hours"), pybind11::arg("minutes"));
		cl.def( pybind11::init( [](long const & a0, long const & a1, class wxLongLongNative const & a2){ return new wxTimeSpan(a0, a1, a2); } ), "doc" , pybind11::arg("hours"), pybind11::arg("minutes"), pybind11::arg("seconds"));
		cl.def( pybind11::init<long, long, class wxLongLongNative, class wxLongLongNative>(), pybind11::arg("hours"), pybind11::arg("minutes"), pybind11::arg("seconds"), pybind11::arg("milliseconds") );

		cl.def( pybind11::init<const class wxLongLongNative &>(), pybind11::arg("diff") );

		cl.def( pybind11::init( [](wxTimeSpan const &o){ return new wxTimeSpan(o); } ) );
		cl.def_static("Milliseconds", (class wxTimeSpan (*)(class wxLongLongNative)) &wxTimeSpan::Milliseconds, "C++: wxTimeSpan::Milliseconds(class wxLongLongNative) --> class wxTimeSpan", pybind11::arg("ms"));
		cl.def_static("Millisecond", (class wxTimeSpan (*)()) &wxTimeSpan::Millisecond, "C++: wxTimeSpan::Millisecond() --> class wxTimeSpan");
		cl.def_static("Seconds", (class wxTimeSpan (*)(class wxLongLongNative)) &wxTimeSpan::Seconds, "C++: wxTimeSpan::Seconds(class wxLongLongNative) --> class wxTimeSpan", pybind11::arg("sec"));
		cl.def_static("Second", (class wxTimeSpan (*)()) &wxTimeSpan::Second, "C++: wxTimeSpan::Second() --> class wxTimeSpan");
		cl.def_static("Minutes", (class wxTimeSpan (*)(long)) &wxTimeSpan::Minutes, "C++: wxTimeSpan::Minutes(long) --> class wxTimeSpan", pybind11::arg("min"));
		cl.def_static("Minute", (class wxTimeSpan (*)()) &wxTimeSpan::Minute, "C++: wxTimeSpan::Minute() --> class wxTimeSpan");
		cl.def_static("Hours", (class wxTimeSpan (*)(long)) &wxTimeSpan::Hours, "C++: wxTimeSpan::Hours(long) --> class wxTimeSpan", pybind11::arg("hours"));
		cl.def_static("Hour", (class wxTimeSpan (*)()) &wxTimeSpan::Hour, "C++: wxTimeSpan::Hour() --> class wxTimeSpan");
		cl.def_static("Days", (class wxTimeSpan (*)(long)) &wxTimeSpan::Days, "C++: wxTimeSpan::Days(long) --> class wxTimeSpan", pybind11::arg("days"));
		cl.def_static("Day", (class wxTimeSpan (*)()) &wxTimeSpan::Day, "C++: wxTimeSpan::Day() --> class wxTimeSpan");
		cl.def_static("Weeks", (class wxTimeSpan (*)(long)) &wxTimeSpan::Weeks, "C++: wxTimeSpan::Weeks(long) --> class wxTimeSpan", pybind11::arg("days"));
		cl.def_static("Week", (class wxTimeSpan (*)()) &wxTimeSpan::Week, "C++: wxTimeSpan::Week() --> class wxTimeSpan");
		cl.def("Add", (class wxTimeSpan & (wxTimeSpan::*)(const class wxTimeSpan &)) &wxTimeSpan::Add, "C++: wxTimeSpan::Add(const class wxTimeSpan &) --> class wxTimeSpan &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__iadd__", (class wxTimeSpan & (wxTimeSpan::*)(const class wxTimeSpan &)) &wxTimeSpan::operator+=, "C++: wxTimeSpan::operator+=(const class wxTimeSpan &) --> class wxTimeSpan &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__add__", (class wxTimeSpan (wxTimeSpan::*)(const class wxTimeSpan &) const) &wxTimeSpan::operator+, "C++: wxTimeSpan::operator+(const class wxTimeSpan &) const --> class wxTimeSpan", pybind11::arg("ts"));
		cl.def("Subtract", (class wxTimeSpan & (wxTimeSpan::*)(const class wxTimeSpan &)) &wxTimeSpan::Subtract, "C++: wxTimeSpan::Subtract(const class wxTimeSpan &) --> class wxTimeSpan &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__isub__", (class wxTimeSpan & (wxTimeSpan::*)(const class wxTimeSpan &)) &wxTimeSpan::operator-=, "C++: wxTimeSpan::operator-=(const class wxTimeSpan &) --> class wxTimeSpan &", pybind11::return_value_policy::automatic, pybind11::arg("diff"));
		cl.def("__sub__", (class wxTimeSpan (wxTimeSpan::*)(const class wxTimeSpan &)) &wxTimeSpan::operator-, "C++: wxTimeSpan::operator-(const class wxTimeSpan &) --> class wxTimeSpan", pybind11::arg("ts"));
		cl.def("Multiply", (class wxTimeSpan & (wxTimeSpan::*)(int)) &wxTimeSpan::Multiply, "C++: wxTimeSpan::Multiply(int) --> class wxTimeSpan &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__imul__", (class wxTimeSpan & (wxTimeSpan::*)(int)) &wxTimeSpan::operator*=, "C++: wxTimeSpan::operator*=(int) --> class wxTimeSpan &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("__mul__", (class wxTimeSpan (wxTimeSpan::*)(int) const) &wxTimeSpan::operator*, "C++: wxTimeSpan::operator*(int) const --> class wxTimeSpan", pybind11::arg("n"));
		cl.def("Negate", (class wxTimeSpan (wxTimeSpan::*)() const) &wxTimeSpan::Negate, "C++: wxTimeSpan::Negate() const --> class wxTimeSpan");
		cl.def("Neg", (class wxTimeSpan & (wxTimeSpan::*)()) &wxTimeSpan::Neg, "C++: wxTimeSpan::Neg() --> class wxTimeSpan &", pybind11::return_value_policy::automatic);
		cl.def("__sub__", (class wxTimeSpan & (wxTimeSpan::*)()) &wxTimeSpan::operator-, "C++: wxTimeSpan::operator-() --> class wxTimeSpan &", pybind11::return_value_policy::automatic);
		cl.def("Abs", (class wxTimeSpan (wxTimeSpan::*)() const) &wxTimeSpan::Abs, "C++: wxTimeSpan::Abs() const --> class wxTimeSpan");
		cl.def("IsNull", (bool (wxTimeSpan::*)() const) &wxTimeSpan::IsNull, "C++: wxTimeSpan::IsNull() const --> bool");
		cl.def("IsPositive", (bool (wxTimeSpan::*)() const) &wxTimeSpan::IsPositive, "C++: wxTimeSpan::IsPositive() const --> bool");
		cl.def("IsNegative", (bool (wxTimeSpan::*)() const) &wxTimeSpan::IsNegative, "C++: wxTimeSpan::IsNegative() const --> bool");
		cl.def("IsEqualTo", (bool (wxTimeSpan::*)(const class wxTimeSpan &) const) &wxTimeSpan::IsEqualTo, "C++: wxTimeSpan::IsEqualTo(const class wxTimeSpan &) const --> bool", pybind11::arg("ts"));
		cl.def("IsLongerThan", (bool (wxTimeSpan::*)(const class wxTimeSpan &) const) &wxTimeSpan::IsLongerThan, "C++: wxTimeSpan::IsLongerThan(const class wxTimeSpan &) const --> bool", pybind11::arg("ts"));
		cl.def("IsShorterThan", (bool (wxTimeSpan::*)(const class wxTimeSpan &) const) &wxTimeSpan::IsShorterThan, "C++: wxTimeSpan::IsShorterThan(const class wxTimeSpan &) const --> bool", pybind11::arg("t"));
		cl.def("__eq__", (bool (wxTimeSpan::*)(const class wxTimeSpan &) const) &wxTimeSpan::operator==, "C++: wxTimeSpan::operator==(const class wxTimeSpan &) const --> bool", pybind11::arg("ts"));
		cl.def("__ne__", (bool (wxTimeSpan::*)(const class wxTimeSpan &) const) &wxTimeSpan::operator!=, "C++: wxTimeSpan::operator!=(const class wxTimeSpan &) const --> bool", pybind11::arg("ts"));
		cl.def("GetWeeks", (int (wxTimeSpan::*)() const) &wxTimeSpan::GetWeeks, "C++: wxTimeSpan::GetWeeks() const --> int");
		cl.def("GetDays", (int (wxTimeSpan::*)() const) &wxTimeSpan::GetDays, "C++: wxTimeSpan::GetDays() const --> int");
		cl.def("GetHours", (int (wxTimeSpan::*)() const) &wxTimeSpan::GetHours, "C++: wxTimeSpan::GetHours() const --> int");
		cl.def("GetMinutes", (int (wxTimeSpan::*)() const) &wxTimeSpan::GetMinutes, "C++: wxTimeSpan::GetMinutes() const --> int");
		cl.def("GetSeconds", (class wxLongLongNative (wxTimeSpan::*)() const) &wxTimeSpan::GetSeconds, "C++: wxTimeSpan::GetSeconds() const --> class wxLongLongNative");
		cl.def("GetMilliseconds", (class wxLongLongNative (wxTimeSpan::*)() const) &wxTimeSpan::GetMilliseconds, "C++: wxTimeSpan::GetMilliseconds() const --> class wxLongLongNative");
		cl.def("Format", [](wxTimeSpan const &o) -> wxString { return o.Format(); }, "");
		cl.def("Format", (class wxString (wxTimeSpan::*)(const class wxString &) const) &wxTimeSpan::Format, "C++: wxTimeSpan::Format(const class wxString &) const --> class wxString", pybind11::arg("format"));
		cl.def("GetValue", (class wxLongLongNative (wxTimeSpan::*)() const) &wxTimeSpan::GetValue, "C++: wxTimeSpan::GetValue() const --> class wxLongLongNative");
	}
	{ // wxDateSpan file: line:1404
		pybind11::class_<wxDateSpan, std::shared_ptr<wxDateSpan>> cl(M(""), "wxDateSpan", "");
		cl.def( pybind11::init( [](){ return new wxDateSpan(); } ), "doc" );
		cl.def( pybind11::init( [](int const & a0){ return new wxDateSpan(a0); } ), "doc" , pybind11::arg("years"));
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxDateSpan(a0, a1); } ), "doc" , pybind11::arg("years"), pybind11::arg("months"));
		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2){ return new wxDateSpan(a0, a1, a2); } ), "doc" , pybind11::arg("years"), pybind11::arg("months"), pybind11::arg("weeks"));
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("years"), pybind11::arg("months"), pybind11::arg("weeks"), pybind11::arg("days") );

		cl.def( pybind11::init( [](wxDateSpan const &o){ return new wxDateSpan(o); } ) );
		cl.def_static("Days", (class wxDateSpan (*)(int)) &wxDateSpan::Days, "C++: wxDateSpan::Days(int) --> class wxDateSpan", pybind11::arg("days"));
		cl.def_static("Day", (class wxDateSpan (*)()) &wxDateSpan::Day, "C++: wxDateSpan::Day() --> class wxDateSpan");
		cl.def_static("Weeks", (class wxDateSpan (*)(int)) &wxDateSpan::Weeks, "C++: wxDateSpan::Weeks(int) --> class wxDateSpan", pybind11::arg("weeks"));
		cl.def_static("Week", (class wxDateSpan (*)()) &wxDateSpan::Week, "C++: wxDateSpan::Week() --> class wxDateSpan");
		cl.def_static("Months", (class wxDateSpan (*)(int)) &wxDateSpan::Months, "C++: wxDateSpan::Months(int) --> class wxDateSpan", pybind11::arg("mon"));
		cl.def_static("Month", (class wxDateSpan (*)()) &wxDateSpan::Month, "C++: wxDateSpan::Month() --> class wxDateSpan");
		cl.def_static("Years", (class wxDateSpan (*)(int)) &wxDateSpan::Years, "C++: wxDateSpan::Years(int) --> class wxDateSpan", pybind11::arg("years"));
		cl.def_static("Year", (class wxDateSpan (*)()) &wxDateSpan::Year, "C++: wxDateSpan::Year() --> class wxDateSpan");
		cl.def("SetYears", (class wxDateSpan & (wxDateSpan::*)(int)) &wxDateSpan::SetYears, "C++: wxDateSpan::SetYears(int) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("SetMonths", (class wxDateSpan & (wxDateSpan::*)(int)) &wxDateSpan::SetMonths, "C++: wxDateSpan::SetMonths(int) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("SetWeeks", (class wxDateSpan & (wxDateSpan::*)(int)) &wxDateSpan::SetWeeks, "C++: wxDateSpan::SetWeeks(int) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("SetDays", (class wxDateSpan & (wxDateSpan::*)(int)) &wxDateSpan::SetDays, "C++: wxDateSpan::SetDays(int) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("n"));
		cl.def("GetYears", (int (wxDateSpan::*)() const) &wxDateSpan::GetYears, "C++: wxDateSpan::GetYears() const --> int");
		cl.def("GetMonths", (int (wxDateSpan::*)() const) &wxDateSpan::GetMonths, "C++: wxDateSpan::GetMonths() const --> int");
		cl.def("GetTotalMonths", (int (wxDateSpan::*)() const) &wxDateSpan::GetTotalMonths, "C++: wxDateSpan::GetTotalMonths() const --> int");
		cl.def("GetWeeks", (int (wxDateSpan::*)() const) &wxDateSpan::GetWeeks, "C++: wxDateSpan::GetWeeks() const --> int");
		cl.def("GetDays", (int (wxDateSpan::*)() const) &wxDateSpan::GetDays, "C++: wxDateSpan::GetDays() const --> int");
		cl.def("GetTotalDays", (int (wxDateSpan::*)() const) &wxDateSpan::GetTotalDays, "C++: wxDateSpan::GetTotalDays() const --> int");
		cl.def("Add", (class wxDateSpan & (wxDateSpan::*)(const class wxDateSpan &)) &wxDateSpan::Add, "C++: wxDateSpan::Add(const class wxDateSpan &) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("__iadd__", (class wxDateSpan & (wxDateSpan::*)(const class wxDateSpan &)) &wxDateSpan::operator+=, "C++: wxDateSpan::operator+=(const class wxDateSpan &) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("__add__", (class wxDateSpan (wxDateSpan::*)(const class wxDateSpan &) const) &wxDateSpan::operator+, "C++: wxDateSpan::operator+(const class wxDateSpan &) const --> class wxDateSpan", pybind11::arg("ds"));
		cl.def("Subtract", (class wxDateSpan & (wxDateSpan::*)(const class wxDateSpan &)) &wxDateSpan::Subtract, "C++: wxDateSpan::Subtract(const class wxDateSpan &) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("__isub__", (class wxDateSpan & (wxDateSpan::*)(const class wxDateSpan &)) &wxDateSpan::operator-=, "C++: wxDateSpan::operator-=(const class wxDateSpan &) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("__sub__", (class wxDateSpan (wxDateSpan::*)(const class wxDateSpan &) const) &wxDateSpan::operator-, "C++: wxDateSpan::operator-(const class wxDateSpan &) const --> class wxDateSpan", pybind11::arg("ds"));
		cl.def("Negate", (class wxDateSpan (wxDateSpan::*)() const) &wxDateSpan::Negate, "C++: wxDateSpan::Negate() const --> class wxDateSpan");
		cl.def("Neg", (class wxDateSpan & (wxDateSpan::*)()) &wxDateSpan::Neg, "C++: wxDateSpan::Neg() --> class wxDateSpan &", pybind11::return_value_policy::automatic);
		cl.def("__sub__", (class wxDateSpan & (wxDateSpan::*)()) &wxDateSpan::operator-, "C++: wxDateSpan::operator-() --> class wxDateSpan &", pybind11::return_value_policy::automatic);
		cl.def("Multiply", (class wxDateSpan & (wxDateSpan::*)(int)) &wxDateSpan::Multiply, "C++: wxDateSpan::Multiply(int) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("factor"));
		cl.def("__imul__", (class wxDateSpan & (wxDateSpan::*)(int)) &wxDateSpan::operator*=, "C++: wxDateSpan::operator*=(int) --> class wxDateSpan &", pybind11::return_value_policy::automatic, pybind11::arg("factor"));
		cl.def("__mul__", (class wxDateSpan (wxDateSpan::*)(int) const) &wxDateSpan::operator*, "C++: wxDateSpan::operator*(int) const --> class wxDateSpan", pybind11::arg("n"));
		cl.def("__eq__", (bool (wxDateSpan::*)(const class wxDateSpan &) const) &wxDateSpan::operator==, "C++: wxDateSpan::operator==(const class wxDateSpan &) const --> bool", pybind11::arg("ds"));
		cl.def("__ne__", (bool (wxDateSpan::*)(const class wxDateSpan &) const) &wxDateSpan::operator!=, "C++: wxDateSpan::operator!=(const class wxDateSpan &) const --> bool", pybind11::arg("ds"));
	}
	{ // wxDateTimeArray file: line:1537
		pybind11::class_<wxDateTimeArray, std::shared_ptr<wxDateTimeArray>> cl(M(""), "wxDateTimeArray", "");
		cl.def( pybind11::init( [](){ return new wxDateTimeArray(); } ) );
		cl.def( pybind11::init( [](wxDateTimeArray const &o){ return new wxDateTimeArray(o); } ) );
		cl.def("assign", (class wxDateTimeArray & (wxDateTimeArray::*)(const class wxDateTimeArray &)) &wxDateTimeArray::operator=, "C++: wxDateTimeArray::operator=(const class wxDateTimeArray &) --> class wxDateTimeArray &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Alloc", (void (wxDateTimeArray::*)(unsigned long)) &wxDateTimeArray::Alloc, "C++: wxDateTimeArray::Alloc(unsigned long) --> void", pybind11::arg("count"));
		cl.def("reserve", (void (wxDateTimeArray::*)(unsigned long)) &wxDateTimeArray::reserve, "C++: wxDateTimeArray::reserve(unsigned long) --> void", pybind11::arg("count"));
		cl.def("GetCount", (unsigned long (wxDateTimeArray::*)() const) &wxDateTimeArray::GetCount, "C++: wxDateTimeArray::GetCount() const --> unsigned long");
		cl.def("size", (unsigned long (wxDateTimeArray::*)() const) &wxDateTimeArray::size, "C++: wxDateTimeArray::size() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxDateTimeArray::*)() const) &wxDateTimeArray::IsEmpty, "C++: wxDateTimeArray::IsEmpty() const --> bool");
		cl.def("empty", (bool (wxDateTimeArray::*)() const) &wxDateTimeArray::empty, "C++: wxDateTimeArray::empty() const --> bool");
		cl.def("Count", (unsigned long (wxDateTimeArray::*)() const) &wxDateTimeArray::Count, "C++: wxDateTimeArray::Count() const --> unsigned long");
		cl.def("Shrink", (void (wxDateTimeArray::*)()) &wxDateTimeArray::Shrink, "C++: wxDateTimeArray::Shrink() --> void");
		cl.def("__getitem__", (class wxDateTime & (wxDateTimeArray::*)(unsigned long) const) &wxDateTimeArray::operator[], "C++: wxDateTimeArray::operator[](unsigned long) const --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (class wxDateTime & (wxDateTimeArray::*)(unsigned long) const) &wxDateTimeArray::Item, "C++: wxDateTimeArray::Item(unsigned long) const --> class wxDateTime &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (class wxDateTime & (wxDateTimeArray::*)() const) &wxDateTimeArray::Last, "C++: wxDateTimeArray::Last() const --> class wxDateTime &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxDateTimeArray const &o, const class wxDateTime & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxDateTimeArray::*)(const class wxDateTime &, bool) const) &wxDateTimeArray::Index, "C++: wxDateTimeArray::Index(const class wxDateTime &, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxDateTimeArray &o, const class wxDateTime & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxDateTimeArray::*)(const class wxDateTime &, unsigned long)) &wxDateTimeArray::Add, "C++: wxDateTimeArray::Add(const class wxDateTime &, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Add", (void (wxDateTimeArray::*)(const class wxDateTime *)) &wxDateTimeArray::Add, "C++: wxDateTimeArray::Add(const class wxDateTime *) --> void", pybind11::arg("pItem"));
		cl.def("push_back", (void (wxDateTimeArray::*)(const class wxDateTime *)) &wxDateTimeArray::push_back, "C++: wxDateTimeArray::push_back(const class wxDateTime *) --> void", pybind11::arg("pItem"));
		cl.def("push_back", (void (wxDateTimeArray::*)(const class wxDateTime &)) &wxDateTimeArray::push_back, "C++: wxDateTimeArray::push_back(const class wxDateTime &) --> void", pybind11::arg("lItem"));
		cl.def("Insert", [](wxDateTimeArray &o, const class wxDateTime & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxDateTimeArray::*)(const class wxDateTime &, unsigned long, unsigned long)) &wxDateTimeArray::Insert, "C++: wxDateTimeArray::Insert(const class wxDateTime &, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("Insert", (void (wxDateTimeArray::*)(const class wxDateTime *, unsigned long)) &wxDateTimeArray::Insert, "C++: wxDateTimeArray::Insert(const class wxDateTime *, unsigned long) --> void", pybind11::arg("pItem"), pybind11::arg("uiIndex"));
		cl.def("Empty", (void (wxDateTimeArray::*)()) &wxDateTimeArray::Empty, "C++: wxDateTimeArray::Empty() --> void");
		cl.def("Clear", (void (wxDateTimeArray::*)()) &wxDateTimeArray::Clear, "C++: wxDateTimeArray::Clear() --> void");
		cl.def("Detach", (class wxDateTime * (wxDateTimeArray::*)(unsigned long)) &wxDateTimeArray::Detach, "C++: wxDateTimeArray::Detach(unsigned long) --> class wxDateTime *", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("RemoveAt", [](wxDateTimeArray &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxDateTimeArray::*)(unsigned long, unsigned long)) &wxDateTimeArray::RemoveAt, "C++: wxDateTimeArray::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
	}
}
