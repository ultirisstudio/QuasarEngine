#pragma once

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void Initialize() = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
};
