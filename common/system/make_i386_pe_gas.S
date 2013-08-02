/*           Copyright Oliver Kowalke 2009.
  Distributed under the Boost Software License, Version 1.0.
     (See accompanying file LICENSE_1_0.txt or copy at
           http://www.boost.org/LICENSE_1_0.txt)
*/

.global _make_fcontext
.text
.align 2

_make_fcontext:
	mov    0x4(%esp),%eax
	lea    -0x34(%eax),%eax
	and    $0xfffffff0,%eax
	mov    0x4(%esp),%ecx
	mov    %ecx,0x18(%eax)
	mov    0x8(%esp),%edx
	mov    %edx,0x1c(%eax)
	neg    %edx
	lea    (%edx,%ecx,1),%ecx
	mov    %ecx,0x20(%eax)
	mov    0xc(%esp),%ecx
	mov    %ecx,0x14(%eax)
	stmxcsr 0x2c(%eax)
	fnstcw 0x30(%eax)
	lea    -0x1c(%eax),%edx
	mov    %edx,0x10(%eax)
	mov    $0x0,%ecx
	mov    %ecx,(%edx)
	mov    %fs:0x18,%ecx
	mov    (%ecx),%edx
	inc    %edx
	je     _make_fcontext+0x4c // <_make_fcontext+0x4c>
	dec    %edx
	xchg   %edx,%ecx
	jmp    _make_fcontext+0x42 // <_make_fcontext+0x42>
	mov    0x4(%ecx),%ecx
	mov    0x10(%eax),%edx
	mov    %ecx,0x18(%edx)
	mov    $0xffffffff,%ecx
	mov    %ecx,0x14(%edx)
	lea    0x14(%edx),%ecx
	mov    %ecx,0x24(%eax)
	ret    

finish:
    xor    %eax,%eax
    mov    %eax,(%esp)
    call   finish+0xa
    hlt
