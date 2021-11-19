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

  Syntax:
    command [arg [arg ...]]:
      arg

"""

# todo: capture return code from Command.do() call for uniform handling of them
# todo: data command for pushing data

import os, sys, subprocess, tempfile
from typing import List, Tuple

default_timeout = 5.0
default_script_folder = "hive_scripts"


class TermiteError(Exception): pass


class Command:
    def do(self, input_data: bytes, **kwargs) -> bytes:
        raise NotImplementedError


# todo: args
def run_worker_script(path: str, instream: bytes = b"", timeout: float = None, arg_string: str = "") -> Tuple[int, bytes]:
    print(path, instream)
    kwargs = {
        "capture_output": True,
        "input": instream
    }
    if timeout is not None:
        kwargs["timeout"] = timeout
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
                f.write(bytes(self.code, encoding="latin1"))
            returncode, output = run_worker_script(os.path.basename(tpath), input_data, arg_string=self.arg_string)
            if returncode != 0:
                returncode, errorout = run_worker_script(
                    "std/spit-error.tm",
                    bytes(chr(returncode), encoding="latin1")
                )
                if returncode != 0:
                    raise Exception(f"return code {returncode} [can't run \"std/spit-error.tm\"]")
                raise TermiteError(str(errorout, encoding="latin1"))
            result = output
        except subprocess.TimeoutExpired:
            pass
        except Exception as e:
            os.close(descriptor)
            os.unlink(tpath)
            raise e
        os.close(descriptor)
        os.unlink(tpath)
        return result


class RunScriptCommand(Command):
    def __init__(self, path: str, arg_string: str = ""):
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
            raise TermiteError(str(errorout, encoding="latin1"))
        return output


class DropOutputCommand(Command):
    def do(self, _input_data: bytes, **kwargs) -> bytes:
        return b""


class AppendStringCommand(Command):
    def __init__(self, string: str):
        self.string = bytes(string, encoding="utf-8")

    def do(self, input_data: bytes, **kwargs) -> bytes:
        return input_data + self.string + b"\x00"


# todo: make it generator-driven
def parse_command_sequence(input_str: str) -> List[Command]:
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
        while parsed != 0:
            result.append(word)
            if index + parsed != len(input_str) and input_str[index + parsed] == "\n":
                parsed += 1
                break
            index += parsed
            word, parsed = find_next_word(input_str[index:])
        return (result, index + parsed)

    result = []
    index = 0
    words, parsed = parse_next_command(input_str)
    while parsed != 0:
        match words:
            case ["run", "args", arg_string, code]:
                result.append(RunCodeCommand(code, arg_string))
            case ["run", code]:
                result.append(RunCodeCommand(code))
            case ["script", path, "args", arg_string]:
                result.append(RunScriptCommand(path, arg_string))
            case ["script", path]:
                result.append(RunScriptCommand(path))
            case ["string", string]:
                result.append(AppendStringCommand(string))
            case ["drop"]:
                result.append(DropOutputCommand())
            case _:
                raise Exception(f"unknown command: {words}")
        index += parsed
        words, parsed = parse_next_command(input_str[index:])
    return result


def process(input_str: str) -> bytes:
    commands = parse_command_sequence(input_str.strip())
    output = b""
    for command in commands:
        output = command.do(output)
    print(output)
    return output


if __name__ == "__main__":
    if len(sys.argv) > 1:
        try:
            with open(sys.argv[1], "r") as f:
                process(f.read())
        except SystemError:
            print(f"can't open {sys.argv[1]}")
    else:
        print("no target supplied")