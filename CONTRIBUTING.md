# Contributing to AutomonScript Language Engine (ams-lang)

First off, thank you for considering contributing to **ams-lang**! 
It's developers like you that make open-source language tools powerful, fast, and accessible. 

Whether you're proposing new language features, fixing bugs, improving our documentation, or optimizing the engine, your help is highly appreciated. This document outlines the process to ensure a smooth and collaborative experience for everyone.

## 1. Getting Started

1. Fork the repository or clone it to your local machine.
    ```
    git clone https://github.com/AutomonScript/ams-lang.git
    ```
2. To work on the `ams-lang` engine, you will need the following installed in your local development environment. 

    Use the given scripts to install the required dependencies.

    ```powershell
    .\install_dependency.ps1
    ```
3. Run the installation script to build the project. On Windows, you can use the provided PowerShell script to perform a clean build. (It also sets up the environment variable `ams` pointing to `ams.exe`).
    ```powershell
    .\ams_install_windows.ps1
    ```
4. Use `ams` commands on your system. 
    ```powershell
    ams build .\examples\hello_world.ams
    .\examples\hello_world.exe   
    ```

## 2. Documentation & About the Project 
If you are new to the codebase, the first step is to learn about the project structure at `docs/PROJECT_STRUCTURE.md`. After understanding the directory layout, you can easily navigate the codebase. Then, read the developer documentation as required at `./docs`.

## 3. How to Contribute

### Submitting Pull Requests
1. **Fork the repository** and create a new branch from `main`.

2. **Branch naming**: Use descriptive names like `feature/<featureName>` or `bugfix/<fixName>`.

3. **Make your changes**, following the project structure and standards.
    
4. **Test your code**: Test using `./test` and `./examples`.

5. **Open a PR**: Describe your changes clearly, link to any relevant issues, and request a review! 

### Suggesting Features / Reporting Bugs
Follow the given templates to open an issue:<br>
`feature`, `bug`, `task`, `documentation`, `help`, `others`

**Feature Suggestion**<br>
When suggesting a feature, include the following information in your issue:
* The feature description (using Gherkin is preferred).
* How it will improve AutomonScript Language.

**Reporting Bugs**<br>
When reporting bugs, include the following information in your issue:
* A minimal `.ams` code snippet that triggers the bug.
* The expected vs. actual output.
* The OS and C++ compiler version you are using.
* Any information that will help the developers fix it.