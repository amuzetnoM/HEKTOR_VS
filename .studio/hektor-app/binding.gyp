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
        "native-addon/src/quantization.cpp",
        "native-addon/src/utils.cpp",
        "native-addon/src/embeddings.cpp",
        "native-addon/src/storage.cpp",
        "native-addon/src/index.cpp",
        "native-addon/src/hybrid.cpp",
        "native-addon/src/rag.cpp",
        "native-addon/src/distributed.cpp",
        "native-addon/src/framework.cpp",
        "native-addon/src/telemetry.cpp",
        "native-addon/src/common.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "native-addon/include",
        "../../include",
        "../../build/_deps/json-src/include",
        "../../build/_deps/fmt-src/include",
        "../../build/_deps/ggml-src/include",
        "../../build/_deps/llama-src/include",
        "../../build/_deps/onnxruntime-src/include",
        "/usr/include/postgresql",
        "/usr/local/include"
      ],
      "libraries": [
        "<(module_root_dir)/../../build/libvdb_core.a",
        "<(module_root_dir)/../../build/_deps/fmt-build/libfmt.a",
        "-lpthread",
        "-ldl"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions", "-std=gnu++17", "-std=gnu++14", "-std=c++17", "-std=c++14"],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS", "NAPI_VERSION=8"],
      "conditions": [
        ["OS=='win'", {
          "defines": ["_HAS_EXCEPTIONS=1", "WIN32_LEAN_AND_MEAN", "NOMINMAX"],
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "RuntimeLibrary": 2,
              "LanguageStandard": "stdcpp23",
              "AdditionalOptions": ["/bigobj", "/EHsc", "/Zc:__cplusplus"]
            },
            "VCLinkerTool": {
              "AdditionalLibraryDirectories": [
                "../../build/Release",
                "../../build/_deps/fmt-build/Release"
              ]
            }
          },
          "libraries": [
            "../../build/Release/vdb_core.lib",
            "../../build/_deps/fmt-build/Release/fmt.lib",
            "ws2_32.lib",
            "advapi32.lib"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
            "OTHER_CPLUSPLUSFLAGS": ["-std=c++23"],
            "OTHER_LDFLAGS": [
              "-stdlib=libc++",
              "-L/usr/local/lib",
              "-L/opt/homebrew/lib"
            ]
          },
          "include_dirs": [
            "/usr/local/include",
            "/opt/homebrew/include"
          ]
        }],
        ["OS=='linux'", {
          "cflags_cc": ["-std=c++23", "-fexceptions", "-fPIC"],
          "ldflags": [
            "-Wl,--no-as-needed",
            "-Wl,--whole-archive",
            "<(module_root_dir)/../../build/libvdb_core.a",
            "-Wl,--no-whole-archive"
          ]
        }]
      ]
    }
  ]
}
