# ams-lang : AutomonScript Language Engine 
This repository houses the compiler, grammar specifications, and the high-performance runtime for the AutomonScript Language.

## ABOUT 
### Technical Architecture
The `ams-lang` core operates on a transparent transpilation model. <br>
It processes `.ams` scripts into efficient high peformance deployable executable file.

### Repository Structure
* **`/grammar`**: Contains AMS.g4, the ANTLR4 grammar definition.
* **`/include/ams/runtime`**: Core C++17 headers for Language Runtime.
* **`/include/ams/compiler`**: Provides asm build (Compiled Execution)
* **`/include/ams/interpretor`**: Provides asm run (Interpreted Execution)
* **`/src/compiler`**: Implementation of the AST Visitor.
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
+ Execute AutomonScript Code in Intrepeted Mode
    ```powershell
    ams run .\examples\hello_world.ams
    ```
+ Execute AutomonScript Code in Compiled Mode
    ```powershell
    ams build .\examples\hello_world.ams
    .\examples\hello_world.exe   
    ```

