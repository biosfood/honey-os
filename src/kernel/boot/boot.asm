section .boot
BITS 32

multibootHeader:
    dd 0xe85250d6
    dd 0
    dd .end - multibootHeader

    dd 0x100000000 - (0xe85250d6 + 0 + (.end - multibootHeader))

    dw 0
    dw 0
    dd 8
.end:

extern kernelMain

%define pageDirectory       0x500000
%define kernelPageTable     0x501000
%define kernelInfoPageTable 0x502000
%define kernelInfoStart     0x500000
%define stackEnd            0x8FFFFF ; end of the kernel info page table
%define mappedStackEnd      0xFFBFFFFF
%define originalPageTable   0x503000

ebxStart: db 0x00000000

global _start
_start:
    mov [ebxStart], ebx
    mov esp, stackEnd
    lgdt [gdt32.end]
    mov ax, 16
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    jmp 0x08:.setupPaging
.setupPaging:
    mov eax, pageDirectory
    mov ebx, originalPageTable
    mov ecx, 0
    call setupFullPageTable

    mov eax, pageDirectory + 0xFFC
    mov ebx, kernelInfoPageTable
    mov ecx, 0x100000
    call setupFullPageTable

    mov eax, pageDirectory + 0xFF8
    mov ebx, kernelPageTable
    mov ecx, kernelInfoStart
    call setupFullPageTable

.enablePaging:
    mov eax, pageDirectory
    mov cr3, eax
 
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax
    mov esp, mappedStackEnd
    jmp higherKernelEntry

; eax: directory entry address
; ebx: pageTable address
; ecx: target address
setupFullPageTable:
    push eax
    push ebx
    push ecx
    push edx
.enterDirectoryEntry:
    mov edx, ebx
    add edx, 3
    mov [eax], edx
.fillPageTable:
    mov edx, 0x400
    add ecx, 3
.loop:
    mov [ebx], ecx
    add ecx, 0x1000
    add ebx, 4
    dec edx
    jnz .loop

    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

ALIGN 4
gdt32:
	dq 0

.code:
	dw 0xffff
	dw 0
	db 0
	db 10011010b
	db 11001111b
	db 0

.data:
	dw 0xffff
	dw 0
	db 0
	db 10010010b
	db 11001111b
	db 0

.userCode:
	dw 0xffff
	dw 0
	db 0
	db 10011010b
	db 11001111b
	db 0
.userData:
	dw 0xffff
	dw 0
	db 0
	db 10010010b
	db 11001111b
	db 0
.end:
    dw .end - gdt32 - 1
    dd gdt32

section .text
newGDT:
    dw gdt32.end - gdt32 - 1
    dd gdt32 + 0xFFB00000
higherKernelEntry:
    lgdt [newGDT]
    mov ebx, [ebxStart]
    push ebx
.cleanOriginalEntryCode:
    mov eax, 0xFF800000
    mov dword [eax], 0
    call kernelMain
    jmp $
