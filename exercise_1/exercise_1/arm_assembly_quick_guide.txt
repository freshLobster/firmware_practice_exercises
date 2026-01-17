# ARM Assembly Quick Guide (Cortex-M/Thumb focus)

Core syntax
- Unified syntax (`.syntax unified`): same mnemonics for ARM/Thumb; Cortex-M uses Thumb only.
- Directives: `.thumb`, `.cpu cortex-m4`, `.fpu fpv4-sp-d16`.
- Labels end with `:`. Comments use `//`.

Common instructions
- `mov rd, rs/#imm`: move register/immediate.
- `ldr rd, =imm`: pseudo-instruction to load an immediate/address via literal pool.
- `ldr rd, [rn, #imm]` / `str rd, [rn, #imm]`: load/store word with offset.
- `ldrb/strb`, `ldrh/strh`: byte/halfword variants.
- `adds/subs rd, rn, #imm`: arithmetic with flags (Thumb-2).
- `cmp rn, #imm/rm`: compare (updates flags).
- `b label`: branch; `bne/beq/blt/bgt/bge/bls/bhi` use flags.
- `bl func`: branch with link (call).
- `bx lr`: return (pop PC from LR).
- `it/itt/ite`: conditional execution block in Thumb (e.g., `itt lt` then two ops).
- `push/pop {r0-r3,lr}`: save/restore regs on stack (Thumb).

Registers (Cortex-M)
- r0–r3: argument/volatile; r4–r11: callee-saved; r12: intra-proc scratch.
- r13: SP; r14: LR; r15: PC; APSR: flags.

Memory and addressing
- Little endian. Word = 32 bits.
- PC-relative literals via `ldr rd, =symbol`.
- Structure access via base + offset: `ldr r0, [r1, #offset]`.

Control flow
- `b .` loops forever. Use condition codes after `cmp` (EQ, NE, LT, GT, CS/HS, CC/LO).
- Function calls: `bl target`; returns with `bx lr`.

Interrupt/vector table basics (Cortex-M)
- Vector table: array of 32-bit words; entry 0 = initial SP, 1 = Reset_Handler, etc.
- Handlers must be Thumb (`.thumb_func`); vector entries point to handler addresses.

Startup patterns
- Set SP from `_estack`.
- Copy `.data` from flash to RAM; zero `.bss`.
- Call `SystemInit`; call `main`; if main returns, loop.

Condition codes (for branches)
- EQ/NE (Z set/clear), CS/HS or CC/LO (carry set/clear), MI/PL (negative/positive),
  VS/VC (overflow), HI/LS (unsigned > / <=), GE/LT (signed >= / <), GT/LE (signed > / <=).

Thumb pseudo-instructions
- `nop`: do nothing.
- `bkpt #imm`: breakpoint/semihosting trap (Cortex-M semihosting uses `bkpt 0xAB`).
- `cpsid i` / `cpsie i`: disable/enable interrupts.

ABI (AAPCS) quick notes
- Args: r0–r3; return: r0 (and r1 for 64-bit).
- Caller-saved: r0–r3, r12, LR; callee-saved: r4–r11 (and SP alignment).
