// Quit button on the in-game menu (outMenuTop), beside Return to Title.
//
// The title screen's QUIT GAME button asks with cmnPopupManager.Selection2Buttons
// (string ids 8, 9, 10 and 11) and, on yes, calls Application.Quit. This button
// saves first, then opens that same dialog with the title screen's own yes/no
// callback, so confirming leaves the game the same way.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "il2cpp-api.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace recomp
{
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

        Il2CppObject* MakeClickAction(Il2CppObject* target, Il2CppClass* actionClass)
        {
            static MethodInfo click{};
            static bool ready = false;
            if (!ready)
            {
                const MethodInfo* sample = Method(Class("Assembly-CSharp.dll", "", "outMenuTop"), "<OnCreateTitleButton>b__47_0", 0);
                click = *sample;
                click.methodPointer = reinterpret_cast<Il2CppMethodPointer>(OnQuitClicked);
                ready = true;
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
    }

    void EnsureMenuQuitButton()
    {
        static int frame = 0;
        if (++frame % 8 != 0)
            return;
        try
        {
            if (!il2cpp_domain_get() || !ImageNamed("Assembly-CSharp.dll"))
                return;
            Il2CppClass* menuClass = Class("Assembly-CSharp.dll", "", "outMenuTop");
            Il2CppClass* objectClass = Class("UnityEngine.CoreModule.dll", "UnityEngine", "Object");
            void* args[] = { TypeObject(menuClass) };
            auto* found = reinterpret_cast<Il2CppArray*>(Invoke(Method(objectClass, "FindObjectsOfType", 1), nullptr, args));
            if (!found)
                return;
            auto** items = reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(found) + kIl2CppSizeOfArray);
            for (size_t i = 0; i < il2cpp_array_length(found); i++)
                if (items[i])
                    AddQuitButton(items[i]);
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
