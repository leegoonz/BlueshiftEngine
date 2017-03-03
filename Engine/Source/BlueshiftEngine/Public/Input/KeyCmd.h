#pragma once

/*
-------------------------------------------------------------------------------

    Key Command System

-------------------------------------------------------------------------------
*/

#include "KeyCodes.h"

BE_NAMESPACE_BEGIN

class File;
class CmdArgs;

class KeyCmdSystem {
public:
    void                    Init();
    void                    Shutdown();

    void                    ClearStates();
    void                    KeyEvent(KeyCode::Enum keynum, bool down);

    const wchar_t *         GetBinding(KeyCode::Enum keynum) const;
    void                    SetBinding(KeyCode::Enum keynum, const wchar_t *cmd);
    void                    WriteBindings(File *fp) const;

    bool                    IsPressed(KeyCode::Enum keynum) const;
    bool                    IsPressedAnyKey() const;

    static KeyCode::Enum    StringToKeynum(const char *str);
    static const char *     KeynumToString(KeyCode::Enum keynum);

private:
    struct Key {
        bool                is_down;
        int                 count;
        wchar_t *           binding;
    };

    Key                     keyList[(int)KeyCode::LastKey];

    static void             Cmd_ListBinds(const CmdArgs &args);
    static void             Cmd_Bind(const CmdArgs &args);
    static void             Cmd_Unbind(const CmdArgs &args);
    static void             Cmd_UnbindAll(const CmdArgs &args);
};

extern KeyCmdSystem         keyCmdSystem;

BE_NAMESPACE_END