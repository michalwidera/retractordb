#!/bin/bash

# https://zfredenburg.medium.com/force-a-bash-script-to-exit-on-error-ec50b374c98d
set -o errexit

foldername=${PWD##*/}          # to assign to a variable
foldername=${foldername:-/}    # to correct for the case where PWD=/

echo "-- Last two lines of ~/.bashrc are:"
tail -n 2 ~/.bashrc

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

echo "-- Note: Current folder is [ $foldername ] and will start build in [ $build_folder ]"

PS3='-- Pick option, please enter your setup choice: '
options=("release" "debug" "conan" "ninja" "toolchain" "bashrc" "quit")
select opt in "${options[@]}"
do
    case $opt in
        "release")
            sed 's/Debug/Release/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default
            conan source $build_folder
            conan install $build_folder -s build_type=Release --build missing
            conan build $build_folder -s build_type=Release --build missing
            break
            ;;
        "debug")
            sed 's/Release/Debug/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default 
            conan source $build_folder
            conan install $build_folder -s build_type=Debug --build missing
            conan build $build_folder -s build_type=Debug --build missing
            break
            ;;
        "toolchain")
            sudo apt-get update
            sudo apt-get upgrade -y
            sudo apt-get -y install git gcc g++ gdb cmake make ninja-build build-essential python3 python3-pip python3-venv valgrind cppcheck mold graphviz feh tmux gnuplot clang-format
            python3 -m venv ~/.venv
            source ~/.venv/bin/activate
            pip3 install --upgrade pip 
            pip3 install conan
            if [ ! -f ~/.conan2/profiles/default ] ; then conan profile detect ; fi
            conan profile show
            break
            ;;
        "conan")
            conan profile detect -f
            sed 's/compiler.cppstd=gnu17/compiler.cppstd=gnu23/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default 
            break
            ;;
        "ninja")
            echo '[conf]' >> ~/.conan2/profiles/default
            echo 'tools.cmake.cmaketoolchain:generator=Ninja' >> ~/.conan2/profiles/default
            cat ~/.conan2/profiles/default
            break
            ;;
        "bashrc")
            cd $build_folder
            if [ "${PWD##*/}" != "retractordb" ] ; then echo "Error: Current folder is not retractordb" ; exit ; fi 
            echo 'export PATH="'${PWD}'/bin:$PATH"' >> ~/.bashrc
            echo 'source ~/.venv/bin/activate' >> ~/.bashrc
            echo "-- Last two lines of ~/.bashrc are:"
            tail -n 2 ~/.bashrc
            break
            ;;
        "quit")
            echo "-- Current conan profile is:"
            cat ~/.conan2/profiles/default
            echo "-- Ok, quit - no action."
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done
