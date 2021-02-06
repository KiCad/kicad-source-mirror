#include <bits/dirent.h> // dirent
#include <bits/dirent.h> // dirent64
#include <bits/dirent_ext.h> // getdents64
#include <bits/fcntl-linux.h> // __pid_type
#include <bits/fcntl-linux.h> // f_owner_ex
#include <bits/fcntl-linux.h> // fallocate
#include <bits/fcntl-linux.h> // fallocate64
#include <bits/fcntl-linux.h> // file_handle
#include <bits/fcntl-linux.h> // name_to_handle_at
#include <bits/fcntl-linux.h> // open_by_handle_at
#include <bits/fcntl-linux.h> // readahead
#include <bits/fcntl-linux.h> // splice
#include <bits/fcntl-linux.h> // sync_file_range
#include <bits/fcntl-linux.h> // tee
#include <bits/fcntl-linux.h> // vmsplice
#include <bits/fcntl.h> // flock
#include <bits/fcntl.h> // flock64
#include <bits/stat.h> // stat
#include <bits/stat.h> // stat64
#include <bits/statx-generic.h> // statx
#include <bits/types/struct_iovec.h> // iovec
#include <bits/unistd_ext.h> // gettid
#include <dirent.h> // getdirentries
#include <dirent.h> // getdirentries64
#include <fcntl.h> // creat
#include <fcntl.h> // creat64
#include <fcntl.h> // fcntl
#include <fcntl.h> // fcntl64
#include <fcntl.h> // open
#include <fcntl.h> // open64
#include <fcntl.h> // openat
#include <fcntl.h> // openat64
#include <fcntl.h> // posix_fadvise
#include <fcntl.h> // posix_fadvise64
#include <fcntl.h> // posix_fallocate
#include <fcntl.h> // posix_fallocate64
#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <sys/stat.h> // __fxstat
#include <sys/stat.h> // __fxstat64
#include <sys/stat.h> // __fxstatat
#include <sys/stat.h> // __fxstatat64
#include <sys/stat.h> // __lxstat
#include <sys/stat.h> // __lxstat64
#include <sys/stat.h> // __xmknod
#include <sys/stat.h> // __xmknodat
#include <sys/stat.h> // __xstat
#include <sys/stat.h> // __xstat64
#include <sys/stat.h> // chmod
#include <sys/stat.h> // fchmod
#include <sys/stat.h> // fchmodat
#include <sys/stat.h> // fstat
#include <sys/stat.h> // fstat64
#include <sys/stat.h> // fstatat
#include <sys/stat.h> // fstatat64
#include <sys/stat.h> // getumask
#include <sys/stat.h> // lchmod
#include <sys/stat.h> // lstat
#include <sys/stat.h> // lstat64
#include <sys/stat.h> // mkdir
#include <sys/stat.h> // mkdirat
#include <sys/stat.h> // mkfifo
#include <sys/stat.h> // mkfifoat
#include <sys/stat.h> // mknod
#include <sys/stat.h> // mknodat
#include <sys/stat.h> // stat
#include <sys/stat.h> // stat64
#include <sys/stat.h> // umask
#include <unistd.h> // __getpgid
#include <unistd.h> // _exit
#include <unistd.h> // access
#include <unistd.h> // acct
#include <unistd.h> // alarm
#include <unistd.h> // brk
#include <unistd.h> // chdir
#include <unistd.h> // chown
#include <unistd.h> // chroot
#include <unistd.h> // close
#include <unistd.h> // confstr
#include <unistd.h> // copy_file_range
#include <unistd.h> // crypt
#include <unistd.h> // daemon
#include <unistd.h> // dup
#include <unistd.h> // dup2
#include <unistd.h> // dup3
#include <unistd.h> // eaccess
#include <unistd.h> // endusershell
#include <unistd.h> // euidaccess
#include <unistd.h> // execl
#include <unistd.h> // execle
#include <unistd.h> // execlp
#include <unistd.h> // faccessat
#include <unistd.h> // fchdir
#include <unistd.h> // fchown
#include <unistd.h> // fchownat
#include <unistd.h> // fdatasync
#include <unistd.h> // fork
#include <unistd.h> // fpathconf
#include <unistd.h> // fsync
#include <unistd.h> // ftruncate
#include <unistd.h> // ftruncate64
#include <unistd.h> // get_current_dir_name
#include <unistd.h> // getcwd
#include <unistd.h> // getdomainname
#include <unistd.h> // getdtablesize
#include <unistd.h> // getegid
#include <unistd.h> // getentropy
#include <unistd.h> // geteuid
#include <unistd.h> // getgid
#include <unistd.h> // gethostid
#include <unistd.h> // gethostname
#include <unistd.h> // getlogin
#include <unistd.h> // getlogin_r
#include <unistd.h> // getpagesize
#include <unistd.h> // getpass
#include <unistd.h> // getpgid
#include <unistd.h> // getpgrp
#include <unistd.h> // getpid
#include <unistd.h> // getppid
#include <unistd.h> // getresgid
#include <unistd.h> // getresuid
#include <unistd.h> // getsid
#include <unistd.h> // getuid
#include <unistd.h> // getusershell
#include <unistd.h> // getwd
#include <unistd.h> // group_member
#include <unistd.h> // isatty
#include <unistd.h> // lchown
#include <unistd.h> // link
#include <unistd.h> // linkat
#include <unistd.h> // lockf
#include <unistd.h> // lockf64
#include <unistd.h> // lseek
#include <unistd.h> // lseek64
#include <unistd.h> // nice
#include <unistd.h> // pathconf
#include <unistd.h> // pause
#include <unistd.h> // pread
#include <unistd.h> // pread64
#include <unistd.h> // profil
#include <unistd.h> // pwrite
#include <unistd.h> // pwrite64
#include <unistd.h> // read
#include <unistd.h> // readlink
#include <unistd.h> // readlinkat
#include <unistd.h> // revoke
#include <unistd.h> // rmdir
#include <unistd.h> // sbrk
#include <unistd.h> // setdomainname
#include <unistd.h> // setegid
#include <unistd.h> // seteuid
#include <unistd.h> // setgid
#include <unistd.h> // sethostid
#include <unistd.h> // sethostname
#include <unistd.h> // setlogin
#include <unistd.h> // setpgid
#include <unistd.h> // setpgrp
#include <unistd.h> // setregid
#include <unistd.h> // setresgid
#include <unistd.h> // setresuid
#include <unistd.h> // setreuid
#include <unistd.h> // setsid
#include <unistd.h> // setuid
#include <unistd.h> // setusershell
#include <unistd.h> // sleep
#include <unistd.h> // swab
#include <unistd.h> // symlink
#include <unistd.h> // symlinkat
#include <unistd.h> // sync
#include <unistd.h> // syncfs
#include <unistd.h> // syscall
#include <unistd.h> // sysconf
#include <unistd.h> // tcgetpgrp
#include <unistd.h> // tcsetpgrp
#include <unistd.h> // truncate
#include <unistd.h> // truncate64
#include <unistd.h> // ttyname
#include <unistd.h> // ttyname_r
#include <unistd.h> // ttyslot
#include <unistd.h> // ualarm
#include <unistd.h> // unlink
#include <unistd.h> // unlinkat
#include <unistd.h> // usleep
#include <unistd.h> // vfork
#include <unistd.h> // vhangup
#include <unistd.h> // write

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_35(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxJoin(const class wxArrayString &, const wchar_t, const wchar_t) file: line:433
	M("").def("wxJoin", [](const class wxArrayString & a0, const wchar_t & a1) -> wxString { return wxJoin(a0, a1); }, "", pybind11::arg("arr"), pybind11::arg("sep"));
	M("").def("wxJoin", (class wxString (*)(const class wxArrayString &, const wchar_t, const wchar_t)) &wxJoin, "C++: wxJoin(const class wxArrayString &, const wchar_t, const wchar_t) --> class wxString", pybind11::arg("arr"), pybind11::arg("sep"), pybind11::arg("escape"));

	// wxSplit(const class wxString &, const wchar_t, const wchar_t) file: line:437
	M("").def("wxSplit", [](const class wxString & a0, const wchar_t & a1) -> wxArrayString { return wxSplit(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("sep"));
	M("").def("wxSplit", (class wxArrayString (*)(const class wxString &, const wchar_t, const wchar_t)) &wxSplit, "C++: wxSplit(const class wxString &, const wchar_t, const wchar_t) --> class wxArrayString", pybind11::arg("str"), pybind11::arg("sep"), pybind11::arg("escape"));

	{ // wxArrayStringsAdapter file: line:456
		pybind11::class_<wxArrayStringsAdapter, std::shared_ptr<wxArrayStringsAdapter>> cl(M(""), "wxArrayStringsAdapter", "");
		cl.def( pybind11::init<const class wxArrayString &>(), pybind11::arg("strings") );

		cl.def( pybind11::init<unsigned int, const class wxString *>(), pybind11::arg("n"), pybind11::arg("strings") );

		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("s") );

		cl.def("GetCount", (unsigned long (wxArrayStringsAdapter::*)() const) &wxArrayStringsAdapter::GetCount, "C++: wxArrayStringsAdapter::GetCount() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxArrayStringsAdapter::*)() const) &wxArrayStringsAdapter::IsEmpty, "C++: wxArrayStringsAdapter::IsEmpty() const --> bool");
		cl.def("__getitem__", (const class wxString & (wxArrayStringsAdapter::*)(unsigned int) const) &wxArrayStringsAdapter::operator[], "C++: wxArrayStringsAdapter::operator[](unsigned int) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("AsArrayString", (class wxArrayString (wxArrayStringsAdapter::*)() const) &wxArrayStringsAdapter::AsArrayString, "C++: wxArrayStringsAdapter::AsArrayString() const --> class wxArrayString");
	}
	{ // stat file:bits/stat.h line:46
		pybind11::class_<stat, std::shared_ptr<stat>> cl(M(""), "stat", "");
		cl.def( pybind11::init( [](){ return new stat(); } ) );
		cl.def_readwrite("st_dev", &stat::st_dev);
		cl.def_readwrite("st_ino", &stat::st_ino);
		cl.def_readwrite("st_nlink", &stat::st_nlink);
		cl.def_readwrite("st_mode", &stat::st_mode);
		cl.def_readwrite("st_uid", &stat::st_uid);
		cl.def_readwrite("st_gid", &stat::st_gid);
		cl.def_readwrite("__pad0", &stat::__pad0);
		cl.def_readwrite("st_rdev", &stat::st_rdev);
		cl.def_readwrite("st_size", &stat::st_size);
		cl.def_readwrite("st_blksize", &stat::st_blksize);
		cl.def_readwrite("st_blocks", &stat::st_blocks);
		cl.def_readwrite("st_atim", &stat::st_atim);
		cl.def_readwrite("st_mtim", &stat::st_mtim);
		cl.def_readwrite("st_ctim", &stat::st_ctim);
	}
	// wxSeekMode file: line:101
	pybind11::enum_<wxSeekMode>(M(""), "wxSeekMode", pybind11::arithmetic(), "")
		.value("wxFromStart", wxFromStart)
		.value("wxFromCurrent", wxFromCurrent)
		.value("wxFromEnd", wxFromEnd)
		.export_values();

;

	// wxFileKind file: line:108
	pybind11::enum_<wxFileKind>(M(""), "wxFileKind", pybind11::arithmetic(), "")
		.value("wxFILE_KIND_UNKNOWN", wxFILE_KIND_UNKNOWN)
		.value("wxFILE_KIND_DISK", wxFILE_KIND_DISK)
		.value("wxFILE_KIND_TERMINAL", wxFILE_KIND_TERMINAL)
		.value("wxFILE_KIND_PIPE", wxFILE_KIND_PIPE)
		.export_values();

;

	// wxPosixPermissions file: line:118
	pybind11::enum_<wxPosixPermissions>(M(""), "wxPosixPermissions", pybind11::arithmetic(), "")
		.value("wxS_IRUSR", wxS_IRUSR)
		.value("wxS_IWUSR", wxS_IWUSR)
		.value("wxS_IXUSR", wxS_IXUSR)
		.value("wxS_IRGRP", wxS_IRGRP)
		.value("wxS_IWGRP", wxS_IWGRP)
		.value("wxS_IXGRP", wxS_IXGRP)
		.value("wxS_IROTH", wxS_IROTH)
		.value("wxS_IWOTH", wxS_IWOTH)
		.value("wxS_IXOTH", wxS_IXOTH)
		.value("wxPOSIX_USER_READ", wxPOSIX_USER_READ)
		.value("wxPOSIX_USER_WRITE", wxPOSIX_USER_WRITE)
		.value("wxPOSIX_USER_EXECUTE", wxPOSIX_USER_EXECUTE)
		.value("wxPOSIX_GROUP_READ", wxPOSIX_GROUP_READ)
		.value("wxPOSIX_GROUP_WRITE", wxPOSIX_GROUP_WRITE)
		.value("wxPOSIX_GROUP_EXECUTE", wxPOSIX_GROUP_EXECUTE)
		.value("wxPOSIX_OTHERS_READ", wxPOSIX_OTHERS_READ)
		.value("wxPOSIX_OTHERS_WRITE", wxPOSIX_OTHERS_WRITE)
		.value("wxPOSIX_OTHERS_EXECUTE", wxPOSIX_OTHERS_EXECUTE)
		.value("wxS_DEFAULT", wxS_DEFAULT)
		.value("wxS_DIR_DEFAULT", wxS_DIR_DEFAULT)
		.export_values();

;

	{ // wxAssert_472 file: line:118
		pybind11::class_<wxAssert_472, std::shared_ptr<wxAssert_472>> cl(M(""), "wxAssert_472", "");
		cl.def( pybind11::init( [](){ return new wxAssert_472(); } ) );
		cl.def_readwrite("BadFileSizeType", &wxAssert_472::BadFileSizeType);
	}
	// wxAccess(const class wxString &, unsigned int) file: line:527
	M("").def("wxAccess", (int (*)(const class wxString &, unsigned int)) &wxAccess, "C++: wxAccess(const class wxString &, unsigned int) --> int", pybind11::arg("path"), pybind11::arg("mode"));

	// wxChmod(const class wxString &, unsigned int) file: line:529
	M("").def("wxChmod", (int (*)(const class wxString &, unsigned int)) &wxChmod, "C++: wxChmod(const class wxString &, unsigned int) --> int", pybind11::arg("path"), pybind11::arg("mode"));

	// wxOpen(const class wxString &, int, unsigned int) file: line:531
	M("").def("wxOpen", (int (*)(const class wxString &, int, unsigned int)) &wxOpen, "C++: wxOpen(const class wxString &, int, unsigned int) --> int", pybind11::arg("path"), pybind11::arg("flags"), pybind11::arg("mode"));

	// wxStat(const class wxString &, struct stat *) file: line:536
	M("").def("wxStat", (int (*)(const class wxString &, struct stat *)) &wxStat, "C++: wxStat(const class wxString &, struct stat *) --> int", pybind11::arg("path"), pybind11::arg("buf"));

	// wxLstat(const class wxString &, struct stat *) file: line:538
	M("").def("wxLstat", (int (*)(const class wxString &, struct stat *)) &wxLstat, "C++: wxLstat(const class wxString &, struct stat *) --> int", pybind11::arg("path"), pybind11::arg("buf"));

	// wxRmDir(const class wxString &) file: line:540
	M("").def("wxRmDir", (int (*)(const class wxString &)) &wxRmDir, "C++: wxRmDir(const class wxString &) --> int", pybind11::arg("path"));

	// wxMkDir(const class wxString &, unsigned int) file: line:547
	M("").def("wxMkDir", (int (*)(const class wxString &, unsigned int)) &wxMkDir, "C++: wxMkDir(const class wxString &, unsigned int) --> int", pybind11::arg("path"), pybind11::arg("mode"));

	// wxFileExists(const class wxString &) file: line:571
	M("").def("wxFileExists", (bool (*)(const class wxString &)) &wxFileExists, "C++: wxFileExists(const class wxString &) --> bool", pybind11::arg("filename"));

	// wxDirExists(const class wxString &) file: line:574
	M("").def("wxDirExists", (bool (*)(const class wxString &)) &wxDirExists, "C++: wxDirExists(const class wxString &) --> bool", pybind11::arg("pathName"));

	// wxIsAbsolutePath(const class wxString &) file: line:576
	M("").def("wxIsAbsolutePath", (bool (*)(const class wxString &)) &wxIsAbsolutePath, "C++: wxIsAbsolutePath(const class wxString &) --> bool", pybind11::arg("filename"));

	// wxFileNameFromPath(wchar_t *) file: line:579
	M("").def("wxFileNameFromPath", (wchar_t * (*)(wchar_t *)) &wxFileNameFromPath, "C++: wxFileNameFromPath(wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("path"));

	// wxFileNameFromPath(const class wxString &) file: line:580
	M("").def("wxFileNameFromPath", (class wxString (*)(const class wxString &)) &wxFileNameFromPath, "C++: wxFileNameFromPath(const class wxString &) --> class wxString", pybind11::arg("path"));

	// wxPathOnly(const class wxString &) file: line:583
	M("").def("wxPathOnly", (class wxString (*)(const class wxString &)) &wxPathOnly, "C++: wxPathOnly(const class wxString &) --> class wxString", pybind11::arg("path"));

	// wxDos2UnixFilename(char *) file: line:588
	M("").def("wxDos2UnixFilename", (void (*)(char *)) &wxDos2UnixFilename, "C++: wxDos2UnixFilename(char *) --> void", pybind11::arg("s"));

	// wxDos2UnixFilename(wchar_t *) file: line:589
	M("").def("wxDos2UnixFilename", (void (*)(wchar_t *)) &wxDos2UnixFilename, "C++: wxDos2UnixFilename(wchar_t *) --> void", pybind11::arg("s"));

	// wxUnix2DosFilename(char *) file: line:592
	M("").def("wxUnix2DosFilename", (void (*)(char *)) &wxUnix2DosFilename, "C++: wxUnix2DosFilename(char *) --> void", pybind11::arg("s"));

	// wxUnix2DosFilename(wchar_t *) file: line:594
	M("").def("wxUnix2DosFilename", (void (*)(wchar_t *)) &wxUnix2DosFilename, "C++: wxUnix2DosFilename(wchar_t *) --> void", pybind11::arg("s"));

	// wxStripExtension(char *) file: line:599
	M("").def("wxStripExtension", (void (*)(char *)) &wxStripExtension, "C++: wxStripExtension(char *) --> void", pybind11::arg("buffer"));

	// wxStripExtension(wchar_t *) file: line:600
	M("").def("wxStripExtension", (void (*)(wchar_t *)) &wxStripExtension, "C++: wxStripExtension(wchar_t *) --> void", pybind11::arg("buffer"));

	// wxStripExtension(class wxString &) file: line:601
	M("").def("wxStripExtension", (void (*)(class wxString &)) &wxStripExtension, "C++: wxStripExtension(class wxString &) --> void", pybind11::arg("buffer"));

	// wxGetTempFileName(const class wxString &, wchar_t *) file: line:604
	M("").def("wxGetTempFileName", [](const class wxString & a0) -> wchar_t * { return wxGetTempFileName(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("prefix"));
	M("").def("wxGetTempFileName", (wchar_t * (*)(const class wxString &, wchar_t *)) &wxGetTempFileName, "C++: wxGetTempFileName(const class wxString &, wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("prefix"), pybind11::arg("buf"));

	// wxGetTempFileName(const class wxString &, class wxString &) file: line:605
	M("").def("wxGetTempFileName", (bool (*)(const class wxString &, class wxString &)) &wxGetTempFileName, "C++: wxGetTempFileName(const class wxString &, class wxString &) --> bool", pybind11::arg("prefix"), pybind11::arg("buf"));

	// wxExpandPath(char *, const class wxString &) file: line:608
	M("").def("wxExpandPath", (char * (*)(char *, const class wxString &)) &wxExpandPath, "C++: wxExpandPath(char *, const class wxString &) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("path"));

	// wxExpandPath(wchar_t *, const class wxString &) file: line:609
	M("").def("wxExpandPath", (wchar_t * (*)(wchar_t *, const class wxString &)) &wxExpandPath, "C++: wxExpandPath(wchar_t *, const class wxString &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("dest"), pybind11::arg("path"));

	// wxContractPath(const class wxString &, const class wxString &, const class wxString &) file: line:616
	M("").def("wxContractPath", [](const class wxString & a0) -> wchar_t * { return wxContractPath(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("filename"));
	M("").def("wxContractPath", [](const class wxString & a0, const class wxString & a1) -> wchar_t * { return wxContractPath(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("filename"), pybind11::arg("envname"));
	M("").def("wxContractPath", (wchar_t * (*)(const class wxString &, const class wxString &, const class wxString &)) &wxContractPath, "C++: wxContractPath(const class wxString &, const class wxString &, const class wxString &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("filename"), pybind11::arg("envname"), pybind11::arg("user"));

	// wxRealPath(char *) file: line:622
	M("").def("wxRealPath", (char * (*)(char *)) &wxRealPath, "C++: wxRealPath(char *) --> char *", pybind11::return_value_policy::automatic, pybind11::arg("path"));

	// wxRealPath(wchar_t *) file: line:623
	M("").def("wxRealPath", (wchar_t * (*)(wchar_t *)) &wxRealPath, "C++: wxRealPath(wchar_t *) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("path"));

	// wxRealPath(const class wxString &) file: line:624
	M("").def("wxRealPath", (class wxString (*)(const class wxString &)) &wxRealPath, "C++: wxRealPath(const class wxString &) --> class wxString", pybind11::arg("path"));

	// wxCopyAbsolutePath(const class wxString &) file: line:628
	M("").def("wxCopyAbsolutePath", (wchar_t * (*)(const class wxString &)) &wxCopyAbsolutePath, "C++: wxCopyAbsolutePath(const class wxString &) --> wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("path"));

	// wxFindFirstFile(const class wxString &, int) file: line:636
	M("").def("wxFindFirstFile", [](const class wxString & a0) -> wxString { return wxFindFirstFile(a0); }, "", pybind11::arg("spec"));
	M("").def("wxFindFirstFile", (class wxString (*)(const class wxString &, int)) &wxFindFirstFile, "C++: wxFindFirstFile(const class wxString &, int) --> class wxString", pybind11::arg("spec"), pybind11::arg("flags"));

	// wxFindNextFile() file: line:637
	M("").def("wxFindNextFile", (class wxString (*)()) &wxFindNextFile, "C++: wxFindNextFile() --> class wxString");

	// wxIsWild(const class wxString &) file: line:640
	M("").def("wxIsWild", (bool (*)(const class wxString &)) &wxIsWild, "C++: wxIsWild(const class wxString &) --> bool", pybind11::arg("pattern"));

	// wxMatchWild(const class wxString &, const class wxString &, bool) file: line:645
	M("").def("wxMatchWild", [](const class wxString & a0, const class wxString & a1) -> bool { return wxMatchWild(a0, a1); }, "", pybind11::arg("pattern"), pybind11::arg("text"));
	M("").def("wxMatchWild", (bool (*)(const class wxString &, const class wxString &, bool)) &wxMatchWild, "C++: wxMatchWild(const class wxString &, const class wxString &, bool) --> bool", pybind11::arg("pattern"), pybind11::arg("text"), pybind11::arg("dot_special"));

	// wxConcatFiles(const class wxString &, const class wxString &, const class wxString &) file: line:648
	M("").def("wxConcatFiles", (bool (*)(const class wxString &, const class wxString &, const class wxString &)) &wxConcatFiles, "C++: wxConcatFiles(const class wxString &, const class wxString &, const class wxString &) --> bool", pybind11::arg("file1"), pybind11::arg("file2"), pybind11::arg("file3"));

	// wxCopyFile(const class wxString &, const class wxString &, bool) file: line:651
	M("").def("wxCopyFile", [](const class wxString & a0, const class wxString & a1) -> bool { return wxCopyFile(a0, a1); }, "", pybind11::arg("file1"), pybind11::arg("file2"));
	M("").def("wxCopyFile", (bool (*)(const class wxString &, const class wxString &, bool)) &wxCopyFile, "C++: wxCopyFile(const class wxString &, const class wxString &, bool) --> bool", pybind11::arg("file1"), pybind11::arg("file2"), pybind11::arg("overwrite"));

	// wxRemoveFile(const class wxString &) file: line:655
	M("").def("wxRemoveFile", (bool (*)(const class wxString &)) &wxRemoveFile, "C++: wxRemoveFile(const class wxString &) --> bool", pybind11::arg("file"));

	// wxRenameFile(const class wxString &, const class wxString &, bool) file: line:658
	M("").def("wxRenameFile", [](const class wxString & a0, const class wxString & a1) -> bool { return wxRenameFile(a0, a1); }, "", pybind11::arg("file1"), pybind11::arg("file2"));
	M("").def("wxRenameFile", (bool (*)(const class wxString &, const class wxString &, bool)) &wxRenameFile, "C++: wxRenameFile(const class wxString &, const class wxString &, bool) --> bool", pybind11::arg("file1"), pybind11::arg("file2"), pybind11::arg("overwrite"));

	// wxGetCwd() file: line:670
	M("").def("wxGetCwd", (class wxString (*)()) &wxGetCwd, "C++: wxGetCwd() --> class wxString");

	// wxSetWorkingDirectory(const class wxString &) file: line:673
	M("").def("wxSetWorkingDirectory", (bool (*)(const class wxString &)) &wxSetWorkingDirectory, "C++: wxSetWorkingDirectory(const class wxString &) --> bool", pybind11::arg("d"));

	// wxMkdir(const class wxString &, int) file: line:676
	M("").def("wxMkdir", [](const class wxString & a0) -> bool { return wxMkdir(a0); }, "", pybind11::arg("dir"));
	M("").def("wxMkdir", (bool (*)(const class wxString &, int)) &wxMkdir, "C++: wxMkdir(const class wxString &, int) --> bool", pybind11::arg("dir"), pybind11::arg("perm"));

}
