package mos6502

import (
	"log"
	"strings"
)


var lines int = 1
var address int = 0x200
func currentTkType(tokens []Token, index int) TokenType {
    return tokens[index].tokenType
}

// I know its an hex number and single byte
func sToN (number string) (int) {
    
    num := getNumber(number[2])
    result := num * 16
    num = getNumber(number[3])
    return result + num
}
func isInModes (modes []string, mode string) (int, bool){
    for index, m := range modes{
        if m == mode {
            return index, true
        }
    }
    return -1, false
}

func getMem (mem []string, opMem int) byte{
    if (opMem > len(mem) - 1) {
        log.Fatalf("Index out of bounds %d %d", opMem, len(mem))
    }
    temp := sToN(mem[opMem])
    return byte(temp)
}

type opCode struct {
    Name string `json:"name"`
    Modes []string  `json:"modes"`
    Mem []string    `json:"mem"`
}

func parse(asm *Assembler, op string, index *int) {


    tkType := currentTkType(asm.tokens, *index)
    if tkType == HASH {
        *index += 1
        if currentTkType(asm.tokens, *index) == NUMBER {
            reqByte := uint16(asm.tokens[*index].value)
            // immediate
            opMem, ok := isInModes(asm.opData[op].modes, "i")
            if !ok {
                // only for branch instructions
                // log.Printf("mode - %s\n", "Relative")
                opMem, ok = isInModes(asm.opData[op].modes, "r")
                if !ok {
                    log.Fatalf("Instruction %s at line %d - not support immediate mode.\n", strings.ToUpper(op), lines)
                }
            }
            
             
            // log.Printf("mode - %s\n", "Immediate")
            asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte))
            *index += 1
            return
        }
        // error
        log.Fatalf("unexpected token at line %d : %s \n", lines,  asm.tokens[*index].toString(asm.input))
    }

    if tkType == OPEN_BRACKET {
        *index += 1

        reqByte := uint16(asm.tokens[*index].value)
        if currentTkType(asm.tokens, *index) != NUMBER {
            // error
                log.Fatalf("unexpected token as line %d :  %s \n", lines,  asm.tokens[*index].toString(asm.input))
        }

        *index += 1
        if currentTkType(asm.tokens, *index) == COMMA {
            *index += 1

            if currentTkType(asm.tokens, *index) != REGISTER_X {
                // error

                log.Fatalf("unexpected token as line %d :  %s \n", lines,  asm.tokens[*index].toString(asm.input))
            }

            *index += 1
            if currentTkType(asm.tokens, *index) != CLOSE_BRACKET {
                //error

                log.Fatalf("unexpected token as line %d :  %s \n", lines,  asm.tokens[*index].toString(asm.input))
            }

            // log.Printf("mode - %s\n", "Indirect X")
            opMem, ok := isInModes(asm.opData[op].modes, "inx")
            if !ok {

                log.Fatalf("Instruction %s at line %d - not support indirect X mode.\n", strings.ToUpper(op), lines)
            }
            
            asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte))
            *index += 1
            return

            // indirect X
        }

        if currentTkType(asm.tokens, *index) == CLOSE_BRACKET {

            *index += 1

            if currentTkType(asm.tokens, *index) == COMMA {
                *index += 1
                if currentTkType(asm.tokens, *index) != REGISTER_Y {
                    // error

                    log.Fatalf("unexpected token as line %d :  %s \n", lines,  asm.tokens[*index].toString(asm.input))
                }


                //indirect Y
                // log.Printf("mode - %s\n", "Indirect Y")
                
                opMem, ok := isInModes(asm.opData[op].modes, "iny")
                if !ok {

                    log.Fatalf("Instruction %s at line %d - not support indirect Y mode.\n", strings.ToUpper(op), lines)
                }
                
                asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte))
                *index += 1
                return
            }

            // log.Printf("mode - %s\n", "Indirect")
            opMem, ok := isInModes(asm.opData[op].modes, "in")
            if !ok {
                log.Fatalf("Instruction %s at line %d - not support indirect mode.\n", strings.ToUpper(op), lines)
            }
                
            asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte & 255), byte(reqByte >> 8))
            // indirect
            return
            
        }


    }
    if tkType == NUMBER {
        
        reqByte := uint16(asm.tokens[*index].value)
        *index += 1
        if asm.tokens[*index - 1].value > 255 {
            // absolute

            tkType = currentTkType(asm.tokens, *index)
            if tkType != COMMA {
                // absolute
                // log.Printf("mode - %s\n", "Absolute")
                opMem, ok := isInModes(asm.opData[op].modes, "a")
                if !ok {
                    log.Fatalf("Instruction %s at line %d - not support absolute mode.\n", strings.ToUpper(op), lines)
                }
                    
                asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte & 255), byte(reqByte >> 8))
                return
            }
            *index += 1
            tkType = currentTkType(asm.tokens, *index)

            if tkType == REGISTER_X{

                // log.Printf("mode - %s\n", "Absolute X")
                opMem, ok := isInModes(asm.opData[op].modes, "ax")
                if !ok {
                    log.Fatalf("Instruction %s at line %d - not support absolute X mode.\n", strings.ToUpper(op), lines)
                }
                    
                asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte & 255), byte(reqByte >> 8))
                // absolute X
                *index += 1
                return
            }

            if tkType == REGISTER_Y {

                // log.Printf("mode - %s\n", "Absolute Y")
                // absolute y
                opMem, ok := isInModes(asm.opData[op].modes, "ay")
                if !ok {
                    log.Fatalf("Instruction %s at line %d - not support absolute Y mode.\n", strings.ToUpper(op), lines)
                }
                    
                asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte & 255), byte(reqByte >> 8))
                *index += 1
                return
            }
            
            // error

            log.Fatalf("unexpected token at line %d : %s \n", lines, asm.tokens[*index].toString(asm.input))
        }

        tkType = currentTkType(asm.tokens, *index)

        if tkType == COMMA {
            
            *index += 1
            tkType = currentTkType(asm.tokens, *index)
            if tkType == REGISTER_X {

                // log.Printf("mode - %s\n", "Zero Page X")
                opMem, ok := isInModes(asm.opData[op].modes, "zx")
                if !ok {
                    log.Fatalf("Instruction %s at line %d - not support zero page X mode.\n", strings.ToUpper(op), lines)
                }
                    
                asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte))
                // zero page x
                *index += 1
                return
            }
            if tkType == REGISTER_Y {

                // log.Printf("mode - %s\n", "Zero Page Y")
                opMem, ok := isInModes(asm.opData[op].modes, "zy")
                if !ok {
                    log.Fatalf("Instruction %s at line %d - not support zero page Y mode.\n", strings.ToUpper(op), lines)
                }
                    
                asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte))
                // zero page y
                *index += 1
                return
            }

            //error
            
            log.Fatalf("unexpected token at line %d : %s \n", lines, asm.tokens[*index].toString(asm.input))
        }


        // log.Printf("mode - %s\n", "Zero Page")
        opMem, ok := isInModes(asm.opData[op].modes, "z")
        if !ok {
            log.Fatalf("Instruction %s at line %d - not support zero page mode.\n", strings.ToUpper(op), lines)
        }
            
        asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem), byte(reqByte))
        //zero page
        return
        
    }

    if tkType == ACCUMULATOR {
        // ACCUMULATOR
        // log.Printf("mode - %s\n", "Accumulator")
        opMem, ok := isInModes(asm.opData[op].modes, "ac")
        if !ok {
            log.Fatalf("Instruction %s at line %d - not support accumulator mode.\n", strings.ToUpper(op), lines)
        }
            
        asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem))
        *index += 1
        return
    }

    // implied
    // log.Printf("mode - %s\n", "Implied")
    opMem, ok := isInModes(asm.opData[op].modes, "im")
    if !ok {
        log.Fatalf("Instruction %s at line %d - not support implied mode.\n", strings.ToUpper(op), lines)
    }
        
    asm.output = append(asm.output, getMem(asm.opData[op].mem, opMem))

}

func statements(asm *Assembler) {
    if len(asm.tokens) == 0 {
        return
    }

    index := 0
    current := asm.tokens[index]
    for ;; {

        if current.tokenType == NEWLINE {
            lines += 1
            index += 1
            current = asm.tokens[index]
            continue
        }
        if current.tokenType == LABLE {

        }
        if current.tokenType == INSTRUCTION {
            index += 1
            parse(asm, strings.ToLower(string(asm.input[current.start : current.start + current.size])), &index)
            current = asm.tokens[index]
            if current.tokenType == EOF {
                break
            }

            if current.tokenType != NEWLINE {
                log.Fatalf("expecting end of line but got at line %d : %s\n", lines, current.toString(asm.input))
            }
            lines += 1
            index += 1
            current = asm.tokens[index]
            continue
        }

        if current.tokenType == EOF {
            break
        }else {
            log.Fatalf("unexpected token at %d : %s\n",lines , asm.tokens[index].toString(asm.input))
        }
    }

}


func Compile(asm *Assembler) {
    statements(asm)
}


