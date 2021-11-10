import sys, string

def count(s: str) -> int:
  print(s)
  count = 0
  i = 0
  while i < len(s):
    if s[i].isnumeric() or s[i] in {'A', 'B', 'C', 'D', 'E', 'F'}:
      i += 1
      if s[i].isnumeric() or s[i] in {'A', 'B', 'C', 'D', 'E', 'F'}:
        count += 1
      else:
        raise Exception(f"Ill-formed hex token '{s[i - 1: i + 1]}'")
    # elif s[i] in {'<', '>', '.', '@', '^', '~', '=', '?', '+', '-', '*', '/', '#', '$', ']', '['}:
    #   count += 1
    elif s[i] in {'\r', '\n', ' ', '\t'}:
      pass
    else:
      count += 1
    i += 1
  return count

if __name__ == "__main__":
  n = count(sys.stdin.read())
  sys.stdout.write(str(hex(n)))
