#pragma once

#include <memory>
#include <string>
#include "Node.h"

namespace QuasarEngine
{
    class Node;

    struct NodeConnection
    {
        std::weak_ptr<Node> fromNode;
        std::string fromPort;
        int fromPortIndex = -1;

        std::weak_ptr<Node> toNode;
        std::string toPort;
        int toPortIndex = -1;

        PortType portType = PortType::Unknown;

        NodeConnection(std::shared_ptr<Node> from, const std::string& outPort,
            std::shared_ptr<Node> to, const std::string& inPort)
            : fromNode(from)
            , fromPort(outPort)
            , toNode(to)
            , toPort(inPort)
        {
        }
    };
}