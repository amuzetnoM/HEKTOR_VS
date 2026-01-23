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
        "../../build/_deps/ggml-src/include",
        "../../build/_deps/llama-src/include",
        "../../build/_deps/onnxruntime-src/include",
        "/usr/include/postgresql",
        "/usr/local/include"
      ],
      "libraries": [
        "<(module_root_dir)/../../build/src/libvdb_core.a",
        "-lpthread",
        "-ldl",
        "-lsqlite3",
        "-lpq",
        "-lssl",
        "-lcrypto",
        "-lz"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ],
      "cflags!": ["-fno-exceptions"],
      "cflags_cc!": ["-fno-exceptions"],
      "defines": ["NAPI_DISABLE_CPP_EXCEPTIONS", "NAPI_VERSION=8"],
      "conditions": [
        ["OS=='win'", {
          "msvs_settings": {
            "VCCLCompilerTool": {
              "ExceptionHandling": 1,
              "AdditionalOptions": ["/std:c++23", "/bigobj"]
            }
          },
          "libraries": [
            "../../build/src/Release/vdb_core.lib",
            "sqlite3.lib",
            "libpq.lib",
            "ws2_32.lib",
            "advapi32.lib"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
            "MACOSX_DEPLOYMENT_TARGET": "10.15",
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
          "cflags_cc": ["-std=c++23", "-fexceptions", "-mavx2", "-fPIC"],
          "ldflags": [
            "-Wl,--no-as-needed",
            "-Wl,--whole-archive",
            "<(module_root_dir)/../../build/src/libvdb_core.a",
            "-Wl,--no-whole-archive"
          ]
        }]
      ]
    }
  ]
}
