//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"

//
// put the headers you need here
//
#include <stdlib.h>
#include <time.h>
#include <shlwapi.h>
#include <string>

const TCHAR sectionName[] = TEXT("NppRossToolsCpp");
const TCHAR keyName[] = TEXT("doCloseTag");
const TCHAR configFileName[] = TEXT("NppRossToolsCpp.ini");

#ifdef UNICODE 
	#define generic_itoa _itow
#else
	#define generic_itoa itoa
#endif

FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;


TCHAR iniFilePath[MAX_PATH];

void pluginInit(HANDLE)
{
}

void pluginCleanUp()
{
}

void commandMenuInit()
{
	//
	// Firstly we get the parameters from your plugin config file (if any)
	//

	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)iniFilePath);

	// if config path doesn't exist, we create it
	if (PathFileExists(iniFilePath) == FALSE)
	{
		::CreateDirectory(iniFilePath, NULL);
	}

	// make your plugin config file full file path name
	PathAppend(iniFilePath, configFileName);

	// get the parameter value from plugin config

    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Remove Trailing Spaces"), RemoveTrailingSpacesCommand, NULL, false);
    setCommand(1, TEXT("Update Ages"), UpdateAgesCommand, NULL, false);
	setCommand(2, TEXT("Update Line Balances"), UpdateLineBalancesCommand, NULL, false);

	setCommand(3, TEXT("---"), NULL, NULL, false);

	setCommand(4, TEXT("Plugin Communication Guide"), GoToPluginCommunicationGuide, NULL, false);
	setCommand(5, TEXT("Plugin Source Code"), GoToPluginRepo, NULL, false);
}

void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
	// delete funcItem[4]._pShKey;
}

void RemoveTrailingSpacesCommand()
{
    // Get the current scintilla
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1)
        return;
    HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

    int lineCount = (int)::SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);

    if (lineCount > 0)
    {
        ::SendMessage(curScintilla, SCI_BEGINUNDOACTION, 0, 0);

        //size_t lineLen;
        std::string lineText = "";
        Sci_Position startPos;
        Sci_Position endPos;

        for (int lineNumber = 0; lineNumber < lineCount; lineNumber++)
        {
            ::SendMessage(curScintilla, SCI_GOTOLINE, lineNumber, 0);

            //startPos = (Sci_Position)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
            //endPos = (Sci_Position)::SendMessage(curScintilla, SCI_GETLINEENDPOSITION, lineNumber, 0);

            //lineLen = endPos - startPos + 1;
            //lineText.resize(lineLen, 'r');
            //::SendMessage(curScintilla, SCI_SETTARGETRANGE, startPos, startPos);
            //::SendMessage(curScintilla, SCI_GETTARGETTEXT, 0, reinterpret_cast<LPARAM>(lineText.data()));
            //lineText.push_back('x');
            //::SendMessage(curScintilla, SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(lineText.data()));

            // string line = editor.GetLine(lineNumber)
            //char* textPtr = new char[10000];
            ::SendMessage(curScintilla, SCI_GETLINE, lineNumber, reinterpret_cast<LPARAM>(lineText.data()));

            // bool endHasLF = line.Length > 0 && line[line.Length - 1] == '\n'
            // string newLine = line.TrimEnd();

            // if (endHasLF)
            // {
            //   newLine = string.Concat(newLine, '\n')
            // }

            // if (!string.Equals(line, newLine))
            // {
            //   editor.SelectCurrentLine()
            startPos = (Sci_Position)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
            endPos = (Sci_Position)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, static_cast<WPARAM>(lineNumber + 1), 0);
            ::SendMessage(curScintilla, SCI_SETSELECTION, startPos, endPos);
            //   editor.ReplaceSel(newLine)
            lineText.push_back('q');
            ::SendMessage(curScintilla, SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(lineText.data()));
            // }
        }

        // Scintilla control has no Unicode mode, so we use (char *) here
        //::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");

        int line = 0;
        ::SendMessage(curScintilla, SCI_GOTOLINE, line, 0);

        ::SendMessage(curScintilla, SCI_ENDUNDOACTION, 0, 0);
    }
}

void UpdateAgesCommand()
{
    RemoveTrailingSpacesCommand();
}

void UpdateLineBalancesCommand()
{
    RemoveTrailingSpacesCommand();
}

//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

void GoToPluginCommunicationGuide()
{
	::ShellExecute(NULL, TEXT("open"), TEXT("https://npp-user-manual.org/docs/plugin-communication/"), NULL, NULL, SW_SHOWNORMAL);
}

void GoToPluginRepo()
{
	::ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/Ross-Thanscheidt/NppRossToolsCpp"), NULL, NULL, SW_SHOWNORMAL);
}
