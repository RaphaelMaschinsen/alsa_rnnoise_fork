============
alsa_rnnoise
============
RNNoise_ based noise removal plugin for ALSA

.. _RNNoise: https://gitlab.xiph.org/xiph/rnnoise/

Build options
-------------
``plugin_dir``: ALSA plugin directory location, usually ``/usr/lib/alsa-lib``.
Depends on ALSA build time configuration

Usage
-----
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
