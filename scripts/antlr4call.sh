#!/bin/bash

export ANTLR_HOME="~/.local/bin"
export ANTLR_JAR="$ANTLR_HOME/antlr-4.9.3-complete.jar"
export CLASSPATH=".:$ANTLR_JAR:$CLASSPATH"
alias antlr4="java -jar $ANTLR_JAR"
alias grun="java org.antlr.v4.gui.TestRig"

cd ~/.local/bin && [ ! -f "antlr-4.9.3-complete.jar" ] && wget https://www.antlr.org/download/antlr-4.9.3-complete.jar
cd -

if [ $1 ] ; then
  java -jar ~/.local/bin/antlr-4.9.3-complete.jar -o Parser -lib Parser -encoding UTF-8 -Dlanguage=Cpp -listener -visitor $1
fi