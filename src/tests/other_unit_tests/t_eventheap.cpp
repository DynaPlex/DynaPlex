#include "dynaplex/vargroup.h"
#include "dynaplex/error.h"
#include <gtest/gtest.h>
#include "dynaplex/modelling/eventheap.h"


struct Event {
    double time;
    int payload;

  //  For EventHeap to work, T must be comparable using < and >. This defines all:
    auto operator<=>(const Event& other) const {
        return time <=> other.time;
    }
   
    //This is needed to be able to compare two heap objects succesfully - underlying type must also be comparable.
    //default comparer simply compares elements. 
    bool operator==(const Event& other) const = default;

};

TEST(EventHeapTest, TestResortFunctionality) {
    DynaPlex::EventHeap<Event> heap{};

    // Push events into the heap.
    heap.push(Event(1.5, 42)); // This will be modified.
    heap.push(Event(0.8, 33));
    heap.push(Event(2.3, 55));

    // Directly modify the first element to simulate an external change.
    // Since `first` returns a reference, we can modify it directly.
    auto& topEvent = heap.first();
    topEvent.time = 3.5; // Change the time making it no longer the smallest.

    // Now, the heap order is incorrect. Let's resort it.
    heap.resort();

    // Verify the heap order is restored by checking the new top event.
    const auto& newTopEvent = heap.first();
    EXPECT_DOUBLE_EQ(newTopEvent.time, 1.5); // The smallest time after resorting.
    EXPECT_EQ(newTopEvent.payload, 42); // Payload should match the event with the smallest time.
}

namespace DynaPlex::Tests {
    TEST(EventHeapTest, TestPushAndTop) {
      
        DynaPlex::EventHeap<Event> heap{};
        heap.push(Event(10.5, 42));
        heap.push(Event(0.8, 33));
        heap.push(Event(2.3, 55));

        const auto& topEvent = heap.first();
        EXPECT_DOUBLE_EQ(topEvent.time, 0.8);  
        EXPECT_EQ(topEvent.payload, 33);
    }

    TEST(EventHeapTest, TestPop) {
        DynaPlex::EventHeap<Event> heap{};
        heap.push(Event(1.5, 42));
        heap.push(Event(0.8, 33));
        heap.push(Event(2.3, 55));

        heap.pop();
        const auto& topEvent = heap.first();
        EXPECT_DOUBLE_EQ(topEvent.time, 1.5);  
        EXPECT_EQ(topEvent.payload, 42);
    }

    TEST(EventHeapTest, TestEquality) {
        DynaPlex::EventHeap<Event> heap1{};
        DynaPlex::EventHeap<Event> heap2{};

        heap1.push(Event(1.5, 42));
        heap1.push(Event(0.8, 33));
        heap1.push(Event(2.3, 55));

        heap2.push(Event(2.3, 55));
        heap2.push(Event(0.8, 33));
        heap2.push(Event(1.5, 42));
     
        EXPECT_TRUE(heap1 == heap2);
    }

    TEST(EventHeapTest, TestInEquality) {
        DynaPlex::EventHeap<Event> heap1{};
        DynaPlex::EventHeap<Event> heap2{};

        heap1.push(Event(1.5, 42));
        heap1.push(Event(0.8, 33));
        heap1.push(Event(2.3, 55));

        heap2.push(Event(2.3, 55));
        //Note that payload is different for this one. 
        heap2.push(Event(0.8, 32));
        heap2.push(Event(1.5, 42));

        EXPECT_TRUE(heap1 != heap2);
    }

    TEST(EventHeapTest, TestIteration) {
        DynaPlex::EventHeap<Event> heap{};
        heap.push(Event(1.5, 42));
        heap.push(Event(0.8, 33));
        heap.push(Event(2.3, 55));

        double total_time = 0;
        for (const auto& event : heap) {
            total_time += event.time;
        }

        


        EXPECT_DOUBLE_EQ(total_time, 1.5 + 0.8 + 2.3);
    }
    
}