// The debug menu inside the game.
//
// Built from the game's own dialog (cmnPopupManager.m_dialog_prefab,
// "dialog_00"): its frame, title bar, text box, font and buttons, on a Canvas
// of its own above everything else. The actions are debug_menu.cpp's.
//
// Opened with the hotkey (F8, [debug] hotkey=) or by clicking both sticks
// (L3 + R3). While it is open the game is paused (Time.timeScale 0) and the
// menu reads the controller from gsJoystick, then clears gsJoystick's button
// flags so the screen underneath does not react to them. The game's UI
// navigation (EventSystem.sendNavigationEvents) is off meanwhile, and a dim
// layer keeps mouse clicks off the game's screens.
//
// It opens on a main page listing the sections (or, once, the warning).
//   Up/Down/Left/Right  move, down to the bottom buttons too
//                       (Left/Right change a value: amount, item)
//   A                   use / open a section
//   B                   back to the main page; on the main page, close
//   LB / RB             previous / next section
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "il2cpp-api.h"
#include "il2cpp-class-internals.h"
#include "il2cpp-object-internals.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace recomp
{
    // debug_menu.cpp
    bool DebugMenuEnabled();
    int DebugMenuHotkey();
    bool DebugMenuWarned();
    void SetDebugMenuWarned();
    size_t DebugActionCount();
    std::string DebugActionGroup(size_t i);
    std::string DebugActionLabel(size_t i);
    bool DebugActionChangesSave(size_t i);
    std::string RunDebugAction(size_t i);
    int DebugAmount();
    void SetDebugAmount(int amount);
    size_t DebugItemCount();
    std::string DebugItemLabel(size_t i);
    std::string GiveDebugItem(size_t i);
    bool DebugItemIsSingle(size_t i);
    bool SaveGameNow();

    namespace
    {
        struct Failure
        {
            std::string what;
        };

        struct Vec2 { float x, y; };
        struct Vec3 { float x, y, z; };
        struct Color { float r, g, b, a; };

        // ------------------------------------------------------------ il2cpp

        Il2CppClass* AnyClass(const char* ns, const char* name)
        {
            static std::map<std::string, Il2CppClass*> cache;
            std::string key = std::string(ns) + "." + name;
            auto it = cache.find(key);
            if (it != cache.end())
                return it->second;
            size_t n = 0;
            const Il2CppAssembly** assemblies = il2cpp_domain_get_assemblies(il2cpp_domain_get(), &n);
            for (size_t i = 0; i < n; i++)
                if (Il2CppClass* k = il2cpp_class_from_name(il2cpp_assembly_get_image(assemblies[i]), ns, name))
                {
                    il2cpp_runtime_class_init(k);
                    return cache[key] = k;
                }
            throw Failure{ key + " was not found" };
        }

        Il2CppClass* GameClass(const char* name) { return AnyClass("", name); }

        const MethodInfo* Method(Il2CppClass* k, const char* name, int argc, const char* argType = nullptr, int argIndex = 0)
        {
            for (; k; k = il2cpp_class_get_parent(k))
            {
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
            if (!obj)
                throw Failure{ std::string(name) + " on null" };
            void* args[sizeof...(A) + 1] = { static_cast<void*>(a)... };
            return Invoke(Method(il2cpp_object_get_class(obj), name, int(sizeof...(A))), obj, args);
        }

        template <typename... A>
        Il2CppObject* CallStatic(Il2CppClass* k, const char* name, A... a)
        {
            void* args[sizeof...(A) + 1] = { static_cast<void*>(a)... };
            return Invoke(Method(k, name, int(sizeof...(A))), nullptr, args);
        }

        bool Truth(Il2CppObject* boxed) { return boxed && *static_cast<uint8_t*>(il2cpp_object_unbox(boxed)); }
        float Float(Il2CppObject* boxed) { return boxed ? *static_cast<float*>(il2cpp_object_unbox(boxed)) : 0.0f; }

        Il2CppObject* Singleton(const char* name)
        {
            Il2CppClass* k = GameClass(name);
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

        FieldInfo* Field(Il2CppClass* k, const char* name)
        {
            for (; k; k = il2cpp_class_get_parent(k))
                if (FieldInfo* f = il2cpp_class_get_field_from_name(k, name))
                    return f;
            throw Failure{ std::string("field ") + name + " was not found" };
        }

        Il2CppObject* TypeObject(Il2CppClass* k) { return il2cpp_type_get_object(il2cpp_class_get_type(k)); }
        Il2CppString* Str(const std::string& utf8) { return il2cpp_string_new(utf8.c_str()); }

        // A Unity object that has not been destroyed.
        bool Alive(Il2CppObject* obj)
        {
            if (!obj)
                return false;
            static const MethodInfo* implicit = Method(AnyClass("UnityEngine", "Object"), "op_Implicit", 1);
            void* args[] = { obj };
            return Truth(Invoke(implicit, nullptr, args));
        }

        // ---------------------------------------------------------- unity UI

        Il2CppObject* AddComponent(Il2CppObject* go, Il2CppClass* k) { return Call(go, "AddComponent", TypeObject(k)); }

        Il2CppObject* GetComponent(Il2CppObject* go, Il2CppClass* k)
        {
            static const MethodInfo* get = Method(AnyClass("UnityEngine", "GameObject"), "GetComponent", 1, "System.Type");
            void* args[] = { TypeObject(k) };
            return Invoke(get, go, args);
        }

        Il2CppObject* TransformOf(Il2CppObject* go) { return Call(go, "get_transform"); }
        Il2CppObject* GameObjectOf(Il2CppObject* component) { return Call(component, "get_gameObject"); }

        Il2CppObject* Find(Il2CppObject* transform, const char* path)
        {
            Il2CppObject* t = Call(transform, "Find", Str(path));
            if (!t)
                throw Failure{ std::string("no ") + path + " in the dialog" };
            return t;
        }

        void SetActive(Il2CppObject* go, bool on) { Call(go, "SetActive", &on); }

        Il2CppObject* Instantiate(Il2CppObject* original, Il2CppObject* parent)
        {
            static const MethodInfo* inst = Method(AnyClass("UnityEngine", "Object"), "Instantiate", 3, "Transform", 1);
            bool world = false;
            void* args[] = { original, parent, &world };
            Il2CppObject* copy = Invoke(inst, nullptr, args);
            if (!copy)
                throw Failure{ "a copy could not be made" };
            return copy;
        }

        Il2CppObject* NewObject(const char* name, Il2CppObject* parentTransform)
        {
            Il2CppClass* goClass = AnyClass("UnityEngine", "GameObject");
            Il2CppObject* go = il2cpp_object_new(goClass);
            Il2CppString* s = Str(name);
            void* args[] = { s };
            Invoke(Method(goClass, ".ctor", 1, "System.String"), go, args);
            AddComponent(go, AnyClass("UnityEngine", "RectTransform"));
            if (parentTransform)
            {
                bool world = false;
                Call(TransformOf(go), "SetParent", parentTransform, &world);
            }
            return go;
        }

        void Place(Il2CppObject* rectTransform, Vec2 anchorMin, Vec2 anchorMax, Vec2 pivot, Vec2 pos, Vec2 size)
        {
            Call(rectTransform, "set_anchorMin", &anchorMin);
            Call(rectTransform, "set_anchorMax", &anchorMax);
            Call(rectTransform, "set_pivot", &pivot);
            Call(rectTransform, "set_sizeDelta", &size);
            Call(rectTransform, "set_anchoredPosition", &pos);
        }

        // Destroys the components with these class names in the whole tree
        // (the game scripts that would set texts, tween or localize it).
        void Strip(Il2CppObject* transform, const std::vector<const char*>& names)
        {
            static Il2CppObject* componentType = TypeObject(AnyClass("UnityEngine", "Component"));
            Il2CppObject* go = GameObjectOf(transform);
            auto* list = reinterpret_cast<Il2CppArray*>(Call(go, "GetComponents", componentType));
            auto** items = list ? reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(list) + kIl2CppSizeOfArray) : nullptr;
            for (size_t i = 0; list && i < il2cpp_array_length(list); i++)
            {
                if (!items[i])
                    continue;
                const char* cls = il2cpp_class_get_name(il2cpp_object_get_class(items[i]));
                for (const char* n : names)
                    if (!strcmp(cls, n))
                    {
                        CallStatic(AnyClass("UnityEngine", "Object"), "DestroyImmediate", items[i]);
                        break;
                    }
            }
            int32_t count = *static_cast<int32_t*>(il2cpp_object_unbox(Call(transform, "get_childCount")));
            for (int32_t i = 0; i < count; i++)
                Strip(Call(transform, "GetChild", &i), names);
        }

        Il2CppObject* TextIn(Il2CppObject* go)
        {
            // the (Type) overload is stripped from this build; (Type, includeInactive) is there
            static const MethodInfo* get = Method(AnyClass("UnityEngine", "GameObject"), "GetComponentInChildren", 2, "System.Type");
            bool inactiveToo = true;
            void* args[] = { TypeObject(AnyClass("UnityEngine.UI", "Text")), &inactiveToo };
            Il2CppObject* text = Invoke(get, go, args);
            if (!text)
                throw Failure{ "a text was not found" };
            return text;
        }

        void FitText(Il2CppObject* text)
        {
            int32_t size = *static_cast<int32_t*>(il2cpp_object_unbox(Call(text, "get_fontSize")));
            bool fit = true;
            int32_t smallest = std::max(10, size / 2);
            Call(text, "set_resizeTextForBestFit", &fit);
            Call(text, "set_resizeTextMinSize", &smallest);
            Call(text, "set_resizeTextMaxSize", &size);
        }

        void Sound(const char* name)
        {
            try
            {
                Call(Singleton("outSoundManager"), name);
            }
            catch (const Failure&)
            {
            }
        }

        std::string Grouped(long long v)
        {
            std::string s = std::to_string(v), out;
            for (size_t i = 0; i < s.size(); i++)
            {
                if (i && (s.size() - i) % 3 == 0)
                    out += ',';
                out += s[i];
            }
            return out;
        }

        // ------------------------------------------------------- save backups
        //
        // A backup is a copy of the files in the save folder
        // (Application.persistentDataPath) in a dated folder beside it:
        //   <save folder> - GoSRecomp backups\2026-11-14 22-05-31\
        // Restoring copies them back (all but option.json, the game's
        // settings) and ends the game at once: it keeps the save in memory
        // and would write it back on a normal exit.

        namespace fs = std::filesystem;

        fs::path SaveFolder()
        {
            auto* s = reinterpret_cast<Il2CppString*>(CallStatic(AnyClass("UnityEngine", "Application"), "get_persistentDataPath"));
            if (!s || !il2cpp_string_length(s))
                throw Failure{ "the save folder is not known yet" };
            std::wstring w(reinterpret_cast<const wchar_t*>(il2cpp_string_chars(s)), size_t(il2cpp_string_length(s)));
            return fs::path(w).make_preferred();
        }

        fs::path BackupRoot()
        {
            fs::path save = SaveFolder();
            return save.parent_path() / (save.filename().wstring() + L" - GoSRecomp backups");
        }

        // Newest first.
        std::vector<fs::path> Backups()
        {
            std::vector<fs::path> list;
            std::error_code ec;
            for (const auto& e : fs::directory_iterator(BackupRoot(), ec))
                if (e.is_directory())
                    list.push_back(e.path());
            std::sort(list.begin(), list.end(), [](const fs::path& a, const fs::path& b) { return a.filename() > b.filename(); });
            return list;
        }

        std::string Narrow(const std::wstring& w)
        {
            std::string s(w.size() * 3, '\0');
            s.resize(size_t(WideCharToMultiByte(CP_UTF8, 0, w.c_str(), int(w.size()), s.data(), int(s.size()), nullptr, nullptr)));
            return s;
        }

        std::string BackupName(const fs::path& p) { return Narrow(p.filename().wstring()); }

        // The files of the save folder (not its download cache) into a new
        // dated folder. Returns that folder.
        fs::path MakeBackup(const wchar_t* note)
        {
            SaveGameNow();
            fs::path from = SaveFolder();
            std::time_t now = std::time(nullptr);
            std::tm local{};
            localtime_s(&local, &now);
            wchar_t stamp[64];
            wcsftime(stamp, 64, L"%Y-%m-%d %H-%M-%S", &local);
            fs::path to = BackupRoot() / (std::wstring(stamp) + note);
            fs::create_directories(to);
            int copied = 0;
            for (const auto& e : fs::directory_iterator(from))
                if (e.is_regular_file())
                {
                    fs::copy_file(e.path(), to / e.path().filename(), fs::copy_options::overwrite_existing);
                    copied++;
                }
            if (!copied)
                throw Failure{ "the save folder has no files" };
            fprintf(stderr, "GoSRecomp: save backed up to %s (%d files)\n", Narrow(to.wstring()).c_str(), copied);
            return to;
        }

        std::string BackUpNow()
        {
            try
            {
                return "Backed up: " + BackupName(MakeBackup(L""));
            }
            catch (const std::exception& e)
            {
                return std::string("Backup failed: ") + e.what();
            }
            catch (const Failure& f)
            {
                return "Backup failed: " + f.what;
            }
        }

        // Does not return when it works: the game ends.
        std::string Restore(const fs::path& backup)
        {
            try
            {
                MakeBackup(L" before restore");
                fs::path to = SaveFolder();
                int copied = 0;
                for (const auto& e : fs::directory_iterator(backup))
                    if (e.is_regular_file() && e.path().filename() != L"option.json")
                    {
                        fs::copy_file(e.path(), to / e.path().filename(), fs::copy_options::overwrite_existing);
                        copied++;
                    }
                if (!copied)
                    return "That backup has no files";
                // start the game again (same program and arguments as now,
                // from its folder, as Play.bat does), then end this one
                wchar_t exe[MAX_PATH * 4];
                DWORD len = GetModuleFileNameW(nullptr, exe, DWORD(std::size(exe)));
                std::wstring folder = fs::path(std::wstring(exe, len)).parent_path().wstring();
                std::wstring cmd = GetCommandLineW();
                STARTUPINFOW si{ sizeof(si) };
                PROCESS_INFORMATION pi{};
                bool started = CreateProcessW(exe, cmd.data(), nullptr, nullptr, FALSE, 0, nullptr, folder.c_str(), &si, &pi) != 0;
                if (started)
                {
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
                fprintf(stderr, "GoSRecomp: save restored from %s (%d files); %s\n", BackupName(backup).c_str(), copied,
                        started ? "restarting the game" : "closing the game (start it again)");
                fflush(stderr);
                TerminateProcess(GetCurrentProcess(), 0);
            }
            catch (const std::exception& e)
            {
                return std::string("Restore failed: ") + e.what();
            }
            catch (const Failure& f)
            {
                return "Restore failed: " + f.what;
            }
            return "";
        }

        // -------------------------------------------------------------- model

        enum class RowKind { Action, Amount, Category, Item, Give, Accept, Close, PageLink, Yes, No, Backup, Restore, Auto };

        // ------------------------------------------------------- auto input
        //
        // Holding attack, jump or sub-weapon in battle repeats the press: the
        // button's gsJoystick flag is cleared for part of each cycle, so the
        // game sees it let go and pressed again. Kept in gosr.ini [controller]:
        //   auto_attack=1  auto_jump=1  auto_subweapon=1  auto_speed=6|10|15

        struct AutoInput
        {
            const char* key;      // gosr.ini name
            const char* label;
            int keyType;          // mvUiManager.KeyType
            bool on = false;
        };
        AutoInput g_auto[] = { { "auto_attack", "Auto attack", 0 }, { "auto_jump", "Auto jump", 1 }, { "auto_subweapon", "Auto sub-weapon", 6 } };
        constexpr size_t kAutoSpeedRow = std::size(g_auto);  // the row after the buttons
        const int kAutoSpeeds[] = { 6, 10, 15 };              // presses per second
        int g_autoSpeed = 10;

        std::wstring IniPath()
        {
            wchar_t path[MAX_PATH * 4];
            DWORD len = GetModuleFileNameW(nullptr, path, DWORD(std::size(path)));
            std::wstring p(path, len);
            return p.substr(0, p.find_last_of(L"\\/") + 1) + L"gosr.ini";
        }

        void LoadAutoInput()
        {
            static bool loaded = false;
            if (loaded)
                return;
            loaded = true;
            std::wstring ini = IniPath();
            for (auto& a : g_auto)
            {
                std::wstring key(a.key, a.key + strlen(a.key));
                a.on = GetPrivateProfileIntW(L"controller", key.c_str(), 0, ini.c_str()) != 0;
            }
            int speed = GetPrivateProfileIntW(L"controller", L"auto_speed", 10, ini.c_str());
            g_autoSpeed = speed <= 7 ? 6 : speed >= 13 ? 15 : 10;
        }

        void SaveAutoInput()
        {
            std::wstring ini = IniPath();
            for (const auto& a : g_auto)
            {
                std::wstring key(a.key, a.key + strlen(a.key));
                WritePrivateProfileStringW(L"controller", key.c_str(), a.on ? L"1" : L"0", ini.c_str());
            }
            WritePrivateProfileStringW(L"controller", L"auto_speed", std::to_wstring(g_autoSpeed).c_str(), ini.c_str());
        }

        std::string AutoLabel(size_t row)
        {
            if (row == kAutoSpeedRow)
                return std::string("Speed: ") + (g_autoSpeed == 6 ? "slow" : g_autoSpeed == 10 ? "normal" : "fast") + " (" +
                       std::to_string(g_autoSpeed) + " a second)";
            return std::string(g_auto[row].label) + (g_auto[row].on ? ": ON" : ": off");
        }

        void ToggleAuto(size_t row)
        {
            if (row == kAutoSpeedRow)
            {
                size_t i = 0;
                while (i < std::size(kAutoSpeeds) && kAutoSpeeds[i] != g_autoSpeed)
                    i++;
                g_autoSpeed = kAutoSpeeds[(i + 1) % std::size(kAutoSpeeds)];
            }
            else
                g_auto[row].on = !g_auto[row].on;
            SaveAutoInput();
        }

        bool IsValue(RowKind k) { return k == RowKind::Amount || k == RowKind::Category || k == RowKind::Item; }
        // ---- the item list by category ("Weapon: Name" -> Weapon), in list order

        struct Category
        {
            std::string name;
            size_t first, end;  // items [first, end)
        };

        const std::vector<Category>& Categories()
        {
            static std::vector<Category> cats;
            static size_t counted = 0;
            size_t n = DebugItemCount();
            if (n == counted)
                return cats;
            cats.clear();
            for (size_t i = 0; i < n; i++)
            {
                std::string label = DebugItemLabel(i);
                std::string kind = label.substr(0, label.find(": "));
                if (cats.empty() || cats.back().name != kind)
                    cats.push_back({ kind, i, i + 1 });
                else
                    cats.back().end = i + 1;
            }
            counted = n;
            return cats;
        }

        size_t CategoryOf(size_t item)
        {
            const auto& cats = Categories();
            for (size_t c = 0; c < cats.size(); c++)
                if (item < cats[c].end)
                    return c;
            return 0;
        }

        // "Weapon: Name" -> "Name"
        std::string ItemName(size_t item)
        {
            std::string label = DebugItemLabel(item);
            size_t colon = label.find(": ");
            return colon == std::string::npos ? label : label.substr(colon + 2);
        }
        struct Row
        {
            RowKind kind;
            size_t action = 0;
        };
        struct Page
        {
            std::string title;
            std::vector<Row> rows;
            bool backups = false;  // rows made from the backups on disk
        };

        constexpr int kColumns = 2, kMaxRows = 10;
        const int kAmounts[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 999999999 };

        // The sections; the main page (Hub) lists them.
        std::vector<Page> BuildPages()
        {
            std::vector<Page> pages;
            for (size_t i = 0; i < DebugActionCount(); i++)
            {
                std::string group = DebugActionGroup(i);
                if (!group.empty() || pages.empty())
                {
                    pages.push_back({ group.empty() ? "Actions" : group, {} });
                    if (group == "Resources")
                        pages.back().rows.push_back({ RowKind::Amount });
                }
                if (pages.back().rows.size() == kMaxRows)  // a full page goes on in the next one
                    pages.push_back({ pages.back().title + " (more)", {} });
                pages.back().rows.push_back({ RowKind::Action, i });
            }
            size_t after = 0;  // giving one item goes right after the resources
            while (after < pages.size() && pages[after].title.rfind("Resources", 0) == 0)
                after++;
            pages.insert(pages.begin() + after, Page{ "Give item", { { RowKind::Category }, { RowKind::Item }, { RowKind::Amount }, { RowKind::Give } } });
            pages.insert(pages.begin(), Page{ "Save backup", {}, true });
            LoadAutoInput();
            Page autoPage{ "Auto input", {} };
            for (size_t i = 0; i <= kAutoSpeedRow; i++)
                autoPage.rows.push_back({ RowKind::Auto, i });
            pages.push_back(autoPage);
            return pages;
        }

        // -------------------------------------------------------------- state

        struct Menu
        {
            uint32_t root = 0, title = 0, status = 0;
            uint32_t bar[3] = {};        // previous page, back/close, next page
            uint32_t barImages[3] = {};
            uint32_t barLabels[3] = {};
            uint32_t rows[kMaxRows] = {};
            uint32_t rowImages[kMaxRows] = {};
            Color rowColor{ 1, 1, 1, 1 }, barColor[3] = {};
            std::vector<Page> pages;
            bool hub = true;             // on the main page, which lists the pages
            bool confirming = false;     // asking before a change to the save
            bool editing = false;        // left/right change the selected value (Amount, item)
            Row pending{ RowKind::Action };
            size_t pendingSel = 0;
            size_t page = 0, sel = 0, item = 0;
            bool open = false, swallow = false;
            float timeScale = 1.0f;
            std::string message;
            std::vector<fs::path> backups;  // newest first, as listed on the Save backup page
        };
        Menu g;

        // The rows of what is showing: the main page lists the sections (or
        // the one-time warning), a section its actions.
        std::vector<Row> CurrentRows()
        {
            if (g.confirming)
                return { { RowKind::Yes }, { RowKind::No } };
            if (!g.hub && g.pages[g.page].backups)
            {
                std::vector<Row> rows{ { RowKind::Backup } };
                for (size_t i = 0; i < g.backups.size() && rows.size() < kMaxRows; i++)
                    rows.push_back({ RowKind::Restore, i });
                return rows;
            }
            if (!g.hub)
                return g.pages[g.page].rows;
            if (!DebugMenuWarned())
                return { { RowKind::Accept }, { RowKind::Close } };
            std::vector<Row> rows;
            for (size_t i = 0; i < g.pages.size() && rows.size() < kMaxRows; i++)
                rows.push_back({ RowKind::PageLink, i });
            return rows;
        }

        // On Give item, a thing given one at a time (character, costume,
        // equipment, accessory, music) has no amount: that row is greyed out.
        bool AmountLocked(RowKind k)
        {
            size_t n = DebugItemCount();
            return k == RowKind::Amount && !g.hub && !g.confirming && g.pages[g.page].title == "Give item" && n &&
                   DebugItemIsSingle(g.item % n);
        }

        // The bottom buttons the cursor can reach: on the main page only the
        // middle one (Close), in a section all three (< LB, Back, RB >).
        std::vector<int> BarItems()
        {
            if (g.confirming)
                return {};
            if (g.hub || g.pages.size() < 2)
                return { 1 };
            return { 0, 1, 2 };
        }

        Il2CppObject* Target(uint32_t handle) { return handle ? il2cpp_gchandle_get_target(handle) : nullptr; }
        uint32_t Keep(Il2CppObject* obj) { return il2cpp_gchandle_new(obj, false); }

        void ReleaseAll()
        {
            for (uint32_t* h : { &g.root, &g.title, &g.status })
                if (*h)
                    il2cpp_gchandle_free(*h), *h = 0;
            for (int i = 0; i < 3; i++)
                for (uint32_t* h : { &g.bar[i], &g.barImages[i], &g.barLabels[i] })
                    if (*h)
                        il2cpp_gchandle_free(*h), *h = 0;
            for (int i = 0; i < kMaxRows; i++)
            {
                if (g.rows[i])
                    il2cpp_gchandle_free(g.rows[i]), g.rows[i] = 0;
                if (g.rowImages[i])
                    il2cpp_gchandle_free(g.rowImages[i]), g.rowImages[i] = 0;
            }
        }

        void Activate(int row);
        void ActivateBar(int which);
        void ChangeValue(const Row& r, int delta);
        void Refresh();
        void TurnPage(int delta);
        void Close();

        // Button clicks (mouse): the delegate's target is the clicked Button.
        uint64_t OnClick(uint64_t target, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t)
        {
            try
            {
                auto* clicked = reinterpret_cast<Il2CppObject*>(target);
                if (!g.open || !clicked)
                    return 0;
                for (int i = 0; i < 3; i++)
                    if (clicked == Target(g.bar[i]))
                    {
                        ActivateBar(i);
                        return 0;
                    }
                std::vector<Row> rows = CurrentRows();
                for (int i = 0; i < kMaxRows; i++)
                    if (clicked == Target(g.rows[i]))
                    {
                        g.sel = size_t(i);
                        RowKind kind = size_t(i) < rows.size() ? rows[size_t(i)].kind : RowKind::Close;
                        if (AmountLocked(kind))
                            Sound("NG");
                        else if (IsValue(kind))
                        {
                            ChangeValue(rows[size_t(i)], 1);  // a click steps the value up
                            Refresh();
                        }
                        else
                            Activate(i);
                    }
            }
            catch (const Failure& f)
            {
                fprintf(stderr, "GoSRecomp: debug menu click: %s\n", f.what.c_str());
            }
            return 0;
        }

        Il2CppObject* ClickAction(Il2CppObject* target, Il2CppClass* actionClass)
        {
            static MethodInfo click{};
            static bool ready = false;
            if (!ready)
            {
                // any parameterless instance method serves as the template
                click = *Method(GameClass("outMenuTop"), "<OnCreateTitleButton>b__47_0", 0);
                click.methodPointer = reinterpret_cast<Il2CppMethodPointer>(OnClick);
                ready = true;
            }
            Il2CppObject* action = il2cpp_object_new(actionClass);
            intptr_t methodBits = reinterpret_cast<intptr_t>(&click);
            void* args[] = { target, &methodBits };
            Invoke(Method(actionClass, ".ctor", 2), action, args);
            return action;
        }

        // A game button that calls OnClick, with its old listeners removed.
        Il2CppObject* Wire(Il2CppObject* go)
        {
            Il2CppObject* button = GetComponent(go, AnyClass("UnityEngine.UI", "Button"));
            if (!button)
                throw Failure{ "a dialog button has no Button" };
            Il2CppObject* clicked = Call(button, "get_onClick");
            Call(clicked, "RemoveAllListeners");
            const MethodInfo* add = Method(il2cpp_object_get_class(clicked), "AddListener", 1);
            Il2CppClass* actionClass = il2cpp_class_from_type(add->parameters[0].parameter_type);
            Call(clicked, "AddListener", ClickAction(button, actionClass));
            return button;
        }

        void SetLabel(Il2CppObject* go, const std::string& text) { Call(TextIn(go), "set_text", Str(text)); }

        void Build()
        {
            Il2CppObject* prefab = Call(Singleton("cmnPopupManager"), "get_m_dialog_prefab");
            if (!prefab)
                throw Failure{ "the game's dialog is not loaded yet" };

            Il2CppObject* root = NewObject("GosrDebugMenu", nullptr);
            SetActive(root, false);  // nothing below starts its scripts until it is shown
            CallStatic(AnyClass("UnityEngine", "Object"), "DontDestroyOnLoad", root);
            Il2CppObject* canvas = AddComponent(root, AnyClass("UnityEngine", "Canvas"));
            int32_t overlay = 0, order = 32000;
            Call(canvas, "set_renderMode", &overlay);
            Call(canvas, "set_sortingOrder", &order);
            Il2CppObject* scaler = AddComponent(root, AnyClass("UnityEngine.UI", "CanvasScaler"));
            int32_t scaleWithScreen = 1;
            Vec2 reference{ 1024, 576 };
            float match = 0.5f;
            Call(scaler, "set_uiScaleMode", &scaleWithScreen);
            Call(scaler, "set_referenceResolution", &reference);
            Call(scaler, "set_matchWidthOrHeight", &match);
            AddComponent(root, AnyClass("UnityEngine.UI", "GraphicRaycaster"));
            Il2CppObject* rootT = TransformOf(root);

            // the dim layer also keeps clicks off the game's screens
            Il2CppObject* dim = NewObject("Dim", rootT);
            Place(TransformOf(dim), { 0, 0 }, { 1, 1 }, { 0.5f, 0.5f }, { 0, 0 }, { 0, 0 });
            Color shade{ 0, 0, 0, 0.6f };
            Call(AddComponent(dim, AnyClass("UnityEngine.UI", "Image")), "set_color", &shade);

            Il2CppObject* dialog = Instantiate(prefab, rootT);
            Il2CppObject* dialogT = TransformOf(dialog);
            Strip(dialogT, { "mvTweenScale", "mvTweenCanvasGroupAlpha", "mvChangeLocalizationText", "cmnUiTextManager", "cmnUiMLTextSupporter" });
            if (Il2CppObject* group = GetComponent(dialog, AnyClass("UnityEngine", "CanvasGroup")))
            {
                float one = 1.0f;
                bool yes = true;
                Call(group, "set_alpha", &one);
                Call(group, "set_interactable", &yes);
                Call(group, "set_blocksRaycasts", &yes);
            }
            Vec3 fit{ 0.9f, 0.9f, 1 };  // a margin around it on a 16:9 screen
            Call(dialogT, "set_localScale", &fit);

            Il2CppObject* title = TextIn(GameObjectOf(Find(dialogT, "dialog/title_win/title_win/Text01")));
            FitText(title);

            // the text box holds the rows (a 2-column grid) and a status line
            Il2CppObject* frame = Find(dialogT, "dialog/contents/frame_01");
            if (Il2CppObject* element = GetComponent(GameObjectOf(frame), AnyClass("UnityEngine.UI", "LayoutElement")))
            {
                float height = 290.0f;
                Call(element, "set_minHeight", &height);
                Call(element, "set_preferredHeight", &height);
            }
            Il2CppObject* message = GameObjectOf(Find(dialogT, "dialog/contents/Text02"));
            Il2CppObject* status = Instantiate(message, frame);
            SetActive(message, false);
            Place(TransformOf(status), { 0, 0 }, { 1, 0 }, { 0.5f, 0 }, { 0, 8 }, { -30, 56 });
            Il2CppObject* statusText = TextIn(status);
            FitText(statusText);

            Il2CppObject* grid = NewObject("Rows", frame);
            Place(TransformOf(grid), { 0, 1 }, { 1, 1 }, { 0.5f, 1 }, { 0, -14 }, { -20, 204 });
            Il2CppObject* layout = AddComponent(grid, AnyClass("UnityEngine.UI", "GridLayoutGroup"));
            Vec2 cell{ 270, 36 }, spacing{ 10, 6 };
            int32_t fixedColumns = 1, columns = kColumns, upperCenter = 1;
            Call(layout, "set_cellSize", &cell);
            Call(layout, "set_spacing", &spacing);
            Call(layout, "set_constraint", &fixedColumns);
            Call(layout, "set_constraintCount", &columns);
            Call(layout, "set_childAlignment", &upperCenter);

            Il2CppObject* buttons = Find(dialogT, "dialog/contents/Buttons");
            Il2CppObject* blue = GameObjectOf(Find(buttons, "Button01"));
            for (int i = 0; i < kMaxRows; i++)
            {
                Il2CppObject* row = Instantiate(blue, TransformOf(grid));
                Il2CppObject* text = TextIn(row);
                Place(Call(text, "get_transform"), { 0, 0 }, { 1, 1 }, { 0.5f, 0.5f }, { 0, 0 }, { -24, 0 });
                FitText(text);
                g.rows[i] = Keep(Wire(row));
                g.rowImages[i] = Keep(GetComponent(row, AnyClass("UnityEngine.UI", "Image")));
            }
            g.rowColor = *static_cast<Color*>(il2cpp_object_unbox(Call(Target(g.rowImages[0]), "get_color")));

            // the dialog's own buttons, smaller and centered: previous page,
            // back/close, next page
            if (Il2CppObject* row = GetComponent(GameObjectOf(buttons), AnyClass("UnityEngine.UI", "HorizontalLayoutGroup")))
            {
                int32_t middleCenter = 4;
                float gap = 16.0f;
                bool no = false;
                Call(row, "set_childAlignment", &middleCenter);
                Call(row, "set_spacing", &gap);
                Call(row, "set_childForceExpandWidth", &no);
                Call(row, "set_childForceExpandHeight", &no);
                Call(row, "set_childControlWidth", &no);
                Call(row, "set_childControlHeight", &no);
            }
            const char* names[] = { "Button01", "Button02", "Button03" };
            for (int i = 0; i < 3; i++)
            {
                Il2CppObject* go = GameObjectOf(Find(buttons, names[i]));
                SetActive(go, true);
                // the row lays them out at their own size: set that, and the
                // layout element to match
                Vec2 size{ 150, 40 };
                Call(TransformOf(go), "set_sizeDelta", &size);
                if (Il2CppObject* element = GetComponent(go, AnyClass("UnityEngine.UI", "LayoutElement")))
                {
                    Call(element, "set_minWidth", &size.x);
                    Call(element, "set_preferredWidth", &size.x);
                    Call(element, "set_minHeight", &size.y);
                    Call(element, "set_preferredHeight", &size.y);
                }
                Il2CppObject* text = TextIn(go);
                Place(Call(text, "get_transform"), { 0, 0 }, { 1, 1 }, { 0.5f, 0.5f }, { 0, 0 }, { -20, 0 });
                FitText(text);
                g.barLabels[i] = Keep(text);
                g.bar[i] = Keep(Wire(go));
                g.barImages[i] = Keep(GetComponent(go, AnyClass("UnityEngine.UI", "Image")));
                g.barColor[i] = *static_cast<Color*>(il2cpp_object_unbox(Call(Target(g.barImages[i]), "get_color")));
            }

            g.root = Keep(root);
            g.title = Keep(title);
            g.status = Keep(statusText);
            fprintf(stderr, "GoSRecomp: in-game debug menu built\n");
        }

        // ------------------------------------------------------------ drawing

        // A value shows its arrows while it is being changed.
        std::string RowLabel(const Row& r, bool editing = false)
        {
            switch (r.kind)
            {
            case RowKind::Action: return DebugActionLabel(r.action);
            case RowKind::Amount:
                if (AmountLocked(r.kind))
                    return "Amount: 1 (one only)";
                return editing ? "Amount: < " + Grouped(DebugAmount()) + " >" : "Amount: " + Grouped(DebugAmount());
            case RowKind::Category:
            {
                if (Categories().empty())
                    return "(no items)";
                std::string name = "Category: " + Categories()[CategoryOf(g.item % DebugItemCount())].name;
                return editing ? "< " + name + " >" : name;
            }
            case RowKind::Item:
            {
                size_t n = DebugItemCount();
                if (!n)
                    return "(no items)";
                size_t item = g.item % n;
                const Category& c = Categories()[CategoryOf(item)];
                std::string name = ItemName(item) + "  (" + std::to_string(item - c.first + 1) + "/" +
                                   std::to_string(c.end - c.first) + ")";
                return editing ? "< " + name + " >" : name;
            }
            case RowKind::Give:
            {
                size_t n = DebugItemCount();
                return n && DebugItemIsSingle(g.item % n) ? "Give item" : "Give item (x Amount)";
            }
            case RowKind::Accept: return "Use the debug menu";
            case RowKind::Close: return "Close";
            case RowKind::PageLink: return g.pages[r.action].title;
            case RowKind::Yes: return g.pending.kind == RowKind::Restore ? "Yes, restore and restart" : "Yes";
            case RowKind::No: return "No";
            case RowKind::Auto: return AutoLabel(r.action);
            case RowKind::Backup: return "Back up now";
            case RowKind::Restore: return "Restore " + BackupName(g.backups[r.action]);
            }
            return "";
        }

        // What the pending change is called in the question.
        std::string PendingName()
        {
            if (g.pending.kind == RowKind::Restore)
                return "Restore the backup from " + BackupName(g.backups[g.pending.action]);
            if (g.pending.kind == RowKind::Give)
            {
                size_t n = DebugItemCount();
                return n ? "Give " + DebugItemLabel(g.item % n) : "Give item";
            }
            return DebugActionLabel(g.pending.action);
        }

        // Selected: gold; a value being changed: green.
        void Highlight(Il2CppObject* button, Il2CppObject* image, Color normal, bool selected, bool editing = false)
        {
            Color c = editing ? Color{ 0.55f, 1.0f, 0.6f, normal.a } : selected ? Color{ 1.0f, 0.82f, 0.35f, normal.a } : normal;
            Call(image, "set_color", &c);
            float s = selected ? 1.05f : 1.0f;
            Vec3 scale{ s, s, 1 };
            Call(Call(button, "get_transform"), "set_localScale", &scale);
        }

        void Refresh()
        {
            std::vector<Row> rows = CurrentRows();
            std::vector<int> bar = BarItems();
            g.sel = std::min(g.sel, rows.size() + bar.size() - 1);

            std::string title = g.confirming && g.pending.kind == RowKind::Restore ? "Restore and restart?"
                                : g.confirming                                   ? "Are you sure?"
                                : g.hub                                          ? "Debug menu"
                                                                                 : g.pages[g.page].title;
            if (!g.confirming && !g.hub && g.pages.size() > 1)
                title += "  " + std::to_string(g.page + 1) + "/" + std::to_string(g.pages.size());
            Call(Target(g.title), "set_text", Str(title));

            std::string status = g.message;
            if (g.confirming && g.pending.kind == RowKind::Restore)
                status = PendingName() + "? Your current save is backed up first. Then the game closes and starts "
                                         "again by itself with the restored save.    A: choose    B: no";
            else if (g.editing)
                status = "Left / Right: change the value    A / B: done";
            else if (g.confirming)
                status = PendingName() + "? This changes your save right away and cannot be undone. Giving yourself "
                                         "resources, unlocks or cleared quests skips the game's progression and can "
                                         "spoil the intended experience.    A: choose    B: no";
            else if (status.empty())
            {
                if (g.hub && !DebugMenuWarned())
                    status = "The debug menu changes your save immediately and its changes cannot be undone. Giving "
                             "yourself resources, unlocks or cleared quests skips the game's progression and can spoil "
                             "the intended experience. Back up your save first (Save backup).";
                else if (g.hub)
                    status = "Always back up your save first (Save backup): changes cannot be undone.    A: open    B: close";
                else if (g.pages[g.page].backups)
                    status = "Backups are kept beside the save folder, newest first.    A: use    B: back";
                else if (g.pages[g.page].title == "Auto input")
                    status = "In battle, holding a button that is ON repeats it. Careful: some characters charge an attack "
                             "by holding the button (Maria, for one); with it ON they repeat instead of charging. "
                             "A: switch    B: back";
                else if (g.pages[g.page].title == "Give item")
                    status = "Gives one thing by name: pick the category, then the item, then the amount (A on a row, "
                             "then Left / Right). Characters, costumes, equipment, accessories and music come one "
                             "at a time.";
                else
                    status = "A: use (on a value: change it)    B: back    LB / RB: page";
            }
            Call(Target(g.status), "set_text", Str(status));
            // the text gets the room the rows leave (box 290 high, rows from 14
            // down, 36 high with 6 between)
            float lines = float((rows.size() + kColumns - 1) / kColumns);
            float used = 14.0f + lines * 36.0f + std::max(0.0f, lines - 1) * 6.0f + 16.0f;
            Vec2 statusSize{ -30, std::max(56.0f, std::min(170.0f, 290.0f - used)) };
            Call(Call(Target(g.status), "get_transform"), "set_sizeDelta", &statusSize);

            for (int i = 0; i < kMaxRows; i++)
            {
                Il2CppObject* button = Target(g.rows[i]);
                Il2CppObject* go = GameObjectOf(button);
                bool used = size_t(i) < rows.size();
                SetActive(go, used);
                if (!used)
                    continue;
                bool selected = size_t(i) == g.sel;
                SetLabel(go, RowLabel(rows[size_t(i)], selected && g.editing));
                if (AmountLocked(rows[size_t(i)].kind))  // greyed out, a little brighter under the cursor
                {
                    float v = selected ? 0.75f : 0.45f;
                    Highlight(button, Target(g.rowImages[i]), Color{ v, v, v, g.rowColor.a }, false);
                    if (selected)
                    {
                        Vec3 scale{ 1.05f, 1.05f, 1 };
                        Call(Call(button, "get_transform"), "set_localScale", &scale);
                    }
                }
                else
                    Highlight(button, Target(g.rowImages[i]), g.rowColor, selected, selected && g.editing);
            }

            const char* labels[] = { "< LB", g.hub ? "Close" : "Back", "RB >" };
            for (int i = 0; i < 3; i++)
            {
                Il2CppObject* button = Target(g.bar[i]);
                int slot = -1;
                for (size_t b = 0; b < bar.size(); b++)
                    if (bar[b] == i)
                        slot = int(b);
                SetActive(GameObjectOf(button), slot >= 0);
                if (slot < 0)
                    continue;
                Call(Target(g.barLabels[i]), "set_text", Str(labels[i]));
                Highlight(button, Target(g.barImages[i]), g.barColor[i], g.sel == rows.size() + size_t(slot));
            }
        }

        // ------------------------------------------------------------ actions

        void ChangeValue(const Row& r, int delta)
        {
            if (r.kind == RowKind::Amount)
            {
                int cur = DebugAmount(), idx = 0;
                for (int i = 0; i < int(std::size(kAmounts)); i++)
                    if (std::abs(double(kAmounts[i]) - cur) < std::abs(double(kAmounts[idx]) - cur))
                        idx = i;
                idx = std::max(0, std::min(int(std::size(kAmounts)) - 1, idx + delta));
                SetDebugAmount(kAmounts[idx]);
            }
            else if (r.kind == RowKind::Category && !Categories().empty())
            {
                // the first item of the next or previous category
                long long count = static_cast<long long>(Categories().size());
                long long c = (static_cast<long long>(CategoryOf(g.item % DebugItemCount())) + (delta > 0 ? 1 : -1) + count) % count;
                g.item = Categories()[size_t(c)].first;
            }
            else if (r.kind == RowKind::Item && DebugItemCount())
            {
                // within the category, wrapping around
                const Category& cat = Categories()[CategoryOf(g.item % DebugItemCount())];
                long long count = static_cast<long long>(cat.end - cat.first);
                long long at = static_cast<long long>(g.item % DebugItemCount() - cat.first);
                g.item = cat.first + size_t(((at + delta) % count + count) % count);
            }
            Sound("SelectGadget");
        }

        // The gold and gem counts at the top of the screen (homeHeader) show
        // a change only when told to.
        void RefreshHeader()
        {
            try
            {
                void* args[] = { TypeObject(GameClass("homeHeader")) };
                auto* found = reinterpret_cast<Il2CppArray*>(
                    Invoke(Method(AnyClass("UnityEngine", "Object"), "FindObjectsOfType", 1, "System.Type"), nullptr, args));
                auto** items = found ? reinterpret_cast<Il2CppObject**>(reinterpret_cast<uint8_t*>(found) + kIl2CppSizeOfArray) : nullptr;
                for (size_t i = 0; found && i < il2cpp_array_length(found); i++)
                    if (Il2CppObject* content = items[i] ? Call(items[i], "get_HeaderContent") : nullptr)
                        Call(content, "UpdateContent");
            }
            catch (const Failure& f)
            {
                fprintf(stderr, "GoSRecomp: debug menu: header not refreshed (%s)\n", f.what.c_str());
            }
        }

        void ShowPage(size_t page)
        {
            g.hub = false;
            g.editing = false;
            g.page = page;
            if (g.pages[page].backups)
            {
                try
                {
                    g.backups = Backups();
                }
                catch (const Failure&)
                {
                    g.backups.clear();
                }
            }
            g.sel = 0;
            g.message.clear();
            Refresh();
        }

        void ShowHub()
        {
            g.hub = true;
            g.editing = false;
            g.sel = g.page;  // back on the section it came from
            g.message.clear();
            Refresh();
        }

        // B: a section goes back to the main page, the main page closes.
        void Back()
        {
            if (g.editing)  // B while changing a value: done
            {
                Sound("Cancel");
                g.editing = false;
                Refresh();
                return;
            }
            if (g.confirming)  // B on the question: no
            {
                Sound("Cancel");
                g.confirming = false;
                g.sel = g.pendingSel;
                g.message.clear();
                Refresh();
                return;
            }
            if (g.hub)
            {
                Close();
                return;
            }
            Sound("Cancel");
            ShowHub();
        }

        void ActivateBar(int which)
        {
            if (which == 0)
                TurnPage(-1);
            else if (which == 2)
                TurnPage(1);
            else
                Back();
        }

        void Activate(int index)
        {
            std::vector<Row> rows = CurrentRows();
            if (size_t(index) >= rows.size())
            {
                std::vector<int> bar = BarItems();
                size_t slot = size_t(index) - rows.size();
                if (slot < bar.size())
                    ActivateBar(bar[slot]);
                return;
            }
            Row r = rows[size_t(index)];
            switch (r.kind)
            {
            case RowKind::Amount:
            case RowKind::Category:
            case RowKind::Item:
                if (AmountLocked(r.kind))
                {
                    Sound("NG");  // one only: nothing to change
                    break;
                }
                Sound(g.editing ? "Cancel" : "ButtonClick");  // A starts and ends changing it
                g.editing = !g.editing;
                break;
            case RowKind::Auto:
                ToggleAuto(r.action);
                Sound(r.action < kAutoSpeedRow ? (g_auto[r.action].on ? "CheckBoxOn" : "CheckBoxOff") : "SelectGadget");
                break;
            case RowKind::Backup:
                Sound("ButtonClick");
                g.message = BackUpNow();
                try
                {
                    g.backups = Backups();
                }
                catch (const Failure&)
                {
                }
                break;
            case RowKind::Restore:
            case RowKind::Give:
            case RowKind::Action:
                if (r.kind == RowKind::Action && !DebugActionChangesSave(r.action))
                {
                    Sound("ButtonClick");  // a battle setting, not the save: no question
                    g.message = RunDebugAction(r.action);
                    break;
                }
                Sound("ButtonClick");
                g.confirming = true;
                g.pending = r;
                g.pendingSel = size_t(index);
                g.sel = 1;  // the cursor starts on No
                g.message.clear();
                break;
            case RowKind::No:
                Back();
                return;
            case RowKind::Yes:
                Sound("ButtonClick");
                g.confirming = false;
                g.sel = g.pendingSel;
                if (g.pending.kind == RowKind::Restore)
                    g.message = Restore(g.backups[g.pending.action]);  // the game ends when this works
                else if (g.pending.kind == RowKind::Give)
                    g.message = DebugItemCount() ? GiveDebugItem(g.item % DebugItemCount()) : "No items";
                else
                    g.message = RunDebugAction(g.pending.action);
                fprintf(stderr, "GoSRecomp: debug menu: %s\n", g.message.c_str());
                RefreshHeader();
                break;
            case RowKind::Accept:
                Sound("ButtonClick");
                SetDebugMenuWarned();
                g.sel = 0;
                g.message.clear();
                break;
            case RowKind::Close:
                Close();
                return;
            case RowKind::PageLink:
                Sound("TabClick");
                ShowPage(r.action);
                return;
            }
            Refresh();
        }

        void TurnPage(int delta)
        {
            if (g.hub || g.confirming || g.pages.size() < 2)
                return;
            int n = int(g.pages.size());
            Sound("TabClick");
            ShowPage(size_t((int(g.page) + delta + n) % n));
        }

        // Up/down move a grid row, left/right a column. Below the last row
        // are the bottom buttons, where left/right move between them.
        void Move(int dx, int dy)
        {
            g.editing = false;
            size_t rows = CurrentRows().size(), bar = BarItems().size();
            int sel = int(g.sel), next = sel;
            if (g.sel < rows)
            {
                if (dy)
                {
                    next = sel + dy * kColumns;
                    if (next >= int(rows))
                        next = int(rows) + int(bar / 2);  // down from the rows: the middle bottom button
                    if (next < 0)
                        next = sel;
                }
                else if (sel % kColumns + dx >= 0 && sel % kColumns + dx < kColumns && sel + dx < int(rows))
                    next = sel + dx;
            }
            else
            {
                int slot = sel - int(rows);
                if (dy < 0 && rows)
                {
                    // up from the bottom buttons: the last row, left or right half
                    int lastRowStart = int((rows - 1) / kColumns) * kColumns;
                    next = std::min(int(rows) - 1, lastRowStart + (slot * 2 >= int(bar) ? kColumns - 1 : 0));
                }
                else if (dx)
                    next = int(rows) + std::max(0, std::min(int(bar) - 1, slot + dx));
            }
            if (next == sel)
                return;
            g.sel = size_t(next);
            Sound("SelectGadget");
            Refresh();
        }

        Il2CppObject* EventSystem()
        {
            return CallStatic(AnyClass("UnityEngine.EventSystems", "EventSystem"), "get_current");
        }

        void Open()
        {
            if (!Alive(Target(g.root)))
            {
                ReleaseAll();
                Build();
            }
            g.pages = BuildPages();
            g.page = std::min(g.page, g.pages.size() - 1);
            g.hub = true;  // it always opens on the main page
            g.editing = g.confirming = false;
            g.sel = 0;
            g.message.clear();
            SetActive(Target(g.root), true);
            Il2CppClass* time = AnyClass("UnityEngine", "Time");
            g.timeScale = Float(CallStatic(time, "get_timeScale"));
            float stop = 0.0f;
            CallStatic(time, "set_timeScale", &stop);
            if (Il2CppObject* events = EventSystem())
            {
                bool off = false;
                Call(events, "set_sendNavigationEvents", &off);
            }
            g.open = true;
            Refresh();
            Sound("DialogOpen");
            fprintf(stderr, "GoSRecomp: in-game debug menu opened\n");
        }

        void Close()
        {
            g.open = false;
            g.swallow = true;
            Sound("DialogClose");
            try
            {
                if (Il2CppObject* root = Target(g.root); Alive(root))
                    SetActive(root, false);
            }
            catch (const Failure&)
            {
            }
            Il2CppClass* time = AnyClass("UnityEngine", "Time");
            if (Float(CallStatic(time, "get_timeScale")) == 0.0f)
                CallStatic(time, "set_timeScale", &g.timeScale);
            if (Il2CppObject* events = EventSystem())
            {
                bool on = true;
                Call(events, "set_sendNavigationEvents", &on);
            }
        }

        // ------------------------------------------------------------- input

        enum : int
        {
            LSTICK_U = 0, LSTICK_R = 1, LSTICK_D = 2, LSTICK_L = 3, BUTTON_A = 8, BUTTON_B = 9, BUTTON_L1 = 12,
            BUTTON_R1 = 13, BUTTON_L3 = 16, BUTTON_R3 = 17, DPAD_U = 20, DPAD_R = 21, DPAD_D = 22, DPAD_L = 23,
            SUBMIT = 24, CANCEL = 25
        };
        constexpr int Bit(int a) { return 1 << a; }

        struct Joystick
        {
            Il2CppObject* obj;
            FieldInfo *flags, *before, *flagsDefault, *beforeDefault, *axisL, *axisR;
        };

        Joystick GetJoystick()
        {
            Joystick j{ Singleton("gsJoystick") };
            Il2CppClass* k = il2cpp_object_get_class(j.obj);
            j.flags = Field(k, "m_flags");
            j.before = Field(k, "m_before_flags");
            j.flagsDefault = Field(k, "m_flags_default");
            j.beforeDefault = Field(k, "m_before_flags_default");
            j.axisL = Field(k, "m_axis_l");
            j.axisR = Field(k, "m_axis_r");
            return j;
        }

        // The game sees no buttons and centered sticks this frame.
        void Swallow(const Joystick& j)
        {
            int32_t zero = 0;
            Vec2 centered{ 0, 0 };
            for (FieldInfo* f : { j.flags, j.before, j.flagsDefault, j.beforeDefault })
                il2cpp_field_set_value(j.obj, f, &zero);
            il2cpp_field_set_value(j.obj, j.axisL, &centered);
            il2cpp_field_set_value(j.obj, j.axisR, &centered);
        }

        // In a running battle: not paused, no popup.
        bool InBattle()
        {
            Il2CppObject* scenes = Singleton("bsSceneManager");
            int32_t current = 0;
            bool active = false;
            il2cpp_field_get_value(scenes, Field(il2cpp_object_get_class(scenes), "<Current>k__BackingField"), &current);
            il2cpp_field_get_value(scenes, Field(il2cpp_object_get_class(scenes), "_isSceneActive"), &active);
            if (current != 4 || !active)  // bsScene.ID.BATTLE
                return false;
            if (Truth(Call(Singleton("cmnPopupManager"), "get_isActivePopup")))
                return false;
            return Float(CallStatic(AnyClass("UnityEngine", "Time"), "get_timeScale")) > 0.0f;
        }

        // The gsJoystick button the game reads for an action, as its button
        // prompts look it up (follows the player's controller settings).
        int AssignFor(int keyType)
        {
            static const MethodInfo* lookup = Method(GameClass("cmnUiTextIconController"), "GetJoystickAssignWithKeyType", 1);
            void* args[] = { &keyType };
            Il2CppObject* r = Invoke(lookup, nullptr, args);
            return r ? *static_cast<int32_t*>(il2cpp_object_unbox(r)) : -1;
        }

        // Held buttons with auto input on: pressed for the first half of each
        // cycle, let go for the second.
        void ApplyAutoInput(const Joystick& j, int32_t raw)
        {
            LoadAutoInput();
            static int held[32] = {};
            bool any = false;
            for (const auto& a : g_auto)
                any |= a.on;
            if (!any)
                return;
            if (!InBattle())
            {
                std::fill(std::begin(held), std::end(held), 0);
                return;
            }
            static int assigns[std::size(g_auto)] = {};
            static ULONGLONG looked = 0;
            ULONGLONG now = GetTickCount64();
            if (now - looked > 1000)  // the buttons can be changed in the game's settings
            {
                for (size_t i = 0; i < std::size(g_auto); i++)
                    assigns[i] = AssignFor(g_auto[i].keyType);
                looked = now;
            }
            int period = std::max(2, int(std::lround(60.0 / g_autoSpeed)));  // frames per press at 60 fps
            int32_t flags = 0, flagsDefault = 0;
            il2cpp_field_get_value(j.obj, j.flags, &flags);
            il2cpp_field_get_value(j.obj, j.flagsDefault, &flagsDefault);
            for (size_t i = 0; i < std::size(g_auto); i++)
            {
                int a = assigns[i];
                if (!g_auto[i].on || a < 0 || a >= 32)
                    continue;
                int32_t bit = Bit(a);
                if (!(raw & bit))
                {
                    held[a] = 0;
                    continue;
                }
                if (held[a]++ % period >= period / 2)
                {
                    flags &= ~bit;
                    flagsDefault &= ~bit;
                }
            }
            il2cpp_field_set_value(j.obj, j.flags, &flags);
            il2cpp_field_set_value(j.obj, j.flagsDefault, &flagsDefault);
        }

        bool GameIsForeground()
        {
            DWORD pid = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &pid);
            return pid == GetCurrentProcessId();
        }

        void Frame()
        {
            Joystick j = GetJoystick();
            int32_t raw = 0;
            il2cpp_field_get_value(j.obj, j.flags, &raw);
            static int32_t last = 0;
            int32_t pressed = raw & ~last;
            last = raw;

            static bool keyWas = false;
            bool keyDown = (GetAsyncKeyState(DebugMenuHotkey()) & 0x8000) != 0;
            bool toggle = keyDown && !keyWas && GameIsForeground();
            keyWas = keyDown;
            int sticks = Bit(BUTTON_L3) | Bit(BUTTON_R3);
            if ((raw & sticks) == sticks && (pressed & sticks))
                toggle = true;

            if (!g.open)
            {
                if (toggle)
                {
                    Open();
                    Swallow(j);
                    return;
                }
                if (g.swallow)  // until the buttons used in the menu are let go
                {
                    if (raw == 0)
                        g.swallow = false;
                    else
                        Swallow(j);
                }
                else
                    ApplyAutoInput(j, raw);
                return;
            }

            if (toggle)
            {
                Close();
                Swallow(j);
                return;
            }
            if (pressed & (Bit(BUTTON_B) | Bit(CANCEL)))
            {
                Back();
                Swallow(j);
                return;
            }

            // directions repeat while held
            int up = Bit(DPAD_U) | Bit(LSTICK_U), down = Bit(DPAD_D) | Bit(LSTICK_D);
            int left = Bit(DPAD_L) | Bit(LSTICK_L), right = Bit(DPAD_R) | Bit(LSTICK_R);
            static int heldDir = 0;
            static ULONGLONG nextRepeat = 0;
            static int repeats = 0;
            int dir = (raw & up) ? 1 : (raw & down) ? 2 : (raw & left) ? 3 : (raw & right) ? 4 : 0;
            bool fire = false;
            ULONGLONG now = GetTickCount64();
            if (dir != heldDir)
            {
                heldDir = dir;
                repeats = 0;
                fire = dir != 0;
                nextRepeat = now + 400;
            }
            else if (dir && now >= nextRepeat)
            {
                fire = true;
                repeats++;
                nextRepeat = now + 70;
            }

            std::vector<Row> rows = CurrentRows();
            const Row* row = g.sel < rows.size() ? &rows[g.sel] : nullptr;
            bool value = g.editing && row && IsValue(row->kind);
            if (fire)
            {
                int step = row && row->kind == RowKind::Item && repeats > 15 ? 10 : 1;
                if (dir == 1)
                    Move(0, -1);
                else if (dir == 2)
                    Move(0, 1);
                else if (value)
                {
                    ChangeValue(*row, dir == 3 ? -step : step);
                    Refresh();
                }
                else
                    Move(dir == 3 ? -1 : 1, 0);
            }
            if (pressed & (Bit(BUTTON_A) | Bit(SUBMIT)))
                Activate(int(g.sel));
            if (pressed & Bit(BUTTON_L1))
                TurnPage(-1);
            if (pressed & Bit(BUTTON_R1))
                TurnPage(1);
            if (g.open)
                Swallow(j);
        }
    }

    void InGameMenuAfterUpdate()
    {
        if (!il2cpp_domain_get())
            return;
        if (!DebugMenuEnabled())  // auto input still applies (set it with window=1 off, or in gosr.ini)
        {
            try
            {
                Joystick j = GetJoystick();
                int32_t raw = 0;
                il2cpp_field_get_value(j.obj, j.flags, &raw);
                ApplyAutoInput(j, raw);
            }
            catch (const Failure&)
            {
            }
            return;
        }
        try
        {
            Frame();
        }
        catch (const Failure& f)
        {
            static std::string lastError;
            if (lastError != f.what)
            {
                lastError = f.what;
                fprintf(stderr, "GoSRecomp: in-game debug menu: %s\n", f.what.c_str());
            }
            if (g.open)
            {
                try
                {
                    Close();
                }
                catch (const Failure&)
                {
                    g.open = false;
                }
            }
        }
    }
}
