package mos6502

import (
	"log"
	"strings"
)

func keywordOrIdenfier(asm *Assembler, keywords map[string]TokenType, index int) (int, Token) {

    char := asm.input[index]

    isNumber, isAlpha := isAlphaNumeric(char)
    var tk Token
    if isAlpha {
        tk.start = index
        index += 1
        for ; index < len(asm.input); index++ {
            char = asm.input[index]
            isNumber, isAlpha = isAlphaNumeric(char)
            if isNumber || isAlpha {
                continue
            }
            break
        }
        tk.size = index - tk.start
        tkType, ok := keywords[strings.ToLower(string(asm.input[tk.start : index]))]
        if ok {
            tk.tokenType = INSTRUCTION 
            tk.value = int(tkType)
            // if tkType == REGISTER_X || tkType == REGISTER_Y || tkType == ACCUMULATOR {
            //     tk.tokenType = tkType
            // }
            
        }else {
            // identifier
            tk.tokenType = INDENTIFIER;
        }
        index -= 1
        return index, tk
    }
    // syntax error 
    log.Fatalf("Syntax error at line: %d, unexpected character '%s'", asm.lineNumber[len(asm.lineNumber) - 1] + 1, string(char));
    // unreachable
    return 0, tk
}

func scan(asm *Assembler){
    var keywords = map[string]TokenType{
        "adc" : ADC, "and" : AND, "asl": ASL,
        "bcc": BCC, "bcs" : BCS, "beq" : BEQ, "bit" : BIT, "bmi" : BMI, "bne" : BNE, "bpl" : BPL, "brk" : BRK, "bvc": BVC,
        "bvs" : BVS, "clc": CLC, "cld" : CLD,
        "cli" : CLI, "clv" : CLV, "cmp" : CMP,
        "cpx" : CPX, "cpy" : CPY, "dec" : DEC,
        "dex" : DEX, "dey" : DEY, "eor" : EOR,
        "inc" : INC, "inx" : INX, "iny" : INY,
        "jmp": JMP, "jsr": JSR, "lda": LDA,
        "ldy": LDY, "lsr" : LSR, "nop":NOP,
        "ora":ORA, "pha": PHA, "php":PHP,
        "pla":PLA, "plp":PLP, "rol":ROL,
        "ror":ROR, "rti":RTI,
        "rts":RTS, "sbc":SBC, "sec":SEC,
        "sed":SED, "sei":SEI,
        "sta":STA, "stx":STX, "sty":STY,
        "tax":TAX, "tay":TAY, "tsx":TSX,
        "txa":TXA, "txs":TXS, "tya":TYA,
        "x" : REGISTER_X, "y" : REGISTER_Y,
        "a" : ACCUMULATOR,
    }
    line := 1

    for index := 0; index < len(asm.input); index++ {

        char := asm.input[index]
if char == ' ' || char == '\r'{
            continue
        }

        if char == ';' {
            for ;; {
                if (index + 1) < len(asm.input) {
                    index += 1
                    char = asm.input[index]
                    if char != '\n' {
                        continue
                    }
                    break
                }else {
                    // endof file
                    break
                }
                
            }

            // end of commment
            asm.lineNumber = append(asm.lineNumber, line)
            asm.tokens = append(asm.tokens, Token{NEWLINE, index, 1, 0})
            line += 1
            continue
        }

        if char == '\n' {

            asm.lineNumber = append(asm.lineNumber, line)
            asm.tokens = append(asm.tokens, Token{NEWLINE, index, 1, 0})
            line += 1
            continue
        }

switch char {

        case '(':
            asm.tokens = append(asm.tokens, Token{OPEN_BRACKET, index, 1, 0})

        case ')':
            asm.tokens = append(asm.tokens, Token{CLOSE_BRACKET, index, 1, 0})

        case '#':
            asm.tokens = append(asm.tokens, Token{HASH, index, 1, 0})

        case ',':
            asm.tokens = append(asm.tokens, Token{COMMA, index, 1, 0})
            
        case '.':
            if(index + 1) < len(asm.input) {
                index += 1
                var tk Token
                index, tk = keywordOrIdenfier(asm, keywords, index)
                if tk.tokenType == INDENTIFIER {
                    tk.tokenType = LABLE
                    asm.tokens = append(asm.tokens, tk)
                    asm.lables[string(asm.input[tk.start + tk.size])] = 0   // parser put required address 
                    continue
                }

                log.Fatalf("Lable name {%s} can't be a Keyword name at line : %d\n", string(asm.input[tk.start + tk.size]), asm.lineNumber[len(asm.lineNumber) - 1] + 1)
            }
            log.Fatalf("Lable name expected at line : %d\n", asm.lineNumber[len(asm.lineNumber) - 1] + 1)

        case '$':
            // number expected
            if (index + 1) < len(asm.input){

                var tk Token
                tk.start = index
                var result int = 0
                index += 1
                for ;(index + 1) < len(asm.input); {
                    char = asm.input[index]
                    num := getNumber(char)
                    if num == 16 {
                        break
                    }

                    if ((result * 16) + num) < 65536 {
                        // not overflow
                        result = (result * 16) + num
                        // fmt.Printf("%X ", result)
                    }
                    index += 1
                    
               }
               index -= 1
               tk.size = index - tk.start
               tk.tokenType = NUMBER
               tk.value = int(result)
               asm.tokens = append(asm.tokens, tk)
               
            }
            
        default:
            // check for keyword or identifier
            var tk Token
            index, tk = keywordOrIdenfier(asm, keywords, index)
            if tk.tokenType == INSTRUCTION {
                if tk.value == int(REGISTER_X) || tk.value == int(REGISTER_Y) || tk.value == int(ACCUMULATOR) {
                    tk.tokenType = TokenType(tk.value)
                }
            }
            asm.tokens = append(asm.tokens, tk)
        }


    }
}
