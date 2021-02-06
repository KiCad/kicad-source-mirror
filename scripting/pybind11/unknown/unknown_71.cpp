#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxStreamBuffer file: line:420
struct PyCallBack_wxStreamBuffer : public wxStreamBuffer {
	using wxStreamBuffer::wxStreamBuffer;

	unsigned long Read(void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "Read");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxStreamBuffer::Read(a0, a1);
	}
	unsigned long Write(const void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "Write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxStreamBuffer::Write(a0, a1);
	}
	char Peek() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "Peek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char>::value) {
				static pybind11::detail::override_caster_t<char> caster;
				return pybind11::detail::cast_ref<char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char>(std::move(o));
		}
		return wxStreamBuffer::Peek();
	}
	char GetChar() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "GetChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char>::value) {
				static pybind11::detail::override_caster_t<char> caster;
				return pybind11::detail::cast_ref<char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char>(std::move(o));
		}
		return wxStreamBuffer::GetChar();
	}
	void PutChar(char a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "PutChar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxStreamBuffer::PutChar(a0);
	}
	long Tell() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "Tell");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxStreamBuffer::Tell();
	}
	long Seek(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStreamBuffer *>(this), "Seek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxStreamBuffer::Seek(a0, a1);
	}
};

// wxBufferedInputStream file: line:551
struct PyCallBack_wxBufferedInputStream : public wxBufferedInputStream {
	using wxBufferedInputStream::wxBufferedInputStream;

	char Peek() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "Peek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char>::value) {
				static pybind11::detail::override_caster_t<char> caster;
				return pybind11::detail::cast_ref<char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char>(std::move(o));
		}
		return wxBufferedInputStream::Peek();
	}
	class wxInputStream & Read(void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "Read");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxInputStream &>::value) {
				static pybind11::detail::override_caster_t<class wxInputStream &> caster;
				return pybind11::detail::cast_ref<class wxInputStream &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxInputStream &>(std::move(o));
		}
		return wxBufferedInputStream::Read(a0, a1);
	}
	long SeekI(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "SeekI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedInputStream::SeekI(a0, a1);
	}
	long TellI() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "TellI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedInputStream::TellI();
	}
	bool IsSeekable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "IsSeekable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBufferedInputStream::IsSeekable();
	}
	unsigned long OnSysRead(void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "OnSysRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxBufferedInputStream::OnSysRead(a0, a1);
	}
	long OnSysSeek(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "OnSysSeek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedInputStream::OnSysSeek(a0, a1);
	}
	long OnSysTell() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "OnSysTell");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedInputStream::OnSysTell();
	}
	long GetLength() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "GetLength");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxFilterInputStream::GetLength();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFilterInputStream::GetClassInfo();
	}
	unsigned long LastRead() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "LastRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxInputStream::LastRead();
	}
	bool CanRead() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "CanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxInputStream::CanRead();
	}
	bool Eof() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "Eof");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxInputStream::Eof();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxStreamBase::IsOk();
	}
	unsigned long GetSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "GetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxStreamBase::GetSize();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedInputStream *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CloneRefData(a0);
	}
};

// wxBufferedOutputStream file: line:600
struct PyCallBack_wxBufferedOutputStream : public wxBufferedOutputStream {
	using wxBufferedOutputStream::wxBufferedOutputStream;

	class wxOutputStream & Write(const void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "Write");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxOutputStream &>::value) {
				static pybind11::detail::override_caster_t<class wxOutputStream &> caster;
				return pybind11::detail::cast_ref<class wxOutputStream &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxOutputStream &>(std::move(o));
		}
		return wxBufferedOutputStream::Write(a0, a1);
	}
	long SeekO(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "SeekO");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedOutputStream::SeekO(a0, a1);
	}
	long TellO() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "TellO");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedOutputStream::TellO();
	}
	bool IsSeekable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "IsSeekable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBufferedOutputStream::IsSeekable();
	}
	void Sync() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "Sync");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxBufferedOutputStream::Sync();
	}
	bool Close() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "Close");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBufferedOutputStream::Close();
	}
	long GetLength() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "GetLength");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedOutputStream::GetLength();
	}
	unsigned long OnSysWrite(const void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "OnSysWrite");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxBufferedOutputStream::OnSysWrite(a0, a1);
	}
	long OnSysSeek(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "OnSysSeek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedOutputStream::OnSysSeek(a0, a1);
	}
	long OnSysTell() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "OnSysTell");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxBufferedOutputStream::OnSysTell();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFilterOutputStream::GetClassInfo();
	}
	unsigned long LastWrite() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "LastWrite");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxOutputStream::LastWrite();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxStreamBase::IsOk();
	}
	unsigned long GetSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "GetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxStreamBase::GetSize();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBufferedOutputStream *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CloneRefData(a0);
	}
};

// wxWrapperInputStream file: line:658
struct PyCallBack_wxWrapperInputStream : public wxWrapperInputStream {
	using wxWrapperInputStream::wxWrapperInputStream;

	long GetLength() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "GetLength");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxWrapperInputStream::GetLength();
	}
	bool IsSeekable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "IsSeekable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxWrapperInputStream::IsSeekable();
	}
	unsigned long OnSysRead(void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "OnSysRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxWrapperInputStream::OnSysRead(a0, a1);
	}
	long OnSysSeek(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "OnSysSeek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxWrapperInputStream::OnSysSeek(a0, a1);
	}
	long OnSysTell() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "OnSysTell");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxWrapperInputStream::OnSysTell();
	}
	char Peek() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "Peek");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<char>::value) {
				static pybind11::detail::override_caster_t<char> caster;
				return pybind11::detail::cast_ref<char>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<char>(std::move(o));
		}
		return wxFilterInputStream::Peek();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFilterInputStream::GetClassInfo();
	}
	class wxInputStream & Read(void * a0, unsigned long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "Read");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxInputStream &>::value) {
				static pybind11::detail::override_caster_t<class wxInputStream &> caster;
				return pybind11::detail::cast_ref<class wxInputStream &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxInputStream &>(std::move(o));
		}
		return wxInputStream::Read(a0, a1);
	}
	unsigned long LastRead() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "LastRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxInputStream::LastRead();
	}
	bool CanRead() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "CanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxInputStream::CanRead();
	}
	bool Eof() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "Eof");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxInputStream::Eof();
	}
	long SeekI(long a0, enum wxSeekMode a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "SeekI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxInputStream::SeekI(a0, a1);
	}
	long TellI() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "TellI");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<long>::value) {
				static pybind11::detail::override_caster_t<long> caster;
				return pybind11::detail::cast_ref<long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<long>(std::move(o));
		}
		return wxInputStream::TellI();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxStreamBase::IsOk();
	}
	unsigned long GetSize() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "GetSize");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxStreamBase::GetSize();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWrapperInputStream *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CloneRefData(a0);
	}
};

void bind_unknown_unknown_71(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxFilterClassFactory file: line:378
		pybind11::class_<wxFilterClassFactory, std::shared_ptr<wxFilterClassFactory>, wxFilterClassFactoryBase> cl(M(""), "wxFilterClassFactory", "");
		cl.def("NewStream", (class wxFilterInputStream * (wxFilterClassFactory::*)(class wxInputStream &) const) &wxFilterClassFactory::NewStream, "C++: wxFilterClassFactory::NewStream(class wxInputStream &) const --> class wxFilterInputStream *", pybind11::return_value_policy::automatic, pybind11::arg("stream"));
		cl.def("NewStream", (class wxFilterOutputStream * (wxFilterClassFactory::*)(class wxOutputStream &) const) &wxFilterClassFactory::NewStream, "C++: wxFilterClassFactory::NewStream(class wxOutputStream &) const --> class wxFilterOutputStream *", pybind11::return_value_policy::automatic, pybind11::arg("stream"));
		cl.def("NewStream", (class wxFilterInputStream * (wxFilterClassFactory::*)(class wxInputStream *) const) &wxFilterClassFactory::NewStream, "C++: wxFilterClassFactory::NewStream(class wxInputStream *) const --> class wxFilterInputStream *", pybind11::return_value_policy::automatic, pybind11::arg("stream"));
		cl.def("NewStream", (class wxFilterOutputStream * (wxFilterClassFactory::*)(class wxOutputStream *) const) &wxFilterClassFactory::NewStream, "C++: wxFilterClassFactory::NewStream(class wxOutputStream *) const --> class wxFilterOutputStream *", pybind11::return_value_policy::automatic, pybind11::arg("stream"));
		cl.def_static("Find", [](const class wxString & a0) -> const wxFilterClassFactory * { return wxFilterClassFactory::Find(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("protocol"));
		cl.def_static("Find", (const class wxFilterClassFactory * (*)(const class wxString &, enum wxStreamProtocolType)) &wxFilterClassFactory::Find, "C++: wxFilterClassFactory::Find(const class wxString &, enum wxStreamProtocolType) --> const class wxFilterClassFactory *", pybind11::return_value_policy::automatic, pybind11::arg("protocol"), pybind11::arg("type"));
		cl.def_static("GetFirst", (const class wxFilterClassFactory * (*)()) &wxFilterClassFactory::GetFirst, "C++: wxFilterClassFactory::GetFirst() --> const class wxFilterClassFactory *", pybind11::return_value_policy::automatic);
		cl.def("GetNext", (const class wxFilterClassFactory * (wxFilterClassFactory::*)() const) &wxFilterClassFactory::GetNext, "C++: wxFilterClassFactory::GetNext() const --> const class wxFilterClassFactory *", pybind11::return_value_policy::automatic);
		cl.def("PushFront", (void (wxFilterClassFactory::*)()) &wxFilterClassFactory::PushFront, "C++: wxFilterClassFactory::PushFront() --> void");
		cl.def("Remove", (void (wxFilterClassFactory::*)()) &wxFilterClassFactory::Remove, "C++: wxFilterClassFactory::Remove() --> void");
		cl.def("GetClassInfo", (class wxClassInfo * (wxFilterClassFactory::*)() const) &wxFilterClassFactory::GetClassInfo, "C++: wxFilterClassFactory::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
	}
	{ // wxStreamBuffer file: line:420
		pybind11::class_<wxStreamBuffer, std::shared_ptr<wxStreamBuffer>, PyCallBack_wxStreamBuffer> cl(M(""), "wxStreamBuffer", "");
		cl.def( pybind11::init<class wxStreamBase &, enum wxStreamBuffer::BufMode>(), pybind11::arg("stream"), pybind11::arg("mode") );

		cl.def( pybind11::init<unsigned long, class wxInputStream &>(), pybind11::arg("bufsize"), pybind11::arg("stream") );

		cl.def( pybind11::init<unsigned long, class wxOutputStream &>(), pybind11::arg("bufsize"), pybind11::arg("stream") );

		cl.def( pybind11::init( [](PyCallBack_wxStreamBuffer const &o){ return new PyCallBack_wxStreamBuffer(o); } ) );
		cl.def( pybind11::init( [](wxStreamBuffer const &o){ return new wxStreamBuffer(o); } ) );
		cl.def( pybind11::init<enum wxStreamBuffer::BufMode>(), pybind11::arg("mode") );


		pybind11::enum_<wxStreamBuffer::BufMode>(cl, "BufMode", pybind11::arithmetic(), "")
			.value("read", wxStreamBuffer::read)
			.value("write", wxStreamBuffer::write)
			.value("read_write", wxStreamBuffer::read_write)
			.export_values();

		cl.def("Read", (unsigned long (wxStreamBuffer::*)(void *, unsigned long)) &wxStreamBuffer::Read, "C++: wxStreamBuffer::Read(void *, unsigned long) --> unsigned long", pybind11::arg("buffer"), pybind11::arg("size"));
		cl.def("Read", (unsigned long (wxStreamBuffer::*)(class wxStreamBuffer *)) &wxStreamBuffer::Read, "C++: wxStreamBuffer::Read(class wxStreamBuffer *) --> unsigned long", pybind11::arg("buf"));
		cl.def("Write", (unsigned long (wxStreamBuffer::*)(const void *, unsigned long)) &wxStreamBuffer::Write, "C++: wxStreamBuffer::Write(const void *, unsigned long) --> unsigned long", pybind11::arg("buffer"), pybind11::arg("size"));
		cl.def("Write", (unsigned long (wxStreamBuffer::*)(class wxStreamBuffer *)) &wxStreamBuffer::Write, "C++: wxStreamBuffer::Write(class wxStreamBuffer *) --> unsigned long", pybind11::arg("buf"));
		cl.def("Peek", (char (wxStreamBuffer::*)()) &wxStreamBuffer::Peek, "C++: wxStreamBuffer::Peek() --> char");
		cl.def("GetChar", (char (wxStreamBuffer::*)()) &wxStreamBuffer::GetChar, "C++: wxStreamBuffer::GetChar() --> char");
		cl.def("PutChar", (void (wxStreamBuffer::*)(char)) &wxStreamBuffer::PutChar, "C++: wxStreamBuffer::PutChar(char) --> void", pybind11::arg("c"));
		cl.def("Tell", (long (wxStreamBuffer::*)() const) &wxStreamBuffer::Tell, "C++: wxStreamBuffer::Tell() const --> long");
		cl.def("Seek", (long (wxStreamBuffer::*)(long, enum wxSeekMode)) &wxStreamBuffer::Seek, "C++: wxStreamBuffer::Seek(long, enum wxSeekMode) --> long", pybind11::arg("pos"), pybind11::arg("mode"));
		cl.def("ResetBuffer", (void (wxStreamBuffer::*)()) &wxStreamBuffer::ResetBuffer, "C++: wxStreamBuffer::ResetBuffer() --> void");
		cl.def("Truncate", (void (wxStreamBuffer::*)()) &wxStreamBuffer::Truncate, "C++: wxStreamBuffer::Truncate() --> void");
		cl.def("SetBufferIO", [](wxStreamBuffer &o, void * a0, void * a1) -> void { return o.SetBufferIO(a0, a1); }, "", pybind11::arg("start"), pybind11::arg("end"));
		cl.def("SetBufferIO", (void (wxStreamBuffer::*)(void *, void *, bool)) &wxStreamBuffer::SetBufferIO, "C++: wxStreamBuffer::SetBufferIO(void *, void *, bool) --> void", pybind11::arg("start"), pybind11::arg("end"), pybind11::arg("takeOwnership"));
		cl.def("SetBufferIO", [](wxStreamBuffer &o, void * a0, unsigned long const & a1) -> void { return o.SetBufferIO(a0, a1); }, "", pybind11::arg("start"), pybind11::arg("len"));
		cl.def("SetBufferIO", (void (wxStreamBuffer::*)(void *, unsigned long, bool)) &wxStreamBuffer::SetBufferIO, "C++: wxStreamBuffer::SetBufferIO(void *, unsigned long, bool) --> void", pybind11::arg("start"), pybind11::arg("len"), pybind11::arg("takeOwnership"));
		cl.def("SetBufferIO", (void (wxStreamBuffer::*)(unsigned long)) &wxStreamBuffer::SetBufferIO, "C++: wxStreamBuffer::SetBufferIO(unsigned long) --> void", pybind11::arg("bufsize"));
		cl.def("GetBufferStart", (void * (wxStreamBuffer::*)() const) &wxStreamBuffer::GetBufferStart, "C++: wxStreamBuffer::GetBufferStart() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetBufferEnd", (void * (wxStreamBuffer::*)() const) &wxStreamBuffer::GetBufferEnd, "C++: wxStreamBuffer::GetBufferEnd() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetBufferPos", (void * (wxStreamBuffer::*)() const) &wxStreamBuffer::GetBufferPos, "C++: wxStreamBuffer::GetBufferPos() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetBufferSize", (unsigned long (wxStreamBuffer::*)() const) &wxStreamBuffer::GetBufferSize, "C++: wxStreamBuffer::GetBufferSize() const --> unsigned long");
		cl.def("GetIntPosition", (unsigned long (wxStreamBuffer::*)() const) &wxStreamBuffer::GetIntPosition, "C++: wxStreamBuffer::GetIntPosition() const --> unsigned long");
		cl.def("SetIntPosition", (void (wxStreamBuffer::*)(unsigned long)) &wxStreamBuffer::SetIntPosition, "C++: wxStreamBuffer::SetIntPosition(unsigned long) --> void", pybind11::arg("pos"));
		cl.def("GetLastAccess", (unsigned long (wxStreamBuffer::*)() const) &wxStreamBuffer::GetLastAccess, "C++: wxStreamBuffer::GetLastAccess() const --> unsigned long");
		cl.def("GetBytesLeft", (unsigned long (wxStreamBuffer::*)() const) &wxStreamBuffer::GetBytesLeft, "C++: wxStreamBuffer::GetBytesLeft() const --> unsigned long");
		cl.def("Fixed", (void (wxStreamBuffer::*)(bool)) &wxStreamBuffer::Fixed, "C++: wxStreamBuffer::Fixed(bool) --> void", pybind11::arg("fixed"));
		cl.def("Flushable", (void (wxStreamBuffer::*)(bool)) &wxStreamBuffer::Flushable, "C++: wxStreamBuffer::Flushable(bool) --> void", pybind11::arg("f"));
		cl.def("FlushBuffer", (bool (wxStreamBuffer::*)()) &wxStreamBuffer::FlushBuffer, "C++: wxStreamBuffer::FlushBuffer() --> bool");
		cl.def("FillBuffer", (bool (wxStreamBuffer::*)()) &wxStreamBuffer::FillBuffer, "C++: wxStreamBuffer::FillBuffer() --> bool");
		cl.def("GetDataLeft", (unsigned long (wxStreamBuffer::*)()) &wxStreamBuffer::GetDataLeft, "C++: wxStreamBuffer::GetDataLeft() --> unsigned long");
		cl.def("GetStream", (class wxStreamBase * (wxStreamBuffer::*)() const) &wxStreamBuffer::GetStream, "C++: wxStreamBuffer::GetStream() const --> class wxStreamBase *", pybind11::return_value_policy::automatic);
		cl.def("HasBuffer", (bool (wxStreamBuffer::*)() const) &wxStreamBuffer::HasBuffer, "C++: wxStreamBuffer::HasBuffer() const --> bool");
		cl.def("IsFixed", (bool (wxStreamBuffer::*)() const) &wxStreamBuffer::IsFixed, "C++: wxStreamBuffer::IsFixed() const --> bool");
		cl.def("IsFlushable", (bool (wxStreamBuffer::*)() const) &wxStreamBuffer::IsFlushable, "C++: wxStreamBuffer::IsFlushable() const --> bool");
		cl.def("GetInputStream", (class wxInputStream * (wxStreamBuffer::*)() const) &wxStreamBuffer::GetInputStream, "C++: wxStreamBuffer::GetInputStream() const --> class wxInputStream *", pybind11::return_value_policy::automatic);
		cl.def("GetOutputStream", (class wxOutputStream * (wxStreamBuffer::*)() const) &wxStreamBuffer::GetOutputStream, "C++: wxStreamBuffer::GetOutputStream() const --> class wxOutputStream *", pybind11::return_value_policy::automatic);
	}
	{ // wxBufferedInputStream file: line:551
		pybind11::class_<wxBufferedInputStream, std::shared_ptr<wxBufferedInputStream>, PyCallBack_wxBufferedInputStream, wxFilterInputStream> cl(M(""), "wxBufferedInputStream", "");
		cl.def( pybind11::init( [](class wxInputStream & a0){ return new wxBufferedInputStream(a0); }, [](class wxInputStream & a0){ return new PyCallBack_wxBufferedInputStream(a0); } ), "doc");
		cl.def( pybind11::init<class wxInputStream &, class wxStreamBuffer *>(), pybind11::arg("stream"), pybind11::arg("buffer") );

		cl.def( pybind11::init<class wxInputStream &, unsigned long>(), pybind11::arg("stream"), pybind11::arg("bufsize") );

		cl.def("Peek", (char (wxBufferedInputStream::*)()) &wxBufferedInputStream::Peek, "C++: wxBufferedInputStream::Peek() --> char");
		cl.def("Read", (class wxInputStream & (wxBufferedInputStream::*)(void *, unsigned long)) &wxBufferedInputStream::Read, "C++: wxBufferedInputStream::Read(void *, unsigned long) --> class wxInputStream &", pybind11::return_value_policy::automatic, pybind11::arg("buffer"), pybind11::arg("size"));
		cl.def("SeekI", [](wxBufferedInputStream &o, long const & a0) -> long { return o.SeekI(a0); }, "", pybind11::arg("pos"));
		cl.def("SeekI", (long (wxBufferedInputStream::*)(long, enum wxSeekMode)) &wxBufferedInputStream::SeekI, "C++: wxBufferedInputStream::SeekI(long, enum wxSeekMode) --> long", pybind11::arg("pos"), pybind11::arg("mode"));
		cl.def("TellI", (long (wxBufferedInputStream::*)() const) &wxBufferedInputStream::TellI, "C++: wxBufferedInputStream::TellI() const --> long");
		cl.def("IsSeekable", (bool (wxBufferedInputStream::*)() const) &wxBufferedInputStream::IsSeekable, "C++: wxBufferedInputStream::IsSeekable() const --> bool");
		cl.def("SetInputStreamBuffer", (void (wxBufferedInputStream::*)(class wxStreamBuffer *)) &wxBufferedInputStream::SetInputStreamBuffer, "C++: wxBufferedInputStream::SetInputStreamBuffer(class wxStreamBuffer *) --> void", pybind11::arg("buffer"));
		cl.def("GetInputStreamBuffer", (class wxStreamBuffer * (wxBufferedInputStream::*)() const) &wxBufferedInputStream::GetInputStreamBuffer, "C++: wxBufferedInputStream::GetInputStreamBuffer() const --> class wxStreamBuffer *", pybind11::return_value_policy::automatic);
	}
	{ // wxBufferedOutputStream file: line:600
		pybind11::class_<wxBufferedOutputStream, std::shared_ptr<wxBufferedOutputStream>, PyCallBack_wxBufferedOutputStream, wxFilterOutputStream> cl(M(""), "wxBufferedOutputStream", "");
		cl.def( pybind11::init( [](class wxOutputStream & a0){ return new wxBufferedOutputStream(a0); }, [](class wxOutputStream & a0){ return new PyCallBack_wxBufferedOutputStream(a0); } ), "doc");
		cl.def( pybind11::init<class wxOutputStream &, class wxStreamBuffer *>(), pybind11::arg("stream"), pybind11::arg("buffer") );

		cl.def( pybind11::init<class wxOutputStream &, unsigned long>(), pybind11::arg("stream"), pybind11::arg("bufsize") );

		cl.def("Write", (class wxOutputStream & (wxBufferedOutputStream::*)(const void *, unsigned long)) &wxBufferedOutputStream::Write, "C++: wxBufferedOutputStream::Write(const void *, unsigned long) --> class wxOutputStream &", pybind11::return_value_policy::automatic, pybind11::arg("buffer"), pybind11::arg("size"));
		cl.def("SeekO", [](wxBufferedOutputStream &o, long const & a0) -> long { return o.SeekO(a0); }, "", pybind11::arg("pos"));
		cl.def("SeekO", (long (wxBufferedOutputStream::*)(long, enum wxSeekMode)) &wxBufferedOutputStream::SeekO, "C++: wxBufferedOutputStream::SeekO(long, enum wxSeekMode) --> long", pybind11::arg("pos"), pybind11::arg("mode"));
		cl.def("TellO", (long (wxBufferedOutputStream::*)() const) &wxBufferedOutputStream::TellO, "C++: wxBufferedOutputStream::TellO() const --> long");
		cl.def("IsSeekable", (bool (wxBufferedOutputStream::*)() const) &wxBufferedOutputStream::IsSeekable, "C++: wxBufferedOutputStream::IsSeekable() const --> bool");
		cl.def("Sync", (void (wxBufferedOutputStream::*)()) &wxBufferedOutputStream::Sync, "C++: wxBufferedOutputStream::Sync() --> void");
		cl.def("Close", (bool (wxBufferedOutputStream::*)()) &wxBufferedOutputStream::Close, "C++: wxBufferedOutputStream::Close() --> bool");
		cl.def("GetLength", (long (wxBufferedOutputStream::*)() const) &wxBufferedOutputStream::GetLength, "C++: wxBufferedOutputStream::GetLength() const --> long");
		cl.def("SetOutputStreamBuffer", (void (wxBufferedOutputStream::*)(class wxStreamBuffer *)) &wxBufferedOutputStream::SetOutputStreamBuffer, "C++: wxBufferedOutputStream::SetOutputStreamBuffer(class wxStreamBuffer *) --> void", pybind11::arg("buffer"));
		cl.def("GetOutputStreamBuffer", (class wxStreamBuffer * (wxBufferedOutputStream::*)() const) &wxBufferedOutputStream::GetOutputStreamBuffer, "C++: wxBufferedOutputStream::GetOutputStreamBuffer() const --> class wxStreamBuffer *", pybind11::return_value_policy::automatic);
	}
	{ // wxWrapperInputStream file: line:658
		pybind11::class_<wxWrapperInputStream, std::shared_ptr<wxWrapperInputStream>, PyCallBack_wxWrapperInputStream, wxFilterInputStream> cl(M(""), "wxWrapperInputStream", "");
		cl.def( pybind11::init<class wxInputStream &>(), pybind11::arg("stream") );

		cl.def( pybind11::init<class wxInputStream *>(), pybind11::arg("stream") );

		cl.def("GetLength", (long (wxWrapperInputStream::*)() const) &wxWrapperInputStream::GetLength, "C++: wxWrapperInputStream::GetLength() const --> long");
		cl.def("IsSeekable", (bool (wxWrapperInputStream::*)() const) &wxWrapperInputStream::IsSeekable, "C++: wxWrapperInputStream::IsSeekable() const --> bool");
	}
	// wxImageResolution file: line:48
	pybind11::enum_<wxImageResolution>(M(""), "wxImageResolution", pybind11::arithmetic(), "")
		.value("wxIMAGE_RESOLUTION_NONE", wxIMAGE_RESOLUTION_NONE)
		.value("wxIMAGE_RESOLUTION_INCHES", wxIMAGE_RESOLUTION_INCHES)
		.value("wxIMAGE_RESOLUTION_CM", wxIMAGE_RESOLUTION_CM)
		.export_values();

;

	// wxImageResizeQuality file: line:61
	pybind11::enum_<wxImageResizeQuality>(M(""), "wxImageResizeQuality", pybind11::arithmetic(), "")
		.value("wxIMAGE_QUALITY_NEAREST", wxIMAGE_QUALITY_NEAREST)
		.value("wxIMAGE_QUALITY_BILINEAR", wxIMAGE_QUALITY_BILINEAR)
		.value("wxIMAGE_QUALITY_BICUBIC", wxIMAGE_QUALITY_BICUBIC)
		.value("wxIMAGE_QUALITY_BOX_AVERAGE", wxIMAGE_QUALITY_BOX_AVERAGE)
		.value("wxIMAGE_QUALITY_NORMAL", wxIMAGE_QUALITY_NORMAL)
		.value("wxIMAGE_QUALITY_HIGH", wxIMAGE_QUALITY_HIGH)
		.export_values();

;

}
