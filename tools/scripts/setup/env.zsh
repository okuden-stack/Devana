#!/usr/bin/env zsh
# Devana Environment Configuration
# Sourced by direnv when entering the project directory

# ============================================================================
# Environment Variables
# ============================================================================

# Core paths
export DEVANA_SRC_DIR="${DEVANA_ROOT}/src"
export DEVANA_BUILD_DIR="${DEVANA_ROOT}/build"
export DEVANA_TOOLS_DIR="${DEVANA_ROOT}/tools"
export DEVANA_LOGS_DIR="${DEVANA_ROOT}/logs"
export DEVANA_CACHE_DIR="${DEVANA_ROOT}/.build_cache"

# Add devana script to PATH
export PATH="${DEVANA_ROOT}/tools:${PATH}"

# ============================================================================
# Core Build Aliases
# ============================================================================

# Basic commands
alias db='devana build'
alias dbr='devana rebuild'
alias dc='devana clean'
alias dca='devana cleanall'
alias dd='devana deploy'
alias dt='devana test'
alias dh='devana help'
alias ds='devana setup'

# Build variations
alias dbd='devana build --debug'
alias dbdr='devana rebuild --debug'
alias dbv='devana build --verbose'
alias dbnc='devana build --no-cache'
alias dbdnc='devana build --debug --no-cache'

# Release builds
alias dbrel='devana build --release'
alias dbrrel='devana rebuild --release'

# Setup variations
alias dsr='devana setup --reconfigure'
alias dsrd='devana setup --reconfigure --debug'

# Cache management
alias dcache='devana cache'
alias dcclear='devana clearcache'

# ============================================================================
# Quick Navigation Aliases
# ============================================================================

alias cdd='cd $DEVANA_ROOT'
alias cdds='cd $DEVANA_SRC_DIR'
alias cddb='cd $DEVANA_BUILD_DIR'
alias cddl='cd $DEVANA_LOGS_DIR'
alias cddc='cd $DEVANA_CACHE_DIR'
alias cddui='cd $DEVANA_SRC_DIR/ui'

# ============================================================================
# Log Viewing Aliases
# ============================================================================

alias dlog='tail -f $DEVANA_LOGS_DIR/build.log'
alias dlogv='less $DEVANA_LOGS_DIR/build.log'
alias dlogc='cat $DEVANA_LOGS_DIR/build.log'
alias dloge='grep -i error $DEVANA_LOGS_DIR/build.log'
alias dlogw='grep -i warning $DEVANA_LOGS_DIR/build.log'
alias dlogclear='> $DEVANA_LOGS_DIR/build.log && echo "Build log cleared"'

# ============================================================================
# File Finding Aliases
# ============================================================================

alias dfindcpp='find $DEVANA_SRC_DIR -name "*.cpp" -o -name "*.h" -o -name "*.hpp"'
alias dfindui='find $DEVANA_SRC_DIR -name "*.ui"'
alias dfindqrc='find $DEVANA_SRC_DIR -name "*.qrc"'
alias dfindqss='find $DEVANA_SRC_DIR -name "*.qss"'
alias dfindcmake='find $DEVANA_ROOT -name "CMakeLists.txt"'

# ============================================================================
# Git Aliases (Project-specific)
# ============================================================================

alias dgst='git status'
alias dgd='git diff'
alias dgds='git diff --staged'
alias dglog='git log --oneline --graph --decorate --all'
alias dgp='git push'
alias dgpl='git pull'
alias dgc='git commit -m'
alias dga='git add'
alias dgaa='git add -A'

# ============================================================================
# Smart Functions
# ============================================================================

# Quick build and run
dbuild-run() {
    echo "Building project..."
    devana build "$@" && {
        echo "Launching application..."
        devana deploy
    }
}

# Build with timing
dbuild-time() {
    local start=$(date +%s)
    devana build "$@"
    local end=$(date +%s)
    local duration=$((end - start))
    echo " Total time: ${duration}s"
}
alias dbt='dbuild-time'

# Watch and rebuild on changes
dwatch() {
    local target="${1:-src}"
    local watch_path="$DEVANA_ROOT/$target"

    if [[ ! -d "$watch_path" ]]; then
        echo "Directory not found: $watch_path"
        return 1
    fi

    echo "Watching $target for changes..."
    echo "   Path: $watch_path"
    echo "   Press Ctrl+C to stop"
    echo ""

    if [[ "$OSTYPE" == "darwin"* ]]; then
        if ! command -v fswatch &> /dev/null; then
            echo "fswatch not found. Install it with:"
            echo "   brew install fswatch"
            return 1
        fi

        fswatch -o "$watch_path" | while read -r num; do
            echo "Change detected, rebuilding..."
            devana build --verbose
            echo ""
            echo "Watching for more changes..."
        done
    else
        if ! command -v inotifywait &> /dev/null; then
            echo "inotifywait not found. Install it with:"
            echo "   Ubuntu/Debian: sudo apt install inotify-tools"
            echo "   Fedora/RHEL:   sudo dnf install inotify-tools"
            return 1
        fi

        while true; do
            inotifywait -r -e modify,create,delete "$watch_path" 2>/dev/null
            echo "Change detected, rebuilding..."
            devana build --verbose
            echo ""
            echo "Watching for more changes..."
        done
    fi
}

# Show project structure
dtree() {
    tree -I 'build|.git|__pycache__|.build_cache|*.pyc' -L "${1:-3}" "$DEVANA_ROOT"
}

# Count lines of code
dloc() {
    echo "Lines of Code Statistics:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    cloc "$DEVANA_SRC_DIR" --exclude-dir=build,.git,__pycache__,.build_cache 2>/dev/null || {
        echo "C++ files:"
        find "$DEVANA_SRC_DIR" -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | xargs wc -l | tail -1
        echo "UI files:"
        find "$DEVANA_SRC_DIR" -name "*.ui" | xargs wc -l 2>/dev/null | tail -1
    }
}

# Show build artifacts
dartifacts() {
    echo "Build Artifacts:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    if [[ -d "$DEVANA_BUILD_DIR" ]]; then
        ls -lh "$DEVANA_BUILD_DIR"
        echo ""
        du -sh "$DEVANA_BUILD_DIR"
    else
        echo "Build directory not found"
    fi
}

# Clean and show what was removed
dclean-verbose() {
    echo "Cleaning build artifacts..."
    du -sh "$DEVANA_BUILD_DIR" 2>/dev/null && {
        local size=$(du -sh "$DEVANA_BUILD_DIR" | cut -f1)
        echo "Removing $size of build data..."
    }
    devana cleanall
    echo "Cleanup complete"
}
alias dcv='dclean-verbose'

# Show cache statistics
dcache-stats() {
    echo "Cache Statistics:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    devana cache
    if [[ -d "$DEVANA_CACHE_DIR" ]]; then
        echo ""
        echo "Cache directory size:"
        du -sh "$DEVANA_CACHE_DIR"
    fi
}

# Search in source files
dsearch() {
    if [[ -z "$1" ]]; then
        echo "Usage: dsearch <pattern>"
        return 1
    fi
    grep -r --include="*.cpp" --include="*.h" --include="*.hpp" --include="*.ui" \
         --color=always -n "$1" "$DEVANA_SRC_DIR"
}

# Search and replace in source files (with confirmation)
dreplace() {
    if [[ $# -lt 2 ]]; then
        echo "Usage: dreplace <old_pattern> <new_pattern>"
        return 1
    fi

    local old_pattern="$1"
    local new_pattern="$2"

    echo "Searching for '$old_pattern'..."
    local matches=$(grep -rl --include="*.cpp" --include="*.h" --include="*.hpp" \
                    "$old_pattern" "$DEVANA_SRC_DIR")

    if [[ -z "$matches" ]]; then
        echo "No matches found"
        return 0
    fi

    echo "Found matches in:"
    echo "$matches"
    echo ""
    echo -n "Replace all occurrences? (y/N) "
    read -r response

    if [[ "$response" =~ ^[Yy]$ ]]; then
        echo "$matches" | while read -r file; do
            sed -i '' "s/$old_pattern/$new_pattern/g" "$file"
            echo "Updated: $file"
        done
        echo "Replacement complete"
    else
        echo "Cancelled"
    fi
}

# Show component information
dcomponents() {
    echo "Project Components:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    for dir in "$DEVANA_SRC_DIR"/*; do
        if [[ -d "$dir" ]] && [[ ! "$dir" =~ (build|\.git|__pycache__|\.build_cache) ]]; then
            local name=$(basename "$dir")
            local cpp_count=$(find "$dir" -name "*.cpp" | wc -l)
            local h_count=$(find "$dir" -name "*.h" -o -name "*.hpp" | wc -l)
            local ui_count=$(find "$dir" -name "*.ui" | wc -l)

            printf "%-20s CPP: %-3d  Headers: %-3d  UI: %-3d\n" \
                   "$name" "$cpp_count" "$h_count" "$ui_count"
        fi
    done
}

# Quick Qt Designer launch
ddesigner() {
    local ui_file="$1"
    if [[ -z "$ui_file" ]]; then
        echo "Available UI files:"
        find "$DEVANA_SRC_DIR" -name "*.ui" -exec basename {} \;
        return 0
    fi

    local full_path=$(find "$DEVANA_SRC_DIR" -name "$ui_file" | head -1)
    if [[ -n "$full_path" ]]; then
        designer "$full_path" &
    else
        echo "UI file not found: $ui_file"
    fi
}

# Show CMake configuration
dconfig() {
    echo " CMake Configuration:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    if [[ -f "$DEVANA_BUILD_DIR/CMakeCache.txt" ]]; then
        grep -E "^(CMAKE_BUILD_TYPE|CMAKE_CXX_COMPILER|Qt6)" "$DEVANA_BUILD_DIR/CMakeCache.txt"
    else
        echo "Build not configured. Run 'ds' first."
    fi
}

# Quick dependency check
ddeps() {
    echo "Checking Dependencies:"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"

    # Check CMake
    if command -v cmake &> /dev/null; then
        echo "CMake: $(cmake --version | head -1)"
    else
        echo "CMake: Not found"
    fi

    # Check Qt6
    if command -v qmake6 &> /dev/null || command -v qmake &> /dev/null; then
        echo "Qt6: Installed"
        qmake6 --version 2>/dev/null || qmake --version
    else
        echo "Qt6: Not found"
    fi

    # Check Ninja (optional)
    if command -v ninja &> /dev/null; then
        echo "Ninja: $(ninja --version)"
    else
        echo " Ninja: Not found (using Make)"
    fi

    # Check Python
    echo "Python: $(python3 --version)"

    # Check pkg-config
    if command -v pkg-config &> /dev/null; then
        echo "pkg-config: $(pkg-config --version)"
    else
        echo "pkg-config: Not found"
    fi

    # Check ONNX Runtime (Devana-specific)
    if pkg-config --exists onnxruntime 2>/dev/null; then
        echo "ONNX Runtime: $(pkg-config --modversion onnxruntime)"
    else
        echo " ONNX Runtime: Not found via pkg-config (may still be installed)"
    fi
}

# Diagnostic function to check Devana setup
ddiag() {
    echo "Devana Environment Diagnostics"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""

    # Check DEVANA_ROOT
    echo "DEVANA_ROOT:"
    if [[ -n "$DEVANA_ROOT" ]]; then
        echo "   Set: $DEVANA_ROOT"
        if [[ -d "$DEVANA_ROOT" ]]; then
            echo "   Directory exists"
        else
            echo "   Directory does not exist!"
        fi
    else
        echo "   Not set! Are you in the Devana directory?"
    fi
    echo ""

    # Check devana script
    echo "Devana script:"
    if [[ -n "$DEVANA_ROOT" ]] && [[ -f "$DEVANA_ROOT/tools/devana" ]]; then
        echo "   Found: $DEVANA_ROOT/tools/devana"
        if [[ -x "$DEVANA_ROOT/tools/devana" ]]; then
            echo "   Executable"
        else
            echo "   Not executable!"
            echo "   Fix: chmod +x $DEVANA_ROOT/tools/devana"
        fi
    else
        echo "   Not found at $DEVANA_ROOT/tools/devana"
    fi
    echo ""

    # Check PATH
    echo " PATH configuration:"
    if command -v devana &> /dev/null; then
        echo "   'devana' command found in PATH"
        echo "   Location: $(which devana)"
    else
        echo "   'devana' command not in PATH"
    fi
    echo ""

    # Check direnv
    echo "direnv setup:"
    if command -v direnv &> /dev/null; then
        echo "   direnv installed"
        if [[ -f "$DEVANA_ROOT/.envrc" ]]; then
            echo "   .envrc file exists"
        else
            echo "   .envrc file not found"
        fi
    else
        echo "   direnv not installed"
    fi
    echo ""

    # Check project structure
    echo "Project structure:"
    if [[ -d "$DEVANA_SRC_DIR" ]]; then
        echo "   src/ directory exists"
    else
        echo "    src/ directory not found (will be created on first build)"
    fi

    if [[ -d "$DEVANA_BUILD_DIR" ]]; then
        echo "   build/ directory exists"
    else
        echo "    build/ directory not found (will be created on first build)"
    fi
    echo ""

    # Summary
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    if command -v devana &> /dev/null; then
        echo "Setup looks good! Try: db"
    else
        echo "Setup incomplete. Run direnv allow in the project root."
    fi
}

# Show recent build history
dbuild-history() {
    local count="${1:-10}"
    echo "Recent Build History (last $count):"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    tail -n "$count" "$DEVANA_LOGS_DIR/build.log" | grep -E "(Starting|completed|failed)" || \
        echo "No recent build history found"
}

# Help function
dhelp() {
    cat << 'EOF'
 Devana Shell Environment Help
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

BUILD COMMANDS:
  db              Build project
  dbr             Rebuild from scratch
  dbd             Build in debug mode
  dbv             Build with verbose output
  dbnc            Build without cache
  dd              Deploy (run) application
  dt              Run tests
  dc              Clean build directory
  dca             Clean all artifacts
  ds              Setup build
  dsr             Setup with reconfigure

NAVIGATION:
  cdd             Go to project root
  cdds            Go to src/
  cddb            Go to build/
  cddl            Go to logs/
  cddui           Go to src/ui/

LOGS:
  dlog            Tail build log
  dlogv           View build log
  dloge           Show errors from log
  dlogw           Show warnings from log

UTILITIES:
  dcache          Show cache info
  dcomponents     List all components
  ddeps           Check dependencies
  ddiag           Diagnose setup issues
  dtree           Show project tree
  dloc            Count lines of code
  dsearch <pat>   Search in source files
  dreplace <o> <n> Find and replace
  ddesigner [f]   Launch Qt Designer
  dwatch [dir]    Auto-rebuild on changes

GIT:
  dgst            Git status
  dgd             Git diff
  dglog           Git log (pretty)

Type 'dhelp' anytime to see this help
EOF
}

# Source local overrides (gitignored, for developer-specific settings)
if [[ -f "$DEVANA_ROOT/tools/scripts/setup/env.local.zsh" ]]; then
    source "$DEVANA_ROOT/tools/scripts/setup/env.local.zsh"
fi
