"""Extract configuration information from `cmake-js print-configure`.

Prints cmake-js configuration on a single line so you can append it to your cmake config
commanline.

.. code::

  cmake foo bar `python cmake-js-configure.py`

"""

import re
import subprocess

with subprocess.Popen(
    ["npx", "cmake-js", "print-configure"],
    stdout=subprocess.PIPE,
    encoding="utf-8",
    shell=True,
) as proc:
    for line in proc.stdout.readlines():
        m = re.search("'-D((?:CMAKE_JS|NODE|CMAKE_CXX)_.*)=(.*)'", line)
        if m:
            print(f"-D{m.group(1)}={m.group(2)}", end=" ")
