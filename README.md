
# Sora - Software Radio

Sora is a work in progress. From a user perspective, its main interesting
feature is the ease with which you can travel the spectrum in GUI mode.

It is meant as a personal playground to play with radio signals, and normal
usage includes modifying and recompiling the code.

Patches are more welcome than complaints about user-unfriendliness.

# Compilation

You need at least GTK 2 and fftw 3, plus any radio-specific library.

Compile Sora from a separate directory for optimal experience:

    $ mkdir build
    $ cd build
    $ ../configure
    $ make -j 8
    $ ./sora <options>

# Usage

```
Usage: sora OPTION ...
Mode of operation
  -d, --adsb-decode          decode ADS-B
  -g, --gui                  display FFT in realtime
      --scan                 use scan mode

Radio front-end
      --alsa                 read from ALSA device
      --file=FILE            read from raw file FILE
      --hackrf-one           use HackRF One frontend
      --rtlsdr               use rtl-sdr frontend
      --uhd                  use UHD frontend

Basic mandatory options
  -f, --frequency=FREQ       set tuner frequency to FREQ
  -s, --sample-rate=RATE     specify sample rate

Other options
      --adsb-from-raw        read 2MS/s IQ stream on stdin
      --adsb-to-bitstring    output ADS-B data as bit string
      --alsa-name=NAME       read from ALSA device NAME
      --fcdhid=HID           set parameters of FCD on given HID
      --file-encoding=FORMAT specify encoding of file as FORMAT
              FORMAT can be any of uc8, sc8, sc16, u8, s16
  -q, --quiet                be less verbose
      --rtlsdr-index=INDEX   specify rtl-sdr device index
      --squelch=DB           squelch value in dB for scan mode
      --uhd-addr=ARGS        use ARGS as UHD arguments
      --uhd-ant=ANT          use antenna ANT for UHD
      --uhd-spec=SPEC        use specification SPEC for UHD
  -v, --verbose              be more verbose
```

# Keys in GUI mode

 * Left and right arrows move the center frequency of the tuner by a quarter
   of the sample rate
 * 'q' quits
 * 'a' makes all signal values fit the window
 * 'r' resets the peak line
 * 'c' toggles cumulation mode

# Examples

    $ sora --rtlsdr -f 100M -s 2M --gui

    $ sora --hackrf -f 100M -s 20M --gui

    $ sora --uhd --uhd-addr addr=192.168.10.2 --uhd-ant TX/RX -f 100M -s 40M --gui

    $ rtl_sdr -f 1090e6 -s 2e6 - | sora --adsb-decode --adsb-from-raw

# License

Sora is in the public domain.
