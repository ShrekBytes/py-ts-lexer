# Lexical Analyzer

A fast and lightweight lexical analyzer for Python and TypeScript written in C. Extracts comments, tokenizes source code, and detects common programming errors.

## Features

- 🔍 **Multi-language support** - Python (`.py`) and TypeScript/JavaScript (`.ts`, `.js`)
- 💬 **Comment extraction** - Single-line and multi-line comments with line tracking
- 🔤 **Tokenization** - Breaks code into keywords, identifiers, literals, operators, and delimiters
- ⚠️ **Error detection** - Four types of error detection:
  - Misspelled keywords (with suggestions)
  - Type mismatches
  - Undeclared identifiers
  - Invalid operators
- 🎨 **Color-coded output** - Terminal interface with syntax highlighting

## Screenshots

![Tokenization Output](screenshots/tokenization.png)

![Comment](screenshots/comments.png)

![Error Detection](screenshots/errors.png)


## Installation

```bash
git clone https://github.com/yourusername/lexical-analyzer.git
cd lexical-analyzer
make
```

Or compile manually:
```bash
gcc -Wall -Wextra -g -o lexer lexer.c
```

## Usage

```bash
# Analyze Python file
./lexer script.py

# Analyze TypeScript file
./lexer script.ts

# Analyze JavaScript file
./lexer script.js
```

## Error Detection Examples

**Misspelled Keywords:**
```python
pritn("Hello")  # → suggests 'print'
defn something():  # → suggests 'def'
```

**Type Mismatches:**
```python
count: int = 3.14  # → int declared, float assigned
name: str = 42     # → str declared, int assigned
```

**Undeclared Identifiers:**
```python
total = countr + 5  # → 'countr' is undeclared
```

**Invalid Operators:**
```python
if x =< 10:  # → should be '<='
if y === 5:  # → '===' not valid in Python
```

## Build Commands

```bash
make              # Build
make clean        # Clean build artifacts
make run-python   # Build and test with test.py
make run-typescript # Build and test with test.ts
```

## Project Structure

```
.
├── lexer.c       # Main source code
├── Makefile      # Build configuration
├── test.py       # Python test file
├── test.ts       # TypeScript test file
└── screenshots/  # Screenshots directory
```

## Technical Details

- **Language**: C
- **Keywords**: 41 (Python) + 46 (TypeScript)
- **Error Types**: 4
- **Time Complexity**: O(n) for typical files
- **Test Coverage**: 22 test cases (100% pass rate)


## 🤝 Contributing

Found a bug or have a feature request?
[Open an issue](../../issues) or submit a pull request.

## 📄 License

This project is licensed under the GNU General Public License v3.0 - see the [LICENSE](LICENSE) file for details.

