package main

import (
	"fmt"
    "asm6502/internal/assembler"
)

const VERSION = "0.0"


func main() {
    fmt.Println("Mos6502 Assembler...")

    var cmdUtils CmdUtil

    cmdUtils.getInputs()

    for i := 0; i < cmdUtils.totalFiles; i += 1 {
        inputCode := cmdUtils.inputCodes[i]
        if inputCode.err != nil {
            println(inputCode.err.Error())
            continue
        }
        assembler.Assemble(inputCode.codeLines)
    }
}
