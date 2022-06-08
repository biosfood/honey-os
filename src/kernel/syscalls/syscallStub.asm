section .sharedFunctions
bits 32

global syscallStub
extern handleSyscall

syscallStub:
  mov ebx, cr3
  mov ecx, 0x500000
  mov cr3, ecx
  push eax
  push ebx
  call handleSyscall
  pop ebx
  mov edx, [eax+4]
  mov ecx, [eax+8]
  mov cr3, ebx
  sysexit
