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
    /* CRITICAL: Restore context using IRET to properly restore CS:EIP */
    /* We build a fake interrupt frame on the new stack and use IRET */

    movl %edx, %esi         /* ESI = pointer to new context */

    /* First, switch to new stack (we need it to build the IRET frame) */
    /* Temporarily switch SS:ESP */
    cli                     /* Disable interrupts during stack switch */
    movw 50(%esi), %cx      /* Load new SS */
    movw %cx, %ss
    movl 28(%esi), %esp     /* Load new ESP */

    /* Build IRET frame on new stack (push in reverse order): */
    /* Stack must be: SS, ESP, EFLAGS, CS, EIP (for IRET) */

    /* For ring 0 -> ring 0, IRET expects: EFLAGS, CS, EIP */
    /* For ring 0 -> ring 3, IRET expects: SS, ESP, EFLAGS, CS, EIP */
    /* We're doing ring 0 -> ring 0, so just push EFLAGS, CS, EIP */

    pushl 36(%esi)          /* Push EFLAGS */

    /* Push CS (zero-extend from uint16_t to uint32_t) */
    movzwl 40(%esi), %eax
    pushl %eax              /* Push CS */

    pushl 32(%esi)          /* Push EIP */

    /* Restore data segment registers */
    movw 42(%esi), %ax      /* ds */
    movw %ax, %ds
    movw 44(%esi), %ax      /* es */
    movw %ax, %es
    movw 46(%esi), %ax      /* fs */
    movw %ax, %fs
    movw 48(%esi), %ax      /* gs */
    movw %ax, %gs

    /* Restore general purpose registers */
    movl 0(%esi), %eax      /* eax */
    movl 4(%esi), %ebx      /* ebx */
    movl 8(%esi), %ecx      /* ecx */
    movl 12(%esi), %edx     /* edx */
    movl 20(%esi), %edi     /* edi */
    movl 24(%esi), %ebp     /* ebp */
    movl 16(%esi), %esi     /* esi (restore last since we're using it) */

    /* IRET will pop EIP, CS, EFLAGS and restore them atomically */
    /* This ensures CS:EIP are set correctly */
    sti                     /* Re-enable interrupts before IRET */
    iret                    /* Pop EIP, CS, EFLAGS from stack */

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
