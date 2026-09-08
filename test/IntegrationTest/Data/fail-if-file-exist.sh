#!/bin/sh

# The purpose of this code is to test generated artifacts
# I've use to create xrdb artifact.
# Placing this into .local/bin causes hang of Xservice
# I need to make sure that no one will generate such artifact
# again.

if [ "$1" ]; then

  ARTIFACTS=$( find ~/.local/bin -name "$1")

  if [ "$ARTIFACTS" != "" ]; then
    echo "Found."
    exit 1
  else
    echo "Ok, not Found."
    exit 0
  fi
else
  echo "missing output artifact file." >&2
  exit 1
fi