section .sharedFunctions

global runFunction
global runEnd
global temporaryESP

extern currentSyscall

temporaryESP: resb 4

runFunction:
  push ebp
  mov eax, esp
  mov [temporaryESP], eax
  mov ebx, [currentSyscall]
  mov ecx, [ebx + 24]
  mov edx, returnPoint
  mov eax, [ebx + 28]
  mov ebx, [ebx + 20]
  mov cr3, eax
  mov eax, ebx
  xor ebp, ebp
  sti
  sysexit
runEnd:
  mov eax, 0
  sysenter

returnPoint:
  ret
