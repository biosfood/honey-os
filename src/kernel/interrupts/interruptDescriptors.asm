section .sharedFunctions

handleInterrupt:
  mov eax, 8
  jmp $

%macro interruptHandler 1
  ALIGN 4
  global idtHandler%1
idtHandler%1:
  mov ebx, %1
  xor ecx, ecx
  jmp handleInterrupt
%endmacro

%macro interruptHandlerError 1
  align 4
  global idtHandler%1
idtHandler%1:
  pop ecx
  mov ebx, %1
  jmp handleInterrupt
%endmacro

interruptHandler 0
interruptHandler 1
interruptHandler 2
interruptHandler 3
interruptHandler 4
interruptHandler 5
interruptHandler 6
interruptHandler 7
interruptHandlerError 8
interruptHandler 9
interruptHandlerError 10
interruptHandlerError 11
interruptHandlerError 12
interruptHandlerError 13
interruptHandlerError 14
interruptHandler 15
interruptHandler 16
interruptHandlerError 17
interruptHandler 18
interruptHandler 19
interruptHandler 20
interruptHandler 21
interruptHandler 22
interruptHandler 23
interruptHandler 24
interruptHandler 25
interruptHandler 26
interruptHandler 27
interruptHandler 28
interruptHandler 29
interruptHandlerError 30
interruptHandler 31
