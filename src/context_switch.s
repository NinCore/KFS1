/* context_switch.s - Context switching for processes (GNU as syntax) */

/* Mark stack as non-executable */
.section .note.GNU-stack,"",@progbits

.section .text
.global context_switch
.global switch_to_process

/* void context_switch(struct process_context *from, struct process_context *to) */
context_switch:
    pushl %ebp
    movl %esp, %ebp

    movl 8(%ebp), %eax      /* from context */
    movl 12(%ebp), %edx     /* to context */

    /* Save current context if from is not NULL */
    testl %eax, %eax
    jz .skip_save

    /* Save general purpose registers */
    movl %eax, 0(%eax)      /* eax */
    movl %ebx, 4(%eax)      /* ebx */
    movl %ecx, 8(%eax)      /* ecx */
    movl %edx, 12(%eax)     /* edx */
    movl %esi, 16(%eax)     /* esi */
    movl %edi, 20(%eax)     /* edi */
    movl %ebp, 24(%eax)     /* ebp */

    /* Save ESP (stack pointer) */
    movl %esp, %ecx
    addl $8, %ecx           /* Adjust for push ebp and return address */
    movl %ecx, 28(%eax)     /* esp */

    /* Save EIP (return address) */
    movl 4(%ebp), %ecx      /* Return address */
    movl %ecx, 32(%eax)     /* eip */

    /* Save EFLAGS */
    pushfl
    popl %ecx
    movl %ecx, 36(%eax)     /* eflags */

    /* Save segment registers */
    movw %cs, 40(%eax)      /* cs */
    movw %ds, 42(%eax)      /* ds */
    movw %es, 44(%eax)      /* es */
    movw %fs, 46(%eax)      /* fs */
    movw %gs, 48(%eax)      /* gs */
    movw %ss, 50(%eax)      /* ss */

.skip_save:
    /* Restore new context */
    movl %edx, %eax         /* to context */

    /* Restore segment registers (except CS which requires far jump) */
    movw 50(%eax), %cx      /* ss - restore FIRST before changing ESP */
    movw %cx, %ss
    movw 42(%eax), %cx      /* ds */
    movw %cx, %ds
    movw 44(%eax), %cx      /* es */
    movw %cx, %es
    movw 46(%eax), %cx      /* fs */
    movw %cx, %fs
    movw 48(%eax), %cx      /* gs */
    movw %cx, %gs

    /* Restore general purpose registers */
    movl 4(%eax), %ebx
    movl 8(%eax), %ecx
    movl 12(%eax), %edx
    movl 16(%eax), %esi
    movl 20(%eax), %edi
    movl 24(%eax), %ebp
    movl 28(%eax), %esp

    /* Push EFLAGS for restoration */
    pushl 36(%eax)
    popfl

    /* Jump to new EIP */
    movl 32(%eax), %ecx     /* EIP */
    movl 0(%eax), %eax      /* Restore eax last */

    jmp *%ecx               /* Jump to new process */

/* Simpler version - switch_to_process */
/* void switch_to_process(struct process_context *from, struct process_context *to) */
switch_to_process:
    pushl %ebp
    movl %esp, %ebp
    pushal                  /* Save all registers */

    movl 8(%ebp), %esi      /* from */
    movl 12(%ebp), %edi     /* to */

    /* Save current context */
    testl %esi, %esi
    jz .load_context

    movl %eax, 0(%esi)
    movl %ebx, 4(%esi)
    movl %ecx, 8(%esi)
    movl %edx, 12(%esi)
    movl %esi, 16(%esi)
    movl %edi, 20(%esi)
    movl %ebp, 24(%esi)
    movl %esp, 28(%esi)

    /* Save EIP (return address) */
    movl 4(%ebp), %eax
    movl %eax, 32(%esi)

    /* Save EFLAGS */
    pushfl
    popl %eax
    movl %eax, 36(%esi)

.load_context:
    /* Load new context */
    movl %edi, %esi

    /* Restore registers */
    movl 0(%esi), %eax
    movl 4(%esi), %ebx
    movl 8(%esi), %ecx
    movl 12(%esi), %edx
    movl 20(%esi), %edi
    movl 24(%esi), %ebp
    movl 28(%esi), %esp

    /* Restore EFLAGS */
    pushl 36(%esi)
    popfl

    /* Jump to new EIP */
    movl 32(%esi), %eax
    movl 16(%esi), %esi     /* Restore esi last */
    jmp *%eax
