# ams-lang : AutomonScript Language Engine 
This repository houses the compiler, grammar specifications, and the high-performance runtime for the AutomonScript Language.

### Technical Architecture
The `ams-lang` core operates on a transparent transpilation model. <br>
It processes `.ams` scripts into efficient high peformance deployable executable file.

### Repository Structure
* **`/grammar`**: Contains `AMS.g4`, the ANTLR4 grammar definition.
* **`/include/ams/runtime`**: Core C++17 headers for Language Runtime.
* **`/src/compiler`**: Implementation of the AST Visitor and C++ code emitter.
* **`/src/driver`**: Source for the `ams` CLI tool that manages the build-and-run lifecycle.
