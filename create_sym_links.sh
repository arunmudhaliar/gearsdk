arch=$(uname -m)

if [ "$arch" = "arm64" ]; then
    echo "ARM architecture"
elif [ "$arch" = "x86_64" ]; then
    echo "x86_64 architecture"
else
    echo "Unknown architecture: $arch"
    exit 1
fi

# 'common sym link'
echo 'creating common symlink'
rm -rf ./common/libs/macos
cd ./common/libs
ln -s ./macos_$arch ./macos
cd ../../

# 'qhiredis sym link'
echo 'creating qhiredis symlink'
rm -rf ./qhiredis/libs/macos
cd ./qhiredis/libs
ln -s ./macos_$arch ./macos
cd ../../

# 'networkcommon sym link'
echo 'creating networkcommon symlink'
rm -rf ./networkcommon/libs/macos
cd ./networkcommon/libs
ln -s ./macos_$arch ./macos
cd ../../

# 'servercommon sym link'
echo 'creating servercommon symlink'
rm -rf ./servercommon/libs/macos
cd ./servercommon/libs
ln -s ./macos_$arch ./macos
cd ../../

# 'qzookeeper sym link'
echo 'creating qzookeeper symlink'
rm -rf ./qzookeeper/libs/macos
cd ./qzookeeper/libs
ln -s ./macos_$arch ./macos
cd ../../

# 'qutils sym link'
echo 'creating qutils symlink'
rm -rf ./qutils/libs/macos
cd ./qutils/libs
ln -s ./macos_$arch ./macos
cd ../../

# 'qunitysdk sym link'
echo 'creating qunitysdk symlink'
rm -rf ./qunityplugin/qunityplugin_unity_proj/Assets/Plugins/qunitysdk
cd ./qunityplugin/qunityplugin_unity_proj
cd ./Assets/Plugins
ln -s ./../../qunitysdk_module/source ./qunitysdk
rm -rf ./qunitysdk/libs
mkdir ./qunitysdk/libs
cd ./qunitysdk/libs
ln -s ./../../../qunitysdk_module/libs/android ./android
ln -s ./../../../qunitysdk_module/libs/macos_$arch ./macos
ln -s ./../../../qunitysdk_module/libs/linux ./linux
cd ../../../../../../
# echo $(pwd)
echo 'symlink script finished.'