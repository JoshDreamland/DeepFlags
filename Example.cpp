#include "DeepFlags.hpp"
#include <string>
#include <vector>
#include <iostream>

struct DisplayFile : Flags::FlagGroup {
  Flags::Flag<std::string> file = Flags::flag(this, "file", 'f')
      .description("Specifies the file to read (reads from stdin by default).");
  Flags::Flag<std::string> label = Flags::flag(this, "label", 'l')
      .description("Assigns a label to this file's tab.");
  Flags::Flag<std::vector<int>> bookmarks = Flags::flag(this, "bookmark", 'b')
      .description("Gives the number of this entity to create.");
  Flags::Switch createIfMissing = Flags::flag(this, 'p')
      .description("Denotes that if this file does not exist, it should be"
          " created. If bookmarks are specified, the file will be sized to"
          " contain the largest bookmark.");

  DisplayFile(CtorArgs args): FlagGroup(args) {}
};

struct AllFlags : Flags::FlagGroup {
  Flags::Flag<Flags::Repeated<DisplayFile>> files =
      Flags::flag(this, "display", 'D')
          .description("Create a tab to display a given file.");
};

using std::cout;
using std::endl;
int main(int argc, char* argv[]) {
  AllFlags flags;
  flags.printHelp(std::cout);
  bool success = flags.parseArgs(argc, argv);
  
  if (!success) {
    cout << "Flag parse failed, but continuing anyway for demo purposes."
         << endl;
  }
  
  auto &files = flags.files.value;
  cout << "I was told to load " << files.size() << " files." << endl;
  for (DisplayFile file : files) {
    if (file.file.present) {
      cout << "I will load \"" << file.file.value << "\", ";
    } else {
      cout << "I will read from stdin, ";
    }
    if (file.label.present) {
      cout << "labeling the tab \"" << file.label.value << "\", ";
    }
    if (file.bookmarks.value.size()) {
      cout << "bookmarking lines ";
      auto &bookmarks = file.bookmarks.value;
      for (size_t i = 0; i < bookmarks.size() - 1; ++i) {
        cout << bookmarks[i] << ", ";
      }
      if (bookmarks.size() > 1) {
        cout << "and ";
      }
      cout << bookmarks[bookmarks.size() - 1] << ", ";
    }
    cout << (file.createIfMissing.present ? "creating the file" : "bailing")
         << " if the file doesn't exist." << endl;
  }
}
