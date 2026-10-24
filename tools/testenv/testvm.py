#!/usr/bin/env python3
"""The test VM of the L2 tests (docs/port/PLAN.md, section 6.3).

The VM is reached as root over SSH with a key of its own (vm/state/
id_ed25519, not the user's ~/.ssh or agent). Its address is taken from
OPENSCP_TEST_VM; the setup (users, ports) is in vm/testvm.env, shared with
vm/provision.sh, which sets the VM up.

Usable as a module (see TestVm) and as a command line tool:

    testvm.py provision [--host HOST]   # (re)provisions the VM
    testvm.py check [--host HOST]       # connects and prints the setup
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path

VM_DIR = Path(__file__).resolve().parent / 'vm'
STATE_DIR = VM_DIR / 'state'


class TestVmError(RuntimeError):
    pass


def load_settings():
    settings = {}
    for line in (VM_DIR / 'testvm.env').read_text().splitlines():
        line = line.strip()
        if line and not line.startswith('#'):
            name, value = line.split('=', 1)
            settings[name] = value
    return settings


class TestVm:
    def __init__(self, host=None, key=None):
        self.host = host or os.environ.get('OPENSCP_TEST_VM')
        if not self.host:
            raise TestVmError('OPENSCP_TEST_VM is not set')
        self.key = Path(key or os.environ.get('OPENSCP_TEST_VM_KEY') or (STATE_DIR / 'id_ed25519'))
        self.known_hosts = STATE_DIR / 'known_hosts'
        self.settings = load_settings()
        self.ssh_port = int(self.settings['SSH_PORT'])
        self.password = self.settings['SSH_PASSWORD']

    def client_key(self):
        """The key of the test users (vm/state/client_ed25519), created on demand."""
        key = STATE_DIR / 'client_ed25519'
        if not key.exists():
            STATE_DIR.mkdir(parents=True, exist_ok=True)
            subprocess.run(['ssh-keygen', '-q', '-t', 'ed25519', '-N', '', '-C', 'openscp-test-client', '-f', str(key)],
                           check=True)
        return key

    def ssh_options(self):
        return [
            '-F', '/dev/null',
            '-i', str(self.key),
            '-o', 'IdentitiesOnly=yes',
            '-o', 'IdentityAgent=none',
            '-o', 'UserKnownHostsFile=%s' % self.known_hosts,
            '-o', 'StrictHostKeyChecking=accept-new',
            '-o', 'BatchMode=yes',
            '-o', 'ConnectTimeout=10',
        ]

    def run(self, command, input=None, check=True, timeout=300):
        """Runs a shell command as root on the VM; returns the CompletedProcess."""
        if isinstance(input, str):
            input = input.encode()
        result = subprocess.run(
            ['ssh', *self.ssh_options(), 'root@%s' % self.host, command],
            input=input, capture_output=True, timeout=timeout)
        if check and result.returncode != 0:
            raise TestVmError('%s failed (%d): %s' % (command, result.returncode, result.stderr.decode(errors='replace')))
        return result

    def output(self, command):
        return self.run(command).stdout.decode()

    def reachable(self):
        try:
            return self.run('true', check=False, timeout=30).returncode == 0
        except subprocess.TimeoutExpired:
            return False

    def provision(self):
        """Uploads vm/provision.sh with the setup and runs it."""
        client_key = self.client_key()
        archive = subprocess.run(
            ['tar', '-c', '-C', str(VM_DIR), 'provision.sh', 'testvm.env', 's3-server.py', 's3-bootstrap.py', 'openscp-s3',
             '-C', str(client_key.parent), '%s.pub' % client_key.name],
            capture_output=True, check=True).stdout
        script = 'set -e; D=$(mktemp -d); tar -x -C "$D"; bash "$D/provision.sh" && status=0 || status=$?; rm -r "$D"; exit $status'
        result = self.run(script, input=archive, check=False, timeout=1800)
        if result.returncode != 0:
            raise TestVmError('provisioning failed:\n%s%s' % (
                result.stdout.decode(errors='replace'), result.stderr.decode(errors='replace')))
        return result.stdout.decode(errors='replace')

    def host_key_fingerprint(self, key_type='ed25519', algorithm='sha256'):
        """The fingerprint of a host key of the test sshd, as WinSCP takes it."""
        output = self.output('ssh-keygen -l -E %s -f /etc/ssh/openscp/ssh_host_%s_key.pub' % (algorithm, key_type))
        fields = output.split()
        bits, checksum = fields[0], fields[1].split(':', 1)[1]
        name = {'ed25519': 'ssh-ed25519', 'ecdsa': 'ecdsa-sha2-nistp256', 'rsa': 'ssh-rsa'}[key_type]
        if key_type == 'ed25519':
            # As PuTTY counts the bits
            bits = '255'
        return '%s %s %s' % (name, bits, checksum)

    def user_home(self, user):
        return self.output('getent passwd %s' % user).split(':')[5]

    def certificate_fingerprint(self, algorithm='sha256'):
        """The fingerprint of the TLS servers' certificate, as WinSCP takes
        it (colon-separated hexadecimal bytes)."""
        output = self.output('openssl x509 -noout -fingerprint -%s -in /etc/openscp/tls/cert.pem' % algorithm)
        return output.split('=', 1)[1].strip().lower()

    def services_active(self, *services):
        """Whether the systemd services of the tests are active."""
        result = self.run('systemctl is-active %s' % ' '.join('%s.service' % s for s in services), check=False)
        return result.returncode == 0


def _main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=('provision', 'check'))
    parser.add_argument('--host', help='the VM (default: OPENSCP_TEST_VM)')
    args = parser.parse_args()
    vm = TestVm(args.host)
    if args.command == 'provision':
        print(vm.provision(), end='')
    else:
        print(vm.output('. /etc/os-release; echo "$PRETTY_NAME"; systemctl is-active openscp-sshd.service'), end='')
        print('test sshd: port %d, host key %s' % (vm.ssh_port, vm.host_key_fingerprint()))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(_main())
    except TestVmError as error:
        print('testvm: %s' % error, file=sys.stderr)
        sys.exit(1)
