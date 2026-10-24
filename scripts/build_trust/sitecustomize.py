"""Install system trust for Python subprocesses launched by scripts/pio.py only."""

import os


if os.environ.get("DESKDUO_BUILD_TRUSTSTORE") == "1":
    try:
        # pip creates its own truststore contexts while installing build tools.
        # Inject that same implementation so importing pip cannot capture an
        # already-patched SSLContext and recurse in certificate verification.
        from pip._vendor import truststore
    except ImportError:
        try:
            import truststore
        except ImportError:
            truststore = None
    if truststore is not None:
        truststore.inject_into_ssl()
