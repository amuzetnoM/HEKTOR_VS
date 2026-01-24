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
        "../../external/fmt/include",
        "../../external/json/include",
        "../../build/_deps/fmt-src/include",
        "../../build/_deps/json-src/include"
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
              "RuntimeLibrary": 2,
              "AdditionalOptions": ["/bigobj", "/EHsc", "/Zc:__cplusplus", "/std:c++latest", "/utf-8"]
            },
            "VCLinkerTool": {
              "AdditionalLibraryDirectories": [
                "../../build/Release"
              ]
            }
          },
          "include_dirs": [
            "../../external/sqlite3"
          ],
          "libraries": [
            "vdb_core.lib",
            "ws2_32.lib",
            "advapi32.lib"
          ],
          "defines": [
            "WIN32",
            "_WINDOWS",
            "NOMINMAX",
            "_CRT_SECURE_NO_WARNINGS"
          ]
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
            "MACOSX_DEPLOYMENT_TARGET": "13.0",
            "OTHER_CPLUSPLUSFLAGS": [
              "-std=c++23",
              "-fexceptions"
            ],
            "OTHER_LDFLAGS": [
              "-stdlib=libc++",
              "-L../../build"
            ]
          },
          "include_dirs": [
            "/usr/local/include",
            "/opt/homebrew/include"
          ],
          "libraries": [
            "-L../../build",
            "-lvdb_core",
            "-lpthread",
            "-lsqlite3"
          ]
        }],
        ["OS=='linux'", {
          "cflags_cc": [
            "-std=c++23",
            "-fexceptions",
            "-fPIC"
          ],
          "libraries": [
            "<(module_root_dir)/../../build/libvdb_core.a",
            "-lpthread",
            "-ldl",
            "-lsqlite3"
          ],
          "ldflags": [
            "-Wl,--no-as-needed"
          ]
        }]
      ]
    }
  ]
}
