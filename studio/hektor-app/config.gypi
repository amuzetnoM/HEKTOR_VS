{
  "variables": {
    "standalone_static_library": 1
  },
  "target_defaults": {
    "conditions": [
      ["OS=='win'", {
        "msvs_settings": {
          "VCCLCompilerTool": {
            "AdditionalOptions": [
              "/std:c++latest",
              "/Zc:__cplusplus"
            ]
          }
        },
        "msvs_configuration_attributes": {
          "CharacterSet": 1
        },
        "configurations": {
          "Release": {
            "msvs_settings": {
              "VCCLCompilerTool": {
                "AdditionalOptions": ["/std:c++latest"]
              }
            }
          },
          "Debug": {
            "msvs_settings": {
              "VCCLCompilerTool": {
                "AdditionalOptions": ["/std:c++latest"]
              }
            }
          }
        }
      }]
    ]
  }
}
