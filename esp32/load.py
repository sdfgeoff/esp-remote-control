#!/bin/python3
"""Loads everything onto the ESP32"""

import argparse
import os
import time
import functools
import logging
import sys

from ampy import pyboard

logging.basicConfig(level=logging.DEBUG)

PORT = "/dev/ttyUSB0"
BAUD = 115200

EXCLUDES = ['.pyc', '.txt', '.gitignore', '__pycache__']

logging.info("Connecting")
pyb = pyboard.Pyboard(PORT, BAUD, 'micro', 'python')
pyb.enter_raw_repl()
logging.info("Connected")

def run_text(text):
    logging.debug("Running: {}".format(repr(text)))
    output = pyb.exec_(text)
    logging.debug("Got: {}".format(repr(output)))
    #pyboard.stdout_write_bytes(output)

    time.sleep(0.1)

    return output

def load_file(input_file, onboard_path):
    ensure_dir(os.path.dirname(onboard_path))
    logging.info("Loading file {} to {}".format(input_file, onboard_path))

    file_contents = open(input_file, 'rb').read()
    file_name = os.path.basename(onboard_path)
    file_str = '''f = open("{}", "wb");
f.write({});
f.close()'''.format(onboard_path, repr(file_contents))

    run_text(file_str)


@functools.lru_cache()
def ensure_dir(directory):
    if not directory:
        return

    parent_dirs = directory.split('/')[:-1]
    for par_dir in parent_dirs:
        ensure_dir(par_dir)

    cmd = "import os; print('{}' in os.listdir('{}'))".format(os.path.basename(directory), os.path.dirname(directory))
    res = run_text(cmd)
    print(res)
    if b'False' in res:
        logging.info("Making Directory {}".format(directory))
        cmd = "import os; os.mkdir('{}')".format(directory)
        res = run_text(cmd)
    elif b'True' in res:
        return False
    else:
        print("ENSURE_DIR FAILED")
    print(res)


def load_folder(folder):
    files = []
    for base_path, subdirs, subfiles in os.walk(folder):
        for filename in subfiles:
            full_path = os.path.join(base_path, filename)
            to_path = os.path.relpath(full_path, folder)

            found = False
            for exclude in EXCLUDES:
                if exclude in to_path:
                    found = True
            if found:
                continue
            files.append(full_path)


    print(files)

    for file_id, file_name in enumerate(files):
        print("{:.0f}%".format(file_id/len(files) * 100))
        to_path = os.path.relpath(file_name, folder)
        load_file(file_name, to_path)


def main(args):
    parser = argparse.ArgumentParser(description='Uploads a folder of scripts to the ESP')
    parser.add_argument(
        '--base_folder', type=str, help='Folder to upload', required=True
    )

    args = parser.parse_args()
    load_folder(args.base_folder)


if __name__ == "__main__":
    main(sys.argv[1:])

pyb.exit_raw_repl()
pyb.close()
