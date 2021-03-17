import _thread
import argparse
import inspect
import itertools
import pickle
import random
import signal
import sys
import threading
import time
from functools import reduce
import importlib
from concurrent.futures import TimeoutError

import pebble


def get_classes_recursive(cls):
    result = [(cls.__name__, cls)]
    for name, attr in cls.__dict__.items():
        if inspect.isclass(attr):
            result += list(map(lambda x: (cls.__name__ + "." + x[0], x[1]), get_classes_recursive(attr)))

    return result


def get_all_defined_classes(module):
    result = []
    for name, obj in inspect.getmembers(module, inspect.isclass):
        if obj.__module__ == module.__name__:
            result.append((name, obj))

    return result


def get_all_defined_classes_recursive(module):
    result = []
    for name, obj in get_all_defined_classes(module):
        result += get_classes_recursive(obj)

    return result


def deepgetattr(obj, attr):
    """Recurses through an attribute chain to get the ultimate value."""
    return reduce(getattr, attr.split('.'), obj)


def import_file(full_name, path):
    """Import a python module from a path. 3.4+ only.

    Does not call sys.modules[full_name] = path
    """

    spec = importlib.util.spec_from_file_location(full_name, path)
    mod = importlib.util.module_from_spec(spec)

    spec.loader.exec_module(mod)
    return mod


def shuffle_raw_data(fname, outfile=None, clip=None):
    data = []
    cnt = 0
    with open(fname, 'rb') as f:
        while True:
            try:
                data.append(pickle.load(f))
                cnt += 1
                print(cnt, end='\r')
            except EOFError:
                break

    print()

    random.shuffle(data)
    random.shuffle(data)
    if outfile is None:
        outfile = fname

    if clip is not None:
        clip = int(clip)

    with open(outfile, 'wb') as f:
        for cnt, d in enumerate(data):
            pickle.dump(d, f, protocol=pickle.HIGHEST_PROTOCOL)
            print(cnt, end='\r')
            if clip is not None and cnt > clip:
                break


def grouper(sizes, iterable):
    if isinstance(sizes, int):
        sizes = [sizes]

    for size in sizes[:-1]:
        args = [iterable] * size
        try:
            yield next(zip(*args))
        except StopIteration:
            pass

    args = [iterable] * sizes[-1]
    yield from itertools.zip_longest(*args)


def call_with_timeout_multiprocess(func, *args, timeout=3):
    pool = pebble.ProcessPool(max_workers=1)
    with pool:
        future = pool.schedule(func, args=args, timeout=timeout)
        return future.result()


class SignalTimeout:
    def __init__(self, seconds=1, error_message='Timeout'):
        self.seconds = seconds
        self.error_message = error_message

    def handle_timeout(self, signum, frame):
        raise TimeoutError(self.error_message)

    def __enter__(self):
        signal.signal(signal.SIGALRM, self.handle_timeout)
        signal.alarm(self.seconds)

    def __exit__(self, type, value, traceback):
        signal.alarm(0)


def quit_function():
    sys.stderr.flush()  # Python 3 stderr is likely buffered.
    while True:
        if threading.main_thread().__custom_threading_interrupted is True:
            break

        _thread.interrupt_main()  # raises KeyboardInterrupt
        time.sleep(5)


def ThreadingTimeout(s, fn, *args, **kwargs):
    #  Use some monkey-patching and no thread-level concurrency to our advantage
    #  Can't just use is_alive() as the main thread is allowed to work afterwords
    threading.main_thread().__custom_threading_interrupted = False
    timer = threading.Timer(s, quit_function)
    timer.start()
    try:
        result = fn(*args, **kwargs)
    finally:
        threading.main_thread().__custom_threading_interrupted = True
        timer.cancel()

    return result

