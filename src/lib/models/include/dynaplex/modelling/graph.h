#pragma once
#include <vector>
#include "dynaplex/vargroup.h"

namespace DynaPlex {

    /// class can be initiated from VarGroup (either from a string representing the grid, or from a list of edges to arbitrary graph with non-negative weights)
    /// after initiation, it will do some pre-computations. Then, paths and distances can be computed efficiently in the grid.
    class Graph {
    public:
        using WeightType = double;
        class Edge {
        public:
            explicit Edge(const VarGroup& config); // Constructor using VarGroup config
            Edge(); // Default constructor
            Edge(uint32_t orig, uint32_t dest, WeightType weight = static_cast<WeightType>(1));

            uint32_t orig, dest;
            WeightType weight;            
        };

        //for iterating over paths in the graph, the following will be usefull:

        class PathIterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = const Graph::Edge;
            using pointer = value_type*;
            using reference = value_type&;

            // Constructor
            PathIterator(const Graph* graph, int64_t currentNode, int64_t destinationNode);

            // Equality and inequality operators
            bool operator==(const PathIterator& other) const;
            bool operator!=(const PathIterator& other) const;

            // Increment operators
            PathIterator& operator++();    // Prefix increment
            PathIterator operator++(int);  // Postfix increment

            // Dereference operators
            reference operator*() const;
            pointer operator->() const;

        private:
            const Graph* graph_;
            int64_t currentNode_;
            int64_t destinationNode_;
            Graph::Edge nextEdge_;
            bool atEnd_;

            void updateNextEdge();
        };

        class PathIterable {
        public:
            // Constructor and methods to get the beginning and end of the iterator
            PathIterable(const Graph& graph, int64_t origin, int64_t dest);

            PathIterator begin() const;
            PathIterator end() const;

        private:
            const Graph& graph_;
            int64_t origin_;
            int64_t dest_;
        };



       

        /// Initiates the graph from the config file, and does all precomputations to efficiently compute paths and distances for the graph.
        explicit Graph(const DynaPlex::VarGroup& config); // Constructor using VarGroup config
        Graph(); 
        
        Graph(int64_t width, int64_t height);
        void AddEdge(uint32_t origin, uint32_t destination, WeightType weight = static_cast<WeightType>(1));

        void Finalize();

        //for graphs that were created from format "grid": returns the index of the node
        //at a certain row and column. 
        int64_t NodeAt(int64_t row, int64_t col) const;

        struct Coords {
            int64_t row;
            int64_t col;
        };

        Coords Coordinates(int64_t node) const;

        bool ExistsPath(int64_t origin_node, int64_t destination_node) const;
        WeightType Distance(int64_t origin_node, int64_t destination_node) const;

        /// usage: for (const edge& edge : graph.Path(orig,dest)) { ... }
        PathIterable Path(int64_t origin_node, int64_t destination_node) const;
        const Edge& NextEdge(int64_t origin_node, int64_t destination_node) const;

        

        const std::vector<Edge>& Edges() const { return edges; }
        int64_t NumNodes() const { return numNodes; }
        int64_t Width() const;
        int64_t Height() const;

    private:
        std::vector<Edge> edges;
        std::vector<std::vector<int32_t>> edgeIndices;
        std::vector<std::vector<WeightType>> distances;
        bool is_manual{ false };
        bool is_finalized{ false };
        bool is_grid{ false };
        int64_t width, height;
        int64_t numNodes{ 0 };
        void UpdateNumNodes();
        void ValidateEdgeVector(bool allow_double_edges);
        void PCSP();
        std::string TypeSetNode(int64_t node_id) const;
    };

} // namespace DynaPlex
