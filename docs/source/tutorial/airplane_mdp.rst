.. _label_airplane:
Airplane MDP formulation
========================

A deliberately simple example: A company sells tickets to a flight. The flight can carry at most 10 passengers, and there are three types of customers:

- Type 1 customers pay r\ :sub:`1` \ = 3000 euros for a seat 

- Type 2 customers pay r\ :sub:`2` \ = 2000 euros for a seat

- Type 3 customers pay r\ :sub:`3` \ = 1000 euros for a seat

Seats are sold for 25 days, and the flight leaves on day 26, even if not all seats are sold yet. The goal of the company is to maximize the total payments received, subject to the constraint that each seat can be sold at most once. On each day, exactly one customer arrives. With 40% probability a Type 1 customer tries to buy a seat, with 30% probability a Type 2 customer tries to buy a seat, with 30% probability a Type 3 customer tries to buy a seat. The company decides on each day whether to accept or reject the customer that arrived. Accepted customers pay their associated price and have a seat assigned. Rejected customers pay nothing and don't get a seat assigned. The company cannot revisit decisions once they are made.