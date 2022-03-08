package assembler

const (
    Adc = iota
    And
    Asl

    Bcc
    Bcs
    Beq
    Bit
    Bmi
    Bne
    Bpl
    Brk
    Bvc
    Bvs

    Clc
    Cld
    Cli
    Clv
    Cmp
    Cpx
    Cpy

    Dec
    Dex
    Dey
    
    Eor

    Inc
    Inx
    Iny

    Jmp
    Jsr

    Lda
    Ldx
    Ldy
    Lsr

    Nop

    Ora
    
    Pha
    Php
    Pla

    Rol
    Ror
    Rti
    Rts

    Sbc
    Sec
    Sed
    Sei
    Sta
    Stx
    Sty

    Tax
    Tay
    Tsx
    Txa
    Txs
    Tya

    Number
    CharSeq
    Address
    Comma
    LeftParen
    RightParen
    Identifier
    Colon
    Goto
    Macro 
    Proc
    End
    Eval
    Define
    Eof
    // Include // hope to add in next versions
    At  // @
)

type Token struct {
    tokenType int
    offsetStart int
    tokenSize int
    tokenLine int
}
