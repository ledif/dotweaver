# DotWeaver

**A modern dotfile management application powered by chezmoi**

DotWeaver provides an intuitive graphical interface for managing your configuration files (dotfiles) using the powerful [chezmoi](https://chezmoi.io) backend. Built with Qt6 and KDE Frameworks for a native Linux desktop experience.

> **Note**: DotWeaver is a community project that uses KDE Frameworks for its UI components. It is not an official KDE project, but follows KDE's Human Interface Guidelines to provide a consistent desktop experience.

## Features

- **Intuitive GUI**: Clean, KDE-native interface for managing dotfiles
- **File Management**: Tree view of all managed dotfiles with status indicators
- **Template Support**: Visual editing of chezmoi templates with syntax highlighting
- **Sync Operations**: Easy synchronization of dotfiles across systems
- **Configuration**: GUI for chezmoi configuration management
- **Live Preview**: See changes before applying them
- **Git Integration**: Built-in support for version control operations

## Installation

### Flatpak (Recommended)

```bash
# Build and install locally
just flatpak-install
```

### From Source

#### Prerequisites

- Qt6 (6.4.0 or later)
- KDE Frameworks 6
- CMake (3.20 or later)
- C++20 compatible compiler
- chezmoi (installed separately)

#### Build Instructions

```bash
# Setup development environment (requires podman)
just setup

# Build the application
just build

# Run tests
just test

# Install
sudo make install
```

## Development

### Development Environment

This project uses a containerized development environment with podman for reproducible builds:

```bash
# Start development shell
just dev-shell

# Build in container
just build

# Format code
just format

# Run linting
just lint
```

### Project Structure

```
src/
├── main.cpp              # Application entry point
├── mainwindow.{h,cpp}    # Main application window
├── chezmoiservice.{h,cpp} # chezmoi backend interface
├── dotfilemanager.{h,cpp} # File tree model and management
└── configeditor.{h,cpp}   # Configuration editor widget

tests/                    # Unit tests
icons/                    # Application icons
Containerfile.dev         # Development container
org.kde.kchezmoi.yml     # Flatpak manifest
justfile                 # Build automation
```

### Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes in the containerized environment
4. Run tests: `just test`
5. Format code: `just format`
6. Submit a pull request

## Usage

1. **First Time Setup**: If chezmoi isn't initialized, KChezmoi will guide you through the setup process
2. **File Management**: Use the tree view to browse your managed dotfiles
3. **Editing**: Double-click files to open them in your configured editor
4. **Templates**: Template files are marked with special icons and can be previewed
5. **Synchronization**: Use the sync button to apply changes across systems
6. **Configuration**: Access chezmoi settings through the preferences dialog

## Requirements

- chezmoi (2.0.0 or later) - must be installed separately
- KDE Plasma 5.24+ or any Qt6-compatible desktop environment
- Linux (tested on Fedora, should work on other distributions)

## License

This project is licensed under the GPL v3 - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [chezmoi](https://github.com/twpayne/chezmoi) - The excellent dotfile manager that powers this application
- Icon: https://www.flaticon.com/free-icon/weave_18878245
- Qt Project - For the robust GUI toolkit
