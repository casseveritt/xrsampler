import sys
import re

def scan_logcat(logfile, package_name):
    pid = None
    pid_re = re.compile(r'Start proc (\d+):' + re.escape(package_name) + r'/')
    proc_line_re = re.compile(r'\s(\d+)\s')  

    try:
        with open(logfile, 'r', encoding='utf-8', errors='replace') as f:
            for line in f:
                if pid is None:
                    m = pid_re.search(line)
                    if m:
                        pid = m.group(1)
                        print(line, end='')
                else:
                    m = proc_line_re.search(line)
                    if m and m.group(1) == pid:
                        print(line, end='')
    except UnicodeDecodeError as e:
        print(f"Unicode decode error: {e}", file=sys.stderr)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python scan_logcat.py <logcat.txt> <package.name>")
        sys.exit(1)
    scan_logcat(sys.argv[1], sys.argv[2])
