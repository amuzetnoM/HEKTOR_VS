#!/usr/bin/env python3
"""Minimal example: create a pyvdb DB, add a text document, and search it."""

try:
    import pyvdb
except Exception as e:
    print('pyvdb not installed; install to run example', e)
    raise

from pathlib import Path

db_path = Path('./vectors/cthulu_roundtrip')
db = pyvdb.create_database(str(db_path), dimension=512)

meta = {'symbol':'EURUSD','timeframe':'5m','source':'example'}
db.add_text('Example signal: mean reversion test', metadata=meta)

res = db.search('mean reversion', k=5)
print('Search results:', res)
