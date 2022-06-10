section .sharedFunctions

global runFunction
global runEnd
global runEndSyscall

extern serviceCR3
extern serviceESP
extern mainFunction

temporaryESP: resb 4

runFunction:
  push ebp
  mov eax, esp
  mov [temporaryESP], eax
  mov ecx, [serviceESP]
  mov ebp, ecx
  mov edx, [mainFunction]
  mov eax, [serviceCR3]
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
