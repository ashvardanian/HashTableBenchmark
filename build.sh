if [[ $1 == "clean" ]]; then
    rm -rf build/*
fi
mkdir -p build &&
cmake -DCMAKE_BUILD_TYPE=Release -B ./build &&
make -j4 -C ./build &&
echo "Success!"
