// -*- c-file-style: "java" -*-
// $Id: MainWindow.h,v 1.7 2004-11-21 22:38:23 zeeb90au Exp $
// This file is part of SmallBASIC
//
// Copyright(C) 2001-2003 Chris Warren-Smith. Gawler, South Australia
// cwarrens@twpo.com.au
//
// This program is distributed under the terms of the GPL v2.0 or later
// Download the GNU Public License (GPL) from www.gnu.org
// 

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <fltk/Window.h>
#include <fltk/TabGroup.h>

#include "Fl_Ansi_Window.h"
#include "EditorWindow.h"

#define C_LINKAGE_BEGIN extern "C" {
#define C_LINKAGE_END }

void trace(const char *format, ...);

struct MainWindow : public Window {
    MainWindow(int w, int h);
    ~MainWindow();

    int handle(int e);
    bool isTurboMode(void)  { return isTurbo; }
    bool isBreakExec(void);
    bool isModal();
    void setModal(bool modal);
    void resetPen();

    int penX;
    int penY;
    int penDownX;
    int penDownY;
    int penState;
    bool isTurbo;
    bool modalLoop;

    // main output
    Fl_Ansi_Window *out;
    EditorWindow* editWnd;

    // tabs
    TabGroup* tabGroup;
    Group* editGroup;
    Group* outputGroup;
    Group* helpGroup;

    // status bar
    Widget* fileStatus;
    Widget* rowStatus;
    Widget* colStatus;
    Widget* runStatus;
    Widget* modStatus;
};

#endif