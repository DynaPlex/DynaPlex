#pragma once
#include <variant>


//for defining the ordering on events;
struct compare_visitor {
	template<typename T>
	std::strong_ordering operator()(const T& a, const T& b) const {
		return a <=> b;
	}
	template<typename T, typename U>
	std::strong_ordering operator()(const T&, const U&) const {
		return std::strong_ordering::equivalent;
	}
};

/**
 *the ScheduledEvent can hold any payload that is vargroupconvertible and that supports sorting via <=> operator.
 *Events will be sorted first on trigger_time, then on type of event (in the order of below list), and finally, to compare two events of the same type with the same trigger_time
 *using the comparison operator <=> of that class.  
 * 
 *Note that for technical reasons, the first element in the template list must have a default constructor, i.e.
 *a constructor without arguments.
 */
template<typename... Ts>
class ScheduledEventType {
public:
	using EventPayLoad = std::variant<Ts...>;
private:
	EventPayLoad payload;
	template<std::size_t I = 0>
	EventPayLoad ReConstructPayload(const DynaPlex::VarGroup& vg, std::size_t index) {
		// Base case: if I is out of bounds
		if constexpr (I >= std::variant_size_v<EventPayLoad>) {
			throw DynaPlex::Error("Index out of bounds for Variant construction.");
		}
		else {
			if (I == index) {
				using VariantType = typename std::variant_alternative<I, EventPayLoad>::type;
				return VariantType{ vg };
			}
			else {
				return ReConstructPayload<I + 1>(vg, index);
			}
		}
	}


public:
	int64_t trigger_time{ 0 };

	ScheduledEventType(int64_t trigger_time, EventPayLoad eventPayload)
		: trigger_time(trigger_time), payload(eventPayload) {}

	auto operator<=>(const ScheduledEventType<Ts...>& other) const {
		if (auto cmp = trigger_time <=> other.trigger_time; cmp != 0) return cmp;
		if (auto cmp = payload.index() <=> other.payload.index(); cmp != 0) return cmp;
		return std::visit(compare_visitor{}, payload, other.payload);
	}

	///gets the index for the payload of the event
	size_t PayLoadIndex()
	{
		return payload.index();
	}

	///checks whether event has payload of this type
	template<typename T>
	bool has() const {
		return std::holds_alternative<T>(payload);
	}

	///gets payload of this type
	template<typename T>
	T& get() {
		return std::get<T>(payload);
	}
	///gets payload of this type
	template<typename T>
	const T& get() const {
		return std::get<T>(payload);
	}

	///calling IndexOf<T>() gives the index of T in the EventPayLoad variant.
	template <typename T, std::size_t I = 0>
	static constexpr std::size_t IndexOf() {
		if constexpr (I == std::variant_size_v<EventPayLoad>) {
			static_assert(I != std::variant_size_v<EventPayLoad>, "Type not found in variant");
			return -1; // This line is never reached but is required for compilation.
		}
		else {
			if constexpr (std::is_same_v<std::variant_alternative_t<I, EventPayLoad>, T>) {
				return I;
			}
			else {
				return IndexOf<T, I + 1>();
			}
		}
	}


	explicit ScheduledEventType(const DynaPlex::VarGroup& varGroup)
	{
		int64_t payload_type;
		varGroup.Get("trigger_time", trigger_time);
		varGroup.Get("payload_type", payload_type);
		size_t index = payload_type;
		payload = ReConstructPayload(varGroup, index);
	}

	DynaPlex::VarGroup ToVarGroup() const
	{
		DynaPlex::VarGroup vg;
		std::visit([&vg](const auto& payload) {
			vg = payload.ToVarGroup();
			}, payload);
		vg.Add("trigger_time", trigger_time);
		int64_t payload_type = payload.index();
		vg.Add("payload_type", payload_type);
		return vg;
	}
};