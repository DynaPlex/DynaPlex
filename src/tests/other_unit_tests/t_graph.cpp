#include <gtest/gtest.h>
#include "dynaplex/error.h"
#include "dynaplex/modelling/graph.h"

namespace DynaPlex::Tests {    
    // Test adding and retrieving an item
    TEST(Graph, CircleGraph) {
        DynaPlex::VarGroup vars;

        int numNodes = 5; // Example number of nodes in the circle
        DynaPlex::VarGroup::VarGroupVec edges;
        for (int i = 0; i < numNodes; ++i) {
            edges.push_back(DynaPlex::VarGroup{
                {"orig", i},
                {"dest", (i + 1) % numNodes},
                {"weight", i+1.0}
                });
        }
        vars.Add("type", "directed");
        vars.Add("edges", edges);

        DynaPlex::Graph g(vars);

        // Test Distance
        EXPECT_DOUBLE_EQ(1.0, g.Distance(0, 1)); //directly 
        EXPECT_DOUBLE_EQ(3.0, g.Distance(0, 2)); //1+2 
        EXPECT_DOUBLE_EQ(6.0, g.Distance(0, 3)); //1+2+3 
        EXPECT_DOUBLE_EQ(5.0, g.Distance(4, 0)); //5 
        EXPECT_DOUBLE_EQ(14.0, g.Distance(1, 0));  //2+3+4+5


       
        std::vector<Graph::Edge> path;
        for (auto& edge : g.Path(0, 2))
        {
            path.push_back(edge);
        }

        ASSERT_EQ(2, path.size()); // Check the number of edges in the path
        EXPECT_EQ(0, path[0].orig); // Check the first edge
        EXPECT_EQ(1, path[0].dest);
        EXPECT_EQ(1, path[1].orig); // Check the second edge
        EXPECT_EQ(2, path[1].dest);


        std::vector<Graph::Edge> path00;
        for (auto& edge : g.Path(0, 0))
        {
            path00.push_back(edge);
        }
        ASSERT_EQ(0, path00.size()); // Check the number of edges in the path

        // Test NextEdge
        auto nextEdge = g.NextEdge(1, 0);
        EXPECT_EQ(1, nextEdge.orig); // Check the first edge of the path
        EXPECT_EQ(2, nextEdge.dest);
        // Add more NextEdge checks

    }

    TEST(Graph, CircleGraphUndirected) {
        DynaPlex::VarGroup vars;

        int numNodes = 5; // Example number of nodes in the circle
        DynaPlex::VarGroup::VarGroupVec edges;
        for (int i = 0; i < numNodes; ++i) {
            edges.push_back(DynaPlex::VarGroup{
                {"orig", i},
                {"dest", (i + 1) % numNodes},
                {"weight", i + 1.0}
                });
        }
        vars.Add("type", "undirected");
        vars.Add("edges", edges);

        DynaPlex::Graph g(vars);

        // Test Distance
        EXPECT_DOUBLE_EQ(1.0, g.Distance(0, 1)); //directly 
        EXPECT_DOUBLE_EQ(3.0, g.Distance(0, 2)); //1+2 
        EXPECT_DOUBLE_EQ(6.0, g.Distance(0, 3)); //1+2+3 
        EXPECT_DOUBLE_EQ(5.0, g.Distance(4, 0)); //5 
        EXPECT_DOUBLE_EQ(1.0, g.Distance(1, 0));  //1 (note: undirected now)

        std::vector<Graph::Edge> path;
        for (auto& edge : g.Path(0, 2))
        {
            path.push_back(edge);
        }
        ASSERT_EQ(2, path.size()); // Check the number of edges in the path
        EXPECT_EQ(0, path[0].orig); // Check the first edge
        EXPECT_EQ(1, path[0].dest);
        EXPECT_EQ(1, path[1].orig); // Check the second edge
        EXPECT_EQ(2, path[1].dest);


        std::vector<Graph::Edge> path00;
        for (auto& edge : g.Path(0, 0))
        {
            path00.push_back(edge);
        }
        ASSERT_EQ(0, path00.size()); // Check the number of edges in the path

        // Test NextEdge
        auto nextEdge = g.NextEdge(1, 0);
        EXPECT_EQ(1, nextEdge.orig); // Check the first edge of the path
        EXPECT_EQ(0, nextEdge.dest);

    }


    TEST(Graph, Grid)
    {
        DynaPlex::VarGroup vars;
        std::vector<std::string> rows =
        {
            "R |R |RD|R |R |R |D ",
            "U |  |D |  |U |  |D ",
            "U |  |D |  |U |  |D ",
            "U |  |D |  |U |  |D ",
            "U |  |D |  |U |  |D ",
            "U |  |D |  |U |  |D ",
            "U |  |D |  |U |  |D ",
            "UR|R |R |R |UR|R |  "
        };
        vars.Add("rows", rows);
        vars.Add("format", "grid");

       
        //note that we have a double edge, so this should throw:
        EXPECT_NO_THROW(DynaPlex::Graph g(vars));
        DynaPlex::Graph g(vars);

        EXPECT_THROW(g.AddEdge(0, 1), DynaPlex::Error);

        auto final = g.NodeAt(7, 6);
        auto origin = g.NodeAt(7, 0);
        auto one_up_origin = g.NodeAt(6, 0);

        auto final_coords = g.Coordinates(final);
        EXPECT_EQ(final_coords.row , 7);

        EXPECT_EQ(final_coords.col , 6);


        EXPECT_EQ(g.Distance(origin, final), 6);
        EXPECT_EQ(g.Distance(one_up_origin, final), 19);
 
        EXPECT_THROW(g.Distance(origin, g.NodeAt(1, 1)),DynaPlex::Error);
        EXPECT_EQ(g.ExistsPath(origin, one_up_origin), true);
        EXPECT_EQ(g.ExistsPath(one_up_origin, origin), false);
        EXPECT_EQ(g.ExistsPath(origin, origin), true);

        //make undirected graph with same data:
        vars.Add("type", "undirected");
        DynaPlex::Graph u_g(vars);
        //now, these paths should exist:
        EXPECT_EQ(u_g.ExistsPath(one_up_origin, origin), true);
        EXPECT_EQ(u_g.ExistsPath(final, one_up_origin), true);
        //travels against the arrows:
        EXPECT_EQ(u_g.Distance(final, one_up_origin), 7);
    }

    TEST(Graph, DoubleEdged) {
        DynaPlex::VarGroup vars;
        int numNodes = 5; // Example number of nodes in the circle
        DynaPlex::VarGroup::VarGroupVec edges;
        for (int i = 0; i < numNodes; ++i) {
            edges.push_back(DynaPlex::VarGroup{
                {"orig", i},
                {"dest", (i + 1) % numNodes},
                {"weight", 1.0}
                });        }
        edges.push_back(DynaPlex::VarGroup{
              {"orig", 0},
                {"dest", 1 },
                {"weight", 0.5}
            });
        vars.Add("edges", edges);
        //note that we have a double edge, so this should throw:
        EXPECT_THROW(DynaPlex::Graph g(vars), DynaPlex::Error);
        vars.Add("allow_double_edges", true);        
        EXPECT_NO_THROW(DynaPlex::Graph g(vars));
        DynaPlex::Graph g(vars);
        EXPECT_EQ(g.Distance(0, 1), 0.5);
    }

    TEST(Graph, GridFromScratch)
    {
        DynaPlex::Graph g(2, 2);
        g.AddEdge(0, 1);
        g.AddEdge(1, 3);
        g.AddEdge(3, 2);
        g.AddEdge(2, 0);
        EXPECT_THROW(g.AddEdge(1, 4), DynaPlex::Error);
        g.Finalize();

        EXPECT_EQ(g.NumNodes(), 4);
        EXPECT_EQ(g.Distance(0, 2), 3);
    }

    TEST(Graph, GraphFromScratch)
    {
        DynaPlex::Graph g{};
        g.AddEdge(0, 1, 10.0);
        g.AddEdge(1, 2,10.0);
        g.Finalize();

        EXPECT_THROW(g.Finalize(), DynaPlex::Error);

        EXPECT_EQ(g.NumNodes(), 3);
        EXPECT_EQ(g.Distance(0, 2), 20);

    }

}