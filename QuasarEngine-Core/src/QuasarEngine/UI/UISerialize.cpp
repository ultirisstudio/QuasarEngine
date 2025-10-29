#include "qepch.h"
#include "UISerialize.h"

#include "UIContainer.h"
#include "UIButton.h"
#include "UIText.h"
#include "UITextInput.h"
#include "UICheckbox.h"
#include "UISlider.h"
#include "UIProgressBar.h"
#include "UITabBar.h"
#include "UIMenu.h"
#include "UITooltipLayer.h"
#include "UIImage.h"
#include "UISeparator.h"

#include <fstream>
#include <queue>
#include <limits>

namespace QuasarEngine {

    static const char* kMagic = "QUI2";
    static constexpr uint16_t kVer = 2;

    template<typename T> static void W(std::ofstream& f, const T& v) { f.write((const char*)&v, sizeof(T)); }
    template<typename T> static void R(std::ifstream& f, T& v) { f.read((char*)&v, sizeof(T)); }

    static void WStr(std::ofstream& f, const std::string& s) {
        uint16_t n = (uint16_t)std::min<size_t>(s.size(), std::numeric_limits<uint16_t>::max());
        W<uint16_t>(f, n);
        if (n) f.write(s.data(), n);
    }
    static std::string RStr(std::ifstream& f) {
        uint16_t n = 0; R<uint16_t>(f, n);
        std::string s; s.resize(n);
        if (n) f.read(s.data(), n);
        return s;
    }

    UISerType UITypeOf(const UIElement* e) {
        if (dynamic_cast<const UIContainer*>(e))   return UISerType::Container;
        if (dynamic_cast<const UIButton*>(e))      return UISerType::Button;
        if (dynamic_cast<const UIText*>(e))        return UISerType::Text;
        if (dynamic_cast<const UICheckbox*>(e))    return UISerType::Checkbox;
        if (dynamic_cast<const UISlider*>(e))      return UISerType::Slider;
        if (dynamic_cast<const UIProgressBar*>(e)) return UISerType::ProgressBar;
        if (dynamic_cast<const UITextInput*>(e))   return UISerType::InputText;
        if (dynamic_cast<const UIImage*>(e))       return UISerType::Image;
        if (dynamic_cast<const UISeparator*>(e))   return UISerType::Separator;
        if (dynamic_cast<const UITabBar*>(e))      return UISerType::TabBar;
        if (dynamic_cast<const UITabs*>(e))        return UISerType::Tabs;
        if (dynamic_cast<const UIMenu*>(e))        return UISerType::Menu;
        if (dynamic_cast<const UITooltipLayer*>(e))return UISerType::TooltipLayer;
        return UISerType::Element;
    }

    struct FlatNode {
        UIElement* ptr;
        int parent = -1;
        UISerType type;
    };

    static void WriteCommon(std::ofstream& f, const UIElement* e) {
        auto& tr = e->Transform();
        W<float>(f, tr.pos.x); W<float>(f, tr.pos.y);
        W<float>(f, tr.size.x); W<float>(f, tr.size.y);
        
        auto st = e->Style();
        W<float>(f, st.padding); W<float>(f, st.radius);
        W<float>(f, st.bg.r); W<float>(f, st.bg.g); W<float>(f, st.bg.b); W<float>(f, st.bg.a);
        W<float>(f, st.fg.r); W<float>(f, st.fg.g); W<float>(f, st.fg.b); W<float>(f, st.fg.a);
    }

    static void ReadCommon(std::ifstream& f, std::shared_ptr<UIElement>& e) {
        auto& tr = e->Transform();
        R<float>(f, tr.pos.x); R<float>(f, tr.pos.y);
        R<float>(f, tr.size.x); R<float>(f, tr.size.y);
        auto& st = e->Style();
        R<float>(f, st.padding); R<float>(f, st.radius);
        R<float>(f, st.bg.r); R<float>(f, st.bg.g); R<float>(f, st.bg.b); R<float>(f, st.bg.a);
        R<float>(f, st.fg.r); R<float>(f, st.fg.g); R<float>(f, st.fg.b); R<float>(f, st.fg.a);
    }

    bool UISaveToFile(const std::shared_ptr<UIElement>& root, const char* path, const UISaveOptions&) {
        if (!root || !path) return false;

        std::vector<FlatNode> flat;
        std::queue<std::pair<UIElement*, int>> q;
        q.push({ root.get(), -1 });
        while (!q.empty()) {
            auto [n, p] = q.front(); q.pop();
            FlatNode fn{ n, p, UITypeOf(n) };
            int myIndex = (int)flat.size();
            flat.push_back(fn);
            int childIdx = 0;
            for (auto& c : n->Children()) {
                (void)childIdx++;
                q.push({ c.get(), myIndex });
            }
        }

        std::ofstream f(path, std::ios::binary);
        if (!f) return false;
        f.write(kMagic, 4);
        W<uint16_t>(f, kVer);
        W<uint32_t>(f, (uint32_t)flat.size());

        for (size_t i = 0; i < flat.size(); ++i) {
            const auto& fn = flat[i];
            uint32_t parent = (fn.parent < 0) ? 0xFFFFFFFFu : (uint32_t)fn.parent;
            W<uint32_t>(f, parent);
            W<uint8_t>(f, (uint8_t)fn.type);
            WStr(f, fn.ptr->Id());
            WriteCommon(f, fn.ptr);

            switch (fn.type) {
            case UISerType::Container: {
                auto* c = static_cast<const UIContainer*>(fn.ptr);
                uint8_t layout = (c->layout == UILayoutType::Horizontal) ? 1 : 0;
                W<uint8_t>(f, layout);
                W<float>(f, c->gap);
            } break;
            case UISerType::Button: {
                auto* b = static_cast<const UIButton*>(fn.ptr);
                WStr(f, b->label);
            } break;
            case UISerType::Text: {
                auto* t = static_cast<const UIText*>(fn.ptr);
                WStr(f, t->text);
                W<float>(f, t->fontSize);
            } break;
            case UISerType::Checkbox: {
                auto* c = static_cast<const UICheckbox*>(fn.ptr);
                WStr(f, c->label);
                uint8_t chk = c->checked ? 1 : 0; W<uint8_t>(f, chk);
            } break;
            case UISerType::ProgressBar: {
                auto* p = static_cast<const UIProgressBar*>(fn.ptr);
                W<float>(f, p->min); W<float>(f, p->max); W<float>(f, p->value);
                uint8_t txt = p->showText ? 1 : 0; W<uint8_t>(f, txt);
            } break;
            case UISerType::Slider: {
                auto* s = static_cast<const UISlider*>(fn.ptr);
                W<float>(f, s->min); W<float>(f, s->max); W<float>(f, s->value);
            } break;
            case UISerType::InputText: {
                auto* it = static_cast<const UITextInput*>(fn.ptr);
                WStr(f, it->text);
                uint8_t ro = it->readOnly ? 1 : 0; W<uint8_t>(f, ro);
                W<float>(f, it->minWidth);
            } break;
            case UISerType::Image: {
                auto* im = static_cast<const UIImage*>(fn.ptr);
                WStr(f, im->textureId);
                uint8_t ka = im->keepAspect ? 1 : 0; W<uint8_t>(f, ka);
                W<float>(f, im->tint.r); W<float>(f, im->tint.g); W<float>(f, im->tint.b); W<float>(f, im->tint.a);
            } break;
            case UISerType::Separator: {
                auto* sep = static_cast<const UISeparator*>(fn.ptr);
                W<float>(f, sep->thickness);
                W<float>(f, sep->color.r); W<float>(f, sep->color.g); W<float>(f, sep->color.b); W<float>(f, sep->color.a);
            } break;
            case UISerType::TabBar: {
                auto* tb = static_cast<const UITabBar*>(fn.ptr);
                W<int>(f, tb->selected);
                uint16_t n = (uint16_t)tb->labels.size(); W<uint16_t>(f, n);
                for (auto& s : tb->labels) WStr(f, s);
            } break;
            case UISerType::Tabs: {
                auto* ts = static_cast<const UITabs*>(fn.ptr);
                W<int>(f, ts->m_Selected);
            } break;
            case UISerType::Menu: {
                auto* m = static_cast<const UIMenu*>(fn.ptr);
                uint16_t n = (uint16_t)m->items.size(); W<uint16_t>(f, n);
                for (auto& it : m->items) { WStr(f, it.label); uint8_t d = it.disabled ? 1 : 0; W<uint8_t>(f, d); }
            } break;
            case UISerType::TooltipLayer: {
                
            } break;
            default: break;
            }
        }
        return (bool)f;
    }

    static std::shared_ptr<UIElement> NewByType(UISerType t, const std::string& id) {
        switch (t) {
        case UISerType::Container:   return std::make_shared<UIContainer>(id);
        case UISerType::Button:      return std::make_shared<UIButton>(id);
        case UISerType::Text:        return std::make_shared<UIText>(id);
        case UISerType::Checkbox:    return std::make_shared<UICheckbox>(id);
        case UISerType::ProgressBar: return std::make_shared<UIProgressBar>(id);
        case UISerType::Slider:      return std::make_shared<UISlider>(id);
        case UISerType::InputText:   return std::make_shared<UITextInput>(id);
        case UISerType::Image:       return std::make_shared<UIImage>(id);
        case UISerType::Separator:   return std::make_shared<UISeparator>(id);
        case UISerType::TabBar:      return std::make_shared<UITabBar>(id);
        case UISerType::Tabs:        return std::make_shared<UITabs>(id);
        case UISerType::Menu:        return std::make_shared<UIMenu>(id);
        case UISerType::TooltipLayer:return std::make_shared<UITooltipLayer>(id);
        default:                     return std::make_shared<UIElement>(id);
        }
    }

    std::shared_ptr<UIElement> UILoadFromFile(const char* path) {
        if (!path) return nullptr;
        std::ifstream f(path, std::ios::binary);
        if (!f) return nullptr;

        char magic[4]; f.read(magic, 4); if (std::memcmp(magic, kMagic, 4) != 0) return nullptr;
        uint16_t ver = 0; R<uint16_t>(f, ver); if (ver != kVer) return nullptr;
        uint32_t count = 0; R<uint32_t>(f, count); if (!count) return nullptr;

        struct NodeTmp { std::shared_ptr<UIElement> e; int parent; UISerType t; };
        std::vector<NodeTmp> nodes; nodes.reserve(count);

        for (uint32_t i = 0; i < count; ++i) {
            uint32_t parent = 0xFFFFFFFFu; R<uint32_t>(f, parent);
            uint8_t t = 0; R<uint8_t>(f, t);
            std::string id = RStr(f);
            auto e = NewByType((UISerType)t, id);
            ReadCommon(f, e);

            switch ((UISerType)t) {
            case UISerType::Container: {
                auto* c = static_cast<UIContainer*>(e.get());
                uint8_t layout = 0; R<uint8_t>(f, layout);
                c->layout = (layout == 1) ? UILayoutType::Horizontal : UILayoutType::Vertical;
                R<float>(f, c->gap);
            } break;
            case UISerType::Button: {
                auto* b = static_cast<UIButton*>(e.get());
                b->label = RStr(f);
            } break;
            case UISerType::Text: {
                auto* t2 = static_cast<UIText*>(e.get());
                t2->text = RStr(f);
                R<float>(f, t2->fontSize);
            } break;
            case UISerType::Checkbox: {
                auto* c = static_cast<UICheckbox*>(e.get());
                c->label = RStr(f);
                uint8_t ch = 0; R<uint8_t>(f, ch); c->checked = (ch != 0);
            } break;
            case UISerType::ProgressBar: {
                auto* p = static_cast<UIProgressBar*>(e.get());
                R<float>(f, p->min); R<float>(f, p->max); R<float>(f, p->value);
                uint8_t st = 0; R<uint8_t>(f, st); p->showText = (st != 0);
            } break;
            case UISerType::Slider: {
                auto* s = static_cast<UISlider*>(e.get());
                R<float>(f, s->min); R<float>(f, s->max); R<float>(f, s->value);
            } break;
            case UISerType::InputText: {
                auto* it = static_cast<UITextInput*>(e.get());
                it->text = RStr(f);
                uint8_t ro = 0; R<uint8_t>(f, ro); it->readOnly = (ro != 0);
                R<float>(f, it->minWidth);
            } break;
            case UISerType::Image: {
                auto* im = static_cast<UIImage*>(e.get());
                im->textureId = RStr(f);
                uint8_t ka = 0; R<uint8_t>(f, ka); im->keepAspect = (ka != 0);
                R<float>(f, im->tint.r); R<float>(f, im->tint.g); R<float>(f, im->tint.b); R<float>(f, im->tint.a);
            } break;
            case UISerType::Separator: {
                auto* sp = static_cast<UISeparator*>(e.get());
                R<float>(f, sp->thickness);
                R<float>(f, sp->color.r); R<float>(f, sp->color.g); R<float>(f, sp->color.b); R<float>(f, sp->color.a);
            } break;
            case UISerType::TabBar: {
                auto* tb = static_cast<UITabBar*>(e.get());
                R<int>(f, tb->selected);
                uint16_t n = 0; R<uint16_t>(f, n);
                tb->labels.clear(); tb->labels.reserve(n);
                for (uint16_t k = 0; k < n; ++k) tb->labels.push_back(RStr(f));
            } break;
            case UISerType::Tabs: {
                auto* ts = static_cast<UITabs*>(e.get());
                R<int>(f, ts->m_Selected);
            } break;
            case UISerType::Menu: {
                auto* m = static_cast<UIMenu*>(e.get());
                uint16_t n = 0; R<uint16_t>(f, n);
                m->items.clear(); m->items.reserve(n);
                for (uint16_t k = 0; k < n; ++k) {
                    UIMenuItem it{};
                    it.label = RStr(f);
                    uint8_t d = 0; R<uint8_t>(f, d); it.disabled = (d != 0);
                    m->items.push_back(std::move(it));
                }
            } break;
            case UISerType::TooltipLayer:
            default: break;
            }

            nodes.push_back({ std::move(e), parent == 0xFFFFFFFFu ? -1 : (int)parent, (UISerType)t });
        }

        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].parent >= 0) {
                nodes[nodes[i].parent].e->AddChild(nodes[i].e);
            }
        }
        return nodes[0].e;
    }
}