# tiny6502, a Simple Cross-Assembler for 65xx CPUs

## Introduction

tiny6502 is a multi-pass cross-assembler for 65xx-based microprocessors. Currently only the 6502, Rockwell R65C00, WDC 65C02 and WDC 65C816 CPUs are supported, but support for more processors may be added in the future.

tiny6502 is a subset of its larger .Net cousin, [6502.Net](https://github.com/informedcitizenry/6502.Net). Most basic functionality in that assembler is available. Assembly syntax is somewhat [TASS-Compatible](http://turbo.style64.org/),  where statements follow the general style of:


```
label mnemonic operand comment
```
or alternatively

```
constant = expression
```

## Compiling from source

Source is C99 compatible, and macOS and Linux users should be able to run `make` with the repository Makefile as-is. Windows users will need to install the [MinGW](https://osdn.net/projects/mingw/) package, or customize the Makefile to work with their setup.


## Usage

The application accepts as a parameter the input source file. The below would simply output a source file to the default binary `a.out`:

`tiny6502 mysource.asm`

Other options are available, including output file name, assembly listing, and target format:

`tiny6502 mysource.asm -o myprogram.prg --format cbm --list mylist.asm`

The `-c`/`--cpu` option is required to specify the target CPU if it is not the 6502. Valid arguments are `6502`, `65C02`, and `65816`:

`tiny6502 my_supernes_game.asm -o my_supernes_rom.bin --format flat --cpu 65816`

Use the `--help`/`-h` option for a full list of all available options.

## Overview

### Literals, Constants and Expressions

Number literals can be expressed in the form of hexadecimal, decimal and binary integers.

```
border  = 53280
chrout  = $ffd2
mask    = %01111111

```
For readability digits can be separated by underscores.

String and single character literals are expressed inside pairs of quote `"` and apostrophe `'` characters, respectively.

```
            lda #'A'
            jsr chrout
            .string "HELLO"
```

tiny6502 also recognizes C/C++ string escape characters, including octal, hex and Unicode escape sequences.

```
            .string "HELLO\n\t\tWORLD"

            lda #'\x93' ; clear screen
            jsr chrout

            .string "\u03c0" ; π
```

Operands are comprised of single number or char literals, or of math expressionss.

```
        lda #(40+COLUMNS*12)/NUM_ROWS
```

Available math operations are in order from highest to lowest precedence:

| Operator  | Operation         | Example   |
| --------- | ----------------- | --------- |
| -         | Negative          | -7        |
| !         | Not               | !true     |
| ~         | Complement        | ~255      |
| ^^        | Power             | 2 ^^ 3    |
| *         | Multiply          | 5 * 2     |
| /         | Divide            | 18 / 3    |
| %         | Modulo            | 4 % 8     |
| +         | Add               | 1 + 1     |
| -         | Subtract          | 2 - 7     |
| <<        | Left shift        | 1 << 16   |
| >>        | Right shift       | 256 >> 4  |
| >>>       | Arithmetic shift  | -2 >>> 1  |
| <         | Less than         | 1 < 2     |
| <=        | Less or euqal     | 2 <= 7    |
| >=        | Greater/equal     | 5 >= 2    |
| >         | Greater than      | 9 > 6     |
| <=>       | Three-way compare | 5 <=> 9   |
| ==        | Equal             | 4/2 == 2  |
| !=        | Not equal         | 8 != 2    |
| &         | Bitwise AND       | $ff & $f  |
| ^         | Bitwise XOR       | 8 ^ 2     |
| \|        | Bitwise OR        | 4 | 1     |
| &&        | Logical AND       | 5>2 && 1  |
| \|\|      | Logical OR        | 1 || 0    |
| <         | LSB               | <$ffd2    |
| >         | MSB               | >$c000    |

### Comments

The assembler supports C/C++ style comments, both inline `//` and multi-line `/*`/`*/`.

```
            ldx #0 // reset .X
            txa
            /* initialize/reset
            buffer region */
            sta buffer,x
```

For backward compatibility semi-colons `;` also mark the start of inline comments.

```
            lda #0 ; reset accumulator
```

### Program Counter symbol

The `*` symbol is used to change or read the state of the current program counter.

```
            * = 49152 ;set program start address to 49152
            lda *+13 ;read 13 bytes from current program counter
```

### Addressing mode sizes

Generally tiny6502 will choose the smallest sized addressing mode possible. For instance, the following is by default assumed to be a zero/direct page operation:

```
            lda $02
```

The assembler can explicitly be told to treat this operation instead as an absolute addressing mode operation by specifying the operand bitwidth in square `[]` brackets:

```
            lda [16] $02 // is interpreted as lda $0002
```

Bitwidth size can be 8, 16 and 24 (for 65816 long mode instructions).

### Labels

Labels can be forward referenced and are resolved after first pass. Colons can follow labels but are optional.

```
            lda #0
            tax
loop:       sta buffer,x
            dex
            bne loop
```

Generally, labels can only be defined once per source, unless preceded by an underscore `_` character, which makes them local to the most recent "regular" label.

```
label1      inx
            bne _done
            jsr output
_done       rts
 ...
label2      dex
            beq _done ; different from above
            jmp somewhere
_done       rts      
```

In addition, anonymous forward and backward labels are supported.

```
-           cpx #6
            beq ++
            bcs +
            jmp less_than_six
+           jmp greater_than_six
+           jmp equal_to_six
```

### Pseudo operations for assembling data

Several pseudo-ops allow for data assembly: `.byte` for single byte values, `.word` for two byte values, `.long` for three byte values, and `.dword` for four bytes. 

```
            .byte $7f,$e2,$97,$00,$1f,-2

            .word $fffc,$fffe

            .long $ff_ffff

            .dword $feedface
```

For string data, there are several string-related pseudo-ops. 

```
            .string "HELLO" ; compiles to: $48,$45,$4c,$4c,$4f

            .cstring "HELLO" ; null-terminates string 

            .pstring "HELLO" ; string data starts with a byte header denoting size

            .lstring "HELLO" ; shift all bits left, set low bit on final byte

            .nstring "HELLO" ; set high bit on final byte
```

The above pseudo-ops also allow numeric values as well.

```
            .cstring "HELLO", $0d
```

To interpret numeric data as strings, use the `.stringify` pseudo-op.

```
            .stringify 2061 ; assembles to: $32, $30, $36, $31
```

Use the `.binary` pseudo-op to include data from a binary file.

```
            .binary "music.sid",$7e,$200 ;126 byte offset, first 512 bytes of data
```

In the example above, offset and length parameters are given, but both are optional.

To include tiny6502-compatible source for compilation, use the `.include` directive.

```
            .include "/libs/mylibrary.asm"
    ...
```

### Reserving space without outputting code

There are a few pseudo-ops that allow you to reserve space for storage without affecting assembly output. The `.align` and `.fill` commands serve this purpose when only one argument is specified.

```
star_field  .fill   FIELD_WIDTH * FIELD_HEIGHT
```

The biggest difference between these two commands is the first argument in `.fill` specifies the number of bytes to reserve space. The `.align` pseudo-op aligns the program counter value to an amount divisable by the argument.

```
            .align  $100    // align to the nearest page
```

If a second (optional) parameter is specified for either one of these pseudo-ops, this changes the behavior so that the output is filled with the values according to the fill length/alignment of the first argument.

```
            .fill   9,$ffd220 // three jsr $ffd2 calls
```
Space can also be reserved using the `.byte`, `.word`, `.long`, `.dword` and string pseudo-ops by passing a `?` argument.

```
            * = $00
zp_byte_var .byte ? // same as .fill 1
zp_word_var .word ? // same as .fill 2
```

### Marking code as relocatable

The assembler actually has two program counters, a "real" program counter tracking the actual offset in the 64KiB address space, and a "logical" program counter to which symbolic addresses resolve. By default both are the same, but for purposes of assembling code that can be relocated, the `.relocate` directive changes the logical program counter without affecting the real PC.

```
            * = $0801
            ;; BASIC stub
            ;; 10 SYS2061
SYS         = $9e
            .word eob, 10
            .stringify start
eob         .long 0
start       ldx #program_end - highcode
-           lda highcode,x
            sta $c000,x
            dex
            bne -
            jmp $c000
highcode    
            .relocate $c000 ; all symbolic addresses will be offset from $c000
            ldx #0
printloop   lda message,x
            beq done
            jsr $ffd2
            inx
            jmp printloop
done        rts
message     .cstring "HELLO, HIGH CODE!"
            .endrelocate 
program_end ; program_end is set to the "real" program end
```
As seen in the example above, the `.endrelocate` directive resynchronizes the logical PC to the real one.

### Pseudo-ops controlling 65816 assembly

For 65816 targets, data size of immediate mode can be either 1 or 2 bytes. This can be controlled in the assembler either with the bitwidth specifier method described above, or setting the register mode. 

```
            rep #$20    // Set accumulator to 16-bit immediate mode
            .m16        // tell the assembler
            lda #$1234  

            sep #$20    // Reset accumulator to 8-bit immediate mode
            .m8         // tell the assembler
            lda #$13        

            rep #$10    // Set .x/.y to 16-bit immediate mode
            .x16        // tell the assembler
            ldx #$5678  
            ldy #$9abc

            sep #$10    // Reset .x/.y to 8-bit immediate mode
            .x8         // tell the assembler
            ldx #$14
            ldy #$15

            .rep #$30   // Set all registers to 16-bit
            .mx16       // tell the assembler
            ...

            .sep #$30   // Reset all registers to 8-bit
            .mx8        // tell the assembler
```
There is one other directive that can subtlely change the way addressing modes are determined, and that is the direct page `.dp` directive. This pseudo-op tells the assembler which is the current logical page and, if the operand's own page is the same, will downsize the addressing mode accordingly.

```
            * = $0200
            pha #2
            pld     ; CPU now in page 2
            .dp $02 ; tell the assembler
            ldx #0
            txy
            lda (page2_table,x) ; assembler will not error 
            sta (page2_table),y ; nor here 

page2_table
            .word some_ptr, some_other_ptr

```

### Macros

Macros are defined between a pair of `.macro` and `.endmacro` directives. Arguments are optional, and are referenced within definitions with a leading `\` character followed by their explicit name in the argument definition list or by their parameter number starting at 1.

```
basic       .macro  sob
            * = \sob ; set program counter 
            .word eob
            .word 10 ; line 10
            .byte $9e ; SYS
            .tostring \2 ; parameter 2 is start address
eob         .long 0
            .endmacro
```

The above macro can be expanded with a leading `.` character.

```
            .basic $0801, start

start       ldx #0
            ;; etc...
```

Macros cannot be defined within other macro definitions, but they can be referenced.

```
inc16       .macro
            inc \1
            bne +
            inc \1+1
+           .endmacro

inc24       .macro
            .inc16 \1
            bne +
            inc \1+2
+           .endmacro
```

## Changelog

*2022-09-23*
- **Version 0.2**
- Fixed unary expression evaluation
- Fixed `.align` pseudo-op
- Added `?` expression for pseudo-op code output

*2022-09-09*
- **Version 0.1 "Hello, tiny6502" Version**