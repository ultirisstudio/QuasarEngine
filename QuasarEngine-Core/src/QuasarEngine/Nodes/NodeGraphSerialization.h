#pragma once

#include <yaml-cpp/yaml.h>
#include "NodeGraph.h"
#include "NodeRegistry.h"

namespace QuasarEngine
{
    YAML::Node SerializeNodeGraph(const NodeGraph& graph);

    void DeserializeNodeGraph(const YAML::Node& root, NodeGraph& graph, NodeRegistry& registry);
}
