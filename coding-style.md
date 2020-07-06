
# Coding style

The DRAMSys project has long run without a coding style. As a result, there is
a substantial amount of code which does not meet any coding style guidelines.

This fact leads people to believe that the coding standards do not matter and
are not enforced. Nevertheless, a code base as large as the DRAMSys project
requires some uniformity of code to make it possible for developers to quickly
understand it. So there is no longer room for strangely-formatted code.

The coding style document should not be read as an absolute law which can
never be transgressed. **If there is a good reason** to go against the style
(a line which becomes far less readable if split to fit within the 80-column
limit, for example), just do it.

Since DRAMSys is a multi-language project, each with its own peculiarities, we
describe the preferred coding style for them in separate.

## Tools

```bash
sudo apt-get -y install astyle
sudo apt-get -y install uncrustify
sudo apt-get -y install clang-format
```

**Hint:**
You can use [install_deb.sh](./utils/install_deb.sh) in order to install dependencies.
First read and understand the script then execute it. Type your password if
required.

## DRAMSys Coding Style for C++ Code

#### Indentation
4 spaces are used for indentation.

#### Breaking long lines and strings
Coding style is all about readability and maintainability using commonly
available tools.

The limit on the length of lines is 80 columns and this is a strongly
preferred limit.

Statements longer than 80 columns will be broken into sensible chunks, unless
exceeding 80 columns significantly increases readability and does not hide
information. Descendants are always substantially shorter than the parent and
are placed substantially to the right. The same applies to function headers
with a long argument list. However, never break user-visible strings such as
printed messages, because that breaks the ability to grep for them.

#### Declaring variables
Declare each variable on a separate line.

Avoid short or meaningless names (e.g., "a", "b", "aux").

Wait when declaring a variable until it is needed.

```c++
// Wrong
int a, b;
char *c, *d;

// Correct
int height;
int width;
char *nameOfThis;
short counter;
char itemDelimiter = ' ';
```

Classes always start with an upper-case letter (e.g, MemoryManager,
TracePlayer, PythonCaller, etc.).
Acronyms are camel-cased (e.g., TlmRecorder, not TLMRecorder, Dram, not DRAM),
being **DRAMSys** a notable exception to the rule.

#### Whitespace
Always use a single space after a keyword and before a curly brace:

```c++
// Wrong
if(foo){
}

// Correct
if (foo) {
}
```

For pointers or references, always use a single space between the type and
'\*' or '&', but no space between the '\*' or '&' and the variable name:

```c++
char *x;
const QString &myString;
const char * const y = "hello";
```

Surround binary operators with spaces.

No space after a cast.

Avoid C-style casts when possible.

```c++
// Wrong
char* blockOfMemory = (char* ) malloc(data.size());

// Correct
char *blockOfMemory = reinterpret_cast<char *>(malloc(data.size()));
```

Do not put multiple statements on one line.

Use a new line for the body of a control flow statement:

```
// Wrong
if (foo) bar();

// Correct
if (foo)
    bar();
```

## DRAMSys Coding Style for Python Code

We follow the PEP8 style guide.

**Hint:**
There is a plugin for VIM. More information can be found in
[vim.org](https://www.vim.org/scripts/script.php?script_id=2914).

## Using Astyle in Qt Creator

+ Select **Help** > **About Plugins** > **C++** > **Beautifier** to enable the
  plugin.

+ Restart Qt Creator to be able to use the plugin (close Qt Creator and open
  it again).

+ Select **Tools** > **Options** > **Beautifier** to specify settings for
  beautifying files.

+ Select the **Enable auto format on file save** check box to automatically
  beautify files when you save them.

## Applying the Coding Style

The script [make_pretty.sh](./utils/make_pretty.sh) applies the coding style
to the project excluding third party code.

## References

[Linux kernel coding
style](https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst)

[C++ Core
Guidelines](https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-cpl)

[PEP 8 -- Style Guide for Python
Code](https://www.python.org/dev/peps/pep-0008/)

[Qt Documentation - Beautifying Source Code](http://doc.qt.io/qtcreator/creator-beautifier.html)

[Artistic Style 3.1 - A Free, Fast, and Small Automatic Formatter for C, C++, C++/CLI, Objective‑C, C#, and Java Source Code](http://astyle.sourceforge.net/astyle.html)

[Qt Coding Style](https://wiki.qt.io/Qt_Coding_Style)

[Qt Coding Conventions](https://wiki.qt.io/Coding_Conventions)

