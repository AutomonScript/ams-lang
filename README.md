# ams-lang : AutomonScript Language Engine 
This repository houses the compiler, grammar specifications, and the high-performance runtime for the AutomonScript Language.

## ABOUT 
### Technical Architecture
The `ams-lang` core operates on a transparent transpilation model. <br>
It processes `.ams` scripts into efficient high peformance deployable executable file.

### Repository Structure
* **`/grammar`**: Contains AMS.g4, the ANTLR4 grammar definition.
* **`/include/core/builder`**: Core C++17 headers for Language Runtime.
* **`/include/core/compiler`**: Provides asm build (Compiled Execution)
* **`/include/runtime`**: Core C++17 headers for Language Runtime.
* **`/include/stdlib`**: Core C++17 headers for Language Runtime.
* **`/src/driver`**: Source for the ams CLI tool that manages the build-and-run lifecycle.

## Setup of ams-lang project Enviroment 
### Windows : (Using PowerShell)
+ STEP 1: Install Dependencies for Project 
    ```powershell
    .\install_dependency.ps1 
    ```
+ STEP 2: Build to produce asm.exe (Language Engine)
    ```powershell
    .\ams_install_windows.ps1
    ```
## Use AutomonScript Language (AMS-Lang Engine)
+ View Available Execution Options
    ```powershell
    ams 
    ```
+ Builds AutomonScript Code in Compi
    ```powershell
    ams build .\examples\hello_world.ams
    .\examples\hello_world.exe   
    ```

