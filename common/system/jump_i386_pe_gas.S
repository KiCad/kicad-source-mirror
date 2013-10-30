/*           Copyright Oliver Kowalke 2009.
  Distributed under the Boost Software License, Version 1.0.
     (See accompanying file LICENSE_1_0.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
*/

.global _jump_fcontext
.text
.align 2

_jump_fcontext:
    mov    0x4(%esp),%ecx
    mov    %edi,(%ecx)
    mov    %esi,0x4(%ecx)
    mov    %ebx,0x8(%ecx)
    mov    %ebp,0xc(%ecx)
    mov    %fs:0x18,%edx
    mov    (%edx),%eax
    mov    %eax,0x24(%ecx)
    mov    0x4(%edx),%eax
    mov    %eax,0x18(%ecx)
    mov    0x8(%edx),%eax
    mov    %eax,0x20(%ecx)
    mov    0x10(%edx),%eax
    mov    %eax,0x28(%ecx)
    lea    0x4(%esp),%eax
    mov    %eax,0x10(%ecx)
    mov    (%esp),%eax
    mov    %eax,0x14(%ecx)
    mov    0x8(%esp),%edx
    mov    (%edx),%edi
    mov    0x4(%edx),%esi
    mov    0x8(%edx),%ebx
    mov    0xc(%edx),%ebp
    mov    0x10(%esp),%eax
    test   %eax,%eax
    je     _jump_fcontext+0x5f
    stmxcsr 0x2c(%ecx)
    fnstcw 0x30(%ecx)
    ldmxcsr 0x2c(%edx)
    fldcw  0x30(%edx)
    mov    %edx,%ecx
    mov    %fs:0x18,%edx
    mov    0x24(%ecx),%eax
    mov    %eax,(%edx)
    mov    0x18(%ecx),%eax
    mov    %eax,0x4(%edx)
    mov    0x20(%ecx),%eax
    mov    %eax,0x8(%edx)
    mov    0x28(%ecx),%eax
    mov    %eax,0x10(%edx)
    mov    0xc(%esp),%eax
    mov    0x10(%ecx),%esp
    mov    %eax,0x4(%esp)
    mov    0x14(%ecx),%ecx
    jmp    *%ecx
