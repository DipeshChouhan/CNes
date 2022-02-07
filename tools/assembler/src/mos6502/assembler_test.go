package mos6502
import (
    "testing"
)

func TestGetNumber (t *testing.T) {

    if getNumber('A') != 10 || getNumber('0') != 0 || getNumber('g') != 16{
        t.Fatalf("getNumber() function failed")
    }
}
