package main

import (
	"bufio"
	"fmt"
	"os"
)

type InputCode struct {
    codeLines []string
    err error
}

type CmdUtil struct {
    totalFiles int
    inputCodes []InputCode
}


func (cmdUtil *CmdUtil) getInputs() {

    cmdArgs := os.Args[1:]
    
    cmdUtil.totalFiles = len(cmdArgs)

    var mFile *os.File
    var err error
    var mScanner *bufio.Scanner
    var mLine string
    for index :=  0; index < len(cmdArgs); index += 1 {
        mFile, err = os.Open(cmdArgs[index])

        if err != nil {
            fmt.Println(err.Error())
            continue
        }

        mScanner = bufio.NewScanner(mFile)
        var inputCode InputCode

        for mScanner.Scan() {
            mLine = mScanner.Text()

            if len(mLine) > 0 {
                inputCode.codeLines = append(inputCode.codeLines, mLine)
            }
        }

        inputCode.err = mScanner.Err()
        cmdUtil.inputCodes = append(cmdUtil.inputCodes, inputCode)
        
    }
    
        
}
