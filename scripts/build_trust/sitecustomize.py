"""Install system trust for Python subprocesses launched by scripts/pio.py only."""

import os


if os.environ.get("DESKDUO_BUILD_TRUSTSTORE") == "1":
    try:
        import truststore
    except ImportError:
        pass
    else:
        truststore.inject_into_ssl()
