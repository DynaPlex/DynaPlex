#include <iostream>
#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include "smallclass.h"
#include <gtest/gtest.h>
#include <unordered_map>
#include <unordered_set>

namespace DynaPlex::Tests {

    TEST(VarGroupTests, KeysMethod) {
        VarGroup vg;

        // Add different types of values
        vg.Add("key1", "value1");
        vg.Add("key2", 100);
        vg.Add("key3", false);

        // Use Keys method to retrieve all keys
        auto keys = vg.Keys();

        // Create an unordered_set from keys for easier comparison
        std::unordered_set<std::string> keysSet(keys.begin(), keys.end());

        // Check if all keys are present
        EXPECT_EQ(keysSet.size(), 3);
        EXPECT_TRUE(keysSet.find("key1") != keysSet.end());
        EXPECT_TRUE(keysSet.find("key2") != keysSet.end());
        EXPECT_TRUE(keysSet.find("key3") != keysSet.end());
    }

    TEST(VarGroupWithSmallClassMap, StoreAndRetrieve) {
        // Create some SmallClass instances
        SmallClass obj1, obj2;
        obj1.name = "First";
        obj1.Size = 1.0;
        obj2.name = "Second";
        obj2.Size = 2.0;

        // Create an unordered_map and add SmallClass instances
        std::unordered_map<std::string, SmallClass> originalMap = {
            {"obj1", obj1},
            {"obj2", obj2}
        };

        // Create a VarGroup and add the map
        VarGroup vg;
        vg.Add("originalMap", originalMap);

        std::unordered_map<std::string, SmallClass> retrievedMap;

        vg.Get("originalMap", retrievedMap);
        
        auto object1 = retrievedMap["obj1"];
        auto object2 = retrievedMap["obj2"];
        //please add asserts:
        ASSERT_EQ(object1.name, std::string("First"));
        ASSERT_EQ(object2.name, std::string("Second"));
        ASSERT_EQ(object1.Size, 1.0);
        ASSERT_EQ(object2.Size, 2.0);


    }

    TEST(VarGroupWithSmallClassMap, StoreAndRetrieveSimple) {
        std::unordered_map<std::string, int64_t> map{ {"one",1},{"two",2 } };
        std::unordered_map<std::string, int64_t> map2{};
        std::unordered_map<std::string, std::string> doesnotfit{};
        std::unordered_map<std::string, int> doesnotcompile{};
        DynaPlex::VarGroup varGroup{};
        varGroup.Add("map", map);
        varGroup.Get("map", map2);

        ASSERT_EQ(map2["one"], 1);
        ASSERT_EQ(map2["two"], 2);
        ASSERT_THROW(varGroup.Get("map", doesnotfit),DynaPlex::Error);



        //should not compile, and doesn't
        //varGroup.Add("does_not_matter", doesnotcompile);
    }

}