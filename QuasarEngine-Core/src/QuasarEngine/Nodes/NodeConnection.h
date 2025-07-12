#pragma once

#include <memory>
#include <string>

namespace QuasarEngine
{
    class Node;

    struct NodeConnection
    {
        std::weak_ptr<Node> fromNode;
        std::string fromPort;
        std::weak_ptr<Node> toNode;
        std::string toPort;

        NodeConnection(std::shared_ptr<Node> from, const std::string& outPort,
            std::shared_ptr<Node> to, const std::string& inPort)
            : fromNode(from), fromPort(outPort), toNode(to), toPort(inPort) {
        }
    };

}