"""Discord bot implementation for interfacing Termite systems

    Supply .env file with DiscordBotToken value for running it from your own bot

"""

# todo: make virtual file system or at least make sure that only internal files are possible to interact with
# todo: env variable with directory of shared access scripts instead of just const
# todo: restrict how many scripts might be stored by certain infividuals and also in general
# todo: manage user profiles, authorship and other meta data for scripts
# todo: create ScriptFolder automatically if it doesn't exist
# todo: passing of interactive console-like stdin handle
# todo: filesystem help notice

import os, traceback
from typing import List

from dotenv import load_dotenv
import discord
from discord.ext import commands

import hivemind

CommandPrefix = ">"
ScriptFolder = "shared"
MaxMessagesPerResponse = 3


# todo: does byte count matters? as string might be lengthed by encoded characters, not bytes
# todo: divide by \n if they are present
def package_string(input_str: str, max_size: int = 1500) -> List[str]:
    result = []
    to_package = input_str
    while len(to_package) >= max_size:
        result.append(to_package[:max_size])
        to_package = to_package[max_size:]
    result.append(to_package)
    return result

# todo: fix
# def format_discord_body_args(input_str: str) -> str:



def strip_discord_formatting(input_str: str) -> str:
    result = input_str.strip()
    if result.startswith("```") and result.endswith("```"):
        l = 0
        for ch in result:
            if ch.isspace():
                break
            l += 1
        result = result[l:-3]
    elif result.startswith("`") and result.endswith("`"):
        result = result[1:-1]
    return result


async def filesystem_dir(ctx, directory: str):
    if directory == ".":
        directory = ""
    try:
        files = (f for f in os.listdir(os.getcwd() + '/' + directory) if f.endswith(".tm"))
        file_str = "\n".join(("- " + file[:-3] for file in files))
        if file_str == "":
            file_str = "[empty]"
        await ctx.send(f"```\n{file_str}```")
    except OSError as e:
        await ctx.send(f"```\n[invalid directory]```")


async def filesystem_file(ctx, path: str):
    if not path.endswith(".tm"):
        path = path + ".tm"
    try:
        with open(os.getcwd() + '/' + path, "r") as f:
            await ctx.send(f"```\n{f.read()}```")
    except OSError:
        await ctx.send(f"```\n[invalid file]```")


async def filesystem_save(ctx, path, code, forced = False):
    path = ScriptFolder + '/' + path
    if not path.endswith(".tm"):
        path = path + ".tm"
    if forced is True or not os.path.isfile(os.getcwd() + '/' + path):
        try:
            with open(os.getcwd() + '/' + path, "w") as f:
                f.write(code)
        except OSError as e:
            await ctx.send(f"```\n[cannot save]```")
    else:
        await ctx.send(f"```\n[file already exists, use 'save forced <path>' to forced rewrite]```")


class Commands(commands.Cog):
    @commands.command(name="hm")
    async def hivemind_op(self, ctx):
        script = ctx.message.content[len(Commands.hivemind_op.name) + len(CommandPrefix):]
        # script = format_discord_body_args(script)
        if script.strip().lower() == "help":
            await ctx.send(hivemind.HelpText)
        else:
            try:
                output = hivemind.process(script)
                if len(output) != 0:
                    packages = package_string(str(output, encoding="latin1"))
                    if len(packages) <= MaxMessagesPerResponse:
                        for pack in packages:
                            await ctx.send(f"""```\n{pack}```""")
                    else:
                        await ctx.send("```\n[outupt is too big for discord]```")
                else:
                    await ctx.send("```\n[no output]```")
            except Exception as e:
                print(traceback.format_exc())
                await ctx.send(f"```\n[error: {e}]```")

    @commands.command(name="tm")
    async def termite_op(self, ctx):
        script = ctx.message.content[len(Commands.termite_op.name) + len(CommandPrefix):]
        script = strip_discord_formatting(script)
        if script.strip().lower() == "help":
            try:
                with open("TERMITE-SPEC") as f:
                    await ctx.send(f"```\n{f.read()}```")
            except OSError:
                await ctx.send("[cannot open TERMITE-SPEC]")
        else:
            formed_script = f'run d "{script}"\nhex'
            try:
                output = hivemind.process(formed_script)
                if len(output) != 0:
                    packages = package_string(str(output, encoding="latin1"))
                    if len(packages) <= MaxMessagesPerResponse:
                        for pack in packages:
                            await ctx.send(f"""```\n{pack}```""")
                    else:
                        await ctx.send("```\n[outupt is too big for discord]```")
                else:
                    await ctx.send("```\n[no output]```")
            except Exception as e:
                await ctx.send(f"```\n[error: {e}]```")

    @commands.command(name="fs")
    async def filesystem(self, ctx):
        script = ctx.message.content[len(Commands.filesystem.name) + len(CommandPrefix):]
        command, _ = hivemind.parse_next_command(script)
        match command:
            case [path]:
                await filesystem_file(ctx, path)
            case ["dir", directory]:
                await filesystem_dir(ctx, directory)
            case ["save", path, code]:
                await filesystem_save(ctx, path, code)
            case ["save", "forced", path, code]:
                await filesystem_save(ctx, path, code, forced=True)
            case _:
                await ctx.send(f"```\n[invalid filesystem command: {command}]```")


bot = commands.Bot(command_prefix=CommandPrefix)

load_dotenv(".env")
token = os.environ.get("DiscordBotToken")
if token is not None:
    bot.add_cog(Commands())
    bot.run(os.environ.get("DiscordBotToken"))
else:
    raise Exception("No token available in environment")
