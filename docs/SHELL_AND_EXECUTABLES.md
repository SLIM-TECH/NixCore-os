# Shell And Executables

## Shell Commands

Current built-ins:

- `ls`
- `cd`
- `pwd`
- `mkdir`
- `touch`
- `rm`
- `cat`
- `nano`
- `echo`
- `clear`
- `help`
- `neofetch`
- `date`
- `whoami`
- `ps`
- `top`
- `df`
- `free`
- `history`
- `run`
- `mkbin`
- `snake`
- `matrix`
- `pianino`
- `nixlang`
- `shutdown`
- `reboot`

## History And Tab

- `Up/Down` cycle command history
- `Tab` attempts completion for shell built-ins
- `Page Up/Page Down` scroll terminal history

## Executable Format

There is currently one supported user executable format:

- text-based `#!nixbin`

Example:

```text
#!nixbin
print Hello from executable
mkdir demo
touch note.txt
write note.txt done
ls
```

Run:

```text
./hello.bin
run hello.bin
```

## Creating New Executables

Use:

```text
mkbin demo.bin
```

Then edit it:

```text
nano demo.bin
```

Then launch:

```text
./demo.bin
```

## NixLang Commands

Supported commands in the interpreter:

- variable assignment: `name = value`
- `print`
- `println`
- `set`
- `add`
- `sub`
- `touch`
- `mkdir`
- `write`
- `append`
- `cat`
- `cd`
- `pwd`
- `ls`
- `exec`
- `shell`
