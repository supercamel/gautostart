import os

project = "GAutostart"
author = "GAutostart contributors"
copyright = "2026, GAutostart contributors"

release = "0.1.0"
version = release

extensions = []

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "sphinx_rtd_theme"
html_title = f"{project} {release}"
html_baseurl = os.environ.get("READTHEDOCS_CANONICAL_URL", "/")

if os.environ.get("READTHEDOCS") == "True":
    html_context = {
        "READTHEDOCS": True,
    }
