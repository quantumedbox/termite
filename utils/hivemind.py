"""Termite program host which interpret output of programs and allows them operate together

  Hivemind listens to special return code 0x98 that signals that < output has command sequences embedded

  Magic byte sequence that starts every command is 98 7F

  Glossary:
    MAGIC - sequence of 98 7F bytes
    NSTRING - sequences of bytes terminated by 00 value, 00 should be at the end
    NAME - NSTRING that is restricted in its possible size to 128 bytes

  Commands:
    # What to do with output of program runned by this command? Embed in place of command?
    MAGIC 10        - Turn off interpreting on magic strings within current output
    MAGIC 11        - Turn on interpreting on magic strings within current output
    MAGIC 20 X:NAME - Interpret file by the name of X
    MAGIC 21 X:NAME - Save data before this command into file by the name of X
                        If file already exists then rewrite its contents
    MAGIC 22 X:NAME - Load content of file at path X into buffer

"""

# todo: capture return code from Command.do() call for uniform handling of them
# todo: read and push file byte contents
# todo: make interface for appending programs to each other
# todo: should data be appended at the end? or at the beginning
#       this way it could be easier to embed other program's results based on input they're read and thus consumed

import os, sys, subprocess, tempfile, time
from typing import List, Tuple, Iterator

DefaultTimeout = 5.0

# todo: make it better, it's confusing af
HelpText = """```
    Hivemind scripts are composed from command sequence
    Every termite invocation operates on output of previous termite program 

    - stuff in [] are optional elements

    Command syntax:
        command [arg | "arg"]+ [: whitespace arg]

    Example:
        run "d":
            mycode

    Hivemind commands:
        . run [args-string] code
            -- run supplied termite code, args are optional
        . script [args-string] path
            -- run termite script at given path, args are optional
        . data content
            -- append termite syntax text into output buffer
        . drop # todo: is it needed even?
            -- zero the output buffer

```"""

# todo: hex any integer to hex string
def int_to_hex(byte: int) -> bytes:
    if byte > 255:
        raise Exception("cannot get hex of int bigger than 255")
    leading = byte // 16 + 0x30
    if leading > 0x39: leading += 7
    following = byte % 16 + 0x30
    if following > 0x39: following += 7
    return bytes(chr(leading), encoding="ascii") + bytes(chr(following), encoding="ascii")

# todo: work on bytes internally? or better yet mutable bytearray
# todo: implement it in termite itself, as it's just data operation 
def raw_to_termite(text: str) -> bytes:
    result = b""
    index = 0
    while index != len(text):
        # todo: isnumeric only checks for numerics in ascii range? if not then it might be erroneous
        if text[index].isnumeric() or text[index] in {'A', 'B', 'C', 'D', 'E', 'F'}:
            if index != len(text) - 1:
                if text[index + 1].isnumeric() or text[index + 1] in {'A', 'B', 'C', 'D', 'E', 'F'}:
                    leading = ord(text[index]) - ord('0')
                    if leading > 9: leading -= 7
                    following = ord(text[index + 1]) - ord('0')
                    if following > 9: following -= 7
                    result += bytes(chr(leading << 4 | following), encoding="latin-1")
                    index += 1
                else:
                    raise Exception(f"[ill-formed hex token '{text[index:index+2]}']")
            else:
                raise Exception(f"[trailing unformed hex token]")
        elif not text[index] in {' ', '\n', '\t'}:
            result += bytes(text[index], encoding="utf-8")
        index += 1
    return result


class Command:
    def do(self, input_data: bytes, **kwargs) -> bytes:
        raise NotImplementedError


# todo: args
def run_worker_script(path: str, instream: bytes = b"", timeout: float = DefaultTimeout, arg_string: str = "") -> Tuple[int, bytes]:
    kwargs = {
        "capture_output": True,
        "input": instream,
        "timeout": timeout,
    }
    execution = subprocess.run(["termite-worker", path, arg_string], **kwargs)
    return (execution.returncode, execution.stdout)


class RunCodeCommand(Command):
    def __init__(self, code: str, arg_string: str = ""):
        self.code = code
        self.arg_string = arg_string

    def do(self, input_data: bytes, **kwargs) -> bytes:
        result = b""
        descriptor, tpath = tempfile.mkstemp(dir=os.getcwd())
        try:
            with open(tpath, "w+b") as f:
                f.write(self.code.encode("utf-8"))
            returncode, output = run_worker_script(os.path.basename(tpath), input_data, arg_string=self.arg_string)
            if returncode != 0:
                returncode, errorout = run_worker_script(
                    "std/spit-error.tm",
                    bytes(chr(returncode), encoding="latin1")
                )
                if returncode != 0:
                    raise Exception(f"return code {returncode} [can't run \"std/spit-error.tm\"]")
                output += b"\n" + errorout
            result = output
        except Exception as e:
            os.close(descriptor)
            os.unlink(tpath)
            raise e
        os.close(descriptor)
        os.unlink(tpath)
        return result


class RunScriptCommand(Command):
    def __init__(self, path: str, arg_string: str = ""):
        if not path.endswith(".tm"):
            path = path + ".tm"
        path = os.getcwd() + "/" + path
        self.path = path
        self.arg_string = arg_string

    def do(self, input_data: bytes, **kwargs) -> bytes:
        returncode, output = run_worker_script(self.path, input_data, arg_string=self.arg_string)
        if returncode != 0:
            returncode, errorout = run_worker_script(
                "std/spit-error.tm",
                bytes(chr(returncode), encoding="latin1")
            )
            if returncode != 0: # todo: show output even on error
                raise Exception(f"return code {returncode} [can't run \"std/spit-error.tm\"]")
            output += b"\n" + errorout
        return output


class DropOutputCommand(Command):
    def do(self, _input_data: bytes, **kwargs) -> bytes:
        return b""


class AppendStringCommand(Command):
    def __init__(self, string: str):
        self.string = bytes(string, encoding="utf-8")

    def do(self, input_data: bytes, **kwargs) -> bytes:
        return input_data + self.string + b"\x00"


class RandomCommand(Command):
    def __init__(self, seed: int = None):
        if seed is None:
            seed = int(time.time())
        seed %= 256
        self.seed = seed

    def do(self, input_data: bytes, **kwargs) -> bytes:
        return input_data + bytes(self.seed)


class PushDataCommand(Command):
    def __init__(self, data: bytes):
        self.data = data

    def do(self, input_data: bytes, **kwargs) -> bytes:
        return input_data + self.data


class InvisToHex(Command):
    def do(self, input_data: bytes, **kwargs) -> bytes:
        result = b""
        for ch in input_data:
            if ch == 0x7F or (ch < 0x20 and ch != 0x9 and ch != 0xA):
                result += int_to_hex(ch)
            else:
                result += bytes(chr(ch), encoding="latin1")
        return result


# todo: could break easily, maybe just use regex instead?
# todo: fix it
def find_next_word(input_str: str) -> Tuple[str, int]:
    index = 0
    while index < len(input_str):
        match input_str[index]:
            case " " | "\t" | "\r":
                index += 1
            case "\n":
                return ("\n", index)
            case "\"":
                index += 1
                beginning = index
                while index < len(input_str):
                    match input_str[index]:
                        case "\\":
                            if index != len(input_str) - 1 and input_str[index + 1] == "\"":
                                index += 2
                            else:
                                index += 1
                        case "\"":
                            return (input_str[beginning:index], index + 1)
                        case _:
                            index += 1
                raise Exception("unclosed \"")
            case ":":
                index += 1
                beginning = index
                while index < len(input_str):
                    newline = input_str[index:].find("\n")
                    if newline == -1:
                        return (input_str[beginning:].strip(), len(input_str))
                    elif newline != len(input_str) - 1 and input_str[index + newline + 1] in (" ", "\t", "\r"):
                        index += newline + 1
                    else:
                        return (input_str[beginning:index + newline].strip(), index + newline)
                return (input_str[beginning:index].strip(), index) # todo: is it okay?
            case _:
                # if something starts by whitespace and followed by it - it will be reported as word even if it's actually empty
                beginning = index
                while index < len(input_str):
                    match input_str[index]:
                        case "\r":
                            if index != len(input_str) - 1 and input_str[index + 1] == "\n":
                                return (input_str[beginning:index], index + 1)
                            else:
                                return (input_str[beginning:index], index)
                        case " " | "\t" | "\n" | ":":
                            return (input_str[beginning:index], index)
                        case _:
                            index += 1
                return (input_str[beginning:], len(input_str) - 1)
    return ("", 0)

def parse_next_command(input_str: str) -> Tuple[List[str], int]:
    result = []
    index = 0
    word, parsed = find_next_word(input_str)
    print(word)
    while parsed != 0:
        result.append(word)
        if index + parsed != len(input_str) and input_str[index + parsed] == "\n":
            parsed += 1
            break
        index += parsed
        word, parsed = find_next_word(input_str[index:])
        print(word)
    return (result, index + parsed)

def parse_commands(input_str: str) -> Iterator[List[str]]:
    index = 0
    words, parsed = parse_next_command(input_str)
    while parsed != 0:
        yield words
        index += parsed
        words, parsed = parse_next_command(input_str[index:])

def parse_command_sequence(input_str: str) -> List[Command]:
    result = []
    for words in parse_commands(input_str):
        match words:
            case ["run", arg_string, code]:
                result.append(RunCodeCommand(code, arg_string))
            case ["run", code]:
                result.append(RunCodeCommand(code))
            case ["script", arg_string, path]:
                result.append(RunScriptCommand(path, arg_string))
            case ["script", path]:
                result.append(RunScriptCommand(path))
            case ["data", data]:
                result.append(PushDataCommand(raw_to_termite(data)))
            case ["hex"]:
                result.append(InvisToHex())
            case ["seed", seed]:
                result.append(RandomCommand(int(seed)))
            case ["seed"]:
                result.append(RandomCommand())
            case ["drop"]:
                result.append(DropOutputCommand())
            case [code]:
                result.append(RunCodeCommand(code, "d"))
            case _:
                raise Exception(f"unknown command: {words}")
    return result


def process(input_str: str) -> bytes:
    commands = parse_command_sequence(input_str.strip())
    output = b""
    for command in commands:
        output = command.do(output)
    return output


if __name__ == "__main__":
    if len(sys.argv) > 1:
        try:
            with open(sys.argv[1], "r") as f:
                output = process(f.read())
                print(str(output, encoding="latin1"))
        except SystemError:
            print(f"can't open {sys.argv[1]}")
    else:
        print("no target supplied")
