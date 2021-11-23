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

import os

from dotenv import load_dotenv
import discord
from discord.ext import commands

import hivemind

CommandPrefix = ">"
ScriptFolder = "shared"


# todo: fix
def discord_to_hivemind(input_str: str) -> str:
    to_process = input_str
    result = ""
    while len(to_process) != 0:
        left = to_process.find("\n```")
        if left != -1:
            right = to_process[left+3:].find("```")
            if right != -1:
                result += to_process[:left+3]
                result += "\t" + to_process[left+3:right].replace("\n", "")
                to_process = to_process[right+3:]
                continue
        result += to_process
        to_process = ""
    return result


def strip_discord_formatting(s: str) -> str:
    result = s.strip()
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
        # script = discord_to_hivemind(script)
        if script.strip().lower() == "help":
            await ctx.send(hivemind.HelpText)
        else:
            try:
                output = hivemind.process(script)
                if len(output) != 0:
                    await ctx.send(f"""```\n{str(output, encoding="latin1")}```""")
                else:
                    await ctx.send("```\n[no output]```")
            except Exception as e:
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
            formed_script = f'run d "{script}"'
            try:
                output = hivemind.process(formed_script)
                if len(output) != 0:
                    await ctx.send(f"""```\n{str(output, encoding="latin1")}```""")
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
