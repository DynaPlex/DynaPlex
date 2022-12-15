#pragma once
#include <string>
#include <functional>

#include <iostream>
class MDP_Implementation
{
public:
	struct State {
		size_t content;
		State(size_t content)
			:content{ content }
		{

		}
	};
	class ActionTraverser;
	class Action {
	private:
		ActionTraverser* traverser;
		size_t curvalue;
	public:
		size_t operator*();
		void operator++();
		bool operator!=(Action action);
		Action(ActionTraverser* traverser, size_t curvalue);
	};
	class ActionTraverser
	{
		std::function<bool(size_t)> IsAllowed;
		size_t maxvalue;
	public:
		ActionTraverser(std::function<bool(size_t)> IsAllowed, size_t maxvalue) :
			IsAllowed{IsAllowed}, maxvalue{maxvalue}
		{
			
		}
		friend Action;
		Action begin();
		Action end();

	};

	int j;
public:

	bool IsAllowed(State& state, size_t action)
	{
		return action % (state.content) == 1;
	}

	size_t NumAllowedActions()
	{
		return 200;
	}

	ActionTraverser AllowedActions(State& state)
	{
		auto IsAllowed = [&state,this](size_t action) {
			return this->IsAllowed(state, action);
		};
		return ActionTraverser(IsAllowed, NumAllowedActions());
	}

	std::string Identifier();

	MDP_Implementation(int j);
};