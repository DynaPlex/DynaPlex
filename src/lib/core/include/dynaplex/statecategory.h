#pragma once
#include <cstdint>
#include <cstddef>
#include "vargroup.h"
#include "error.h"

namespace DynaPlex {

    class StateCategory {
    private:
        //use enum here to avoid many casts and improve readability
        //cannot leak since private.
        enum StateType : uint64_t {
            AWAIT_ACTION = 0x1ULL << 60,
            AWAIT_EVENT = 0x2ULL << 60,
            FINAL = 0x3ULL << 60,
            EOH = 0x4ULL << 60 
        };
        uint64_t state;
        // Private constructor
        explicit StateCategory(uint64_t state) : state(state) {}

    public:

            static StateCategory AwaitAction() {
                return StateCategory(AWAIT_ACTION);
            }

            static StateCategory AwaitAction(int64_t index) {
                if (index < 0)
                {
                    throw DynaPlex::Error("StateCategory: Negative index not allowed");
                }
                return StateCategory(AWAIT_ACTION | index);
            }

            static StateCategory AwaitEvent(int64_t index) {
                if (index < 0)
                {
                    throw DynaPlex::Error("StateCategory: Negative index not allowed");
                }
                return StateCategory(AWAIT_EVENT | index);
            }
            static StateCategory AwaitEvent() {
                return StateCategory(AWAIT_EVENT);
            }

            static StateCategory Final() {
                return StateCategory(FINAL);
            }

            static StateCategory EndOfHorizon() {
                return StateCategory(EOH);
            }

       

        VarGroup ToVarGroup() const {
            VarGroup vg{};
            int64_t index = Index();
            std::string awaits;
            if (IsAwaitAction()) awaits = "action";
            else if (IsAwaitEvent()) awaits = "event";
            else if (IsFinal()) awaits = "-";
            else if (IsEndOfHorizon()) awaits = "EOH";
            vg.Add("await", awaits);
            if (index > 0)
            {
                vg.Add("index", index);
            }
            return vg;
        }

        StateCategory() {
            *this = StateCategory::Final();
        }

        explicit StateCategory(const VarGroup& varGroup) {
            int64_t index{ 0 };
            std::string awaits;

            if (varGroup.HasKey("index"))
            {
                varGroup.Get("index", index);
            }
            varGroup.Get("await", awaits);

            if (awaits == "action") {
                state = AWAIT_ACTION | index;
            }
            else if (awaits == "event") {
                state = AWAIT_EVENT | index;
            }
            else if (awaits == "-") {
                state = FINAL | index;
            }
            else if (awaits == "EOH") {
                state = EOH | index; 
            }
            else {
                throw DynaPlex::Error("StateCategory: Invalid category in VarGroup");
            }
        }

        bool IsAwaitAction() const {
            return (state & (0xFULL << 60)) == AWAIT_ACTION;
        }

        bool IsAwaitEvent() const {
            return (state & (0xFULL << 60)) == AWAIT_EVENT;
        }

        bool IsFinal() const {
            return (state & (0xFULL << 60)) == FINAL;
        }

        bool IsEndOfHorizon() const {
            return (state & (0xFULL << 60)) == EOH;
        }

        std::int64_t Index() const {
            return state & ~(0xFULL << 60); 
        }
    };

} // namespace DynaPlex

