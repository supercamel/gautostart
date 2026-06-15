Development
===========

Build And Test
--------------

.. code-block:: sh

   meson setup build
   meson compile -C build
   meson test -C build

The Linux XDG smoke test writes into a temporary ``XDG_CONFIG_HOME`` so it does
not modify the developer's real autostart settings.

Documentation
-------------

The documentation is built with Sphinx and configured for Read the Docs through
``.readthedocs.yaml``.

Create a local documentation environment:

.. code-block:: sh

   python -m venv .venv-docs
   . .venv-docs/bin/activate
   python -m pip install -r docs/requirements.txt

Build the HTML docs:

.. code-block:: sh

   python -m sphinx -W -b html docs docs/_build/html

The generated HTML lives under ``docs/_build/html`` and is ignored by Git.
