"""
Logging configuration for the Devana build system.

Provides colored console output and file logging with timestamps.
"""

import logging
import time
from pathlib import Path


class ColoredFormatter(logging.Formatter):
    COLORS = {
        'DEBUG': '\033[36m',
        'INFO': '\033[32m',
        'WARNING': '\033[33m',
        'ERROR': '\033[31m',
        'CRITICAL': '\033[35m',
    }
    RESET = '\033[0m'

    def format(self, record):
        log_color = self.COLORS.get(record.levelname, '')
        record.levelname = f"{log_color}{record.levelname}{self.RESET}"
        return super().format(record)


def setup_logging(logs_dir: Path, level: int = logging.INFO, command: str = "devana") -> Path:
    logs_dir.mkdir(parents=True, exist_ok=True)
    timestamp = time.strftime('%Y-%m-%d_%H-%M-%S')
    log_file = logs_dir / f"devana_{command}_{timestamp}.log"

    logging.basicConfig(
        level=level,
        format="%(asctime)s - %(levelname)s - %(message)s",
        handlers=[
            logging.FileHandler(log_file, mode='w'),
            logging.StreamHandler()
        ]
    )

    console_handler = logging.getLogger().handlers[1]
    console_handler.setFormatter(ColoredFormatter("%(levelname)s - %(message)s"))

    return log_file


def get_logger(name: str) -> logging.Logger:
    return logging.getLogger(name)
