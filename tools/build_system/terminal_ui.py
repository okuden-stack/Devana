"""
Enhanced Terminal UI for Devana Build System

Provides rich terminal output with:
- Progress bars
- Spinners
- Tables
- Boxes
- Color themes
- Status indicators
"""

import sys
import time
import shutil
from typing import Optional, List, Dict, Any
from dataclasses import dataclass
from enum import Enum


class Color:
    """ANSI color codes for terminal output"""
    BLACK = '\033[30m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'
    BRIGHT_BLACK = '\033[90m'
    BRIGHT_RED = '\033[91m'
    BRIGHT_GREEN = '\033[92m'
    BRIGHT_YELLOW = '\033[93m'
    BRIGHT_BLUE = '\033[94m'
    BRIGHT_MAGENTA = '\033[95m'
    BRIGHT_CYAN = '\033[96m'
    BRIGHT_WHITE = '\033[97m'
    BOLD = '\033[1m'
    DIM = '\033[2m'
    ITALIC = '\033[3m'
    UNDERLINE = '\033[4m'
    BLINK = '\033[5m'
    REVERSE = '\033[7m'
    RESET = '\033[0m'

    @staticmethod
    def rgb(r: int, g: int, b: int) -> str:
        return f'\033[38;2;{r};{g};{b}m'


class Icon:
    SUCCESS = '✓'
    ERROR = '✗'
    WARNING = '⚠'
    INFO = 'ℹ'
    ARROW = '→'
    BULLET = '•'
    STAR = '★'
    CHECK = '✓'
    CROSS = '✗'
    WRENCH = ''
    HAMMER = ''
    PACKAGE = ''
    FOLDER = ''
    FILE = ''
    ROCKET = ''
    CLOCK = ''
    FIRE = ''
    SPARKLES = ''
    TARGET = ''
    MAGNIFY = ''


class Theme:
    @dataclass
    class ColorScheme:
        primary: str
        secondary: str
        success: str
        warning: str
        error: str
        info: str
        dim: str

    DEFAULT = ColorScheme(
        primary=Color.BRIGHT_CYAN,
        secondary=Color.BRIGHT_BLUE,
        success=Color.BRIGHT_GREEN,
        warning=Color.BRIGHT_YELLOW,
        error=Color.BRIGHT_RED,
        info=Color.BRIGHT_BLUE,
        dim=Color.BRIGHT_BLACK
    )

    MINIMAL = ColorScheme(
        primary=Color.CYAN,
        secondary=Color.BLUE,
        success=Color.GREEN,
        warning=Color.YELLOW,
        error=Color.RED,
        info=Color.BLUE,
        dim=Color.BLACK
    )


class Terminal:
    @staticmethod
    def width() -> int:
        return shutil.get_terminal_size((80, 20)).columns

    @staticmethod
    def height() -> int:
        return shutil.get_terminal_size((80, 20)).lines

    @staticmethod
    def clear_line():
        sys.stdout.write('\r\033[K')
        sys.stdout.flush()

    @staticmethod
    def move_up(lines: int = 1):
        sys.stdout.write(f'\033[{lines}A')
        sys.stdout.flush()

    @staticmethod
    def hide_cursor():
        sys.stdout.write('\033[?25l')
        sys.stdout.flush()

    @staticmethod
    def show_cursor():
        sys.stdout.write('\033[?25h')
        sys.stdout.flush()


class Box:
    TL = '┌'
    TR = '┐'
    BL = '└'
    BR = '┘'
    H = '─'
    V = '│'
    TL_DBL = '╔'
    TR_DBL = '╗'
    BL_DBL = '╚'
    BR_DBL = '╝'
    H_DBL = '═'
    V_DBL = '║'

    @classmethod
    def draw(cls, text: str, width: Optional[int] = None,
             color: str = Color.CYAN, double: bool = False,
             padding: int = 1) -> str:
        lines = text.split('\n')
        if width is None:
            width = max(len(line) for line in lines) + (padding * 2)
        if double:
            tl, tr, bl, br, h, v = cls.TL_DBL, cls.TR_DBL, cls.BL_DBL, cls.BR_DBL, cls.H_DBL, cls.V_DBL
        else:
            tl, tr, bl, br, h, v = cls.TL, cls.TR, cls.BL, cls.BR, cls.H, cls.V
        result = []
        result.append(f"{color}{tl}{h * (width - 2)}{tr}{Color.RESET}")
        for line in lines:
            padded = line.center(width - 2)
            result.append(f"{color}{v}{Color.RESET}{padded}{color}{v}{Color.RESET}")
        result.append(f"{color}{bl}{h * (width - 2)}{br}{Color.RESET}")
        return '\n'.join(result)


class ProgressBar:
    def __init__(self, total: int, width: int = 40,
                 prefix: str = '', theme: Theme.ColorScheme = Theme.DEFAULT):
        self.total = total
        self.current = 0
        self.width = width
        self.prefix = prefix
        self.theme = theme
        self.start_time = time.time()

    def update(self, current: int):
        self.current = current
        self.render()

    def increment(self):
        self.update(self.current + 1)

    def render(self):
        percent = (self.current / self.total) * 100 if self.total > 0 else 0
        filled = int(self.width * self.current / self.total) if self.total > 0 else 0
        bar = '█' * filled + '░' * (self.width - filled)
        elapsed = time.time() - self.start_time
        rate = self.current / elapsed if elapsed > 0 else 0
        eta = (self.total - self.current) / rate if rate > 0 else 0
        Terminal.clear_line()
        sys.stdout.write(
            f"{self.prefix} "
            f"{self.theme.primary}[{self.theme.success}{bar}{self.theme.primary}]{Color.RESET} "
            f"{self.theme.info}{percent:5.1f}%{Color.RESET} "
            f"{self.theme.dim}({self.current}/{self.total}){Color.RESET} "
            f"{self.theme.dim}ETA: {eta:.0f}s{Color.RESET}"
        )
        sys.stdout.flush()

    def finish(self):
        self.update(self.total)
        print()


class Spinner:
    FRAMES = ['⠋', '⠙', '⠹', '⠸', '⠼', '⠴', '⠦', '⠧', '⠇', '⠏']

    def __init__(self, message: str = '', theme: Theme.ColorScheme = Theme.DEFAULT):
        self.message = message
        self.theme = theme
        self.frame = 0
        self.running = False

    def start(self):
        self.running = True
        Terminal.hide_cursor()
        self.render()

    def update(self, message: str):
        self.message = message
        self.render()

    def render(self):
        if not self.running:
            return
        frame = self.FRAMES[self.frame % len(self.FRAMES)]
        Terminal.clear_line()
        sys.stdout.write(f"{self.theme.primary}{frame}{Color.RESET} {self.message}")
        sys.stdout.flush()
        self.frame += 1

    def stop(self, final_message: str = '', success: bool = True):
        self.running = False
        Terminal.clear_line()
        if final_message:
            icon = Icon.SUCCESS if success else Icon.ERROR
            color = self.theme.success if success else self.theme.error
            print(f"{color}{icon}{Color.RESET} {final_message}")
        Terminal.show_cursor()


class Table:
    @staticmethod
    def _strip_ansi(text: str) -> str:
        import re
        return re.compile(r'\x1b\[[0-9;]*m').sub('', text)

    @staticmethod
    def _visible_len(text: str) -> int:
        return len(Table._strip_ansi(str(text)))

    @staticmethod
    def _pad_cell(cell: str, width: int) -> str:
        return cell + (' ' * (width - Table._visible_len(cell)))

    @staticmethod
    def format(headers: List[str], rows: List[List[str]],
               theme: Theme.ColorScheme = Theme.DEFAULT) -> str:
        widths = [Table._visible_len(h) for h in headers]
        for row in rows:
            for i, cell in enumerate(row):
                widths[i] = max(widths[i], Table._visible_len(cell))

        lines = []
        lines.append(f"{theme.dim}{'┌' + '┬'.join('─' * (w + 2) for w in widths) + '┐'}{Color.RESET}")

        header_cells = [f" {Table._pad_cell(h, w)} " for h, w in zip(headers, widths)]
        lines.append(
            f"{theme.dim}│{Color.RESET}"
            f"{theme.primary}{Color.BOLD}{'│'.join(header_cells)}{Color.RESET}"
            f"{theme.dim}│{Color.RESET}"
        )

        lines.append(f"{theme.dim}{'├' + '┼'.join('─' * (w + 2) for w in widths) + '┤'}{Color.RESET}")

        for row in rows:
            cells = [f" {Table._pad_cell(str(cell), w)} " for cell, w in zip(row, widths)]
            lines.append(f"{theme.dim}│{Color.RESET}{'│'.join(cells)}{theme.dim}│{Color.RESET}")

        lines.append(f"{theme.dim}{'└' + '┴'.join('─' * (w + 2) for w in widths) + '┘'}{Color.RESET}")
        return '\n'.join(lines)


class StatusLine:
    @staticmethod
    def success(message: str, theme: Theme.ColorScheme = Theme.DEFAULT):
        print(f"{theme.success}{Icon.SUCCESS}{Color.RESET} {message}")

    @staticmethod
    def error(message: str, theme: Theme.ColorScheme = Theme.DEFAULT):
        print(f"{theme.error}{Icon.ERROR}{Color.RESET} {message}")

    @staticmethod
    def warning(message: str, theme: Theme.ColorScheme = Theme.DEFAULT):
        print(f"{theme.warning}{Icon.WARNING}{Color.RESET} {message}")

    @staticmethod
    def info(message: str, theme: Theme.ColorScheme = Theme.DEFAULT):
        print(f"{theme.info}{Icon.INFO}{Color.RESET} {message}")

    @staticmethod
    def step(number: int, total: int, message: str, theme: Theme.ColorScheme = Theme.DEFAULT):
        print(f"{theme.primary}[{number}/{total}]{Color.RESET} {message}")


class Banner:
    @staticmethod
    def show(title: str, version: str = '', subtitle: str = '',
             theme: Theme.ColorScheme = Theme.DEFAULT):
        width = Terminal.width()
        print()
        print(f"{theme.primary}{'═' * width}{Color.RESET}")
        print(f"{theme.primary}{Color.BOLD}{title.center(width)}{Color.RESET}")
        if version:
            print(f"{theme.secondary}{f'v{version}'.center(width)}{Color.RESET}")
        if subtitle:
            print(f"{theme.dim}{subtitle.center(width)}{Color.RESET}")
        print(f"{theme.primary}{'═' * width}{Color.RESET}")
        print()


def format_duration(seconds: float) -> str:
    if seconds < 1:
        return f"{seconds * 1000:.0f}ms"
    elif seconds < 60:
        return f"{seconds:.2f}s"
    else:
        mins = int(seconds // 60)
        secs = seconds % 60
        return f"{mins}m {secs:.0f}s"


def format_size(bytes: int) -> str:
    for unit in ['B', 'KB', 'MB', 'GB']:
        if bytes < 1024:
            return f"{bytes:.1f}{unit}"
        bytes /= 1024
    return f"{bytes:.1f}TB"


def success(msg: str): StatusLine.success(msg)
def error(msg: str): StatusLine.error(msg)
def warning(msg: str): StatusLine.warning(msg)
def info(msg: str): StatusLine.info(msg)


def header(text: str, char: str = '=', color: str = Color.CYAN):
    width = Terminal.width()
    print(f"\n{color}{char * width}{Color.RESET}")
    print(f"{color}{Color.BOLD}{text}{Color.RESET}")
    print(f"{color}{char * width}{Color.RESET}\n")
