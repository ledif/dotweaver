# DotWeaver - A modern dotfile management application powered by chezmoi
# Build and development management with justfile

# Default recipe - show available commands
default:
    @just --list

# Build the application in container
build:
    @echo "Building DotWeaver in container..."
    podman build -t dotweaver-dev -f Containerfile.dev .
    podman run --rm -v $(pwd):/workspace:Z -w /workspace dotweaver-dev just _build-native

# Internal build command (runs inside container)
_build-native:
    mkdir -p build
    cd build && cmake .. -DCMAKE_BUILD_TYPE=Debug
    cd build && make -j$(nproc)

# Build release version
build-release:
    @echo "Building DotWeaver release in container..."
    podman build -t dotweaver-dev -f Containerfile.dev .
    podman run --rm -v $(pwd):/workspace:Z -w /workspace dotweaver-dev just _build-release-native

_build-release-native:
    mkdir -p build-release
    cd build-release && cmake .. -DCMAKE_BUILD_TYPE=Release
    cd build-release && make -j$(nproc)

# Run the application
run: build
    @echo "Running DotWeaver..."
    ./build/src/dotweaver

# Clean build artifacts
clean:
    rm -rf build build-release

# Format code
format:
    @echo "Formatting code with clang-format..."
    find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# Run tests
test: build
    @echo "Running tests..."
    cd build && ctest --verbose

# Development shell in container
dev-shell:
    @echo "Starting development shell..."
    podman build -t dotweaver-dev -f Containerfile.dev .
    podman run -it --rm -v $(pwd):/workspace:Z -w /workspace dotweaver-dev bash

# Build Flatpak
flatpak-build:
    @echo "Building Flatpak..."
    flatpak-builder --force-clean build-flatpak io.github.ledif.dotweaver.yml

# Install Flatpak locally
flatpak-install:
    @echo "Installing Flatpak locally..."
    flatpak-builder --install --user --force-clean build-flatpak io.github.ledif.dotweaver.yml

# Lint code
lint:
    @echo "Linting code..."
    podman run --rm -v $(pwd):/workspace:Z -w /workspace kchezmoi-dev just _lint-native

_lint-native:
    find src -name "*.cpp" -o -name "*.h" | xargs clang-tidy

# Setup development environment
setup:
    @echo "Setting up development environment..."
    @echo "Ensure you have podman installed and SELinux configured for development"
    podman build -t kchezmoi-dev -f Containerfile.dev .

# Generate compilation database for IDEs
compile-db: build
    @echo "Generating compile_commands.json..."
    cd build && cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    ln -sf build/compile_commands.json .
