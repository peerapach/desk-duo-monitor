"""Regression checks for scoped build trust and pip's TLS context."""

import os
from pathlib import Path
import subprocess
import sys
import unittest


ROOT = Path(__file__).resolve().parents[1]


class BuildTrust(unittest.TestCase):
    def run_python(self, source, scoped):
        environment = os.environ.copy()
        environment["PYTHONPATH"] = str(ROOT / "scripts/build_trust")
        environment.pop("DESKDUO_BUILD_TRUSTSTORE", None)
        if scoped:
            environment["DESKDUO_BUILD_TRUSTSTORE"] = "1"
        return subprocess.run(
            [sys.executable, "-c", source], env=environment,
            capture_output=True, text=True, timeout=20,
        )

    def test_scoped_build_allows_pip_to_require_certificate_verification(self):
        # The build hook must not inject a second truststore implementation
        # before pip imports its own copy: that recurses in verify_mode.
        result = self.run_python("""
import ssl
from pip._internal.cli.index_command import _create_truststore_ssl_context
context = _create_truststore_ssl_context()
context.verify_mode = ssl.CERT_REQUIRED
assert context.verify_mode == ssl.CERT_REQUIRED
assert context.check_hostname
# Also exercise the context used by non-pip build tools.
context = ssl.create_default_context()
assert context.verify_mode == ssl.CERT_REQUIRED
assert context.check_hostname
""", scoped=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_unscoped_python_keeps_standard_ssl_context(self):
        result = self.run_python("""
import ssl
assert ssl.SSLContext.__module__ == 'ssl'
assert ssl.create_default_context().verify_mode == ssl.CERT_REQUIRED
""", scoped=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
