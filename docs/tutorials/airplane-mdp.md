# Airplane ticket selling MDP formulation

A deliberately simple example: a company sells tickets to a flight. The flight
can carry at most 10 passengers, and there are three types of customers:

- Type 1 customers pay r<sub>1</sub> = 3000 euros for a seat
- Type 2 customers pay r<sub>2</sub> = 2000 euros for a seat
- Type 3 customers pay r<sub>3</sub> = 1000 euros for a seat

Seats are sold for 25 days, and the flight leaves on day 26, even if not all
seats are sold yet. The goal of the company is to maximize the total payments
received, subject to the constraint that each seat can be sold at most once.
On each day, exactly one customer arrives. With 40% probability a Type 1
customer tries to buy a seat, with 30% probability a Type 2 customer tries to
buy a seat, with 30% probability a Type 3 customer tries to buy a seat. The
company decides on each day whether to accept or reject the customer that
arrived. Accepted customers pay their associated price and have a seat
assigned. Rejected customers pay nothing and don't get a seat assigned. The
company cannot revisit decisions once they are made.

## The components of the MDP

1. **States (S):** The state has only 3 variables: (i) the remaining days
   until the flight leaves, (ii) the number of remaining seats that can still
   be offered to customers, and (iii) the price that the new customer is
   willing to pay (1000, 2000 or 3000).

    We consider 3 types of states to which we can transition: (i) a state
    after an event (new customer arrival) happened, right before the decision
    to accept or reject that customer (`AWAIT_ACTION`), (ii) a state right
    after a customer is accepted or rejected, before the next customer
    arrives (`AWAIT_EVENT`), and (iii) the final state, when no more seats
    can be sold (`FINAL`).

2. **Actions (A):** The action is simple: sell the seat to the customer or
   reject the customer.

3. **Randomness / events:** On each day, exactly one customer arrives, the
   type of which is random.

4. **Transitions:**
    - When we are in the `AWAIT_EVENT` state and a new customer arrives, the
      state transitions to `AWAIT_ACTION`.
    - When we are in the `AWAIT_ACTION` state and a customer is accepted or
      rejected, the state transitions to `AWAIT_EVENT`.
    - No more changes after the state becomes `FINAL`.

5. **Costs (C):** The rewards of selling a seat against the different prices.

!!! note
    DynaPlex is costs based, so for this MDP we will denote negative costs,
    which are equivalent to positive rewards.

## Policy

A closely related concept to MDPs are policies. A policy is a function that
maps a state to an action.

**Policy (π):** Apart from the RL algorithms available through DynaPlex, you
can supply your own policy, which you could use as a benchmark.

For this MDP we will implement a simple rule-based benchmark:

1. When there are more than 5 seats left, we sell to all customers.
2. When there are 1 to 5 seats and 9 or fewer days remaining, we sell to
   Type 1 and Type 2 customers.
3. When there are between 1 and 5 seats and 10 or more days remaining, we
   sell only to Type 1 customers.
4. When there are no seats remaining, we cannot sell to anybody.

Continue to the [Python code](airplane-mdp-code.md) for the complete
implementation.
