#!/bin/bash

if [ "$1" != "" ]; then
FILE=$1
else
FILE=query-mwnd.rql
fi

if ! xcompiler -q $FILE ; then exit 1 ; fi 

if [ "$2" == "x" ]; then exit 1 ; fi

tmux has-session -t dev
if [ $? != 0 ]
then
    tmux new-session -s dev -n "TEST" -d
    tmux split-window -v
    tmux resize-pane -t dev:0.0 -y 6
    tmux split-window -h
    tmux split-window -h
    tmux split-window -h
    tmux split-window -h
    tmux split-window -h

    tmux set -g pane-border-status top
    tmux set -g pane-border-format "#{pane_index}"

    tmux resize-pane -t dev:0.1 -x 12
    tmux resize-pane -t dev:0.2 -x 12
    tmux resize-pane -t dev:0.3 -x 12
    tmux resize-pane -t dev:0.4 -x 12
    tmux resize-pane -t dev:0.5 -x 12
    tmux resize-pane -t dev:0.6 -x 12

    tmux send-keys -t dev:0.0 'xretractor -v' C-m
    sleep 2
    tmux send-keys -t dev:0.1 'clear; xqry -s core' C-m
    tmux send-keys -t dev:0.2 'clear; xqry -s agregation' C-m
    tmux send-keys -t dev:0.3 'clear; xqry -s agregationSerialization' C-m
    tmux send-keys -t dev:0.4 'clear; xqry -s agregationMirror' C-m
    tmux send-keys -t dev:0.5 'clear; xqry -s agregationSelective' C-m
    tmux send-keys -t dev:0.6 'clear; xqry -s serialization' C-m
fi
tmux attach -t dev
