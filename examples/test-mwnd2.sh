#!/bin/bash

if [ "$1" != "" ]; then
FILE=$1
else
FILE=query-mwnd2.rql
fi

if ! xcompiler -q $FILE ; then exit 1 ; fi

if [ "$2" == "x" ]; then exit 1 ; fi

tmux has-session -t dev
if [ $? != 0 ]
then
    tmux new-session -s dev -n "TEST" -d
    tmux split-window -v
    tmux resize-pane -t 0 -y 5
    tmux split-window -h
    tmux split-window -h
    tmux split-window -h
    tmux split-window -h

    tmux set -g pane-border-status top
    tmux set -g pane-border-format "#{pane_index}"

    tmux resize-pane -t 1 -x 15
    tmux resize-pane -t 2 -x 15
    tmux resize-pane -t 3 -x 15
    tmux resize-pane -t 4 -x 15
    tmux resize-pane -t 5 -x 15

    # https://github.com/tmux/tmux/issues/1778
    # "Probably the enter is being sent before the shell
    # has started and it is eating it."
    # Therefore sleep 5 here.
    sleep 5

    tmux send-keys -t 0 "xretractor -v" Enter
    sleep 1
    tmux send-keys -t 1 "sleep 2; clear; xqry -s core0" Enter
    tmux send-keys -t 2 "sleep 2; clear; xqry -s str1" Enter
    tmux send-keys -t 3 "sleep 2; clear; xqry -s str2" Enter
    tmux send-keys -t 4 "sleep 2; clear; xqry -s str3" Enter
    tmux send-keys -t 5 "sleep 2; clear; xqry -s str4" Enter
fi
tmux attach -t dev
