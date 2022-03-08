package main

import (
	"fmt"
)

const VERSION = "0.0"


func main() {
    fmt.Println("Mos6502 Assembler...")

    var cmdUtils CmdUtil

    cmdUtils.getInputs()

    if cmdUtils.totalFiles > 0 {
        for i := 0; i < len(cmdUtils.inputCodes[0].codeLines); i += 1 {
            println(cmdUtils.inputCodes[0].codeLines[i])
        }
    }
}
