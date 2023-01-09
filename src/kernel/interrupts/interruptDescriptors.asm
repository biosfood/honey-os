section .sharedFunctions

extern onInterrupt
extern temporaryESP
extern handleSyscallEnd
extern onException

global interruptStack
interruptStack: resb 1024

interruptReturn:
  pop eax
  pop ebx
  pop ecx
  pop edx
  ret

exceptionAbort:
  mov ebx, [esp+24]
  cmp ecx, 0x500000
  je $
  mov ecx, 0x500000
  mov cr3, ecx
  call onException
  mov eax, [temporaryESP]
  mov esp, eax
  pop ebp
  ret

handleInterrupt:
.saveRegisters:
  push eax
  push ebx
  push ecx
  push edx
  mov ecx, cr3
  push ecx
.checkException:
  mov eax, [esp+20]
  cmp eax, 31
  jng exceptionAbort
.goToKernelPages:
  mov eax, 0x500000
  mov cr3, eax
  call onInterrupt
  pop eax
  mov cr3, eax
  pop edx
  pop ecx
  pop ebx
  pop eax
  add esp, 8
  iretd

%macro interruptHandler 1
  ALIGN 4
  global idtHandler%1
idtHandler%1:
  push 0
  push %1
  jmp handleInterrupt
%endmacro

%macro interruptHandlerError 1
  align 4
  global idtHandler%1
idtHandler%1:
  push %1
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
interruptHandler 32
interruptHandler 33
interruptHandler 34
interruptHandler 35
interruptHandler 36
interruptHandler 37
interruptHandler 38
interruptHandler 39
interruptHandler 40
interruptHandler 41
interruptHandler 42
interruptHandler 43
interruptHandler 44
interruptHandler 45
interruptHandler 46
interruptHandler 47
interruptHandler 48
interruptHandler 49
interruptHandler 50
interruptHandler 51
interruptHandler 52
interruptHandler 53
interruptHandler 54
interruptHandler 55
interruptHandler 56
interruptHandler 57
interruptHandler 58
interruptHandler 59
interruptHandler 60
interruptHandler 61
interruptHandler 62
interruptHandler 63
interruptHandler 64
interruptHandler 65
interruptHandler 66
interruptHandler 67
interruptHandler 68
interruptHandler 69
interruptHandler 70
interruptHandler 71
interruptHandler 72
interruptHandler 73
interruptHandler 74
interruptHandler 75
interruptHandler 76
interruptHandler 77
interruptHandler 78
interruptHandler 79
interruptHandler 80
interruptHandler 81
interruptHandler 82
interruptHandler 83
interruptHandler 84
interruptHandler 85
interruptHandler 86
interruptHandler 87
interruptHandler 88
interruptHandler 89
interruptHandler 90
interruptHandler 91
interruptHandler 92
interruptHandler 93
interruptHandler 94
interruptHandler 95
interruptHandler 96
interruptHandler 97
interruptHandler 98
interruptHandler 99
interruptHandler 100
interruptHandler 101
interruptHandler 102
interruptHandler 103
interruptHandler 104
interruptHandler 105
interruptHandler 106
interruptHandler 107
interruptHandler 108
interruptHandler 109
interruptHandler 110
interruptHandler 111
interruptHandler 112
interruptHandler 113
interruptHandler 114
interruptHandler 115
interruptHandler 116
interruptHandler 117
interruptHandler 118
interruptHandler 119
interruptHandler 120
interruptHandler 121
interruptHandler 122
interruptHandler 123
interruptHandler 124
interruptHandler 125
interruptHandler 126
interruptHandler 127
interruptHandler 128
interruptHandler 129
interruptHandler 130
interruptHandler 131
interruptHandler 132
interruptHandler 133
interruptHandler 134
interruptHandler 135
interruptHandler 136
interruptHandler 137
interruptHandler 138
interruptHandler 139
interruptHandler 140
interruptHandler 141
interruptHandler 142
interruptHandler 143
interruptHandler 144
interruptHandler 145
interruptHandler 146
interruptHandler 147
interruptHandler 148
interruptHandler 149
interruptHandler 150
interruptHandler 151
interruptHandler 152
interruptHandler 153
interruptHandler 154
interruptHandler 155
interruptHandler 156
interruptHandler 157
interruptHandler 158
interruptHandler 159
interruptHandler 160
interruptHandler 161
interruptHandler 162
interruptHandler 163
interruptHandler 164
interruptHandler 165
interruptHandler 166
interruptHandler 167
interruptHandler 168
interruptHandler 169
interruptHandler 170
interruptHandler 171
interruptHandler 172
interruptHandler 173
interruptHandler 174
interruptHandler 175
interruptHandler 176
interruptHandler 177
interruptHandler 178
interruptHandler 179
interruptHandler 180
interruptHandler 181
interruptHandler 182
interruptHandler 183
interruptHandler 184
interruptHandler 185
interruptHandler 186
interruptHandler 187
interruptHandler 188
interruptHandler 189
interruptHandler 190
interruptHandler 191
interruptHandler 192
interruptHandler 193
interruptHandler 194
interruptHandler 195
interruptHandler 196
interruptHandler 197
interruptHandler 198
interruptHandler 199
interruptHandler 200
interruptHandler 201
interruptHandler 202
interruptHandler 203
interruptHandler 204
interruptHandler 205
interruptHandler 206
interruptHandler 207
interruptHandler 208
interruptHandler 209
interruptHandler 210
interruptHandler 211
interruptHandler 212
interruptHandler 213
interruptHandler 214
interruptHandler 215
interruptHandler 216
interruptHandler 217
interruptHandler 218
interruptHandler 219
interruptHandler 220
interruptHandler 221
interruptHandler 222
interruptHandler 223
interruptHandler 224
interruptHandler 225
interruptHandler 226
interruptHandler 227
interruptHandler 228
interruptHandler 229
interruptHandler 230
interruptHandler 231
interruptHandler 232
interruptHandler 233
interruptHandler 234
interruptHandler 235
interruptHandler 236
interruptHandler 237
interruptHandler 238
interruptHandler 239
interruptHandler 240
interruptHandler 241
interruptHandler 242
interruptHandler 243
interruptHandler 244
interruptHandler 245
interruptHandler 246
interruptHandler 247
interruptHandler 248
interruptHandler 249
interruptHandler 250
interruptHandler 251
interruptHandler 252
interruptHandler 253
interruptHandler 254
interruptHandler 255
