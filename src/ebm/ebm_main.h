// -*- c-file-style: "java" -*-
// $Id: ebm_main.h,v 1.3 2004-07-03 23:06:57 zeeb90au Exp $
// This file is part of SmallBASIC
//
// Copyright(C) 2001-2003 Chris Warren-Smith. Gawler, South Australia
// cwarrens@twpo.com.au
//
/*                  _.-_:\
//                 /      \
//                 \_.--*_/
//                       v
*/
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
// 

#ifndef EBM_MAIN_HH
#define EBM_MAIN_HH

#include <gui.h>
#include "AnsiWindow.h"

struct SBWindow : public AnsiWindow {
    SBWindow();
    ~SBWindow();
    
    bool wasBreakEv(void)   { return isBreak; }
    bool isTurboMode(void)  { return isTurbo; }
    bool isMenuActive(void) { return menuActive; }

    void run(const char* file);
    void doShell();
    void resetPen() {
        penX = 0;
        penY = 0;
        penDownX = 0;
        penDownY = 0;
        penState = 0;
        menuActive = 0;
    }

    U16 penX;
    U16 penY;
    U16 penDownX;
    U16 penDownY;
    U16 penState;
    bool menuActive;
    S32 MsgHandler(MSG_TYPE type, CViewable *from, S32 data);

    private:
    bool isTurbo;
    bool isBreak;
    CMenu *menu;

    void doKey(S32 key);
    void doAboutBasFile();
    void doKeyboard();
    void doList();
    void sendKeys(const char* s);
};

#endif
