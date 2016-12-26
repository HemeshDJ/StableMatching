#ifndef BIPARTITE_GRAPH_H
#define BIPARTITE_GRAPH_H

#include <map>
#include <memory>
#include <ostream>
#include "Vertex.h"

class Vertex;  // forward definition for Vertex

class BipartiteGraph {
public:
    typedef std::map<IdType, VertexPtr> ContainerType;
    typedef ContainerType::iterator Iterator;

private:
    /// the partitions A and B
    ContainerType A_;
    ContainerType B_;

public:
    BipartiteGraph(const ContainerType& A, const ContainerType& B);
    virtual ~BipartiteGraph();

    const ContainerType& get_A_partition();
    const ContainerType& get_B_partition();

    friend std::ostream& operator<<(std::ostream& out,
                                    const std::unique_ptr<BipartiteGraph>& G);
};

#endif
