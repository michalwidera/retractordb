import VersoManual
import ProfsManual

open Verso Doc
open Verso.Genre Manual

def config : Config where
  destination := "build/verso"
  emitTeX := false
  emitHtmlSingle := .immediately
  emitHtmlMulti := .immediately
  htmlDepth := 1

def main := manualMain (%doc ProfsManual) (config := { config with })
