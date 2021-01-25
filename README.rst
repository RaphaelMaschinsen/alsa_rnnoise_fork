============
alsa_rnnoise
============
RNNoise_ based noise removal plugin for ALSA

.. _RNNoise: https://gitlab.xiph.org/xiph/rnnoise/

Building
--------
``alsa_rnnoise`` depends on ``alsa-lib`` and ``rnnoise``.
You can get development files for ALSA through a package called
``libasound2-dev`` on Debian, ``alsa-lib-devel`` on Fedora, or
``media-libs/alsa-lib`` on Gentoo. RNNoise_ might require manual installation,
since it is not packaged by many distributions. See the RNNoise README_

After installing RNNoise and ALSA, building ``alsa_rnnoise`` is a standard
meson process:

.. code-block:: sh

    $ meson build  # generates the ``build'' directory
    $ ninja -C build
    $ sudo ninja -C build install

.. _README: https://gitlab.xiph.org/xiph/rnnoise/-/blob/master/README

Build options
-------------
Build options are specified via ``-Doption=value`` flags to the ``meson``
invocation.

``plugin_dir``
    ALSA plugin directory location, usually ``/usr/lib/alsa-lib``.
    Depends on ALSA build time configuration

Usage
-----
The plugin has no settings other than the slave PCM. There is currently a
known bug where if something requests to know the accepted sample rate
interval, the plugin reports an empty interval. Please make sure to specify the
sample rate as exactly ``48000`` as seen in the ``rnnoise`` PCM below. I'm
currently not aware of a way to fix this with ALSAs external plugin API.

.. code-block::

    pcm.urnnoise {
        type rnnoise
        slave.pcm "sysdefault"
    }

    pcm.rnnoise {
        type plug
        slave {
            rate 48000
            pcm "urnnoise"
        }
    }

    pcm.!default {
        type asym
        playback.pcm "cards.pcm.default"
        capture.pcm "rnnoise"
    }

License
-------
``alsa_rnnoise`` is Free software licensed under the GNU General Public
License, version 3.
