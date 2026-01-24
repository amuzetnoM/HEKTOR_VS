#!/usr/bin/env python3
"""
collect_benchmarks.py

Collect system probes and optionally run benchmark scripts to populate
hektor/docs/.SCOPE/bench.json with machine-readable, thread-level data.

Usage:
  ./collect_benchmarks.py --output bench.json [--run-bench ./run_benchmarks.sh] [--bench-args "--dataset sift1m --threads 16"]

By default it will update the bench.json created earlier and fill available fields.
"""

import argparse
import json
import os
import platform
import pwd
import socket
import subprocess
import sys
import time
from datetime import datetime

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
DEFAULT_OUT = os.path.join(os.path.dirname(__file__), 'bench.json')


def run_cmd(cmd, timeout=30, check=False, capture=True):
    try:
        r = subprocess.run(cmd, shell=isinstance(cmd, str), stdout=subprocess.PIPE if capture else None,
                       stderr=subprocess.PIPE, timeout=timeout, check=check, universal_newlines=True)
        return r.returncode, r.stdout.strip() if capture else ""
    except Exception as e:
        return 1, str(e)


def load_existing(path):
    if os.path.exists(path):
        try:
            with open(path, 'r') as f:
                return json.load(f)
        except Exception:
            return None
    return None


def save_json(path, data):
    with open(path + '.tmp', 'w') as f:
        json.dump(data, f, indent=2, sort_keys=False)
    os.replace(path + '.tmp', path)


def probe_basic():
    data = {}
    data['metadata'] = {
        'generated_by': 'collect_benchmarks.py',
        'generated_at': datetime.utcnow().isoformat() + 'Z',
        'host': socket.gethostname(),
        'user': pwd.getpwuid(os.geteuid()).pw_name,
        'repo': 'amuzetnoM/hektor'
    }
    # git info
    code, out = run_cmd('git -C "{}" rev-parse --verify HEAD'.format(ROOT))
    data['metadata']['repo_commit'] = out if code == 0 else None
    code, out = run_cmd('git -C "{}" rev-parse --abbrev-ref HEAD'.format(ROOT))
    data['metadata']['branch'] = out if code == 0 else None

    # system
    data['system'] = {
        'architecture': platform.machine(),
        'os': {'name': platform.system(), 'pretty': platform.platform()},
        'kernel': {'release': platform.release(), 'version': platform.version()},
        'uptime_seconds': None,
        'boot_time_unix': None
    }
    # uptime
    rc, up = run_cmd('cat /proc/uptime')
    if rc == 0:
        try:
            uptime = float(up.split()[0])
            data['system']['uptime_seconds'] = uptime
            data['system']['boot_time_unix'] = int(time.time() - uptime)
        except Exception:
            pass

    # cpu via lscpu and /proc/cpuinfo
    cpu = {}
    rc, lscpu_json = run_cmd('lscpu -J', timeout=5)
    if rc == 0:
        try:
            lj = json.loads(lscpu_json)
            kvm = {entry['field'].strip(':'): entry['data'] for entry in lj.get('lscpu', [])}
            cpu['model_name'] = kvm.get('Model name')
            cpu['vendor'] = kvm.get('Vendor ID')
            cpu['logical_processors'] = int(kvm.get('CPU(s)', 0)) if kvm.get('CPU(s)') else None
            cpu['sockets'] = int(kvm.get('Socket(s)', 0)) if kvm.get('Socket(s)') else None
            cpu['cores_per_socket'] = int(kvm.get('Core(s) per socket', 0)) if kvm.get('Core(s) per socket') else None
            cpu['threads_per_core'] = None
            if cpu['logical_processors'] and cpu['cores_per_socket'] and cpu['sockets']:
                total_cores = cpu['cores_per_socket'] * (cpu['sockets'] or 1)
                cpu['threads_per_core'] = int(cpu['logical_processors'] / total_cores) if total_cores > 0 else None
            cpu['flags'] = kvm.get('Flags', '').split()
            cpu['base_frequency_ghz'] = float(kvm.get('CPU MHz'))/1000.0 if kvm.get('CPU MHz') else None
        except Exception:
            pass
    else:
        # fallback to /proc/cpuinfo
        rc, cinfo = run_cmd('cat /proc/cpuinfo')
        if rc == 0:
            cpu['model_name'] = None
            for line in cinfo.splitlines():
                if 'model name' in line and cpu.get('model_name') is None:
                    cpu['model_name'] = line.split(':',1)[1].strip()
    # topology (simple)
    rc, topo = run_cmd("egrep '^processor|^core id|^physical id' /proc/cpuinfo || true")
    if rc == 0:
        # build simple mapping
        pts = {}
        pid = None
        core = None
        proc = None
        import re
        for ln in topo.splitlines():
            if ln.strip() == '':
                continue
            # strip leading line numbers if present (e.g., "23:processor	: 0")
            if ':' in ln and ln.split(':',1)[0].strip().isdigit():
                ln = ln.split(':',1)[1]
            parts = ln.split(':')
            if len(parts) < 2:
                continue
            key = parts[1].strip()
            try:
                if 'processor' in ln:
                    proc = int(key)
                    pts.setdefault(proc, {})
                elif 'core id' in ln and proc is not None:
                    pts[proc]['core_id'] = int(key)
                elif 'physical id' in ln and proc is not None:
                    pts[proc]['package_id'] = int(key)
            except ValueError:
                # attempt to extract a trailing integer if parsing fails
                m = re.search(r"(\d+)", key)
                if m:
                    val = int(m.group(1))
                    if 'processor' in ln:
                        proc = val
                        pts.setdefault(proc, {})
                    elif 'core id' in ln and proc is not None:
                        pts[proc]['core_id'] = val
                    elif 'physical id' in ln and proc is not None:
                        pts[proc]['package_id'] = val
                else:
                    continue
        cpu['per_logical_cpu'] = {str(k): {'core_id': v.get('core_id'), 'physical_package': v.get('package_id')} for k,v in pts.items()}

    cpu['caches'] = {'L1d_bytes': None, 'L1i_bytes': None, 'L2_bytes': None, 'L3_bytes': None}
    data['cpu'] = cpu

    # memory
    mem = {}
    rc, meminfo = run_cmd('cat /proc/meminfo')
    if rc == 0:
        for ln in meminfo.splitlines():
            if ':' not in ln:
                continue
            k,v = ln.split(':',1)
            v = v.strip().split()[0]
            try:
                mem['total_bytes'] = int(v) * 1024 if k == 'MemTotal' else mem.get('total_bytes')
            except Exception:
                pass
    rc, free_b = run_cmd('free -b')
    if rc == 0:
        # parse second line
        try:
            l = free_b.splitlines()[1].split()
            mem['total_bytes'] = int(l[1])
            mem['available_bytes'] = int(l[6]) if len(l) > 6 else None
        except Exception:
            pass
    data['memory'] = mem

    # storage
    rc, lsblk = run_cmd('lsblk -J -b')
    devs = []
    if rc == 0:
        try:
            lj = json.loads(lsblk)
            for d in lj.get('blockdevices', []):
                devs.append({'name': '/dev/' + d.get('name'), 'type': d.get('type'), 'model': d.get('model'), 'serial': d.get('serial'), 'size_bytes': int(d.get('size')) if d.get('size') else None, 'rotational': d.get('rota') == True if d.get('rota') is not None else None, 'mounts': []})
                if d.get('mountpoint'):
                    devs[-1]['mounts'].append({'mountpoint': d.get('mountpoint'), 'fs_type': d.get('fstype')})
        except Exception:
            pass
    data['storage'] = {'devices': devs}

    # network
    rc, ipj = run_cmd('ip -j address')
    if rc == 0:
        try:
            ipjobj = json.loads(ipj)
            nifs = []
            for itf in ipjobj:
                nifs.append({'name': itf.get('ifname'), 'mac': itf.get('address'), 'ipv4': None, 'ipv6': None, 'mtu': itf.get('mtu')})
                if 'addr_info' in itf:
                    for ai in itf['addr_info']:
                        if ai.get('family') == 'inet' and nifs[-1]['ipv4'] is None:
                            nifs[-1]['ipv4'] = ai.get('local')
                        if ai.get('family') == 'inet6' and nifs[-1]['ipv6'] is None:
                            nifs[-1]['ipv6'] = ai.get('local')
            data['network'] = {'interfaces': nifs}
        except Exception:
            pass

    # gpus
    gpus = []
    rc, nvsmi = run_cmd('nvidia-smi --query-gpu=name,memory.total --format=csv,noheader,nounits')
    if rc == 0:
        for ln in nvsmi.splitlines():
            parts = [p.strip() for p in ln.split(',')]
            if parts:
                gpus.append({'id': 'GPU{}'.format(len(gpus)), 'vendor': 'NVIDIA', 'model': parts[0], 'vram_bytes': int(parts[1]) * 1024 * 1024 if len(parts)>1 else None})
    else:
        # fallback to lspci
        rc, lsp = run_cmd('lspci -mm')
        if rc == 0:
            for ln in lsp.splitlines():
                if 'VGA' in ln or '3D controller' in ln or 'Display controller' in ln:
                    gpus.append({'id': 'GPU{}'.format(len(gpus)), 'vendor': None, 'model': ln.strip(), 'vram_bytes': None})
    data['gpus'] = gpus

    # software
    soft = {'python': {'executable': sys.executable, 'version': platform.python_version(), 'pip_packages': {}}}
    # pip list
    rc, pipj = run_cmd('{} -m pip list --format=json'.format(sys.executable), timeout=30)
    if rc == 0:
        try:
            soft['python']['pip_packages'] = {p['name']: p['version'] for p in json.loads(pipj)}
        except Exception:
            pass
    rc, gccv = run_cmd('gcc --version')
    soft['gcc'] = {'version': gccv.splitlines()[0] if rc == 0 else None}
    rc, cmv = run_cmd('cmake --version')
    soft['cmake'] = {'version': cmv.splitlines()[0] if rc == 0 else None}
    data['software'] = soft

    # measurement defaults
    data['measurement'] = {'tooling': ['perf', 'rdtsc', 'getrusage', 'clock_gettime'], 'perf_events': ['cycles','instructions','cache-misses','branches']}

    return data


def merge_and_write(outpath, collected):
    existing = load_existing(outpath) or {}
    # shallow merge: replace top-level keys with collected, keep others
    for k,v in collected.items():
        existing[k] = v

    # ensure results array exists
    existing.setdefault('results', [])
    save_json(outpath, existing)
    print('Wrote benchmark JSON to', outpath)


def run_bench_script(script, args, cwd=ROOT):
    # run script, capture output to artifact folder
    stamp = datetime.utcnow().strftime('%Y%m%dT%H%M%SZ')
    outdir = os.path.join(os.path.dirname(DEFAULT_OUT), 'artifacts', stamp)
    os.makedirs(outdir, exist_ok=True)
    logfile = os.path.join(outdir, 'run.log')
    cmd = '{} {}'.format(script, args or '')
    print('Running benchmark script:', cmd)
    with open(logfile, 'w') as f:
        p = subprocess.Popen(cmd, shell=True, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
        for line in p.stdout:
            f.write(line)
            sys.stdout.write(line)
        p.wait()
    return outdir, logfile, p.returncode


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--output', '-o', default=DEFAULT_OUT, help='JSON output path')
    p.add_argument('--run-bench', default=None, help='Path to run_benchmarks.sh to execute (optional)')
    p.add_argument('--bench-args', default='', help='Arguments passed to benchmark script')
    p.add_argument('--no-write', action='store_true', help='Collect but do not write file')
    args = p.parse_args()

    collected = probe_basic()

    if args.run_bench:
        outdir, logfile, rc = run_bench_script(args.run_bench, args.bench_args)
        collected.setdefault('raw_artifacts', {})
        collected['raw_artifacts']['last_run_log'] = logfile
        collected['raw_artifacts']['last_run_dir'] = outdir
        collected.setdefault('results', [])
        # attempt to parse known output files (benchmarks results) if any
        # leave parsing to a later enhancement; we'll store artifacts path for now

    if not args.no_write:
        merge_and_write(args.output, collected)
    else:
        print(json.dumps(collected, indent=2))


if __name__ == '__main__':
    main()
