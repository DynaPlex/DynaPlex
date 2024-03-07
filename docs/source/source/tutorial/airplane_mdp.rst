.. _label_airplane:

Airplane MDP formulation
========================

A deliberately simple example: A company sells tickets to a flight. The flight can carry at most 10 passengers, and there are three types of customers:

- Type 1 customers pay r\ :sub:`1` \ = 3000 euros for a seat 

- Type 2 customers pay r\ :sub:`2` \ = 2000 euros for a seat

- Type 3 customers pay r\ :sub:`3` \ = 1000 euros for a seat

Seats are sold for 25 days, and the flight leaves on day 26, even if not all seats are sold yet. The goal of the company is to maximize the total payments received, subject to the constraint that each seat can be sold at most once. On each day, exactly one customer arrives. With 40% probability a Type 1 customer tries to buy a seat, with 30% probability a Type 2 customer tries to buy a seat, with 30% probability a Type 3 customer tries to buy a seat. The company decides on each day whether to accept or reject the customer that arrived. Accepted customers pay their associated price and have a seat assigned. Rejected customers pay nothing and don't get a seat assigned. The company cannot revisit decisions once they are made.

The Components of the MDP
-------------------------

1. **States (S):** The state has only 3 variables: (i) the remaining days until the flight leaves, (ii) the number of remaining seats that can still be offered to customers, and (iii) the price that the new customer is willing to pay (1000,2000 or 3000).

2. **Actions (A):** The action is simple: sell the seat to the customer or reject the customer. The action space is constrained, since the carrier canot sell more than 10 seats.

3. **Transitions:** We consider 3 types of states to which we can transition: a state before an action and after an event (new customer arrival) happend (pre-action), the state after the action (post-action), and the final state, when no more seats can be sold.

4. **Rewards (R):** The rewards of selling a seat against the different prices.

.. note::
	DynaPlex is costs based, so for this MDP we consider negative rewards.

5. **Policy (π):** Apart from the RL-algorithms available through DynaPlex, you can supply your own policy, which you could use as benchmark.

For this MDP we will implement a simple rule based benchmark:

	1. When there are more than 5 seats left, we sell to all customers. 

	2. When there are 1 to 5 seats and 9 or less days remaining, we sell to Type 1 and Type 2 customers.
	
	3. When there are between 1 and 5 seats and 10 or more days remaining, we sell only to Type 1 customers. 

	4. When there are no seats remaining, we cannot sell to anybody.