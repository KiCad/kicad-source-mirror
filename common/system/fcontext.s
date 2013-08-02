/*
	Boost::Context assembly wrapper - done to avoid compiling the whole boost binary library
	which may be unpleasant, in particular under Windows (we don't support VC++, while boost::context
	does not support mingw */


#if __i386__

    #ifdef __WIN32__
        #include "jump_i386_pe_gas.S"
        #include "make_i386_pe_gas.S"
    #else
        #include "jump_i386_sysv_elf_gas.S"
        #include "make_i386_sysv_elf_gas.S"
    #endif

#elif __x86_64__
    #include "jump_x86_64_sysv_elf_gas.S"
    #include "make_x86_64_sysv_elf_gas.S"
#endif

