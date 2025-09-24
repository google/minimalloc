#!/usr/bin/env python3

"""
Minimal test to isolate the pybind11 import issue
"""

import sys
import os

# Add the build directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'python'))

try:
    print("Testing direct _core import...")
    # Try to import the core module directly
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        '_core',
        '/home/fpedd/minimalloc/python/minimalloc/_core.cpython-312-x86_64-linux-gnu.so'
    )
    core = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(core)
    print("SUCCESS: Direct _core import works!")
    print("Available:", [name for name in dir(core) if not name.startswith('_')])
except Exception as e:
    print(f"FAILED: {e}")

    # Try in a completely clean environment
    print("\nTrying with clean Python interpreter...")
    import subprocess
    result = subprocess.run([
        sys.executable, '-c',
        '''
import importlib.util
spec = importlib.util.spec_from_file_location("_core", "/home/fpedd/minimalloc/python/minimalloc/_core.cpython-312-x86_64-linux-gnu.so")
core = importlib.util.module_from_spec(spec)
spec.loader.exec_module(core)
print("SUCCESS in clean environment!")
        '''
    ], capture_output=True, text=True)

    print("STDOUT:", result.stdout)
    print("STDERR:", result.stderr)
    print("Return code:", result.returncode)