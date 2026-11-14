// Skip the Apple Arcade movie and the Konami logo, then leave the title menu up.
//
// Those two are konamiLogoMain, which finishes by asking bsSceneManager for
// the title scene. The fade-in and the logo shine are held at their finished
// pose for the whole time that title is up, so Continue is usable immediately.
// Continue is not pressed for you.
//
// gosr.ini:
//   [game]
//   skip_intro=1   ; 0 shows the logos and the title fade and logo shine
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "il2cpp-api.h"
#include "il2cpp-object-internals.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace recomp
{
    namespace
    {
        enum Scene : int32_t { kCopyright = 1, kTitle = 2 };

        struct Failure
        {
            std::string what;
        };

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

        Il2CppClass* AnyClass(const char* ns, const char* name)
        {
            size_t n = 0;
            const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &n);
            for (size_t i = 0; i < n; i++)
                if (Il2CppClass* k = il2cpp_class_from_name(il2cpp_assembly_get_image(assemblies[i]), ns, name))
                    return k;
            throw Failure{ std::string(name) + " was not found" };
        }

        const MethodInfo* Method(Il2CppClass* k, const char* name, int argc)
        {
            for (; k; k = il2cpp_class_get_parent(k))
            {
                il2cpp_runtime_class_init(k);
                if (const MethodInfo* m = il2cpp_class_get_method_from_name(k, name, argc))
                    return m;
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

        bool Truth(Il2CppObject* boxed)
        {
            return boxed && *static_cast<uint8_t*>(il2cpp_object_unbox(boxed));
        }

        template <typename T>
        T Get(Il2CppObject* obj, const char* name)
        {
            T value{};
            FieldInfo* f = nullptr;
            for (Il2CppClass* k = il2cpp_object_get_class(obj); k && !f; k = il2cpp_class_get_parent(k))
                f = il2cpp_class_get_field_from_name(k, name);
            if (!f)
                throw Failure{ std::string(name) + " was not found" };
            il2cpp_field_get_value(obj, f, &value);
            return value;
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

        std::vector<Il2CppObject*> OfType(Il2CppClass* kind)
        {
            Il2CppClass* object = AnyClass("UnityEngine", "Object");
            Il2CppObject* type = il2cpp_type_get_object(il2cpp_class_get_type(kind));
            void* args[] = { type };
            auto* found = reinterpret_cast<Il2CppArray*>(Invoke(Method(object, "FindObjectsOfType", 1), nullptr, args));
            std::vector<Il2CppObject*> out;
            if (!found)
                return out;
            auto** items = reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(found) + kIl2CppSizeOfArray);
            out.assign(items, items + il2cpp_array_length(found));
            return out;
        }

        bool Enabled()
        {
            static const int on = [] {
                wchar_t path[MAX_PATH * 4];
                DWORD len = GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
                std::wstring ini(path, len);
                ini = ini.substr(0, ini.find_last_of(L"\\/") + 1) + L"gosr.ini";
                // missing key skips; skip_intro=0 keeps the logos and the title menu
                return GetPrivateProfileIntW(L"game", L"skip_intro", 1, ini.c_str()) != 0;
            }();
            return on;
        }

        // The Apple Arcade movie and the Konami logo are both konamiLogoMain,
        // which then asks the scene manager for the title. Stop that as soon
        // as the logo object exists, and close the movie so it cannot keep playing.
        void CloseSplashVideo()
        {
            Il2CppObject* player = Singleton("gsVideoPlayer");
            Il2CppObject* root = Get<Il2CppObject*>(player, "root");
            if (root && Truth(Call(root, "get_activeSelf")))
                Call(player, "Close");
        }

        void SkipLogos()
        {
            static bool done = false;
            if (done)
                return;
            std::vector<Il2CppObject*> logos = OfType(Class("Assembly-CSharp.dll", "", "konamiLogoMain"));
            Il2CppObject* scenes = nullptr;
            try
            {
                scenes = Singleton("bsSceneManager");
            }
            catch (const Failure&)
            {
                return;
            }
            int32_t current = Get<int32_t>(scenes, "<Current>k__BackingField");
            int32_t next = Get<int32_t>(scenes, "<Next>k__BackingField");
            if (logos.empty())
            {
                if (current >= kTitle || next == kTitle)
                    done = true;
                return;
            }
            if (next == kTitle)
            {
                try
                {
                    CloseSplashVideo();
                }
                catch (const Failure&)
                {
                }
                for (Il2CppObject* logo : logos)
                    Call(logo, "StopAllCoroutines");
                done = true;
                return;
            }
            if (!Get<bool>(scenes, "_isSceneActive") && current != kCopyright && current != 0)
                return;

            // Leave the logos running until the fade manager can take us to the title.
            bool fadeIn = false;
            Call(Singleton("bsFadeManager"), "Immediately", &fadeIn);
            try
            {
                CloseSplashVideo();
            }
            catch (const Failure&)
            {
            }
            for (Il2CppObject* logo : logos)
                Call(logo, "StopAllCoroutines");
            try
            {
                bool show = true;
                Call(Singleton("TouchEffectManager"), "SetShow", &show);
            }
            catch (const Failure&)
            {
            }
            int32_t title = kTitle;
            bool wait = false;
            Call(scenes, "Request", &title, &wait);
            done = true;
            fprintf(stderr, "GoSRecomp: skipped the startup logos\n");
        }

        void AllowGroup(Il2CppObject* group)
        {
            if (!group)
                return;
            bool on = true;
            Call(group, "set_interactable", &on);
            Call(group, "set_blocksRaycasts", &on);
        }

        void UnlockBranch(Il2CppObject* component, Il2CppClass* canvasGroup)
        {
            if (!component)
                return;
            Il2CppObject* type = il2cpp_type_get_object(il2cpp_class_get_type(canvasGroup));
            Il2CppObject* go = Call(component, "get_gameObject");
            bool active = true;
            Call(go, "SetActive", &active);
            for (int i = 0; i < 8 && go; i++)
            {
                AllowGroup(Call(go, "GetComponent", type));
                Il2CppObject* parent = Call(Call(go, "get_transform"), "get_parent");
                go = parent ? Call(parent, "get_gameObject") : nullptr;
            }
        }

        // The title sits on screen for a long time before PlayAnim runs the fade
        // and the logo shine. A short timer expires during that wait, so keep
        // finishing those effects until this title instance is actually left.
        void EndTween(Il2CppObject* tween)
        {
            if (!tween)
                return;
            bool end = true;
            bool finishCallback = false;
            Call(tween, "Immediate", &end, &finishCallback);
        }

        void FinishPlayingTweens(Il2CppObject* list)
        {
            if (!list)
                return;
            int32_t count = 0;
            if (Il2CppObject* boxed = Call(list, "get_Count"))
                count = *static_cast<int32_t*>(il2cpp_object_unbox(boxed));
            for (int32_t i = 0; i < count; i++)
            {
                Il2CppObject* tween = Call(list, "get_Item", &i);
                if (tween && Truth(Call(tween, "IsPlay")))
                    EndTween(tween);
            }
        }

        void HoldAnimators(Il2CppObject* title, bool enabled)
        {
            auto* ators = reinterpret_cast<Il2CppArray*>(Get<Il2CppObject*>(title, "m_ators"));
            if (!ators)
                return;
            auto** items = reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(ators) + kIl2CppSizeOfArray);
            for (uintptr_t i = 0; i < il2cpp_array_length(ators); i++)
            {
                if (!items[i])
                    continue;
                Call(items[i], "set_enabled", &enabled);
            }
        }

        void ShowMenu(Il2CppObject* title)
        {
            if (!Enabled() || !title)
                return;
            // One settled title. A later visit (return to title) is a new object.
            static Il2CppObject* settled = nullptr;
            if (title == settled)
                return;
            if (Get<bool>(title, "m_isStart") || Get<bool>(title, "<isCloseInProgress>k__BackingField"))
            {
                try
                {
                    bool on = true;
                    HoldAnimators(title, on);
                }
                catch (const Failure&)
                {
                }
                settled = title;
                return;
            }

            for (const char* name : { "m_bgScaleAnim", "m_leftScaleAnim", "m_jpLogoFlashAnim", "m_enLogoFlashAnim" })
            {
                try
                {
                    EndTween(Get<Il2CppObject*>(title, name));
                }
                catch (const Failure&)
                {
                }
            }
            try
            {
                FinishPlayingTweens(Get<Il2CppObject*>(title, "m_tweens"));
            }
            catch (const Failure&)
            {
            }
            try
            {
                bool off = false;
                HoldAnimators(title, off);
            }
            catch (const Failure&)
            {
            }

            try
            {
                Il2CppObject* fade = Singleton("bsFadeManager");
                if (!Truth(Call(fade, "IsFinish")) && Truth(Call(fade, "IsFadeIn")))
                {
                    bool show = true;
                    Call(fade, "Immediately", &show);
                }
            }
            catch (const Failure&)
            {
            }

            try
            {
                if (Il2CppObject* group = Get<Il2CppObject*>(title, "m_topCanvasGroup"))
                {
                    float full = 1.f;
                    Call(group, "set_alpha", &full);
                    AllowGroup(group);
                }
            }
            catch (const Failure&)
            {
            }

            bool on = true;
            Il2CppClass* canvasGroup = AnyClass("UnityEngine", "CanvasGroup");
            for (const char* name : { "m_BtnContinue", "m_BtnLoad", "m_BtnQuit" })
            {
                try
                {
                    Il2CppObject* button = Get<Il2CppObject*>(title, name);
                    if (!button)
                        continue;
                    Call(button, "set_interactable", &on);
                    UnlockBranch(button, canvasGroup);
                    float full = 1.f;
                    Il2CppObject* type = il2cpp_type_get_object(il2cpp_class_get_type(canvasGroup));
                    Il2CppObject* go = Call(button, "get_gameObject");
                    for (int i = 0; i < 8 && go; i++)
                    {
                        if (Il2CppObject* group = Call(go, "GetComponent", type))
                            Call(group, "set_alpha", &full);
                        Il2CppObject* parent = Call(Call(go, "get_transform"), "get_parent");
                        go = parent ? Call(parent, "get_gameObject") : nullptr;
                    }
                }
                catch (const Failure&)
                {
                }
            }

            static bool logged = false;
            if (!logged && Get<Il2CppObject*>(title, "m_BtnContinue"))
            {
                logged = true;
                fprintf(stderr, "GoSRecomp: title menu shown ahead of the fade\n");
            }
        }

        void ShowOpenTitleMenus()
        {
            if (!Enabled())
                return;
            try
            {
                for (Il2CppObject* title : OfType(Class("Assembly-CSharp.dll", "", "ttlUiEmpty")))
                    ShowMenu(title);
            }
            catch (const Failure&)
            {
            }
        }

        using UpdateFn = void (*)(void*, const void*);
        // methodPointers slot of ttlUiEmpty.Update (0x1BE269C)
        constexpr uint64_t kTitleUpdateSlot = 0x3e51488;
        UpdateFn g_origTitleUpdate = nullptr;

        void HookedTitleUpdate(void* self, const void* method)
        {
            g_origTitleUpdate(self, method);
            try
            {
                ShowMenu(reinterpret_cast<Il2CppObject*>(self));
            }
            catch (const Failure&)
            {
            }
        }
    }

    void SkipIntro()
    {
        if (!Enabled())
            return;
        try
        {
            SkipLogos();
            ShowOpenTitleMenus();
        }
        catch (const Failure&)
        {
        }
    }

    void InstallSkipIntro(uint64_t imageBase)
    {
        auto* slot = reinterpret_cast<UpdateFn*>(imageBase + kTitleUpdateSlot);
        g_origTitleUpdate = *slot;
        *slot = HookedTitleUpdate;
    }
}
