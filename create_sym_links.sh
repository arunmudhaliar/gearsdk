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

echo 'symlink script finished.'