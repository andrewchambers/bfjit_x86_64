00000000  4889F8            mov rax,rdi
00000003  800001            add byte [rax],0x1
00000006  480501000000      add rax,0x1
0000000C  8000FF            add byte [rax],0xff
0000000F  482D01000000      sub rax,0x1
00000015  803800            cmp byte [rax],0x0
00000018  0F841B000000      jz dword 0x39
0000001E  50                push rax
0000001F  8A00              mov al,[rax]
00000021  89C7              mov edi,eax
00000023  48BB100740000000  mov rbx,0x400710
         -0000
0000002D  FFD3              call rbx
0000002F  58                pop rax
00000030  803800            cmp byte [rax],0x0
00000033  0F85E5FFFFFF      jnz dword 0x1e
00000039  C3                ret
