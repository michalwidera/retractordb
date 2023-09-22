#!/bin/bash

# https://zfredenburg.medium.com/force-a-bash-script-to-exit-on-error-ec50b374c98d
set -o errexit

foldername=${PWD##*/}          # to assign to a variable
foldername=${foldername:-/}    # to correct for the case where PWD=/

case "$foldername" in
  "retractordb") build_folder=".";;
  "scripts")     build_folder="..";;
  "build")       build_folder="..";;
  "Release")     build_folder="../..";;
  "Debug")       build_folder="../..";;
  *)             echo "Unknown current build folder << $foldername >>"
                 exit
                 ;;
esac

echo "Note: Current folder is << $foldername >> and will start build in << $build_folder >>"

PS3='Build RetractorDB, please enter your choice: '
options=("Release" "Debug" "Relase (build only)" "Debug (build only)" "Relase (make)" "Debug (make)" "Reset (clean)" "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "Release")
            conan source $build_folder
            conan install $build_folder -s build_type=Release --build missing
            conan build $build_folder -s build_type=Release --build missing
            break
            ;;
        "Debug")
            conan source $build_folder
            conan install $build_folder -s build_type=Debug --build missing
            conan build $build_folder -s build_type=Debug --build missing
            break
            ;;
        "Relase (build only)")
            conan build $build_folder -s build_type=Release
            break
            ;;
        "Debug (build only)")
            conan build $build_folder -s build_type=Debug
            break
            ;;
        "Relase (make)")
            cd $build_folder/build/Release && make
            break
            ;;
        "Debug (make)")
            cd $build_folder/build/Debug && make
            break
            ;;
        "Reset (clean)")
            cd ~/.local/bin && rm -f -- antlr-* xqry xretractor xtrdb plotblock.py
            break
            ;;
        "Quit")
            echo "Ok, quit - no action."
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done
