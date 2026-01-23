{
  "targets": [
    {
      "target_name": "hektor_native",
      "sources": [
        "native-addon/src/binding.cpp",
        "native-addon/src/database.cpp",
        "native-addon/src/async_operations.cpp",
        "native-addon/src/search.cpp",
        "native-addon/src/collections.cpp",
        "native-addon/src/ingestion.cpp",
        "native-addon/src/index_mgmt.cpp",
        "native-addon/src/quantization.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "native-addon/include",
        "../../include",
        "../../build/_deps/ggml-src/include",
        "../../build/_deps/llama-src/include"
      ],
      "libraries": [
        "<(module_root_dir)/../../build/libvdb_core.a",
        "-lpthread",
        "-ldl"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS"],
      "conditions": [
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++23"]
            }
          },
          "libraries": [
            "../../build/Release/vdb_core.lib"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
            "OTHER_LDFLAGS": ["-stdlib=libc++"]
          }
        }],
        ["OS=='linux'", {
          "cflags_cc": ["-std=c++23", "-fexceptions", "-mavx2"],
          "ldflags": ["-Wl,--no-as-needed"]
        }]
      ]
    }
  ]
}
