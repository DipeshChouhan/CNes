package main

import (
	"assembler/src/mos6502"
	"fmt"
	"log"
	"os"
	"strings"
)


func check(err error) {
    if err != nil {
        log.Fatal(err)
    }
}

func outputCode(output []byte, flag int, path string) {

    
    // files or std output
    
    fmt.Println(string(output))
}

func main(){

    
    args := os.Args[1:]

    if len(args) == 0 {
        log.Fatal("Not enough arguments...")
    }
    
    for _, arg := range args {

        file, err := os.Open(arg)
            
        check(err)

        fileInfo, _ := file.Stat()

        if !fileInfo.IsDir() {
            data := make([]byte, fileInfo.Size())
            _, err := file.Read(data)
            check(err)
            output := mos6502.Assemble(data)
            
            outputFile := strings.TrimSuffix(file.Name(),fileInfo.Name()[strings.Index(fileInfo.Name(), "."):])
            outputFile += ".bin"
            err = os.WriteFile(outputFile, output, 0644)
            check(err)
            file.Close()
            continue
        }

        // file is a directory

        entries, err := file.ReadDir(mos6502.MaxFiles)
        check(err)

        for _, entry := range entries {

            fullName := file.Name() + entry.Name()
            fmt.Println(fullName)

            if !entry.IsDir(){

                for _, suffix := range mos6502.SupportedFiles {
                    if strings.HasSuffix(entry.Name(), suffix) {
                        data, err := os.ReadFile(fullName)
                        check(err)
                        mos6502.Assemble(data)
                    }
                }

            }
        }
        file.Close()
    }

}
