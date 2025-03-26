#include "dynaplex/modelling/graph.h"
#include <queue>
#include <vector>
#include <functional>
#include <limits>
#include <sstream>
#include "dynaplex/error.h"
#include "dynaplex/modelling/graph.h"



namespace DynaPlex {
	Graph::Edge::Edge(const VarGroup& config) {
		int64_t orig_, dest_;
		config.Get("orig", orig_);
		config.Get("dest", dest_);
		if (orig_<0 || orig_> std::numeric_limits<uint32_t>::max()
			|| dest_<0 || dest_> std::numeric_limits<uint32_t>::max())
			throw DynaPlex::Error("Graph::Edge - orig and dest must be non-negative integers that fit uint32_t.");

		orig = static_cast<uint32_t>(orig_);
		dest = static_cast<uint32_t>(dest_);
		if (orig == dest)
			throw DynaPlex::Error("Graph::Edge - self-connections not allowed (orig=dest)");

		config.GetOrDefault("weight", weight,1.0);
		if (weight <= 0.0)
		{
			throw DynaPlex::Error("Graph::Edge - weight must be strictly positive");
		}
	}

	Graph::Edge::Edge() : orig(0), dest(0), weight(1.0) {
	}

	Graph::Edge::Edge(uint32_t orig, uint32_t dest, WeightType weight) : orig(orig), dest(dest), weight(weight) {
	}



	Graph::Graph(const VarGroup& config) {
		std::string format{};
		config.GetOrDefault("format", format, "edge_list");
		
		if (format == std::string("edge_list"))
		{
			config.Get("edges", edges);
		}
		else
		{
			if (format == std::string("grid"))
			{
				is_grid = true;
			
				std::vector<std::string> rows;
				config.Get("rows", rows); // Assuming `config` can retrieve a vector of strings for "rows"

				// The number of rows is simply the size of the `rows` vector
				height = rows.size();
			
				// Assuming all rows are of the same length, so calculate width based on the first row
				// Counting the number of '|' plus one gives us the number of nodes per row
				width = std::count(rows[0].begin(), rows[0].end(), '|') + 1;
			
				std::vector<std::string> nodes;
				int64_t rows_parsed{ 0 };
				// Flatten the rows into a single vector of node strings
				for (const std::string& row : rows) {
					int row_width = std::count(row.begin(), row.end(), '|') + 1;
					if (row_width != width)
						throw DynaPlex::Error("Graph: number of | separators is different for different rows. Only rectangular grids are supported.");

					std::istringstream iss(row);
					std::string node;
					while (std::getline(iss, node, '|')) {
						// Trim spaces from the node string if necessary
						node.erase(std::remove_if(node.begin(), node.end(), isspace), node.end());
						nodes.push_back(node);
					}
					if (!row.empty() && row.back() == '|') {
						nodes.push_back(""); // Add an empty node for the last position
					}
					if (nodes.size() != ++rows_parsed * width)
						throw DynaPlex::Error("Graph: issue while parsing string");
				}

				if (nodes.size() != width * height)
					throw DynaPlex::Error("Graph: issue while parsing grid");

				for (int64_t row = 0; row < height; ++row) {
					for (int64_t col = 0; col < width; ++col) {
						uint32_t idx = NodeAt(row, col);

						if (idx >= nodes.size()) continue;

						std::string connections = nodes[idx];

						for (char dir : connections) {
							int intDir = static_cast<unsigned char>(dir); // Cast to unsigned and then to int
							std::string loc = "(row,col)=(" + std::to_string(row) + "," + std::to_string(col) + ")";
							switch (intDir) {
							case 'U':
								if (row <= 0) throw DynaPlex::Error("Up (U) arrow at " + loc + " points outside of the grid");
								edges.push_back({ idx, static_cast<uint32_t>(NodeAt(row - 1, col)) });
								break;
							case 'D':
								if (row >= height - 1) throw DynaPlex::Error("Down (D) arrow at " + loc + " points outside of the grid");
								edges.push_back({ idx, static_cast<uint32_t>(NodeAt(row + 1, col)) });
								break;
							case 'L':
								if (col <= 0) throw DynaPlex::Error("Left (L) arrow at " + loc + " points outside of the grid");
								edges.push_back({ idx, static_cast<uint32_t>(NodeAt(row, col - 1)) });
								break;
							case 'R':
								if (col >= width - 1) throw DynaPlex::Error("Right (R) arrow at "+ loc + " points outside of the grid");
								edges.push_back({ idx, static_cast<uint32_t>(NodeAt(row, col + 1)) });
								break;
							case ' ':
								// Do nothing for space character
								break;
							default:
								throw DynaPlex::Error("Unrecognized character in node connections");
							}
						}
					}
				}
			}
			else
				throw DynaPlex::Error("Graph - format '" + format + "' not supported. Only support grid and edge_list format.");
		}
		

		

		std::string type{};
		config.GetOrDefault("type", type, "directed");
		if (type == std::string("undirected"))
		{  //add the opposite of all edges to the edges vector:
			size_t current = edges.size();
			edges.reserve(current * 2);
			for (size_t i = 0; i < current; i++)
			{
				//add the opposite edge for all edges currently in the graph.
				Edge edge{};
				edge.orig = edges[i].dest;
				edge.dest = edges[i].orig;
				edge.weight = edges[i].weight;
				edges.push_back(edge);
			}
		}
		else
			if (type != std::string("directed"))
				throw DynaPlex::Error("Graph:: type " + type + " not supported. Only support directed or undirected.");
	
		bool allow_double_edges{};
		config.GetOrDefault("allow_double_edges", allow_double_edges, false);
		UpdateNumNodes();
		ValidateEdgeVector(allow_double_edges);
		PCSP();
	}


	int64_t Graph::NodeAt(int64_t row, int64_t col) const {
		if (!is_grid)
			throw DynaPlex::Error("Graph::NodeAt - format is not a grid.");
		if (row < 0 || row >= height)
			throw DynaPlex::Error("Graph::NodeAt - invalid row");
		if (col < 0 || col >= width)
			throw DynaPlex::Error("Graph::NodeAt - invalid col");
		return row * width + col;
	}

	int64_t Graph::Width() const {
		if (!is_grid)
			throw DynaPlex::Error("Graph::NodeAt - format is not a grid.");
		return width;
	}
	int64_t Graph::Height() const {
		if (!is_grid)
			throw DynaPlex::Error("Graph::NodeAt - format is not a grid.");
		return height;
	}

	Graph::Coords Graph::Coordinates(int64_t node) const	{
		if (!is_grid)
			throw DynaPlex::Error("Graph::Coordinates - format is not a grid.");
		if (node < 0 || node >= width * height)
			throw DynaPlex::Error("Graph::Coordinates - node not part of grid.");
		int64_t row = node / width;
		int64_t col = node - row * width;
		return { row,col };
	}


	Graph::Graph() : numNodes(0), is_grid{ false }, is_manual{ true } {
	}

	Graph::Graph(int64_t width, int64_t height)
		:width{ width }, height{ height },is_grid{true},is_manual{true}
	{
	}

	void Graph::AddEdge(uint32_t origin, uint32_t destination, WeightType weight)
	{
		if (!is_manual)
			throw DynaPlex::Error("Graph::AddEdge - only supported when graph was constructed manually using Graph(width,height) constructor.");
		if (is_finalized)
			throw DynaPlex::Error("Graph::AddEdge - cannot add edges anymode after finalizing.");

		if (is_grid)
		{
			if (origin >= width * height || origin < 0)
				throw DynaPlex::Error("Graph::AddEdge - origin falls outside of the grid");
			if (destination >= width * height || destination < 0)
				throw DynaPlex::Error("Graph::AddEdge - origin falls outside of the grid");
		}

		edges.emplace_back(origin, destination, weight);
	}

	void Graph::Finalize() {
		if(is_finalized)
			throw DynaPlex::Error("Graph::Finalize - allready finalized.");
		if (is_grid)
		{
			numNodes = width * height;
		}
		else
		{
			this->UpdateNumNodes();
		}	
		this->PCSP();
	}


	bool Graph::ExistsPath(int64_t origin_node, int64_t destination_node) const {
		if (origin_node < 0 || origin_node >= numNodes || destination_node < 0 || destination_node >= numNodes) {
			throw DynaPlex::Error("Invalid node index in Graph::Distance query " + TypeSetNode(origin_node) + " -> " + TypeSetNode(destination_node));
		}
		return distances[destination_node][origin_node] != std::numeric_limits<double>::max();
	}

	Graph::WeightType Graph::Distance(int64_t origin_node, int64_t destination_node) const {
		if (origin_node < 0 || origin_node >= numNodes || destination_node < 0 || destination_node >= numNodes) {
			throw DynaPlex::Error("Invalid node index in Graph::Distance query " + TypeSetNode(origin_node) + " -> " + TypeSetNode(destination_node));
		}
		if (distances[destination_node][origin_node] == std::numeric_limits<double>::max()) {
			throw DynaPlex::Error("No valid path exists in Distance query " + TypeSetNode(origin_node) + " -> " + TypeSetNode(destination_node));
		}
		return distances[destination_node][origin_node];
	}

	Graph::PathIterable Graph::Path(int64_t origin_node, int64_t destination_node) const {
		return PathIterable(*this, origin_node, destination_node);
	}


	Graph::PathIterator::PathIterator(const Graph* graph, int64_t currentNode, int64_t destinationNode)
		: graph_(graph), currentNode_(currentNode), destinationNode_(destinationNode),
		atEnd_(currentNode == destinationNode || currentNode == -1) {
		if (!atEnd_) {
			updateNextEdge();
		}
	}

	bool Graph::PathIterator::operator==(const PathIterator& other) const {
		return (atEnd_ && other.atEnd_) || (currentNode_ == other.currentNode_ && destinationNode_ == other.destinationNode_);
	}

	bool Graph::PathIterator::operator!=(const PathIterator& other) const {
		return !(*this == other);
	}

	Graph::PathIterator& Graph::PathIterator::operator++() {
		if (!atEnd_) {
			currentNode_ = nextEdge_.dest;
			if (currentNode_ == destinationNode_) {
				atEnd_ = true;
			}
			else {
				updateNextEdge();
			}
		}
		return *this;
	}

	Graph::PathIterator Graph::PathIterator::operator++(int) {
		PathIterator tmp = *this;
		++(*this);
		return tmp;
	}

	Graph::PathIterator::reference Graph::PathIterator::operator*() const {
		return nextEdge_;
	}

	Graph::PathIterator::pointer Graph::PathIterator::operator->() const {
		return &nextEdge_;
	}

	void Graph::PathIterator::updateNextEdge() {
		if (currentNode_ == destinationNode_ || atEnd_) {
			atEnd_ = true;
			return;
		}

		int edgeIndex = graph_->edgeIndices[destinationNode_][currentNode_];
		if (edgeIndex == -1) {
			// No valid next edge, mark iterator as at the end
			atEnd_ = true;
		}
		else {
			// Update `nextEdge_` to the next edge in the path
			nextEdge_ = graph_->edges[edgeIndex];
		}
	}


	Graph::PathIterable::PathIterable(const Graph& graph, int64_t origin, int64_t dest)
		: graph_(graph), origin_(origin), dest_(dest) {}

	Graph::PathIterator Graph::PathIterable::begin() const {
		return PathIterator(&graph_, origin_, dest_);
	}

	Graph::PathIterator Graph::PathIterable::end() const {
		return Graph::PathIterator(&graph_, dest_, dest_);
	}

	const Graph::Edge& Graph::NextEdge(int64_t origin_node, int64_t destination_node) const {
		if (origin_node < 0 || origin_node >= numNodes){			
			throw DynaPlex::Error("Invalid origin node index in NextEdge query"+ TypeSetNode(origin_node) );
		}

		if (destination_node < 0 || destination_node >= numNodes) {
			throw DynaPlex::Error("Invalid destination node index in NextEdge query" + TypeSetNode(destination_node) );
		}
		if (distances[destination_node][origin_node] == std::numeric_limits<WeightType>::max()) {
			throw DynaPlex::Error("No valid path for NextEdge query " + TypeSetNode(origin_node)+ " -> " + TypeSetNode(destination_node));
		}

		if (origin_node == destination_node)
			throw DynaPlex::Error("origin equals destination in NextEdge query " + TypeSetNode(origin_node) + " -> " + TypeSetNode(destination_node));

		int edgeIndex = edgeIndices[destination_node][origin_node];
		if (edgeIndex == -1 || edgeIndex >= edges.size()) {
			throw DynaPlex::Error("Failed to find next edge in NextEdge query " + TypeSetNode(origin_node) + " -> " + TypeSetNode(destination_node));
		}

		return edges[edgeIndex];
	}
	void Graph::UpdateNumNodes() {
		for (auto& edge : edges)
		{
			if (edge.dest >= numNodes)
				numNodes = edge.dest + 1;
			if (edge.orig >= numNodes)
				numNodes = edge.orig + 1;
		}
	}

	void Graph::ValidateEdgeVector(bool allow_double_edges) {
		

		std::sort(edges.begin(), edges.end(), [](const Edge& a, const Edge& b) {
			return a.orig < b.orig || (a.orig == b.orig && a.dest < b.dest);
			});
		if (!allow_double_edges)
		{	//we don't accept duplicate edges, because they are likely an input error. 

			for (size_t i = 1; i < edges.size(); ++i) {
				if (edges[i].orig == edges[i - 1].orig && edges[i].dest == edges[i - 1].dest) {
					throw DynaPlex::Error("Graph:: Duplicate edge found : " + std::to_string(edges[i].orig) + " -> " + std::to_string(edges[i].dest));
				}
			}
		}
		else
		{  //if explicitly requested, we allow double edges, removing any edges
			//with same origin and destination but with higher weight.

			std::vector<Edge> edges_with_duplicates = std::move(edges);
			edges.clear(); // Clear the original edges vector
			edges.reserve(edges_with_duplicates.size());

			// Iterate through the sorted edges with duplicates
			for (size_t i = 0; i < edges_with_duplicates.size(); ++i) {
				// Check if this edge is a duplicate
				if (!edges.empty() &&
					edges.back().orig == edges_with_duplicates[i].orig &&
					edges.back().dest == edges_with_duplicates[i].dest) {

					// Update the weight if the current edge has a lower weight
					if (edges_with_duplicates[i].weight < edges.back().weight) {
						edges.back().weight = edges_with_duplicates[i].weight;
					}
				}
				else {
					// If it's not a duplicate, add the edge to the vector
					edges.push_back(edges_with_duplicates[i]);
				}
			}
		}
	}

	
	//precomputeshortestpath
	void Graph::PCSP() {
		is_finalized = true;
		distances.resize(numNodes, std::vector<WeightType>(numNodes, std::numeric_limits<WeightType>::max()));
		edgeIndices.resize(numNodes, std::vector<int32_t>(numNodes, -1));
		
		// Define the Connection structure within this function
		struct Connection {
			uint32_t orig; // Destination node of the edge
			WeightType weight; // Weight of the edge
			uint32_t edge_index;
		};

		// Create the adjacency list from the existing edges
		std::vector<std::vector<Connection>> reverseAdjacencyList(numNodes);
		for (uint32_t i=0;i<edges.size();i++) {
			auto& edge = edges[i];
			reverseAdjacencyList[edge.dest].push_back(Connection{ edge.orig, edge.weight,i });
		}
		
		using QueueElement = std::pair<WeightType, uint32_t>;

		for (uint32_t target = 0; target < numNodes; ++target) {
			distances[target][target] = static_cast<WeightType>(0);
			std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<QueueElement>> activeVertices;
			activeVertices.push({ static_cast<WeightType>(0), target });

			while (!activeVertices.empty()) {
				auto where = activeVertices.top().second;
				WeightType dist = activeVertices.top().first;
				activeVertices.pop();

				if (dist > distances[target][where]) continue;

				for (const auto& connection : reverseAdjacencyList[where]) {
					if (distances[target][connection.orig] > distances[target][where] + connection.weight) {
						distances[target][connection.orig] = distances[target][where] + connection.weight;
						edgeIndices[target][connection.orig] = connection.edge_index;
						activeVertices.push({ distances[target][connection.orig], connection.orig });
					}
				}
			}
		}

	}
	std::string DynaPlex::Graph::TypeSetNode(int64_t node_id) const
	{
		if (is_grid && width>0)
		{
			int64_t row = node_id / width;
			int64_t col = node_id - row * width;
			return std::to_string(node_id) + "=(" + std::to_string(row) + ","+ std::to_string(col) + ")";
		}
		return std::to_string(node_id);
	}
}
