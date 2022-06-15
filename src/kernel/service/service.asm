section .sharedFunctions

global runFunction
global runEnd
global runEndSyscall

extern currentSyscall

temporaryESP: resb 4

runFunction:
  push ebp
  mov eax, esp
  mov [temporaryESP], eax
  mov ebx, [currentSyscall]
  mov ecx, [ebx + 8]
  mov ebp, ecx
  mov edx, [ebx + 4]
  mov eax, [ebx + 12]
  mov cr3, eax
  sysexit
runEnd:
  mov eax, 0
  sysenter
runEndSyscall:
  mov eax, [temporaryESP]
  mov esp, eax
  pop ebp
  ret
