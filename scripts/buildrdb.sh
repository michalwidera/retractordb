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
options=("Release" "Debug" "Reset (clean)" "Init Conan Profile" "setup gcc23" "setup ninja" "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "Release")
            sed 's/Debug/Release/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv /.conan2/profiles/temp ~/.conan2/profiles/default
            conan source $build_folder
            conan install $build_folder -s build_type=Release --build missing
            conan build $build_folder -s build_type=Release --build missing
            break
            ;;
        "Debug")
            sed 's/Release/Debug/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default 
            conan source $build_folder
            conan install $build_folder -s build_type=Debug --build missing
            conan build $build_folder -s build_type=Debug --build missing
            break
            ;;
        "Reset (clean)")
            rm -rf $build_folder/build
            rm -rf ~/.conan2/
            break
            ;;
        "setup gcc23")
            sed 's/compiler.cppstd=gnu17/compiler.cppstd=gnu23/g' <~/.conan2/profiles/default >~/.conan2/profiles/temp && mv ~/.conan2/profiles/temp ~/.conan2/profiles/default 
            cat ~/.conan2/profiles/default
            break
            ;;
        "Init Conan Profile")
            conan profile detect -f
            break
            ;;
        "setup ninja")
            echo '[conf]' >> ~/.conan2/profiles/default
            echo 'tools.cmake.cmaketoolchain:generator=Ninja' >> ~/.conan2/profiles/default
            cat ~/.conan2/profiles/default
            break
            ;;
        "Quit")
            echo "Ok, quit - no action."
            cat ~/.conan2/profiles/default
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done
