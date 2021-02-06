#include <sstream> // __str__

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxThread file: line:462
struct PyCallBack_wxThread : public wxThread {
	using wxThread::wxThread;

	bool TestDestroy() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThread *>(this), "TestDestroy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxThread::TestDestroy();
	}
	void * Entry() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThread *>(this), "Entry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxThread::Entry\"");
	}
	void OnDelete() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThread *>(this), "OnDelete");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxThread::OnDelete();
	}
	void OnKill() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThread *>(this), "OnKill");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxThread::OnKill();
	}
};

// wxThreadHelperThread file: line:663
struct PyCallBack_wxThreadHelperThread : public wxThreadHelperThread {
	using wxThreadHelperThread::wxThreadHelperThread;

	void * Entry() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadHelperThread *>(this), "Entry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return wxThreadHelperThread::Entry();
	}
	bool TestDestroy() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadHelperThread *>(this), "TestDestroy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxThread::TestDestroy();
	}
	void OnDelete() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadHelperThread *>(this), "OnDelete");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxThread::OnDelete();
	}
	void OnKill() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadHelperThread *>(this), "OnKill");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxThread::OnKill();
	}
};

// wxThreadHelper file: line:691
struct PyCallBack_wxThreadHelper : public wxThreadHelper {
	using wxThreadHelper::wxThreadHelper;

	void * Entry() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadHelper *>(this), "Entry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxThreadHelper::Entry\"");
	}
};

// wxTrackerNode file: line:19
struct PyCallBack_wxTrackerNode : public wxTrackerNode {
	using wxTrackerNode::wxTrackerNode;

	void OnObjectDestroy() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTrackerNode *>(this), "OnObjectDestroy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTrackerNode::OnObjectDestroy\"");
	}
	class wxEventConnectionRef * ToEventConnection() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTrackerNode *>(this), "ToEventConnection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventConnectionRef *>::value) {
				static pybind11::detail::override_caster_t<class wxEventConnectionRef *> caster;
				return pybind11::detail::cast_ref<class wxEventConnectionRef *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventConnectionRef *>(std::move(o));
		}
		return wxTrackerNode::ToEventConnection();
	}
};

// wxAnyValueType file: line:51
struct PyCallBack_wxAnyValueType : public wxAnyValueType {
	using wxAnyValueType::wxAnyValueType;

	bool IsSameType(const class wxAnyValueType * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueType *>(this), "IsSameType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::IsSameType\"");
	}
	void DeleteValue(union wxAnyValueBuffer & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueType *>(this), "DeleteValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::DeleteValue\"");
	}
	void CopyBuffer(const union wxAnyValueBuffer & a0, union wxAnyValueBuffer & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueType *>(this), "CopyBuffer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::CopyBuffer\"");
	}
	bool ConvertValue(const union wxAnyValueBuffer & a0, class wxAnyValueType * a1, union wxAnyValueBuffer & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAnyValueType *>(this), "ConvertValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAnyValueType::ConvertValue\"");
	}
};

void bind_unknown_unknown_45(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxMutexError file: line:28
	pybind11::enum_<wxMutexError>(M(""), "wxMutexError", pybind11::arithmetic(), "")
		.value("wxMUTEX_NO_ERROR", wxMUTEX_NO_ERROR)
		.value("wxMUTEX_INVALID", wxMUTEX_INVALID)
		.value("wxMUTEX_DEAD_LOCK", wxMUTEX_DEAD_LOCK)
		.value("wxMUTEX_BUSY", wxMUTEX_BUSY)
		.value("wxMUTEX_UNLOCKED", wxMUTEX_UNLOCKED)
		.value("wxMUTEX_TIMEOUT", wxMUTEX_TIMEOUT)
		.value("wxMUTEX_MISC_ERROR", wxMUTEX_MISC_ERROR)
		.export_values();

;

	// wxCondError file: line:39
	pybind11::enum_<wxCondError>(M(""), "wxCondError", pybind11::arithmetic(), "")
		.value("wxCOND_NO_ERROR", wxCOND_NO_ERROR)
		.value("wxCOND_INVALID", wxCOND_INVALID)
		.value("wxCOND_TIMEOUT", wxCOND_TIMEOUT)
		.value("wxCOND_MISC_ERROR", wxCOND_MISC_ERROR)
		.export_values();

;

	// wxSemaError file: line:47
	pybind11::enum_<wxSemaError>(M(""), "wxSemaError", pybind11::arithmetic(), "")
		.value("wxSEMA_NO_ERROR", wxSEMA_NO_ERROR)
		.value("wxSEMA_INVALID", wxSEMA_INVALID)
		.value("wxSEMA_BUSY", wxSEMA_BUSY)
		.value("wxSEMA_TIMEOUT", wxSEMA_TIMEOUT)
		.value("wxSEMA_OVERFLOW", wxSEMA_OVERFLOW)
		.value("wxSEMA_MISC_ERROR", wxSEMA_MISC_ERROR)
		.export_values();

;

	// wxThreadError file: line:57
	pybind11::enum_<wxThreadError>(M(""), "wxThreadError", pybind11::arithmetic(), "")
		.value("wxTHREAD_NO_ERROR", wxTHREAD_NO_ERROR)
		.value("wxTHREAD_NO_RESOURCE", wxTHREAD_NO_RESOURCE)
		.value("wxTHREAD_RUNNING", wxTHREAD_RUNNING)
		.value("wxTHREAD_NOT_RUNNING", wxTHREAD_NOT_RUNNING)
		.value("wxTHREAD_KILLED", wxTHREAD_KILLED)
		.value("wxTHREAD_MISC_ERROR", wxTHREAD_MISC_ERROR)
		.export_values();

;

	// wxThreadKind file: line:67
	pybind11::enum_<wxThreadKind>(M(""), "wxThreadKind", pybind11::arithmetic(), "")
		.value("wxTHREAD_DETACHED", wxTHREAD_DETACHED)
		.value("wxTHREAD_JOINABLE", wxTHREAD_JOINABLE)
		.export_values();

;

	// wxThreadWait file: line:73
	pybind11::enum_<wxThreadWait>(M(""), "wxThreadWait", pybind11::arithmetic(), "")
		.value("wxTHREAD_WAIT_BLOCK", wxTHREAD_WAIT_BLOCK)
		.value("wxTHREAD_WAIT_YIELD", wxTHREAD_WAIT_YIELD)
		.value("wxTHREAD_WAIT_DEFAULT", wxTHREAD_WAIT_DEFAULT)
		.export_values();

;

	// wxMutexType file: line:112
	pybind11::enum_<wxMutexType>(M(""), "wxMutexType", pybind11::arithmetic(), "")
		.value("wxMUTEX_DEFAULT", wxMUTEX_DEFAULT)
		.value("wxMUTEX_RECURSIVE", wxMUTEX_RECURSIVE)
		.export_values();

;

	{ // wxMutex file: line:137
		pybind11::class_<wxMutex, std::shared_ptr<wxMutex>> cl(M(""), "wxMutex", "");
		cl.def( pybind11::init( [](){ return new wxMutex(); } ), "doc" );
		cl.def( pybind11::init<enum wxMutexType>(), pybind11::arg("mutexType") );

		cl.def("IsOk", (bool (wxMutex::*)() const) &wxMutex::IsOk, "C++: wxMutex::IsOk() const --> bool");
		cl.def("Lock", (enum wxMutexError (wxMutex::*)()) &wxMutex::Lock, "C++: wxMutex::Lock() --> enum wxMutexError");
		cl.def("LockTimeout", (enum wxMutexError (wxMutex::*)(unsigned long)) &wxMutex::LockTimeout, "C++: wxMutex::LockTimeout(unsigned long) --> enum wxMutexError", pybind11::arg("ms"));
		cl.def("TryLock", (enum wxMutexError (wxMutex::*)()) &wxMutex::TryLock, "C++: wxMutex::TryLock() --> enum wxMutexError");
		cl.def("Unlock", (enum wxMutexError (wxMutex::*)()) &wxMutex::Unlock, "C++: wxMutex::Unlock() --> enum wxMutexError");
	}
	{ // wxMutexLocker file: line:184
		pybind11::class_<wxMutexLocker, std::shared_ptr<wxMutexLocker>> cl(M(""), "wxMutexLocker", "");
		cl.def( pybind11::init<class wxMutex &>(), pybind11::arg("mutex") );

		cl.def("IsOk", (bool (wxMutexLocker::*)() const) &wxMutexLocker::IsOk, "C++: wxMutexLocker::IsOk() const --> bool");
	}
	// wxCriticalSectionType file: line:231
	pybind11::enum_<wxCriticalSectionType>(M(""), "wxCriticalSectionType", pybind11::arithmetic(), "")
		.value("wxCRITSEC_DEFAULT", wxCRITSEC_DEFAULT)
		.value("wxCRITSEC_NON_RECURSIVE", wxCRITSEC_NON_RECURSIVE)
		.export_values();

;

	{ // wxCriticalSection file: line:242
		pybind11::class_<wxCriticalSection, std::shared_ptr<wxCriticalSection>> cl(M(""), "wxCriticalSection", "");
		cl.def( pybind11::init( [](){ return new wxCriticalSection(); } ), "doc" );
		cl.def( pybind11::init<enum wxCriticalSectionType>(), pybind11::arg("critSecType") );

		cl.def("Enter", (void (wxCriticalSection::*)()) &wxCriticalSection::Enter, "C++: wxCriticalSection::Enter() --> void");
		cl.def("TryEnter", (bool (wxCriticalSection::*)()) &wxCriticalSection::TryEnter, "C++: wxCriticalSection::TryEnter() --> bool");
		cl.def("Leave", (void (wxCriticalSection::*)()) &wxCriticalSection::Leave, "C++: wxCriticalSection::Leave() --> void");
	}
	{ // wxCriticalSectionLocker file: line:301
		pybind11::class_<wxCriticalSectionLocker, std::shared_ptr<wxCriticalSectionLocker>> cl(M(""), "wxCriticalSectionLocker", "");
		cl.def( pybind11::init<class wxCriticalSection &>(), pybind11::arg("cs") );

	}
	{ // wxCondition file: line:326
		pybind11::class_<wxCondition, std::shared_ptr<wxCondition>> cl(M(""), "wxCondition", "");
		cl.def( pybind11::init<class wxMutex &>(), pybind11::arg("mutex") );

		cl.def("IsOk", (bool (wxCondition::*)() const) &wxCondition::IsOk, "C++: wxCondition::IsOk() const --> bool");
		cl.def("Wait", (enum wxCondError (wxCondition::*)()) &wxCondition::Wait, "C++: wxCondition::Wait() --> enum wxCondError");
		cl.def("WaitTimeout", (enum wxCondError (wxCondition::*)(unsigned long)) &wxCondition::WaitTimeout, "C++: wxCondition::WaitTimeout(unsigned long) --> enum wxCondError", pybind11::arg("milliseconds"));
		cl.def("Signal", (enum wxCondError (wxCondition::*)()) &wxCondition::Signal, "C++: wxCondition::Signal() --> enum wxCondError");
		cl.def("Broadcast", (enum wxCondError (wxCondition::*)()) &wxCondition::Broadcast, "C++: wxCondition::Broadcast() --> enum wxCondError");
	}
	{ // wxSemaphore file: line:407
		pybind11::class_<wxSemaphore, std::shared_ptr<wxSemaphore>> cl(M(""), "wxSemaphore", "");
		cl.def( pybind11::init( [](){ return new wxSemaphore(); } ), "doc" );
		cl.def( pybind11::init( [](int const & a0){ return new wxSemaphore(a0); } ), "doc" , pybind11::arg("initialcount"));
		cl.def( pybind11::init<int, int>(), pybind11::arg("initialcount"), pybind11::arg("maxcount") );

		cl.def("IsOk", (bool (wxSemaphore::*)() const) &wxSemaphore::IsOk, "C++: wxSemaphore::IsOk() const --> bool");
		cl.def("Wait", (enum wxSemaError (wxSemaphore::*)()) &wxSemaphore::Wait, "C++: wxSemaphore::Wait() --> enum wxSemaError");
		cl.def("TryWait", (enum wxSemaError (wxSemaphore::*)()) &wxSemaphore::TryWait, "C++: wxSemaphore::TryWait() --> enum wxSemaError");
		cl.def("WaitTimeout", (enum wxSemaError (wxSemaphore::*)(unsigned long)) &wxSemaphore::WaitTimeout, "C++: wxSemaphore::WaitTimeout(unsigned long) --> enum wxSemaError", pybind11::arg("milliseconds"));
		cl.def("Post", (enum wxSemaError (wxSemaphore::*)()) &wxSemaphore::Post, "C++: wxSemaphore::Post() --> enum wxSemaError");
	}
	{ // wxThread file: line:462
		pybind11::class_<wxThread, std::shared_ptr<wxThread>, PyCallBack_wxThread> cl(M(""), "wxThread", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxThread(); } ), "doc");
		cl.def( pybind11::init<enum wxThreadKind>(), pybind11::arg("kind") );

		cl.def_static("This", (class wxThread * (*)()) &wxThread::This, "C++: wxThread::This() --> class wxThread *", pybind11::return_value_policy::automatic);
		cl.def_static("IsMain", (bool (*)()) &wxThread::IsMain, "C++: wxThread::IsMain() --> bool");
		cl.def_static("GetMainId", (unsigned long (*)()) &wxThread::GetMainId, "C++: wxThread::GetMainId() --> unsigned long");
		cl.def_static("Yield", (void (*)()) &wxThread::Yield, "C++: wxThread::Yield() --> void");
		cl.def_static("Sleep", (void (*)(unsigned long)) &wxThread::Sleep, "C++: wxThread::Sleep(unsigned long) --> void", pybind11::arg("milliseconds"));
		cl.def_static("GetCPUCount", (int (*)()) &wxThread::GetCPUCount, "C++: wxThread::GetCPUCount() --> int");
		cl.def_static("GetCurrentId", (unsigned long (*)()) &wxThread::GetCurrentId, "C++: wxThread::GetCurrentId() --> unsigned long");
		cl.def_static("SetConcurrency", (bool (*)(unsigned long)) &wxThread::SetConcurrency, "C++: wxThread::SetConcurrency(unsigned long) --> bool", pybind11::arg("level"));
		cl.def("Create", [](wxThread &o) -> wxThreadError { return o.Create(); }, "");
		cl.def("Create", (enum wxThreadError (wxThread::*)(unsigned int)) &wxThread::Create, "C++: wxThread::Create(unsigned int) --> enum wxThreadError", pybind11::arg("stackSize"));
		cl.def("Run", (enum wxThreadError (wxThread::*)()) &wxThread::Run, "C++: wxThread::Run() --> enum wxThreadError");
		cl.def("Wait", [](wxThread &o) -> void * { return o.Wait(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Wait", (void * (wxThread::*)(enum wxThreadWait)) &wxThread::Wait, "C++: wxThread::Wait(enum wxThreadWait) --> void *", pybind11::return_value_policy::automatic, pybind11::arg("waitMode"));
		cl.def("Kill", (enum wxThreadError (wxThread::*)()) &wxThread::Kill, "C++: wxThread::Kill() --> enum wxThreadError");
		cl.def("Pause", (enum wxThreadError (wxThread::*)()) &wxThread::Pause, "C++: wxThread::Pause() --> enum wxThreadError");
		cl.def("Resume", (enum wxThreadError (wxThread::*)()) &wxThread::Resume, "C++: wxThread::Resume() --> enum wxThreadError");
		cl.def("SetPriority", (void (wxThread::*)(unsigned int)) &wxThread::SetPriority, "C++: wxThread::SetPriority(unsigned int) --> void", pybind11::arg("prio"));
		cl.def("GetPriority", (unsigned int (wxThread::*)() const) &wxThread::GetPriority, "C++: wxThread::GetPriority() const --> unsigned int");
		cl.def("IsAlive", (bool (wxThread::*)() const) &wxThread::IsAlive, "C++: wxThread::IsAlive() const --> bool");
		cl.def("IsRunning", (bool (wxThread::*)() const) &wxThread::IsRunning, "C++: wxThread::IsRunning() const --> bool");
		cl.def("IsPaused", (bool (wxThread::*)() const) &wxThread::IsPaused, "C++: wxThread::IsPaused() const --> bool");
		cl.def("IsDetached", (bool (wxThread::*)() const) &wxThread::IsDetached, "C++: wxThread::IsDetached() const --> bool");
		cl.def("GetId", (unsigned long (wxThread::*)() const) &wxThread::GetId, "C++: wxThread::GetId() const --> unsigned long");
		cl.def("GetKind", (enum wxThreadKind (wxThread::*)() const) &wxThread::GetKind, "C++: wxThread::GetKind() const --> enum wxThreadKind");
		cl.def("TestDestroy", (bool (wxThread::*)()) &wxThread::TestDestroy, "C++: wxThread::TestDestroy() --> bool");
	}
	{ // wxThreadHelperThread file: line:663
		pybind11::class_<wxThreadHelperThread, std::shared_ptr<wxThreadHelperThread>, PyCallBack_wxThreadHelperThread, wxThread> cl(M(""), "wxThreadHelperThread", "");
		cl.def( pybind11::init<class wxThreadHelper &, enum wxThreadKind>(), pybind11::arg("owner"), pybind11::arg("kind") );

	}
	{ // wxThreadHelper file: line:691
		pybind11::class_<wxThreadHelper, std::shared_ptr<wxThreadHelper>, PyCallBack_wxThreadHelper> cl(M(""), "wxThreadHelper", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxThreadHelper(); } ), "doc");
		cl.def( pybind11::init<enum wxThreadKind>(), pybind11::arg("kind") );

		cl.def("Create", [](wxThreadHelper &o) -> wxThreadError { return o.Create(); }, "");
		cl.def("Create", (enum wxThreadError (wxThreadHelper::*)(unsigned int)) &wxThreadHelper::Create, "C++: wxThreadHelper::Create(unsigned int) --> enum wxThreadError", pybind11::arg("stackSize"));
		cl.def("CreateThread", [](wxThreadHelper &o) -> wxThreadError { return o.CreateThread(); }, "");
		cl.def("CreateThread", [](wxThreadHelper &o, enum wxThreadKind const & a0) -> wxThreadError { return o.CreateThread(a0); }, "", pybind11::arg("kind"));
		cl.def("CreateThread", (enum wxThreadError (wxThreadHelper::*)(enum wxThreadKind, unsigned int)) &wxThreadHelper::CreateThread, "C++: wxThreadHelper::CreateThread(enum wxThreadKind, unsigned int) --> enum wxThreadError", pybind11::arg("kind"), pybind11::arg("stackSize"));
		cl.def("Entry", (void * (wxThreadHelper::*)()) &wxThreadHelper::Entry, "C++: wxThreadHelper::Entry() --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetThread", (class wxThread * (wxThreadHelper::*)() const) &wxThreadHelper::GetThread, "C++: wxThreadHelper::GetThread() const --> class wxThread *", pybind11::return_value_policy::automatic);
	}
	// wxMutexGuiEnter() file: line:787
	M("").def("wxMutexGuiEnter", (void (*)()) &wxMutexGuiEnter, "C++: wxMutexGuiEnter() --> void");

	// wxMutexGuiLeave() file: line:788
	M("").def("wxMutexGuiLeave", (void (*)()) &wxMutexGuiLeave, "C++: wxMutexGuiLeave() --> void");

	// wxIsMainThread() file: line:800
	M("").def("wxIsMainThread", (bool (*)()) &wxIsMainThread, "C++: wxIsMainThread() --> bool");

	{ // wxMutexGuiLocker file: line:846
		pybind11::class_<wxMutexGuiLocker, std::shared_ptr<wxMutexGuiLocker>> cl(M(""), "wxMutexGuiLocker", "");
		cl.def( pybind11::init( [](){ return new wxMutexGuiLocker(); } ) );
	}
	{ // wxTrackerNode file: line:19
		pybind11::class_<wxTrackerNode, std::shared_ptr<wxTrackerNode>, PyCallBack_wxTrackerNode> cl(M(""), "wxTrackerNode", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxTrackerNode(); } ) );
		cl.def(pybind11::init<PyCallBack_wxTrackerNode const &>());
		cl.def("OnObjectDestroy", (void (wxTrackerNode::*)()) &wxTrackerNode::OnObjectDestroy, "C++: wxTrackerNode::OnObjectDestroy() --> void");
		cl.def("ToEventConnection", (class wxEventConnectionRef * (wxTrackerNode::*)()) &wxTrackerNode::ToEventConnection, "C++: wxTrackerNode::ToEventConnection() --> class wxEventConnectionRef *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxTrackerNode & (wxTrackerNode::*)(const class wxTrackerNode &)) &wxTrackerNode::operator=, "C++: wxTrackerNode::operator=(const class wxTrackerNode &) --> class wxTrackerNode &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxTrackable file: line:37
		pybind11::class_<wxTrackable, wxTrackable*> cl(M(""), "wxTrackable", "");
		cl.def("AddNode", (void (wxTrackable::*)(class wxTrackerNode *)) &wxTrackable::AddNode, "C++: wxTrackable::AddNode(class wxTrackerNode *) --> void", pybind11::arg("prn"));
		cl.def("RemoveNode", (void (wxTrackable::*)(class wxTrackerNode *)) &wxTrackable::RemoveNode, "C++: wxTrackable::RemoveNode(class wxTrackerNode *) --> void", pybind11::arg("prn"));
		cl.def("GetFirst", (class wxTrackerNode * (wxTrackable::*)() const) &wxTrackable::GetFirst, "C++: wxTrackable::GetFirst() const --> class wxTrackerNode *", pybind11::return_value_policy::automatic);
	}
	{ // wxTypeIdentifier file: line:63
		pybind11::class_<wxTypeIdentifier, std::shared_ptr<wxTypeIdentifier>> cl(M(""), "wxTypeIdentifier", "");
		cl.def( pybind11::init<const char *>(), pybind11::arg("className") );

		cl.def( pybind11::init( [](wxTypeIdentifier const &o){ return new wxTypeIdentifier(o); } ) );
		cl.def("__eq__", (bool (wxTypeIdentifier::*)(const class wxTypeIdentifier &)) &wxTypeIdentifier::operator==, "C++: wxTypeIdentifier::operator==(const class wxTypeIdentifier &) --> bool", pybind11::arg("other"));
		cl.def("__ne__", (bool (wxTypeIdentifier::*)(const class wxTypeIdentifier &)) &wxTypeIdentifier::operator!=, "C++: wxTypeIdentifier::operator!=(const class wxTypeIdentifier &) --> bool", pybind11::arg("other"));
	}
	{ // wxAnyValueBuffer file: line:30
		pybind11::class_<wxAnyValueBuffer, std::shared_ptr<wxAnyValueBuffer>> cl(M(""), "wxAnyValueBuffer", "");
		cl.def( pybind11::init( [](){ return new wxAnyValueBuffer(); } ) );
		cl.def( pybind11::init( [](wxAnyValueBuffer const &o){ return new wxAnyValueBuffer(o); } ) );
		cl.def_readwrite("m_alignment", &wxAnyValueBuffer::m_alignment);

		{ // wxAnyValueBuffer::Alignment file: line:32
			auto & enclosing_class = cl;
			pybind11::class_<wxAnyValueBuffer::Alignment, std::shared_ptr<wxAnyValueBuffer::Alignment>> cl(enclosing_class, "Alignment", "");
			cl.def( pybind11::init( [](){ return new wxAnyValueBuffer::Alignment(); } ) );
			cl.def( pybind11::init( [](wxAnyValueBuffer::Alignment const &o){ return new wxAnyValueBuffer::Alignment(o); } ) );
			cl.def_readwrite("m_int64", &wxAnyValueBuffer::Alignment::m_int64);
			cl.def_readwrite("m_longDouble", &wxAnyValueBuffer::Alignment::m_longDouble);
			cl.def_readwrite("m_mFuncPtr", &wxAnyValueBuffer::Alignment::m_mFuncPtr);
		}

	}
	{ // wxAnyValueType file: line:51
		pybind11::class_<wxAnyValueType, std::shared_ptr<wxAnyValueType>, PyCallBack_wxAnyValueType> cl(M(""), "wxAnyValueType", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAnyValueType(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAnyValueType const &>());
		cl.def("IsSameType", (bool (wxAnyValueType::*)(const class wxAnyValueType *) const) &wxAnyValueType::IsSameType, "This function is used for internal type matching.\n\nC++: wxAnyValueType::IsSameType(const class wxAnyValueType *) const --> bool", pybind11::arg("otherType"));
		cl.def("DeleteValue", (void (wxAnyValueType::*)(union wxAnyValueBuffer &) const) &wxAnyValueType::DeleteValue, "This function is called every time the data in wxAny\n        buffer needs to be freed.\n\nC++: wxAnyValueType::DeleteValue(union wxAnyValueBuffer &) const --> void", pybind11::arg("buf"));
		cl.def("CopyBuffer", (void (wxAnyValueType::*)(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const) &wxAnyValueType::CopyBuffer, "Implement this for buffer-to-buffer copy.\n\n        \n\n            This is the source data buffer.\n\n        \n\n            This is the destination data buffer that is in either\n            uninitialized or freed state.\n\nC++: wxAnyValueType::CopyBuffer(const union wxAnyValueBuffer &, union wxAnyValueBuffer &) const --> void", pybind11::arg("src"), pybind11::arg("dst"));
		cl.def("ConvertValue", (bool (wxAnyValueType::*)(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const) &wxAnyValueType::ConvertValue, "Convert value into buffer of different type. Return false if\n        not possible.\n\nC++: wxAnyValueType::ConvertValue(const union wxAnyValueBuffer &, class wxAnyValueType *, union wxAnyValueBuffer &) const --> bool", pybind11::arg("src"), pybind11::arg("dstType"), pybind11::arg("dst"));
		cl.def("assign", (class wxAnyValueType & (wxAnyValueType::*)(const class wxAnyValueType &)) &wxAnyValueType::operator=, "C++: wxAnyValueType::operator=(const class wxAnyValueType &) --> class wxAnyValueType &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}
