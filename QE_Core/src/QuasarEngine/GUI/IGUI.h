#pragma once

class IGUI {
public:
    virtual ~IGUI() = default;
    virtual void Initialize() = 0;
    virtual void Render() = 0;
    virtual void Shutdown() = 0;
};
