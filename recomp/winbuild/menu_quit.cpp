// Quit button on the in-game menu (outMenuTop), beside Return to Title.
//
// The title screen's QUIT GAME button asks with cmnPopupManager.Selection2Buttons
// (string ids 8, 9, 10 and 11) and, on yes, calls Application.Quit. This button
// saves first, then opens that same dialog with the title screen's own yes/no
// callback, so confirming leaves the game the same way.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "il2cpp-api.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace recomp
{
    void OpenAchievements();  // ingame_menu.cpp

    namespace
    {
        struct Failure
        {
            std::string what;
        };

        struct Vec2
        {
            float x, y;
        };

        struct Nav
        {
            int32_t mode;
            uint8_t wrap;
            uint8_t pad[3];
            Il2CppObject* up;
            Il2CppObject* down;
            Il2CppObject* left;
            Il2CppObject* right;
        };
        static_assert(sizeof(Nav) == 0x28, "Navigation layout");

        const Il2CppImage* ImageNamed(const char* file)
        {
            size_t n = 0;
            const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &n);
            for (size_t i = 0; i < n; i++)
            {
                const Il2CppImage* img = il2cpp_assembly_get_image(assemblies[i]);
                if (!strcmp(il2cpp_image_get_name(img), file))
                    return img;
            }
            return nullptr;
        }

        Il2CppClass* Class(const char* image, const char* ns, const char* name)
        {
            const Il2CppImage* img = ImageNamed(image);
            if (!img)
                throw Failure{ std::string(image) + " is not loaded" };
            Il2CppClass* k = il2cpp_class_from_name(img, ns, name);
            if (!k)
                throw Failure{ std::string(name) + " was not found" };
            il2cpp_runtime_class_init(k);
            return k;
        }

        Il2CppClass* Nested(Il2CppClass* outer, const char* name)
        {
            void* iter = nullptr;
            while (Il2CppClass* k = il2cpp_class_get_nested_types(outer, &iter))
                if (!strcmp(il2cpp_class_get_name(k), name))
                    return k;
            throw Failure{ std::string(name) + " was not found" };
        }

        const MethodInfo* Method(Il2CppClass* k, const char* name, int argc, const char* argType = nullptr, int argIndex = 0)
        {
            for (; k; k = il2cpp_class_get_parent(k))
            {
                il2cpp_runtime_class_init(k);
                bool inflated = il2cpp_class_is_inflated(k);
                void* iter = nullptr;
                while (const MethodInfo* m = il2cpp_class_get_methods(k, &iter))
                {
                    if (m->is_generic || (m->is_inflated && !inflated))
                        continue;
                    if (strcmp(il2cpp_method_get_name(m), name) != 0 || int(il2cpp_method_get_param_count(m)) != argc)
                        continue;
                    if (argType)
                    {
                        char* typeName = il2cpp_type_get_name(m->parameters[argIndex].parameter_type);
                        bool match = typeName && strstr(typeName, argType);
                        il2cpp_free(typeName);
                        if (!match)
                            continue;
                    }
                    return m;
                }
            }
            throw Failure{ std::string(name) + " was not found" };
        }

        Il2CppObject* Invoke(const MethodInfo* m, Il2CppObject* obj, void** args)
        {
            Il2CppException* exc = nullptr;
            Il2CppObject* r = il2cpp_runtime_invoke(m, obj, args, &exc);
            if (exc)
            {
                char msg[512] = {};
                il2cpp_format_exception(exc, msg, sizeof(msg));
                throw Failure{ std::string(il2cpp_method_get_name(m)) + ": " + msg };
            }
            return r;
        }

        template <typename... A>
        Il2CppObject* Call(Il2CppObject* obj, const char* name, A... a)
        {
            void* args[sizeof...(A) + 1] = { static_cast<void*>(a)... };
            return Invoke(Method(il2cpp_object_get_class(obj), name, int(sizeof...(A))), obj, args);
        }

        template <typename... A>
        Il2CppObject* CallStatic(Il2CppClass* k, const char* name, A... a)
        {
            void* args[sizeof...(A) + 1] = { static_cast<void*>(a)... };
            return Invoke(Method(k, name, int(sizeof...(A))), nullptr, args);
        }

        bool Truth(Il2CppObject* boxed)
        {
            return boxed && *static_cast<uint8_t*>(il2cpp_object_unbox(boxed));
        }

        Il2CppObject* Singleton(const char* name)
        {
            Il2CppClass* k = Class("Assembly-CSharp.dll", "", name);
            FieldInfo* f = nullptr;
            for (Il2CppClass* c = k; c && !f; c = il2cpp_class_get_parent(c))
                f = il2cpp_class_get_field_from_name(c, "<Instance>k__BackingField");
            if (!f)
                throw Failure{ std::string(name) + " has no instance" };
            Il2CppObject* obj = nullptr;
            il2cpp_field_static_get_value(f, &obj);
            if (!obj)
                throw Failure{ std::string(name) + " is not created yet" };
            return obj;
        }

        Il2CppObject* TypeObject(Il2CppClass* k)
        {
            return il2cpp_type_get_object(il2cpp_class_get_type(k));
        }

        Il2CppObject* GetComponent(Il2CppObject* component, Il2CppClass* kind)
        {
            Il2CppObject* type = TypeObject(kind);
            return Call(component, "GetComponent", type);
        }

        Il2CppString* NewString(const char* utf8)
        {
            return il2cpp_string_new(utf8);
        }

        bool SameText(Il2CppString* a, Il2CppString* b)
        {
            if (!a || !b)
                return false;
            int32_t n = il2cpp_string_length(a);
            if (n != il2cpp_string_length(b))
                return false;
            return memcmp(il2cpp_string_chars(a), il2cpp_string_chars(b), size_t(n) * sizeof(Il2CppChar)) == 0;
        }

        bool NameIs(Il2CppObject* obj, const char* utf8)
        {
            return SameText(reinterpret_cast<Il2CppString*>(Call(obj, "get_name")), NewString(utf8));
        }

        Il2CppString* TableString(int32_t id)
        {
            return reinterpret_cast<Il2CppString*>(CallStatic(Class("Assembly-CSharp.dll", "", "gdTable"), "GetString", &id));
        }

        // Title-screen quit dialog, from ttlUiEmpty.<AddEventListeners>b__89_2.
        void ShowTitleQuitDialog()
        {
            Il2CppClass* popupClass = Class("Assembly-CSharp.dll", "", "cmnPopupManager");
            const MethodInfo* show = Method(popupClass, "Selection2Buttons", 12);
            Il2CppClass* holder = Nested(Class("Assembly-CSharp.dll", "", "ttlUiEmpty"), "<>c");
            il2cpp_runtime_class_init(holder);
            FieldInfo* cached = il2cpp_class_get_field_from_name(holder, "<>9__89_3");
            Il2CppObject* action = nullptr;
            if (cached)
                il2cpp_field_static_get_value(cached, &action);
            if (!action)
            {
                Il2CppClass* actionClass = il2cpp_class_from_type(show->parameters[1].parameter_type);
                const MethodInfo* yes = Method(holder, "<AddEventListeners>b__89_3", 1);
                FieldInfo* singletonField = il2cpp_class_get_field_from_name(holder, "<>9");
                Il2CppObject* target = nullptr;
                if (singletonField)
                    il2cpp_field_static_get_value(singletonField, &target);
                if (!target)
                    throw Failure{ "title quit callback is not ready" };
                action = il2cpp_object_new(actionClass);
                intptr_t methodBits = reinterpret_cast<intptr_t>(yes);
                void* ctorArgs[] = { target, &methodBits };
                Invoke(Method(actionClass, ".ctor", 2), action, ctorArgs);
            }

            int32_t messageId = 9, titleId = 8, leftId = 10, rightId = 11;
            Il2CppString* message = TableString(messageId);
            Il2CppString* title = TableString(titleId);
            Il2CppString* left = TableString(leftId);
            Il2CppString* right = TableString(rightId);
            int32_t gray = 1;
            int32_t anchor = 4;
            uint8_t forceMove = 0, scrollLock = 1, header = 0, negativeFocus = 1, yesNo = 0;
            void* args[] = {
                message, action, &gray, title, left, right, &anchor,
                &forceMove, &scrollLock, &header, &negativeFocus, &yesNo
            };
            Invoke(show, Singleton("cmnPopupManager"), args);
        }

        bool SaveGame()
        {
            Il2CppObject* save = Call(Singleton("gdUser"), "get_saveData");
            if (!save || !Call(save, "GetContents"))
                throw Failure{ "no save is loaded" };
            Call(save, "UpdateWorkWithCurrentContents");
            bool force = true;
            bool countSync = false;
            return Truth(Call(save, "Save", &force, &countSync));
        }

        uint64_t OnQuitClicked(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
        {
            static uint64_t lastTick = 0;
            uint64_t now = GetTickCount64();
            if (now - lastTick < 600)
                return 0;
            lastTick = now;
            try
            {
                try
                {
                    Call(Singleton("outSoundManager"), "ButtonClick");
                }
                catch (const Failure&)
                {
                }
                if (!SaveGame())
                    throw Failure{ "the save was not written" };
                ShowTitleQuitDialog();
                fprintf(stderr, "GoSRecomp: saved, then opened the quit dialog\n");
            }
            catch (const Failure& f)
            {
                fprintf(stderr, "GoSRecomp: quit was not opened (%s)\n", f.what.c_str());
            }
            catch (...)
            {
                fprintf(stderr, "GoSRecomp: quit was not opened\n");
            }
            return 0;
        }

        using ClickHandler = uint64_t (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

        uint64_t OnAchievementsClicked(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
        {
            static uint64_t lastTick = 0;
            uint64_t now = GetTickCount64();
            if (now - lastTick < 600)
                return 0;
            lastTick = now;
            try
            {
                Call(Singleton("outSoundManager"), "ButtonClick");
            }
            catch (const Failure&)
            {
            }
            recomp::OpenAchievements();
            return 0;
        }

        Il2CppObject* MakeClickAction(Il2CppObject* target, Il2CppClass* actionClass, ClickHandler handler = OnQuitClicked)
        {
            // one method record per handler, from any parameterless instance method
            static MethodInfo quit{}, achievements{};
            MethodInfo& click = handler == OnQuitClicked ? quit : achievements;
            if (!click.methodPointer)
            {
                const MethodInfo* sample = Method(Class("Assembly-CSharp.dll", "", "outMenuTop"), "<OnCreateTitleButton>b__47_0", 0);
                click = *sample;
                click.methodPointer = reinterpret_cast<Il2CppMethodPointer>(handler);
            }
            Il2CppObject* action = il2cpp_object_new(actionClass);
            intptr_t methodBits = reinterpret_cast<intptr_t>(&click);
            void* args[] = { target, &methodBits };
            Invoke(Method(actionClass, ".ctor", 2), action, args);
            return action;
        }

        int ReplaceMatchingText(Il2CppObject* root, Il2CppString* from, Il2CppString* to, Il2CppClass* componentClass)
        {
            Il2CppObject* type = TypeObject(componentClass);
            auto* list = reinterpret_cast<Il2CppArray*>(Call(root, "GetComponents", type));
            if (!list)
                return 0;
            int replaced = 0;
            auto** items = reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(list) + kIl2CppSizeOfArray);
            for (size_t i = 0; i < il2cpp_array_length(list); i++)
            {
                Il2CppObject* c = items[i];
                if (!c || !il2cpp_class_get_method_from_name(il2cpp_object_get_class(c), "get_text", 0))
                    continue;
                auto* text = reinterpret_cast<Il2CppString*>(Call(c, "get_text"));
                if (SameText(text, from))
                {
                    Call(c, "set_text", to);
                    replaced++;
                }
            }
            return replaced;
        }

        int RetextTree(Il2CppObject* transform, Il2CppString* from, Il2CppString* to, Il2CppClass* componentClass)
        {
            int replaced = ReplaceMatchingText(Call(transform, "get_gameObject"), from, to, componentClass);
            int32_t count = *static_cast<int32_t*>(il2cpp_object_unbox(Call(transform, "get_childCount")));
            for (int32_t i = 0; i < count; i++)
                replaced += RetextTree(Call(transform, "GetChild", &i), from, to, componentClass);
            return replaced;
        }

        bool TreeHasName(Il2CppObject* transform, const char* name)
        {
            if (NameIs(transform, name) || NameIs(Call(transform, "get_gameObject"), name))
                return true;
            int32_t count = *static_cast<int32_t*>(il2cpp_object_unbox(Call(transform, "get_childCount")));
            for (int32_t i = 0; i < count; i++)
                if (TreeHasName(Call(transform, "GetChild", &i), name))
                    return true;
            return false;
        }

        Il2CppObject* ButtonOf(Il2CppObject* component, Il2CppClass* buttonClass)
        {
            for (Il2CppObject* current = component; current; )
            {
                if (Il2CppObject* button = GetComponent(current, buttonClass))
                    return button;
                Il2CppObject* transform = Call(current, "get_transform");
                Il2CppObject* parent = Call(transform, "get_parent");
                if (!parent)
                    return nullptr;
                current = Call(parent, "get_gameObject");
            }
            return nullptr;
        }

        Il2CppObject* FindLabeledButton(Il2CppObject* transform, Il2CppString* label, Il2CppClass* buttonClass, Il2CppClass* componentClass)
        {
            Il2CppObject* type = TypeObject(componentClass);
            auto* list = reinterpret_cast<Il2CppArray*>(Call(Call(transform, "get_gameObject"), "GetComponents", type));
            if (list)
            {
                auto** items = reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(list) + kIl2CppSizeOfArray);
                for (size_t i = 0; i < il2cpp_array_length(list); i++)
                {
                    Il2CppObject* c = items[i];
                    if (!c || !il2cpp_class_get_method_from_name(il2cpp_object_get_class(c), "get_text", 0))
                        continue;
                    if (!SameText(reinterpret_cast<Il2CppString*>(Call(c, "get_text")), label))
                        continue;
                    if (Il2CppObject* button = ButtonOf(c, buttonClass))
                        return button;
                }
            }
            int32_t count = *static_cast<int32_t*>(il2cpp_object_unbox(Call(transform, "get_childCount")));
            for (int32_t i = 0; i < count; i++)
                if (Il2CppObject* found = FindLabeledButton(Call(transform, "GetChild", &i), label, buttonClass, componentClass))
                    return found;
            return nullptr;
        }

        bool HasLayout(Il2CppObject* component, const char* name)
        {
            Il2CppClass* layout = Class("UnityEngine.UI.dll", "UnityEngine.UI", name);
            return GetComponent(component, layout) != nullptr;
        }

        void PlaceBeside(Il2CppObject* titleButton, Il2CppObject* quitObject)
        {
            Il2CppObject* titleTransform = Call(titleButton, "get_transform");
            Il2CppObject* parent = Call(titleTransform, "get_parent");
            bool stay = false;
            Call(Call(quitObject, "get_transform"), "SetParent", parent, &stay);
            int32_t index = *static_cast<int32_t*>(il2cpp_object_unbox(Call(titleTransform, "GetSiblingIndex"))) + 1;
            Call(Call(quitObject, "get_transform"), "SetSiblingIndex", &index);

            bool arranged = HasLayout(Call(parent, "get_gameObject"), "GridLayoutGroup") || HasLayout(Call(parent, "get_gameObject"), "HorizontalLayoutGroup");
            fprintf(stderr, "GoSRecomp: quit button parent %s (%s)\n", il2cpp_class_get_name(il2cpp_object_get_class(parent)), arranged ? "row layout" : "offset beside the title button");
            if (!arranged)
            {
                Il2CppObject* quitTransform = Call(quitObject, "get_transform");
                auto* pos = static_cast<Vec2*>(il2cpp_object_unbox(Call(titleTransform, "get_anchoredPosition")));
                auto* rect = static_cast<float*>(il2cpp_object_unbox(Call(titleTransform, "get_rect")));
                Vec2 moved{ pos->x + rect[2] + 16.0f, pos->y };
                Call(quitTransform, "set_anchoredPosition", &moved);
                if (HasLayout(Call(parent, "get_gameObject"), "VerticalLayoutGroup"))
                {
                    Il2CppClass* elementClass = Class("UnityEngine.UI.dll", "UnityEngine.UI", "LayoutElement");
                    Il2CppObject* element = Call(quitObject, "AddComponent", TypeObject(elementClass));
                    bool ignore = true;
                    if (element)
                        Call(element, "set_ignoreLayout", &ignore);
                }
            }
            if (Il2CppClass* rebuilder = Class("UnityEngine.UI.dll", "UnityEngine.UI", "LayoutRebuilder"))
                CallStatic(rebuilder, "ForceRebuildLayoutImmediate", parent);
        }

        void LinkNavigation(Il2CppObject* titleButton, Il2CppObject* quitButton)
        {
            auto* titleNav = static_cast<Nav*>(il2cpp_object_unbox(Call(titleButton, "get_navigation")));
            auto* quitNav = static_cast<Nav*>(il2cpp_object_unbox(Call(quitButton, "get_navigation")));
            if (titleNav->mode != 4)
                return;
            Nav title = *titleNav;
            Nav quit = *quitNav;
            quit.left = titleButton;
            quit.right = title.right;
            quit.up = title.up;
            quit.down = title.down;
            title.right = quitButton;
            Call(titleButton, "set_navigation", &title);
            Call(quitButton, "set_navigation", &quit);
        }

        void AddQuitButton(Il2CppObject* menu)
        {
            if (!Truth(Call(Call(menu, "get_gameObject"), "get_activeInHierarchy")))
                return;
            Il2CppObject* root = Call(menu, "get_transform");
            if (TreeHasName(root, "GosrQuit"))
                return;

            int32_t titleId = 139; // outMenuTop title-button label, read by SetButtonNames via GetString
            Il2CppString* titleLabel = TableString(titleId);
            if (!titleLabel || il2cpp_string_length(titleLabel) == 0)
                return;
            Il2CppClass* buttonClass = Class("UnityEngine.UI.dll", "UnityEngine.UI", "Button");
            Il2CppClass* componentClass = Class("UnityEngine.CoreModule.dll", "UnityEngine", "Component");
            Il2CppObject* titleButton = FindLabeledButton(root, titleLabel, buttonClass, componentClass);
            if (!titleButton)
                return;

            Il2CppObject* titleObject = Call(titleButton, "get_gameObject");
            Il2CppObject* parent = Call(Call(titleButton, "get_transform"), "get_parent");
            if (!parent)
                throw Failure{ "Return to Title has no parent" };
            Il2CppClass* objectClass = Class("UnityEngine.CoreModule.dll", "UnityEngine", "Object");
            bool world = false;
            void* instantiateArgs[] = { titleObject, parent, &world };
            Il2CppObject* quitObject = Invoke(Method(objectClass, "Instantiate", 3, "Transform", 1), nullptr, instantiateArgs);
            if (!quitObject)
                throw Failure{ "the button could not be copied" };
            try
            {
                Call(quitObject, "set_name", NewString("GosrQuit"));
                PlaceBeside(titleButton, quitObject);
                int replaced = RetextTree(Call(quitObject, "get_transform"), titleLabel, NewString("Quit Game"), componentClass);
                if (!replaced)
                    throw Failure{ "the copied label could not be changed" };

                Il2CppObject* quitButton = GetComponent(quitObject, buttonClass);
                if (!quitButton)
                    throw Failure{ "the copied object has no button" };
                Il2CppObject* click = Call(quitButton, "get_onClick");
                Call(click, "RemoveAllListeners");
                const MethodInfo* add = Method(il2cpp_object_get_class(click), "AddListener", 1);
                Il2CppClass* actionClass = il2cpp_class_from_type(add->parameters[0].parameter_type);
                Call(click, "AddListener", MakeClickAction(menu, actionClass));
                try
                {
                    LinkNavigation(titleButton, quitButton);
                }
                catch (const Failure& nav)
                {
                    fprintf(stderr, "GoSRecomp: quit button has no controller link (%s)\n", nav.what.c_str());
                }
                fprintf(stderr, "GoSRecomp: added Quit Game beside Return to Title\n");
            }
            catch (...)
            {
                CallStatic(objectClass, "Destroy", quitObject);
                throw;
            }
        }

        // The menu's objects, kept for the check every frame (MenuFrame).
        struct MenuObjects
        {
            uint32_t menu = 0, section = 0, button = 0, scroll = 0, screen = 0;
        };
        MenuObjects g_menu;
        ULONGLONG g_keepFocusUntil = 0;  // the menu just opened: Achievements keeps the cursor

        Il2CppObject* Held(uint32_t h) { return h ? il2cpp_gchandle_get_target(h) : nullptr; }

        void Remember(Il2CppObject* menu, Il2CppObject* section, Il2CppObject* button, Il2CppObject* content, Il2CppObject* root)
        {
            if (Held(g_menu.menu) == menu && Held(g_menu.button) == button)
                return;
            for (uint32_t* h : { &g_menu.menu, &g_menu.section, &g_menu.button, &g_menu.scroll, &g_menu.screen })
                if (*h)
                    il2cpp_gchandle_free(*h), *h = 0;
            g_menu.menu = il2cpp_gchandle_new(menu, false);
            g_menu.section = il2cpp_gchandle_new(section, false);
            g_menu.button = il2cpp_gchandle_new(button, false);
            if (Il2CppObject* view = Call(root, "Find", NewString("list/contents/Table View00")))
                if (Il2CppObject* scroll = GetComponent(Call(view, "get_gameObject"), Class("UnityEngine.UI.dll", "UnityEngine.UI", "ScrollRect")))
                    g_menu.scroll = il2cpp_gchandle_new(scroll, false);
            if (Il2CppObject* screen = Call(content, "Find", NewString("menu_parts2/btn_set/Button01")))
                g_menu.screen = il2cpp_gchandle_new(Call(screen, "get_gameObject"), false);
        }

        void ShowAchievementsButton(Il2CppObject* menu);
        void FocusAchievements(Il2CppObject* menu, Il2CppObject* buttonObject);

        // Every frame while the menu is open: the section shown again the
        // moment the menu hides it (it does on each opening), the cursor
        // taken back from Screen once when the menu puts it there as it
        // finishes opening, and the list scrolled to the very top whenever
        // Achievements is selected (the heading above it too).
        void MenuFrame()
        {
            Il2CppObject* menu = Held(g_menu.menu);
            if (!menu)
                return;
            try
            {
                if (!Truth(Call(Call(menu, "get_gameObject"), "get_activeInHierarchy")))
                    return;  // closed
                if (!Truth(Call(Held(g_menu.section), "get_activeSelf")))
                {
                    ShowAchievementsButton(menu);  // just reopened
                    return;
                }
                Il2CppObject* events = CallStatic(Class("UnityEngine.UI.dll", "UnityEngine.EventSystems", "EventSystem"), "get_current");
                Il2CppObject* selected = events ? Call(events, "get_currentSelectedGameObject") : nullptr;
                Il2CppObject* button = Held(g_menu.button);
                if (g_keepFocusUntil)
                {
                    if (GetTickCount64() > g_keepFocusUntil)
                        g_keepFocusUntil = 0;
                    else if (selected && selected == Held(g_menu.screen))
                    {
                        g_keepFocusUntil = 0;
                        FocusAchievements(menu, button);
                        selected = button;
                    }
                }
                if (selected && selected == button)
                    if (Il2CppObject* scroll = Held(g_menu.scroll))
                    {
                        float at = *static_cast<float*>(il2cpp_object_unbox(Call(scroll, "get_verticalNormalizedPosition")));
                        if (at < 0.999f)
                        {
                            float top = 1.0f;
                            Call(scroll, "set_verticalNormalizedPosition", &top);
                        }
                    }
            }
            catch (const Failure&)
            {
                for (uint32_t* h : { &g_menu.menu, &g_menu.section, &g_menu.button, &g_menu.scroll, &g_menu.screen })
                    if (*h)
                        il2cpp_gchandle_free(*h), *h = 0;
            }
        }

        // The menu opens with its cursor on Achievements, the top button:
        // selected, the game's cursor frame moved there, and made the
        // menu's starting button in its cursor selector.
        void FocusAchievements(Il2CppObject* menu, Il2CppObject* buttonObject)
        {
            try
            {
                Il2CppClass* events = Class("UnityEngine.UI.dll", "UnityEngine.EventSystems", "EventSystem");
                if (Il2CppObject* current = CallStatic(events, "get_current"))
                    Call(current, "SetSelectedGameObject", buttonObject);
                Il2CppClass* menuClass = il2cpp_object_get_class(menu);
                FieldInfo* selectorField = il2cpp_class_get_field_from_name(menuClass, "m_cmnCursorSelectorScr");
                FieldInfo* indexField = il2cpp_class_get_field_from_name(menuClass, "m_registIndex");
                Il2CppObject* selector = nullptr;
                int32_t index = -1;
                if (selectorField)
                    il2cpp_field_get_value(menu, selectorField, &selector);
                if (indexField)
                    il2cpp_field_get_value(menu, indexField, &index);
                if (!selector)
                    return;
                // The menu set up its scrolling and registered its buttons
                // with the cursor selector while the section was hidden: do
                // both again, so Achievements is one of them (the first) and
                // scrolling counts the section.
                try
                {
                    Call(menu, "CreateScroll");
                }
                catch (const Failure& f)
                {
                    fprintf(stderr, "GoSRecomp: menu scroll not set up again (%s)\n", f.what.c_str());
                }
                if (index >= 0)
                {
                    const MethodInfo* add = Method(il2cpp_object_get_class(selector), "AddFocusCursorObject", 5);
                    bool clear = true, shown = true;
                    void* args[] = { Call(menu, "get_transform"), &clear, Call(buttonObject, "get_transform"), &shown, &index };
                    Invoke(add, selector, args);
                }
                if (Il2CppObject* current = CallStatic(events, "get_current"))
                    Call(current, "SetSelectedGameObject", buttonObject);
                Call(selector, "SetCursorTargetWorldPositionDirect", buttonObject);
                FieldInfo* defaults = il2cpp_class_get_field_from_name(il2cpp_object_get_class(selector), "m_defaultEventSystemObject");
                Il2CppArray* list = nullptr;
                if (defaults)
                    il2cpp_field_get_value(selector, defaults, &list);
                if (list && index >= 0 && uint32_t(index) < il2cpp_array_length(list))
                    reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(list) + kIl2CppSizeOfArray)[index] = buttonObject;
            }
            catch (const Failure& f)
            {
                fprintf(stderr, "GoSRecomp: menu cursor not moved to Achievements (%s)\n", f.what.c_str());
            }
        }

        // The menu's own Achievements button: the "User Information" section
        // at the top (menu_parts1), which opened Game Center and is hidden
        // where there is none. Shown again, opening the achievements list
        // (ingame_menu.cpp) instead.
        void ShowAchievementsButton(Il2CppObject* menu)
        {
            if (!Truth(Call(Call(menu, "get_gameObject"), "get_activeInHierarchy")))
                return;
            Il2CppObject* root = Call(menu, "get_transform");
            Il2CppObject* content = Call(root, "Find", NewString("list/contents/Table View00/Scroll Content"));
            Il2CppObject* section = content ? Call(content, "Find", NewString("menu_parts1")) : nullptr;
            // Button02 until wired, renamed GosrAchievements after
            Il2CppObject* buttonT = section ? Call(section, "Find", NewString("btn_set/GosrAchievements")) : nullptr;
            if (!buttonT && section)
                buttonT = Call(section, "Find", NewString("btn_set/Button02"));
            if (!buttonT)
                return;
            // the section, its heading, the button row and the button: the
            // menu hides some of them again when it reopens
            Il2CppObject* buttonObject = Call(buttonT, "get_gameObject");
            const char* parts[] = { "", "headingtext", "line", "btn_set", "button" };
            std::string reshown;
            for (const char* part : parts)
            {
                Il2CppObject* t = !*part ? section : !strcmp(part, "button") ? buttonT : Call(section, "Find", NewString(part));
                if (!t)
                    continue;
                Il2CppObject* go = Call(t, "get_gameObject");
                if (Truth(Call(go, "get_activeSelf")))
                    continue;
                bool on = true;
                Call(go, "SetActive", &on);
                reshown += std::string(reshown.empty() ? "" : ", ") + (*part ? part : "menu_parts1");
            }
            if (!reshown.empty())
            {
                static int logged = 0;
                if (logged++ < 6)
                    fprintf(stderr, "GoSRecomp: the menu hid its Achievements section (%s); shown again\n", reshown.c_str());
                if (Il2CppClass* rebuilder = Class("UnityEngine.UI.dll", "UnityEngine.UI", "LayoutRebuilder"))
                    CallStatic(rebuilder, "ForceRebuildLayoutImmediate", content);
                // the menu scrolls to its first shown section as it opens:
                // back to the top, where this one is
                if (Il2CppObject* view = Call(root, "Find", NewString("list/contents/Table View00")))
                    if (Il2CppObject* scroll = GetComponent(Call(view, "get_gameObject"), Class("UnityEngine.UI.dll", "UnityEngine.UI", "ScrollRect")))
                    {
                        float top = 1.0f;
                        Call(scroll, "set_verticalNormalizedPosition", &top);
                    }
            }
            // remembered for MenuFrame, which checks the menu every frame
            Remember(menu, Call(section, "get_gameObject"), buttonObject, content, root);
            if (!reshown.empty())
                g_keepFocusUntil = GetTickCount64() + 2000;
            if (NameIs(buttonObject, "GosrAchievements"))  // wired already
            {
                if (!reshown.empty())  // the menu just opened (and hid the section)
                    FocusAchievements(menu, buttonObject);
                if (reshown.empty())
                    return;
            }
            Il2CppClass* buttonClass = Class("UnityEngine.UI.dll", "UnityEngine.UI", "Button");
            Il2CppObject* button = GetComponent(buttonObject, buttonClass);
            if (!button)
                throw Failure{ "the Achievements button has no Button" };
            Il2CppObject* click = Call(button, "get_onClick");
            Call(click, "RemoveAllListeners");  // the Game Center one
            const MethodInfo* add = Method(il2cpp_object_get_class(click), "AddListener", 1);
            Il2CppClass* actionClass = il2cpp_class_from_type(add->parameters[0].parameter_type);
            Call(click, "AddListener", MakeClickAction(menu, actionClass, OnAchievementsClicked));
            Call(buttonObject, "set_name", NewString("GosrAchievements"));

            // controller: up from the first Settings button reaches it
            if (Il2CppObject* screenT = Call(content, "Find", NewString("menu_parts2/btn_set/Button01")))
                if (Il2CppObject* screen = GetComponent(Call(screenT, "get_gameObject"), buttonClass))
                {
                    auto* screenNav = static_cast<Nav*>(il2cpp_object_unbox(Call(screen, "get_navigation")));
                    if (screenNav->mode == 4)
                    {
                        Nav below = *screenNav;
                        Nav mine = *static_cast<Nav*>(il2cpp_object_unbox(Call(button, "get_navigation")));
                        mine.mode = 4;
                        mine.down = screen;
                        mine.up = mine.left = mine.right = nullptr;
                        below.up = button;
                        Call(screen, "set_navigation", &below);
                        Call(button, "set_navigation", &mine);
                    }
                }
            if (Il2CppClass* rebuilder = Class("UnityEngine.UI.dll", "UnityEngine.UI", "LayoutRebuilder"))
                CallStatic(rebuilder, "ForceRebuildLayoutImmediate", content);
            FocusAchievements(menu, buttonObject);
            fprintf(stderr, "GoSRecomp: the menu's Achievements section is shown, opening the achievements list\n");
        }
    }

    void EnsureMenuQuitButton()
    {
        MenuFrame();
        static int frame = 0;
        if (++frame % 8 != 0)
            return;
        try
        {
            if (!il2cpp_domain_get() || !ImageNamed("Assembly-CSharp.dll"))
                return;
            Il2CppClass* menuClass = Class("Assembly-CSharp.dll", "", "outMenuTop");
            Il2CppClass* objectClass = Class("UnityEngine.CoreModule.dll", "UnityEngine", "Object");
            // the menus showing: from the game's screen cache (outUiManager.
            // ui_cache), which also has the menu when it is opened again,
            // and from FindObjectsOfType
            std::vector<Il2CppObject*> menus;
            try
            {
                Il2CppObject* manager = Singleton("outUiManager");
                FieldInfo* cacheField = il2cpp_class_get_field_from_name(il2cpp_object_get_class(manager), "ui_cache");
                Il2CppObject* dict = nullptr;
                if (cacheField)
                    il2cpp_field_get_value(manager, cacheField, &dict);
                if (dict)
                {
                    Il2CppClass* dk = il2cpp_object_get_class(dict);
                    Il2CppArray* entries = nullptr;
                    int32_t count = 0;
                    il2cpp_field_get_value(dict, il2cpp_class_get_field_from_name(dk, "entries"), &entries);
                    il2cpp_field_get_value(dict, il2cpp_class_get_field_from_name(dk, "count"), &count);
                    // Dictionary<string, ref>.Entry: hashCode, next, key, value
                    struct Entry { int32_t hashCode, next; Il2CppObject* key; Il2CppObject* value; };
                    if (entries && il2cpp_array_element_size(il2cpp_object_get_class(reinterpret_cast<Il2CppObject*>(entries))) == sizeof(Entry))
                    {
                        auto* e = reinterpret_cast<Entry*>(reinterpret_cast<uint8_t*>(entries) + kIl2CppSizeOfArray);
                        for (int32_t i = 0; i < count && i < int32_t(il2cpp_array_length(entries)); i++)
                            if (e[i].hashCode >= 0 && e[i].value && il2cpp_object_get_class(e[i].value) == menuClass)
                                menus.push_back(e[i].value);
                    }
                }
            }
            catch (const Failure&)
            {
            }
            void* args[] = { TypeObject(menuClass) };
            if (auto* found = reinterpret_cast<Il2CppArray*>(Invoke(Method(objectClass, "FindObjectsOfType", 1), nullptr, args)))
            {
                auto** items = reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(found) + kIl2CppSizeOfArray);
                for (size_t i = 0; i < il2cpp_array_length(found); i++)
                    if (items[i] && std::find(menus.begin(), menus.end(), items[i]) == menus.end())
                        menus.push_back(items[i]);
            }
            for (Il2CppObject* menu : menus)
            {
                AddQuitButton(menu);
                ShowAchievementsButton(menu);
            }
        }
        catch (const Failure& f)
        {
            static std::string last;
            if (last != f.what)
            {
                last = f.what;
                fprintf(stderr, "GoSRecomp: quit button was not added (%s)\n", f.what.c_str());
            }
        }
    }
}
