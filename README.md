# YgoMasterArchiveTool
A small tool for quickly managing archives of Offline Yu-Gi-Oh! Master Duel (PC) `https://github.com/pixeltris/YgoMaster.git`


---

# Functions
- Backup the archive of YgoMaster
- Restore the archive of YgoMaster


----

# Usage
- Download and copy the tool to 
    - a sub directory of the YgoMaster directory (recommended method, the sub directory name can be self selected)
    - or the YgoMaster directory
    - or the same directory as YgoMaster directory

- Run the exe tool and enter commands as prompted
- Follow the prompts to enter the corresponding numerical commands


---

# Build
### 1. Pull code
    > `git clone --recursive https://github.com/fcyctop/YgoMasterArciveTool.git`
### 2. Build
    - If the Visual Studio C++compiler is installed
        - run `win_fuild_stcript.bat`

    - Using cmake, run in project directory
        > - `mkdir build`
        > - `cd build`
        > - `cmake ..`

    