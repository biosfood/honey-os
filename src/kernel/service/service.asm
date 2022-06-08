section .sharedFunctions

global runFunction
global runEnd

extern currentCr3
extern returnStack
extern currentEsp
extern mainFunction

runFunction:
  mov eax, esp
  mov [returnStack], eax
  mov ecx, [currentEsp]
  add ecx, 0xFFC
  mov edx, [mainFunction]
  mov eax, [currentCr3]
  mov cr3, eax
  sysexit
runEnd:
  ; todo: make a sysenter call to go back to ring 0, writing to cr3 is not possible in ring 3
  jmp $
  mov eax, 0x500000
  mov cr3, eax
  mov eax, [returnStack]
  mov esp, eax
  jmp $
  ret
