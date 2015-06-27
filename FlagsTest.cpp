#include <gtest/gtest.h>
#include "DeepFlags.hpp"
using std::vector;
using std::string;

namespace BasicTest {

  struct EntityFlags: Flags::FlagGroup {
    Flags::Flag<int64_t> id = Flags::flag(this, "id")
        .description("The id of the entity.");
    Flags::Flag<string>  name = Flags::flag(this, "name")
        .description("The human-readable name of this entity.");
    Flags::Flag<double>  x = Flags::flag(this, "x", 'x')
        .description("The x-coordinate of this entity.");
    Flags::Flag<double>  y = Flags::flag(this, "y", 'y')
        .description("The y-coordinate of this entity.");
    EntityFlags(CtorArgs args): FlagGroup(args) {}
  };

  struct MyFlagGroup: Flags::FlagGroup {
    Flags::Flag<bool> alive = Flags::flag(this, "alive")
        .description("Whether this program is alive.");
    Flags::Flag<int64_t> param = Flags::flag(this, "param")
        .description("The parameter to this program, which is an int64.");
    Flags::Flag<vector<EntityFlags>> entities = Flags::flag(this, "entity")
        .description("Specifies an entity. You may list more than one.");
    Flags::Flag<vector<int>> inds = Flags::flag(this, "ind")
        .description("Specifies an entity. You may list more than one.");
    Flags::Switch toggle1 = Flags::flag(this, "toggle")
        .description("If toggled, something might behave differently.");
    Flags::Switch toggle2 = Flags::flag(this, "toggle2")
        .description("Similar to toggle, but presumably subtly different.");
  } allFlags;

  TEST(FlagsTest, BasicFlags) {
    const char* argv[] = {
      "flagstext.exe",
      "--alive=true",
      "--param", "20",
      "--ind=10",
        "--ind", "11",
        "--ind", "12",
        "--ind=13",
      "--toggle2",
      "--entity",
        "--id", "1337",
        "--name", "some name",
        "-x", "10.5",
        "-y", ".5",
      "--entity",
        "--id=1338",
        "--name=other name",
        "-x", "2.75",
        "-y", "4.125",
      "--ind", "14", "15", "16", "--ind", "17"
    };
    constexpr size_t argc = sizeof(argv) / sizeof(const char*);
    ASSERT_TRUE(allFlags.parseArgs(argc, argv));
    
    ASSERT_TRUE(allFlags.alive.value);
    ASSERT_EQ(20, allFlags.param.value);
    ASSERT_EQ(2u, allFlags.entities.value.size());
    ASSERT_EQ(1337, allFlags.entities.value[0].id.value);
    ASSERT_EQ("some name", allFlags.entities.value[0].name.value);
    ASSERT_EQ(10.5, allFlags.entities.value[0].x.value);
    ASSERT_EQ(0.5,  allFlags.entities.value[0].y.value);
    ASSERT_EQ(1338, allFlags.entities.value[1].id.value);
    ASSERT_EQ("other name", allFlags.entities.value[1].name.value);
    ASSERT_EQ(2.75, allFlags.entities.value[1].x.value);
    ASSERT_EQ(4.125,  allFlags.entities.value[1].y.value);
    ASSERT_EQ(8u, allFlags.inds.value.size());
    ASSERT_EQ(10, allFlags.inds.value[0]);
    ASSERT_EQ(11, allFlags.inds.value[1]);
    ASSERT_EQ(12, allFlags.inds.value[2]);
    ASSERT_EQ(13, allFlags.inds.value[3]);
    ASSERT_EQ(14, allFlags.inds.value[4]);
    ASSERT_EQ(15, allFlags.inds.value[5]);
    ASSERT_EQ(16, allFlags.inds.value[6]);
    ASSERT_EQ(17, allFlags.inds.value[7]);
    ASSERT_FALSE(allFlags.toggle1.present);
    ASSERT_TRUE(allFlags.toggle2.present);
  }

  TEST(FlagsTest, HelpPrinting) {
    std::stringstream sstream;
    allFlags.printHelp(sstream);
    std::string pstr = sstream.str();
    
  }
}

namespace Test2 {

  struct RepeatableGroup: Flags::FlagGroup {
    Flags::Flag<Flags::Sequential<int>> id = Flags::flag(this, "weights");
    RepeatableGroup(CtorArgs args): FlagGroup(args) {}
  };

  struct MyFlagGroup: Flags::FlagGroup {
    Flags::Flag<int64_t>                 param = Flags::flag(this, "param");
    Flags::Flag<vector<RepeatableGroup>> lists = Flags::flag(this, "lists");
  } allFlags;

  TEST(FlagsTest, SequentialFlagsCannotBeReentered) {
    const char* argv[] = {
      "flagstext.exe",
      "--lists",
        "--weights", "1", "2", "3",
        "--weights", "4", "5", "6",
        "--weights", "7", "8", "9",
      "--param", "1336",
      "--lists",
        "--weights", "10", "11", "12",
        "--weights", "13", "14", "15"
    };
    
    constexpr size_t argc = sizeof(argv) / sizeof(const char*);
    ASSERT_TRUE(allFlags.parseArgs(argc, argv));
    
    ASSERT_EQ(5u, allFlags.lists.value.size());
    int check = 0;
    for (int i = 0; i < 5; ++i) {
      ASSERT_EQ(3u, allFlags.lists.value[i].id.value.size())
            << "Entries (" << i << ", *) have incorrect count";
      for (int j = 0; j < 3; ++j) {
        ASSERT_EQ(++check, allFlags.lists.value[i].id.value[j])
            << "Entry (" << i << ", " << j << ") had incorrect value";
      }
    }
    
    ASSERT_EQ(1336, allFlags.param.value);
  }
}
