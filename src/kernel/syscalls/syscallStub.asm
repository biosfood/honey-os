section .sharedFunctions
bits 32

global syscallStub
global handleSyscallEnd
extern handleSyscall
extern temporaryESP

temporaryEAX: resb 4

syscallStub:
  mov [temporaryEAX], eax
  mov eax, 0x500000
  mov cr3, eax
  mov eax, [temporaryEAX]
  push esi
  push edx
  push ecx
  push ebx
  push eax
  push edi
  call handleSyscall
handleSyscallEnd:
  mov eax, [temporaryESP]
  mov esp, eax
  pop ebp
  ret
