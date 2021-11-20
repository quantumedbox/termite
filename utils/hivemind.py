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
    command [arg val [arg val ...]]:
      input

"""

import os, sys, subprocess, tempfile
from typing import List, Tuple

default_timeout = 5.0


class Command:
    def do(self, input_data: bytes, **kwargs) -> bytes:
        raise NotImplementedError


# todo: args
def run_worker_script(path: str, instream: bytes = b"", timeout: float = None) -> Tuple[int, bytes]:
    print(path)
    kwargs = {"capture_output": True, "input": instream}
    if timeout is not None:
        kwargs["timeout"] = timeout
    execution = subprocess.run(["termite-worker", path], **kwargs)
    return (execution.returncode, execution.stdout)


class RunCodeCommand(Command):
    def __init__(self, code: str):
        self.code = code

    def do(self, input_data: bytes, **kwargs) -> bytes:
        result = b""
        descriptor, tpath = tempfile.mkstemp(dir=os.path.dirname(os.path.abspath(sys.argv[0])))
        try:
            with open(tpath, "w+b") as f:
                f.write(bytes(self.code, encoding="latin1"))
            returncode, output = run_worker_script(tpath)
            print(output)
            if returncode != 0:
                returncode, errorout = run_worker_script("std/spit-error")
                if returncode != 0:
                    raise Exception(f"error while running error spitter, code {returncode}")
                raise Exception(str(errorout, encoding="latin1"))
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
                    beginning = index + 1
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
                            return (input_str[beginning:], len(input_str))
                        elif newline != len(input_str) - 1 and input_str[index + 1] in (" ", "\t", "\r"):
                            index += newline + 1
                            continue
                        else:
                            return (input_str[beginning:newline], newline)
                    return (input_str[beginning:index], index)
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
        # print(result)
        while parsed != 0:
            result.append(word)
            if index + parsed != len(input_str) and input_str[index + parsed] == "\n":
                break
            index += parsed
            word, parsed = find_next_word(input_str[index:])
            # print(result)
        return (result, index + parsed)

    result = []
    index = 0
    words, parsed = parse_next_command(input_str)
    while parsed != 0:
        match words:
            case ["run", "code", code]:
                result.append(RunCodeCommand(code))
            case _:
                raise Exception("unknown command")
        index += parsed
        words, parsed = parse_next_command(input_str[index:])
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
                process(f.read())
        except SystemError:
            print(f"can't open {sys.argv[1]}")
    else:
        print("no target supplied")
