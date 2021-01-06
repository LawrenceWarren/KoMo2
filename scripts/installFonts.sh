#!/bin/sh

# /////////////////////////////////////////////////////////////////
#                                                                 #
#  hack-linux-installer.sh                                        #
#  A shell script that installs the Hack fonts from repository    #
#  releases by release version number                             #
#                                                                 #
#  Copyright 2018 Christopher Simpkins                            #
#  MIT License                                                    #
#                                                                 #
# /////////////////////////////////////////////////////////////////

# This shell script was altered by Lawrence Warren for installing the archive
# directory for Fira-Code 5.2 release.

FIRA_CODE_INSTALL_PATH="/usr/local/share/fonts"
FIRA_CODE_ARCHIVE_PATH="../res/FiraCode-5.2.tar.gz"
FIRA_CODE_TTF_PATH="FiraCode-5.2/distr/ttf"

# Unzips the archive here
echo " "
echo "Unpacking the font files..."
if [ -f "$FIRA_CODE_ARCHIVE_PATH" ]; then
    tar -xzvf "$FIRA_CODE_ARCHIVE_PATH"
else
    echo "Unable to find the pulled archive file.  Install failed."
    exit 1
fi

# install
if [ -d "FiraCode-5.2" ]; then
    echo " "
    echo "Installing the Fira Code fonts..."

    # move fonts to install directory
    echo "Installing FiraCode-Bold.ttf on path $FIRA_CODE_INSTALL_PATH/FiraCode-Bold.ttf"
    mv "$FIRA_CODE_TTF_PATH/FiraCode-Bold.ttf" "$FIRA_CODE_INSTALL_PATH/FiraCode-Bold.ttf"

    echo "Installing FiraCode-Light.ttf on path $FIRA_CODE_INSTALL_PATH/FiraCode-Light.ttf"
    mv "$FIRA_CODE_TTF_PATH/FiraCode-Light.ttf" "$FIRA_CODE_INSTALL_PATH/FiraCode-Light.ttf"

    echo "Installing FiraCode-Medium.ttf on path $FIRA_CODE_INSTALL_PATH/FiraCode-Medium.ttf"
    mv "$FIRA_CODE_TTF_PATH/FiraCode-Medium.ttf" "$FIRA_CODE_INSTALL_PATH/FiraCode-Medium.ttf"
    
    echo "Installing FiraCode-Regular.ttf on path $FIRA_CODE_INSTALL_PATH/FiraCode-Regular.ttf"
    mv "$FIRA_CODE_TTF_PATH/FiraCode-Regular.ttf" "$FIRA_CODE_INSTALL_PATH/FiraCode-Regular.ttf"

    echo "Installing FiraCode-Retina.ttf on path $FIRA_CODE_INSTALL_PATH/FiraCode-Retina.ttf"
    mv "$FIRA_CODE_TTF_PATH/FiraCode-Retina.ttf" "$FIRA_CODE_INSTALL_PATH/FiraCode-Retina.ttf"

    echo "Installing FiraCode-SemiBold.ttf on path $FIRA_CODE_INSTALL_PATH/FiraCode-SemiBold.ttf"
    mv "$FIRA_CODE_TTF_PATH/FiraCode-SemiBold.ttf" "$FIRA_CODE_INSTALL_PATH/FiraCode-SemiBold.ttf"

    # Remove the rest of the uncompressed archive file.
    echo " "
    echo "Cleaning up..."
    rm -rf FiraCode-5.2

    # clear and regenerate font cache
    echo " "
    echo "Clearing and regenerating the font cache.  You will see a stream of text as this occurs..."
    echo " "
    fc-cache -f -v

    # Lists of the installed Fira Code font family
    echo " "
    echo "Testing. You should see the expected install filepaths in the output below..."
    fc-list | grep "Fira Code"

    # exit
    echo " "
    echo "Install of Fira Code 5.2 complete."
    exit 0
else
    echo "Unable to identify the unpacked font directory. Install failed."
    exit 1
fi
