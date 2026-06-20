# CPACK_PROJECT_CONFIG_FILE — wykonywany przez cpack osobno dla każdego pakietu.
# Prefiks instalacji /usr ma sens tylko dla pakietów BINARNYCH (DEB/TGZ z układem
# /usr/bin, /usr/lib/systemd/system). Pakiet ŹRÓDŁOWY powinien mieć drzewo źródeł
# w korzeniu archiwum, a nie zagnieżdżone pod usr/ — dlatego dla niego zerujemy
# prefiks.
#
# Rozróżnienie: przy budowie źródła cpack ustawia CPACK_SOURCE_INSTALLED_DIRECTORIES
# (a CPACK_INSTALL_CMAKE_PROJECTS jest puste); przy binarnym odwrotnie.
if(CPACK_SOURCE_INSTALLED_DIRECTORIES)
  set(CPACK_PACKAGING_INSTALL_PREFIX "")
endif()
