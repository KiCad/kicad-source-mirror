This directory contains modified context switching code from the Boost library.

It is licensed under the Boost Software License, with the license text in this directory.

The reasons for creating a custom libcontext/coroutines library instead of using the Boost versions are described in several places. It was first introduced and discussed on the mailing list here: https://www.mail-archive.com/kicad-developers@lists.launchpad.net/msg23327.html


From https://gitlab.com/kicad/code/kicad/-/issues/237:

---------------

@ Tom, from reading the list messages it sounds like this was an ABI incompatibility (not API?) resulting from boost wanting MSVC build chain.

If that's the case, could we keep libcontext for MSW only?  That would resolve the platform issue as the *nix builds could use libboost context. We keep the API, so there's no KiCad changes.

---------------

@ Seth

There were multiple critical elements:
- the issue you mentioned was about building boost::context under mingw without having to get proprietary binaries for MASM/MASM64 (Microsoft Macro Assembler). This was fixed by Boost at some point in 2015.
- boost::context API is unstable - that is the definitions of jump_fcontext and make_fcontext change from time to time. We had to put a number of #ifdefs in KiCad coroutine library to accommodate for different versions of Boost.
- In the latest versions of Boost (at least 1.61 and newer), the fcontext api is no longer public (detail namespace). Therefore there's no guarantee of its stability. The now-official boost coroutine library is very complex and incomprehensible. Linking against a non-public API is not a good idea IMHO.
- Last but not least, since we moved to C++11, most features that used to be provided by Boost are now in the C++ standard library. I'd rather use C++11-features only in the longer run than rely on Boost.

All of the above forced me to develop libcontext, which is merely a two-file version of boost::context, including all original copyrights. I made it only for platforms I could test the library with.

@ tijuca: See above. We support x86, x86_64, ARM and ARM64 for now (although I've never met any KiCad user running it on an ARM box). I've never seen anyone trying to run Kicad on a PPC/Mips machine. As you correctly pointed out, armv5-t is a sort of historical instruction set by today's standards, so there's no need to worry about it. For other exotic platforms like PPC, SPARC, DEC Alpha, MIPS, PDP-10, old IBM mainframe or Commodore 64, I guess if someone still manages to use such a machine, writing a context switch function should be trivial to him/her.

Tom

----------------------------
From https://www.mail-archive.com/kicad-developers@lists.launchpad.net/msg31321.html

If we could depend on boost to stop changing their coroutine code every couple of version then we wouldn't have to fork it and maintain our own version.  Unfortunately, history has proven otherwise.  Your change would be the forth time this code as been changed to accommodate boost. The short answer is yes, we actually need our own coroutine code because we cannot depend on boost to not cut us off at the knees every few versions.  Maybe when couroutines are available in the standard c++ library, then we can get rid of our library.  Please do not change coroutine code without a really good reason.
