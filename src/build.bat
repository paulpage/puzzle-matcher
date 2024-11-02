pushd ..\projects\VS2022\
msbuild game.sln /target:game /property:Configuration=Debug /property:Platform=x64 /property:RaylibSrcPath="C:\dev\lib\raylib\src"
popd
