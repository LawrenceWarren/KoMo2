if kmd ; then
    echo "KMD exists."
else
    echo "Adding KMD to path."
    KMD_PATH=$(readlink -f bin/kmd)
    echo "Aliasing kmd path"
    $(export PATH="$PATH:$KMD_PATH")
    echo "Path set."
fi
