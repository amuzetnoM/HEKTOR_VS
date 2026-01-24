{
  "variables": {
    "msvs_enable_exceptions": 1
  },
  "target_defaults": {
    "msvs_settings": {
      "VCCLCompilerTool": {
        "LanguageStandard": "stdcpp23",
        "AdditionalOptions": ["/Zc:__cplusplus"]
      }
    },
    "conditions": [
      ["OS=='linux'", {
        "cflags_cc!": ["-std=gnu++17", "-std=gnu++14"],
        "cflags_cc": ["-std=c++23"]
      }],
      ["OS=='mac'", {
        "xcode_settings": {
          "CLANG_CXX_LANGUAGE_STANDARD": "c++23",
          "OTHER_CPLUSPLUSFLAGS": ["-std=c++23"]
        }
      }]
    ]
  }
}
