#pragma once

#include <string>

class UIStyleSettings;

class Configurator {
 public:
  explicit Configurator();

  void Initialize();
  int Run(bool installing);
  int UpdateWorkspace(bool report_errors = false);
  int DictManagement();
  int SyncUserData();
  int CreateWord();
  int UpdateUserPhrase(const std::string& schema_id,
                      const std::string& code,
                      const std::string& text,
                      bool remove);
};
