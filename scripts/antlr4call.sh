#!/bin/bash

# This file is auto-generted by retractordb/conanfile.py by conan install ..

export ANTLR_HOME="~/.local/bin"
export ANTLR_JAR="$ANTLR_HOME/antlr-4.10.1-complete.jar"
export CLASSPATH=".:$ANTLR_JAR:$CLASSPATH"
alias antlr4="java -jar $ANTLR_JAR"
alias grun="java org.antlr.v4.gui.TestRig"

cd ~/.local/bin && [ ! -f "antlr-4.10.1-complete.jar" ] && wget https://www.antlr.org/download/antlr-4.10.1-complete.jar
cd -

if [ $1 ] ; then
  java -jar ~/.local/bin/antlr-4.10.1-complete.jar -o Parser -lib Parser -encoding UTF-8 -Dlanguage=Cpp -listener $1
fi
