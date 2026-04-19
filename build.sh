bazel build -c opt -s --config=wasm //lib:main-wasm
sudo cp bazel-bin/lib/main-wasm/main.wasm ./docs/main.wasm
sudo cp bazel-bin/lib/main-wasm/main.js ./docs/main.js