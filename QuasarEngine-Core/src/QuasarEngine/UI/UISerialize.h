#pragma once

#include <memory>
#include <string>
#include <vector>
#include "UIElement.h"

namespace QuasarEngine {

    enum class UISerType : uint8_t {
        Element = 0,
        Container,
        Button,
        Text,
        Checkbox,
        ProgressBar,
        Slider,
        InputText,
        Image,
        Separator,
        TabBar,
        Tabs,
        Menu,
        TooltipLayer
    };

    struct UISaveOptions {
        bool stripEditorOnly = true;
    };

    bool UISaveToFile(const std::shared_ptr<UIElement>& root, const char* path, const UISaveOptions& opt = {});

    std::shared_ptr<UIElement> UILoadFromFile(const char* path);

    UISerType UITypeOf(const UIElement* e);
}