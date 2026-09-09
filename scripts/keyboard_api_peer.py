#!/usr/bin/env python3
"""Controlled stock-byte peer for INTEG-009's ordinary MOS API exerciser.

Stage synchronization uses a fixture-owned SD file, not the keyboard wire.
The emulated P4 peer receives only EMOS-owned locale/General Poll requests;
it sends stock KEY/KEYSTATE packets. No memory, vector or register injection.
This proves target behavior, not real P4 code, browser mapping or UART timing.
"""
import argparse
import json
import os
from pathlib import Path
import select
import socket
import subprocess
import tempfile
import time


def key(ascii, virtual=0, down=1, modifiers=0):
    return bytes([0x81, 4, ascii, modifiers, virtual, down])


def stage_packets(stage):
    # Delays let the unbuffered stock getkey/editor observe each down event.
    simple = {
        1: [key(97, 22)], 2: [key(98, 23, modifiers=2), key(97, 22, 0, modifiers=2)],
        3: [key(98, 23, 0)], 4: [key(120, 45)], 5: [key(120, 45, 0)],
        6: [key(0, 0, 1-(i % 2)) for i in range(260)],
        7: [bytes([0x88, 5, 0xF4, 1, 33, 0, 5])],
        8: [key(55, 9, 0), key(55, 9)],
        10: [key(27, 125), key(27, 125, 0)],
        11: [key(120, 45), key(120, 45, 0)],
    }
    if stage == 9:
        # "cab", Backspace, left, "r", right, Enter => "cra".
        simple[9] = [key(55, 9, 0)]
        for ascii, virtual in [(99,24), (97,22), (98,23), (127,132),
                               (8,154), (114,39), (21,156), (13,143)]:
            simple[9].extend([key(ascii, virtual), key(ascii, virtual, 0)])
    if stage not in simple:
        raise ValueError('Unexpected fixture stage: '+str(stage))
    return [(0.008 if stage == 6 else 0.08, packet) for packet in simple[stage]]


def run(args):
    media = args.sdcard.resolve(strict=True)
    marker = media/'keyboard-state.bin'
    if marker.is_symlink():
        raise ValueError('Refusing a symlinked fixture output')
    marker.unlink(missing_ok=True)
    env = os.environ.copy()
    env['LD_LIBRARY_PATH'] = str(Path.home()/'.local/lib')+os.pathsep+env.get('LD_LIBRARY_PATH', '')
    transcript = bytearray()
    record = {'scope': 'emulated UART1 resident API effects; not physical/browser qualification',
              'outcome': 'fail', 'stages': [], 'locales': [], 'polls': [], 'tx': [], 'rx': []}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    def save():
        args.output.write_text(json.dumps(record, indent=2)+'\n')
        args.output.with_suffix('.log').write_bytes(transcript)
    with tempfile.TemporaryDirectory(prefix='emos-key-peer-') as temporary:
        server = socket.socket(socket.AF_UNIX)
        path = temporary+'/uart1'
        server.bind(path); server.listen(1)
        command = [str(args.emulator.resolve()), '--mos', str(args.firmware.resolve()),
                   '--sdcard', str(media), '--zero', '--uart1-peer', path]
        if args.graphical:
            command += ['--vdp', str(args.vdp.resolve()), '--renderer', 'sw', '--verbose']
        process = subprocess.Popen(command, env=env, cwd=temporary, stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        peer = None
        started = time.monotonic()
        received, scheduled = bytearray(), []
        last_stage, success_at, prompt_sent = 0, None, False
        try:
            while True:
                now = time.monotonic()
                if success_at is None and now-started > args.timeout:
                    raise RuntimeError('Fixture deadline exceeded at stage '+str(last_stage))
                readers = [process.stdout, peer or server]
                for ready in select.select(readers, [], [], 0.005)[0]:
                    if ready is process.stdout:
                        block = os.read(ready.fileno(), 65536)
                        transcript.extend(block)
                    elif peer is None:
                        peer, _ = server.accept(); peer.setblocking(False)
                    else:
                        block = peer.recv(4096)
                        if not block:
                            if success_at is None:
                                raise RuntimeError('UART peer closed before PASS')
                        received.extend(block)
                while len(received) >= 4:
                    request = bytes(received[:4]); del received[:4]
                    record['rx'].append(request.hex(' '))
                    if request[:3] == bytes([23,0,0x81]):
                        record['locales'].append(request[3])
                    elif request[:3] == bytes([23,0,0x80]):
                        record['polls'].append(request[3])
                        scheduled.append((now+0.05, bytes([0x80,1,request[3]])))
                    else:
                        raise RuntimeError('Unexpected EMOS request: '+request.hex(' '))
                value = marker.read_bytes() if marker.exists() else b''
                if len(value) == 1 and value[0] != last_stage:
                    stage = value[0]
                    if stage == 255:
                        if args.graphical:
                            record['error'] = 'Guest rejected stage '+str(last_stage)
                            print('FAIL: '+record['error']+'; see the emulator window', flush=True)
                            # The guest cleared its callback and returned. Keep
                            # the visible failure available for human review.
                            last_stage, success_at = 255, now
                            save()
                            continue
                        raise RuntimeError('Guest rejected stage '+str(last_stage))
                    if stage == 127:
                        if record['stages'] != list(range(1,12)) or record['locales'] != [1,2,2] or len(record['polls']) != 2:
                            raise RuntimeError('Incomplete stages or layout/readiness exchanges')
                        record['outcome'] = 'pass'
                        success_at = now
                        if args.graphical:
                            print('PASS: resident UART1 keyboard API exerciser', flush=True)
                        save()
                    else:
                        if stage != last_stage+1:
                            raise RuntimeError('Out-of-order stage')
                        record['stages'].append(stage)
                        # getkey/editor can return on key-down while its
                        # release remains queued. Finish it before next stage.
                        due = max(now+0.15, scheduled[-1][0]+0.15 if scheduled else now)
                        for delay, packet in stage_packets(stage):
                            scheduled.append((due,packet)); due += delay
                    last_stage = stage
                while scheduled and scheduled[0][0] <= now:
                    if peer is None:
                        raise RuntimeError('No UART1 peer for scheduled packet')
                    _, packet = scheduled.pop(0)
                    peer.sendall(packet)
                    record['tx'].append(packet.hex(' '))
                if success_at is not None and not args.graphical:
                    at_prompt = transcript.rstrip().endswith(b'/ *')
                    # Stock mos_EDITLINE queries mode after printing the prompt.
                    # Fab CLI's send_keys blocks its fake VDP while typing a
                    # whole line; allow that pending query to finish first.
                    if not prompt_sent and at_prompt and now-success_at >= 0.5:
                        process.stdin.write(b'echo KEYBOARD REVIEW PROMPT RETURNED\n'); process.stdin.flush()
                        prompt_sent = True
                    if (b'KEYBOARD REVIEW PROMPT RETURNED' in transcript.splitlines()
                            and at_prompt):
                        record['prompt_return'] = True
                        print('PASS: resident UART1 API effects and mainboard prompt return', flush=True)
                        break
                    if now-success_at > 5:
                        raise RuntimeError('No mainboard CLI command after source return')
                if process.poll() is not None:
                    if success_at is None or process.returncode:
                        raise RuntimeError('Emulator exited before a clean result')
                    break
        except Exception as error:
            record['outcome'] = 'fail'; record['error'] = str(error)
            raise
        finally:
            process.terminate()
            try:
                remainder, _ = process.communicate(timeout=5)
                transcript.extend(remainder)
            except subprocess.TimeoutExpired:
                process.kill(); remainder, _ = process.communicate(); transcript.extend(remainder)
            if peer: peer.close()
            server.close()
            save()
    if record['outcome'] != 'pass':
        raise SystemExit(1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('emulator', 'firmware', 'sdcard', 'output'):
        parser.add_argument('--'+name, type=Path, required=True)
    parser.add_argument('--vdp', type=Path)
    parser.add_argument('--graphical', action='store_true')
    parser.add_argument('--timeout', type=float, default=45)
    args = parser.parse_args()
    if args.graphical and not args.vdp:
        parser.error('--graphical requires --vdp')
    run(args)


if __name__ == '__main__':
    main()
