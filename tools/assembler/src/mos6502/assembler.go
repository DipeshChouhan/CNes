package mos6502

import (
	"encoding/json"
	"fmt"
	"log"
	"os"
)

const MaxFiles int = 255



var SupportedFiles = [3]string{".bin", ".asm8", ".nes"}

type opInfo struct {
    modes []string
    mem []string
}

type JsonInfo struct {
    Name string `json:"name"`
    Modes []string  `json:"modes"`
    Mem []string    `json:"mem"`
}

func loadOpData(fileName string) map[string]opInfo {
    result := make(map[string]opInfo)
    jsonData, err := os.ReadFile(fileName)

    
    if err != nil {
        log.Fatal(err)
    }

    var ops []JsonInfo

    json.Unmarshal(jsonData, &ops)

    for _, op := range ops {
        result[op.Name] = opInfo{op.Modes, op.Mem}
    }
    return result
}

// first return is isAlpha or _ another is alpha_numeric
func isAlphaNumeric(char byte) (bool, bool) {

    char = char | 0b00100000
    return (char > 47) && (char < 58), ((char > 96) && (char < 123)) || char == '_'
}

func getNumber(char byte) int{

    char = char | 0b00100000

    if (char > 47) && (char < 58) {
        return int(char - '0')
    }

    if (char > 96) && (char < 123) {
        return int((char - 'a')) + 10
    }

    return 16
}

type TokenType int



const (
    NUMBER TokenType = iota
    LABLE
    INDENTIFIER
    EOF
    OPEN_BRACKET 
    CLOSE_BRACKET
    NEWLINE
    COMMA
    HASH
    ACCUMULATOR
    REGISTER_X
    REGISTER_Y
    INSTRUCTION
    ADC
    AND
    ASL
    BCC
    BCS
    BEQ
    BIT
    BMI
    BNE
    BPL
    BRK
    BVC
    BVS
    CLC
    CLD
    CLI
    CLV
    CMP
    CPX
    CPY
    DEC
    DEX
    DEY
    EOR
    INC
    INX
    INY
    JMP
    JSR
    LDA
    LDX
    LDY
    LSR
    NOP
    ORA
    PHA
    PHP
    PLA
    PLP
    ROL
    ROR
    RTI
    RTS
    SBC
    SEC
    SED
    SEI
    STA
    STX
    STY
    TAX
    TAY
    TSX
    TXA
    TXS
    TYA

    // This Constants are not going to used in scanner, just for printing data
    INPUT
    OUTPUT
    TOKENS
    LINES
    IMMEDIATE = "i"
    ZERO_PAGE = "z"
    ZERO_PAGE_X = "zx"
    ZERO_PAGE_Y = "zy"
    ABSOLUTE    = "a"
    ABSOLUTE_X  = "ax"
    ABSOLUTE_Y  = "ay"
    INDIRECT    = "in"
    INDIRECT_X  = "inx"
    INDIRECT_Y  = "iny"
    MODE_ACCUMULATOR = "ac"
    RELATIVE    = "r"
    IMPLIED = "im"

)

type Token struct {
    tokenType TokenType
    start int 
    size int
    value int
}

func (tk Token) toString(input []byte) string {
    
    if tk.tokenType == NEWLINE{
        return fmt.Sprintf("{ '\\n' }")
    }else if tk.tokenType == NUMBER {
        return fmt.Sprintf("{ 'number' , $%X }", tk.value)
    }

    return fmt.Sprintf("{ '%s' }", input[tk.start : tk.start + tk.size])
}
func printData (asm *Assembler, what TokenType) {
    switch what {
    case INPUT:

        fmt.Println("--INPUT--\n", string(asm.input))

    case TOKENS:
        fmt.Println("--TOKENS--")
        for _, tk := range asm.tokens {
            fmt.Println(tk.toString(asm.input))
        }

    case OUTPUT: {
        for i := 0; i < len(asm.output); i++ {
            if (i+1) % 14 == 0 {
                fmt.Println()
            }
            fmt.Printf("0x%X  ", asm.output[i])
        }
        fmt.Println()
    }

    case LINES:
        
        fmt.Println("--Lines--")
        fmt.Println(asm.lineNumber)
    }
}

type Assembler struct {
    input []byte
    tokens []Token
    lineNumber []int
    output []byte
    opData map[string]opInfo
    lables map[string]int
}



func Assemble(input []byte) ([]byte){
    var tokens []Token 
    var lineNumber []int
    var output []byte
    lables := make(map[string]int)
    
    opData := loadOpData("/home/akku/programming/emulators/mos_6502/tools/assembler/src/data/opcodes.json")
    asm := Assembler{input, tokens, lineNumber, output, opData, lables}
    scan(&asm)
    asm.tokens = append(asm.tokens, Token{EOF, 0, 0, 0})

    // printData(&asm, INPUT)
    // printData(&asm, LINES)
    // printData(&asm, TOKENS)
    
    Compile(&asm)
    printData(&asm, OUTPUT)
    return asm.output
}
