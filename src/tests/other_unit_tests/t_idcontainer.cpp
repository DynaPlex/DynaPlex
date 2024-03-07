#include <gtest/gtest.h>
#include "include/smallclass.h"
#include "dynaplex/modelling/idcontainer.h"

namespace DynaPlex::Tests {
    


    // Test adding and retrieving an item
    TEST(IdContainerTest, AddRetrieveDelete) {
        DynaPlex::IdContainer<SmallClass> container;

        // Prepare VarGroup for initial items
        DynaPlex::VarGroup vg;
        SmallClass obj1;
        obj1.name = "TestObject";
        obj1.Size = 123.456;
        vg.Add("1", obj1.ToVarGroup()); // Simulate adding an item to VarGroup

        // Initialize container with VarGroup
        container = DynaPlex::IdContainer<SmallClass>(vg);

        // Test retrieving the added item
        auto& retrievedItem = container[1];
        EXPECT_EQ(retrievedItem.name, "TestObject");
        EXPECT_DOUBLE_EQ(retrievedItem.Size, 123.456);

        // Test adding a new item
        auto& [id, item] = container.AddNew();

        //auto& [id,item] = container.AddNew();
        item.Size = 123;
        EXPECT_NO_THROW(container[id]);
        auto& item_ = container[id];
        EXPECT_EQ(item_.Size, 123);
        // Test deleting an item
        container.Delete(1);
        EXPECT_THROW(container[1], DynaPlex::Error);
    }



    TEST(IdContainerVarGroupTest, VarGroupReInitialization) {
        // Step 1: Create and populate an initial IdContainer
        DynaPlex::IdContainer<SmallClass> initialContainer;
        auto& [id,item1_] = initialContainer.AddNew();
        auto& item1 = initialContainer[id];
        item1.name = "Item1";
        item1.Size = 10.0;

        // Step 2: Convert the initial IdContainer to VarGroup
        DynaPlex::VarGroup vg = initialContainer.ToVarGroup();

        // Step 3: Create a new IdContainer using the retrieved VarGroup
        DynaPlex::IdContainer<SmallClass> newContainer(vg);
        

        // Step 4: Verify the contents of the new IdContainer
        // This step depends on the capabilities of your IdContainer and SmallClass
        // For example, checking if the new container has an item with id1
        auto& newItem = newContainer[id];
        EXPECT_EQ(newItem.name, "Item1");
        EXPECT_DOUBLE_EQ(newItem.Size, 10.0);
        EXPECT_EQ(newContainer.size(), 1);
        //this should compile:
        for (auto& item : newContainer)
        {
        }
    }

}