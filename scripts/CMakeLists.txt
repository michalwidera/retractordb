cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

add_custom_target(
  vimconf
  COMMAND cp -r .vim ~
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/scripts)

# Lokalne odtworzenie jobow CircleCI w kontenerze dockerowym.
#
# Poza `all` i poza `ninja test`: kazdy profil stawia kontener, przenosi do
# niego drzewo robocze i buduje od zera na 4 vCPU — to sa minuty do
# kilkudziesieciu minut, nie sekundy. Uruchamia sie jawnie, przed commitem,
# zamiast sprawdzac gałąź przebiegiem CI.
#
# Profile odwzorowuja joby z .circleci/config.yml; ich lista i opisy siedza w
# scripts/test-ci.sh (`--list`) i tam tez nalezy je poprawiac razem z
# config.yml.
set(RDB_TEST_CI_PROFILES commit release smoke ablation-all-off
                         ablation-probe-on gate)

foreach(rdb_ci_profile IN LISTS RDB_TEST_CI_PROFILES)
  add_custom_target(
    test-ci-${rdb_ci_profile}
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/test-ci.sh --profile ${rdb_ci_profile}
            --out-dir ${CMAKE_BINARY_DIR}/test-ci
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "CI lokalnie w dockerze: profil ${rdb_ci_profile}"
    USES_TERMINAL VERBATIM)
endforeach()

# Skrot na profil commitowy — jedyny job, ktory CircleCI uruchamia po pushu na
# galaz (workflow `commit`).
add_custom_target(test-ci COMMENT "CI lokalnie w dockerze: profil commit")
add_dependencies(test-ci test-ci-commit)

# Ten sam profil, ale z katalogiem build/ zachowanym miedzy przebiegami. Osobna
# nazwa zamiast przelacznika przy `test-ci`, bo to NIE jest juz odtworzenie CI:
# CircleCI buduje z czystego checkoutu, a tutaj zostaje CMakeCache i obiekty z
# poprzedniego biegu. Do sprawdzenia przed commitem, nie do wnioskowania o
# przebiegu na CI.
add_custom_target(
  test-ci-fast
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/test-ci.sh --profile commit --reuse-build
          --out-dir ${CMAKE_BINARY_DIR}/test-ci
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "CI lokalnie w dockerze: profil commit, build inkrementalny"
  USES_TERMINAL VERBATIM)
