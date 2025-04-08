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
#include <algorithm>

const TCHAR sectionName[] = TEXT("NppRossToolsCpp");
const TCHAR keyName[] = TEXT("doCloseTag");
const TCHAR configFileName[] = TEXT("NppRossToolsCpp.ini");

#ifdef UNICODE 
	#define generic_itoa _itow
#else
	#define generic_itoa itoa
#endif

#define REQUIRED_CURRENCY_REGEX "((\\-|\\+)?[0-9]+(,?[0-9]{3})*(\\.[0-9]{0,2})?)"
#define OPTIONAL_CURRENCY_REGEX "((\\-|\\+)?[0-9]*(,?[0-9]{3})*(\\.[0-9]{0,2})?)"

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

static ShortcutKey* CreateShortcutKey(std::string modifiers, UCHAR key)
{
    ShortcutKey* shKey = new ShortcutKey;
    shKey->_isAlt = std::regex_search(modifiers, std::regex("Alt", std::regex_constants::icase));
    shKey->_isCtrl = std::regex_search(modifiers, std::regex("Ctl|Ctrl|Control", std::regex_constants::icase));
    shKey->_isShift = std::regex_search(modifiers, std::regex("Shift", std::regex_constants::icase));
    shKey->_key = key;
    return shKey;
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
	setCommand(2, TEXT("Update Line Balances"), UpdateLineBalancesCommand, CreateShortcutKey("Alt", 'B'), false);
	setCommand(3, TEXT("---"), NULL, NULL, false);
	setCommand(4, TEXT("Plugin Source Code"), GoToPluginRepo, NULL, false);

    // Update the nbFunc constant in PluginDefinition.h if the highest index number changes (set to highest index + 1)
}

void commandMenuCleanUp()
{
	// Deallocate shortcut keys
    for (int index = 0; index < nbFunc; index++)
    {
        if (funcItem[index]._pShKey != NULL)
        {
            delete funcItem[index]._pShKey;
        }
    }
}

static HWND GetScintillaHandle()
{
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    return which == -1 ? 0 : which == 0 ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

// ParseAmount will remove all commas, add ".##" to the end if missing, and store currency as an int with an implied 2 decimal points.
// ParseAmount presumes that the amount passed in will only be those that matched a regex with "", ".", ".#", or ".##" at the end of the string.
static int ParseAmount(std::string amountString)
{
    amountString.erase(std::remove(amountString.begin(), amountString.end(), ','), amountString.end());

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

// Given an amount as an int representing currency with an implied 2 decimal places,
// format as a string with commas for thousands separators and always 2 decimal places.
static std::string FormatAmount(int amount)
{
    std::stringstream ssDollars;
    std::stringstream ssCents;

    ssDollars.imbue(std::locale("en-US.UTF-8"));
    ssDollars << (amount / 100);

    ssCents << std::setw(2) << std::setfill('0') << (abs(amount) % 100);

    std::string ssResult = ssDollars.str().append(".").append(ssCents.str());

    if (amount < 0 && amount > -100)
    {
        ssResult = "-" + ssResult;
    }

    return ssResult;
}

// Remove all trailing spaces at the end of every line in the current document.
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

// Update the age and year on every line of the current document that starts with a date and has (# in yyyy) later on the line:
//   mm/dd/yyyy <anything> (# in yyyy) <anything>
// Also remove all trailing spaces at the end of every line in the current document.
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

// Starting at the bottom of the current document and moving up, find starting balances and update the current balance on each transaction line.
//
// A line that starts with "#.## Balance" will reset the current balance to that amount:
// [?]<balance><restOfLine> where <restOfLine> has "Balance" in it somewhere
//
// A line with this format is considered a transaction line and its current balance will be inserted or updated if it has a balance line below it
// (the current balance will depend on the starting balance line below and any transaction lines between the balance line and the current transaction line)
// [?][<balance>] ([<sign>]<transAmount>[?])<restOfLine>
// Where the first ? is optional and typically indicates uncertainty or a bill that hasn't been paid yet
// Where the <currentBalance> may or may not be there, and the calculated <currentBalance> will be inserted (or updated if it is already there)
// Where the <sign> for the <transAmount> can be +, -, or not specified (implies +)
// Where the ? after <transAmount> is optional and typically indicates that <transAmount> is an estimate for now
// 
// A line that starts with "* * *" is considered a section break, where each section can have its own starting balance line and transaction lines
// (this is useful for updating a note in Joplin - see joplinapp.org - where Notepad++ can be used as an external editor)
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

        // Format: [?]<balance><restOfLine> where <restOfLine> has "Balance" in it somewhere
        // <prefix>=$1, <balance>=$2, <restOfLine>=$6
        std::regex regexBalance("^(\\??)" REQUIRED_CURRENCY_REGEX "(\\s+Balance.*)$");
        
        // Format: [?][<balance>] ([<sign>]<transAmount>[?])<restOfLine>
        // <prefix>=$1, <balance>=$2, <transAmountInParentheses>=$6 <transAmount>=$8 <restOfLine>=$13
        std::regex regexTransaction("^(\\?*)" OPTIONAL_CURRENCY_REGEX " *((\\(|\\[|\\\\\\[)" REQUIRED_CURRENCY_REGEX ".*(\\)|\\]|\\\\\\]))(.*)$");
        
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
                }
                else if (std::regex_search(newLine, match, regexBalance))
                {
                    int balance = ParseAmount(match[2]);
                    currentBalance = balance;
                }
                else if (currentBalance.has_value() && std::regex_search(newLine, match, regexTransaction))
                {
                    std::string oldBalanceString = match[2];

                    int transAmount = ParseAmount(match[8]);
                    currentBalance = currentBalance.value() + transAmount;
                    std::string newBalanceString = FormatAmount(currentBalance.value());
                    
                    if (oldBalanceString.compare(newBalanceString) != 0)
                    {
                        // $1 is optional '?' prefix, $6 is '(transAmount)', $13 is rest of line after transAmount
                        lineFormat.assign("$1").append("{newBalanceString}").append(" $6$13");
                        newLine = std::regex_replace(newLine, regexTransaction, lineFormat);
                        newLine = std::regex_replace(newLine, std::regex("\\{newBalanceString\\}"), newBalanceString);
                    }
                }
            }

            if (currentLine.compare(newLine) != 0)
            {
                ::SendMessage(curScintilla, SCI_REPLACETARGET, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(newLine.data()));
            }
        }

        ::SendMessage(curScintilla, SCI_ENDUNDOACTION, 0, 0);
    }
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