# NppRossToolsCpp - A Notepad++ Plugin written in C++

## Tools

- **Remove Trailing Spaces**

  This will trim all spaces at the end of every line.

- **Update Ages**

  This will update every line in the current document that starts with a birthdate (in `MM/dd/yyyy` format) that has `(# in yyyy)` somewhere on the line, which should reflect their age for that year.
  
  For example, if you had a document like this:
  
  ```
  Celebrities
  
  05/04/1928 Maynard Ferguson (94 in 2022) (died at 78 on 08/23/2006)
  05/31/1930 Clint Eastwood (92 in 2022)
  06/09/1961 Michael J. Fox (61 in 2022)
  ```
  
  Then after running `Update Ages` in 2023 the document should look like this:
  
  ```
  Celebrities
  
  05/04/1928 Maynard Ferguson (95 in 2023) (died at 78 on 08/23/2006)
  05/31/1930 Clint Eastwood (93 in 2023)
  06/09/1961 Michael J. Fox (62 in 2023)
  ```
  
- **Update Line Balances**

  This will update the balance on each transaction line in the current document.
  
  For example, if you had a document like this:
  
  ```
  Checking Account
  
  (-10.82) 06/11 Sonic
  63.28 (-25.32) 06/03 Hobby Lobby
  88.60 (-17.30) 06/01 Walmart
  105.90 Balance
  ```
  
  Then after running `Update Line Balances` the document should look like this:
  
  ```
  Checking Account
  
  52.46 (-10.82) 06/11 Sonic
  63.28 (-25.32) 06/03 Hobby Lobby
  88.60 (-17.30) 06/01 Walmart
  105.90 Balance
  ```
  
## Development Environment

  Notepad++ - v8.6.7 (64-bit)  
  Visual Studio 2022 Preview - v17.11 Preview 2.1  

  Building the project in Visual Studio should add (or update) `NppRossToolsCpp\NppRossToolsCpp.dll` under `C:\Program Files\Notepad++\plugins`, which will add `Ross Tools` under `Plugins` when you run Notepad++.  

## References

  https://npp-user-manual.org/docs/plugins/#how-to-develop-a-plugin  
  https://npp-user-manual.org/docs/plugin-communication/  
  https://github.com/npp-plugins/plugindemo  