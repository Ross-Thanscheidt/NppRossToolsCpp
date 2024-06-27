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
#include <iomanip>
#include <locale>
#include <optional>
#include <regex>
#include <shlwapi.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <time.h>


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

static HWND GetScintillaHandle()
{
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    return which == -1 ? 0 : which == 0 ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

static int ParseAmount(std::string amountString)
{
    if (amountString.find('.') == std::string::npos)
    {
        amountString.append(".00");
    }
    else
    {
        while (amountString.length() < 3 || amountString[amountString.length() - 3] != '.')
        {
            amountString.append("0");
        }
    }

    amountString.erase(amountString.length() - 3, 1);

    return stoi(amountString);
}

static std::string FormatAmount(int amount)
{
    std::string result;
    std::stringstream ssDollars;
    std::stringstream ssCents;

    ssDollars << (amount / 100);

    ssCents << std::setw(2) << std::setfill('0') << (amount % 100);

    result = ssDollars.str().append(".").append(ssCents.str());

    return result;
}

void RemoveTrailingSpacesCommand()
{
    HWND curScintilla = GetScintillaHandle();

    if (curScintilla == 0)
        return;

    int lineCount = (int)::SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);

    if (lineCount > 0)
    {
        Sci_Position startPos;
        Sci_Position endPos;
        std::string currentLine = "";
        std::string newLine = "";

        ::SendMessage(curScintilla, SCI_BEGINUNDOACTION, 0, 0);

        for (int lineNumber = 0; lineNumber < lineCount; lineNumber++)
        {
            startPos = (Sci_Position)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
            endPos = (Sci_Position)::SendMessage(curScintilla, SCI_GETLINEENDPOSITION, lineNumber, 0);
            currentLine.resize(endPos - startPos);
            ::SendMessage(curScintilla, SCI_SETTARGETRANGE, startPos, endPos);
            ::SendMessage(curScintilla, SCI_GETTARGETTEXT, 0, reinterpret_cast<LPARAM>(currentLine.data()));

            newLine = std::regex_replace(currentLine, std::regex(" +$|(\\S+)"), "$1");

            if (currentLine.compare(newLine) != 0)
            {
                ::SendMessage(curScintilla, SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(newLine.data()));
            }
        }

        ::SendMessage(curScintilla, SCI_GOTOLINE, 0, 0);
        ::SendMessage(curScintilla, SCI_ENDUNDOACTION, 0, 0);
    }
}

void UpdateAgesCommand()
{
    HWND curScintilla = GetScintillaHandle();

    if (curScintilla == 0)
        return;

    int lineCount = (int)::SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);

    if (lineCount > 0)
    {
        Sci_Position startPos;
        Sci_Position endPos;
        std::string currentLine = "";
        std::string newLine = "";

        std::time_t t = std::time(nullptr);
        int currentYear = 1900 + std::localtime(&t)->tm_year;
        std::regex regex("^(\\d{2}\\/.{2}\\/(\\d{4}).*)\\(\\d+ in \\d{4}\\)(.*)$");
        std::smatch match;
        int age;
        std::string lineFormat;

        ::SendMessage(curScintilla, SCI_BEGINUNDOACTION, 0, 0);

        for (int lineNumber = 0; lineNumber < lineCount; lineNumber++)
        {
            startPos = (Sci_Position)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
            endPos = (Sci_Position)::SendMessage(curScintilla, SCI_GETLINEENDPOSITION, lineNumber, 0);

            currentLine.resize(endPos - startPos);
            ::SendMessage(curScintilla, SCI_SETTARGETRANGE, startPos, endPos);
            ::SendMessage(curScintilla, SCI_GETTARGETTEXT, 0, reinterpret_cast<LPARAM>(currentLine.data()));

            newLine = currentLine;

            if (std::regex_search(newLine, match, regex))
            {
                age = currentYear - std::stoi(match[2]);
                lineFormat.assign("$1(").append(std::to_string(age)).append(" in ").append(std::to_string(currentYear)).append(")$3");
                newLine = std::regex_replace(newLine, regex, lineFormat);
            }

            newLine = std::regex_replace(newLine, std::regex(" +$|(\\S+)"), "$1");

            if (currentLine.compare(newLine) != 0)
            {
                ::SendMessage(curScintilla, SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(newLine.data()));
            }
        }

        ::SendMessage(curScintilla, SCI_GOTOLINE, 0, 0);
        ::SendMessage(curScintilla, SCI_ENDUNDOACTION, 0, 0);
    }
}

void UpdateLineBalancesCommand()
{
    HWND curScintilla = GetScintillaHandle();

    if (curScintilla == 0)
        return;

    int lineCount = (int)::SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);

    if (lineCount > 0)
    {
        Sci_Position startPos;
        Sci_Position endPos;
        std::string currentLine = "";
        std::string newLine = "";

        std::regex regex("^(\\d{2}\\/.{2}\\/(\\d{4}).*)\\(\\d+ in \\d{4}\\)(.*)$");
        //var regexBalance = new Regex(@"^(?<prefix>\??)(?<balance>\-?[0-9]+(?>,?[0-9]{3})*(?>\.[0-9]{0,2})?)(?<eol>.*Balance.*)$");
        // prefix=$1, balance=$2, eol=$5
        std::regex regexBalance("^(\\??)(\\-?[0-9]+(,?[0-9]{3})*(\\.[0-9]{0,2})?)(.*Balance.*)$");
        //var regexTransaction = new Regex(@"^(?<prefix>\?*)(?<balance>-?[0-9]*(?>,?[0-9]{3})*(?>\.[0-9]{0,2})?)(?<suffix>\??) *(?<transall>(?>\(|\[|\\\[)(?<transamount>(?>\-|\+)?[0-9]*(?>,?[0-9]{3})*(?>\.[0-9]{0,2})?).*?(?>\)|\]|\\\]))(?<eol>.*)$");
        // prefix=$1, balance=$2, transall=$5, transamount=$7, suffix=$8, eol=$13
        // ?63.28? (-25.32) 06/03 Hobby Lobby
        // 0:[63.28 (-25.32) 06/03 Hobby Lobby] 1:[?] 2:[63.28] 3:[] 4:[.28] 5:[?] 6:[(-25.32)] 7:[(] 8:[-25.32] 9:[-] 10:[] 11:[.32] 12:[)] 13:[ 06/03 Hobby Lobby]
        std::regex regexTransaction("^(\\?*)(-?[0-9]*(,?[0-9]{3})*(\\.[0-9]{0,2})?) *((\\(|\\[|\\\\\\[)((\\-|\\+)?[0-9]*(,?[0-9]{3})*(\\.[0-9]{0,2})?).*(\\)|\\]|\\\\\\]))(.*)$");
        std::smatch match;
        std::string lineFormat;
        std::optional<int> currentBalance;

        ::SendMessage(curScintilla, SCI_BEGINUNDOACTION, 0, 0);

        for (int lineNumber = lineCount - 1; lineNumber >= 0; lineNumber--)
        {
            startPos = (Sci_Position)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
            endPos = (Sci_Position)::SendMessage(curScintilla, SCI_GETLINEENDPOSITION, lineNumber, 0);

            currentLine.resize(endPos - startPos);
            ::SendMessage(curScintilla, SCI_SETTARGETRANGE, startPos, endPos);
            ::SendMessage(curScintilla, SCI_GETTARGETTEXT, 0, reinterpret_cast<LPARAM>(currentLine.data()));

            newLine = std::regex_replace(currentLine, std::regex(" +$|(\\S+)"), "$1");

            if (newLine.length() > 0)
            {
                if (std::regex_search(newLine, std::regex("^\\* \\* \\*")))
                {
                    currentBalance.reset();

                    newLine.append(" currentBalance=reset");
                }
                else if (std::regex_search(newLine, match, regexBalance))
                {
                    int balance = ParseAmount(match[2]);
                    currentBalance = balance;

                    newLine.append(" balance=").append(std::to_string(balance));
                }
                else if (currentBalance.has_value() && std::regex_search(newLine, match, regexTransaction))
                {
                    std::string oldBalanceString = match[2];

                    int transAmount = ParseAmount(match[7]);
                    currentBalance = currentBalance.value() + transAmount;
                    std::string newBalanceString = FormatAmount(currentBalance.value());
                    
                    if (oldBalanceString.compare(newBalanceString) != 0)
                    {
                        // $1 is option '?' prefix, $5 is '(transAmount)', $12 is rest of line after transAmount
                        lineFormat.assign("$1").append("{newBalanceString}").append(" $5$12");
                        newLine = std::regex_replace(newLine, regexTransaction, lineFormat);
                        newLine = std::regex_replace(newLine, std::regex("\\{newBalanceString\\}"), newBalanceString);
                    }

                    newLine
                        .append(" oldBalanceString=").append(oldBalanceString)
                        .append(", transAmount=").append(std::to_string(transAmount))
                        .append(", currentBalance=").append(std::to_string(currentBalance.value()))
                        .append(", newBalanceString=").append(newBalanceString);
                }
            }

            if (currentLine.compare(newLine) != 0)
            {
                ::SendMessage(curScintilla, SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(newLine.data()));
            }
        }

        ::SendMessage(curScintilla, SCI_GOTOLINE, 0, 0);
        ::SendMessage(curScintilla, SCI_ENDUNDOACTION, 0, 0);
    }
}

void GoToPluginCommunicationGuide()
{
	::ShellExecute(NULL, TEXT("open"), TEXT("https://npp-user-manual.org/docs/plugin-communication/"), NULL, NULL, SW_SHOWNORMAL);
}

void GoToPluginRepo()
{
	::ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/Ross-Thanscheidt/NppRossToolsCpp"), NULL, NULL, SW_SHOWNORMAL);
}

//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, const TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit)
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