
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>

#include <ads-b/planes-main-loop.h>
#include <common/frequency.h>
#include <radio/fcdhid.h>
#include <radio/radio-audio.h>
#include <radio/radio-file.h>
#include <radio/hackrf.h>
#include <radio/rtlsdr.h>
#include <radio/uhd.h>
#include <scan/scan-main-loop.h>
#include <ui/gtk-ui.h>
#include <ui/widget-fft.h>
#include <util/hash.h>
#include <util/memory.h>

#define OPTION_FCDAUDIO		256
#define OPTION_FCDHID		257
#define OPTION_RTLSDR_INDEX	258
#define OPTION_UHD_ADDR		259
#define OPTION_UHD_ANT		260
#define OPTION_UHD_SPEC		261
#define OPTION_FILE_NAME	262
#define OPTION_FILE_ENCODING	263
#define OPTION_SQUELCH		264
#define OPTION_ALSA_NAME	265

const char *program_name;
int option_do_set_frequency = 0;
int option_gui = 0;
int option_quiet = 0;
int option_verbose = 0;
int option_adsb_decode = 0;
int option_adsb_from_raw = 0;
int option_adsb_to_bitstring = 0;
int option_do_set_sample_rate = 0;
int option_do_scan = 0;
double option_squelch_db = 10;
const char *option_file_name = NULL;
int option_file_encoding = 0;
#ifdef HAVE_LIBHACKRF
int option_use_hackrf_one = 0;
#endif
#ifdef HAVE_LIBRTLSDR
int option_use_rtlsdr = 0;
int option_rtlsdr_index = 0;
#endif
#ifdef HAVE_UHD
int option_use_uhd = 0;
const char *option_uhd_addr = "";
const char *option_uhd_spec = "";
const char *option_uhd_ant = "";
#endif
#ifdef USE_ALSA
int option_use_alsa = 0;
const char *option_alsa_name = "";
#endif

static char *option_fcdaudio_path = NULL;
static char *option_fcdhid_path = NULL;
static t_frequency current_frequency;		/* in Hz */
static t_frequency current_sample_rate;		/* in Hz */

struct option options[] = {
    { "adsb-decode", no_argument, NULL, 'd' },
    { "adsb-from-raw", no_argument, &option_adsb_from_raw, 1 },
    { "adsb-to-bitstring", no_argument, &option_adsb_to_bitstring, 1 },
#ifdef USE_ALSA
    { "alsa", no_argument, &option_use_alsa, 1 },
    { "alsa-name", required_argument, NULL, OPTION_ALSA_NAME },
#endif
    { "fcdaudio", required_argument, NULL, OPTION_FCDAUDIO },
    { "fcdhid", required_argument, NULL, OPTION_FCDHID },
    { "file", required_argument, NULL, OPTION_FILE_NAME },
    { "file-encoding", required_argument, NULL, OPTION_FILE_ENCODING },
    { "frequency", required_argument, NULL, 'f' },
    { "gui", no_argument, NULL, 'g' },
#ifdef HAVE_LIBHACKRF
    { "hackrf-one", no_argument, &option_use_hackrf_one, 1 },
#endif
    { "quiet", no_argument, NULL, 'q' },
#ifdef HAVE_LIBRTLSDR
    { "rtlsdr", no_argument, &option_use_rtlsdr, 1 },
    { "rtlsdr-index", required_argument, NULL, OPTION_RTLSDR_INDEX },
#endif
    { "sample-rate", required_argument, NULL, 's' },
    { "scan", no_argument, &option_do_scan, 1 },
    { "squelch", required_argument, NULL, OPTION_SQUELCH },
#ifdef HAVE_UHD
    { "uhd", no_argument, &option_use_uhd, 1 },
    { "uhd-addr", required_argument, NULL, OPTION_UHD_ADDR },
    { "uhd-ant", required_argument, NULL, OPTION_UHD_ANT },
    { "uhd-spec", required_argument, NULL, OPTION_UHD_SPEC },
#endif
    { "verbose", no_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 }
};

static void
usage(int status, const char *argv0) {
    FILE *out = status == EXIT_SUCCESS? stdout : stderr;

    fprintf(out, "usage: %s OPTION ...\n"
	"Mode of operation\n"
	"  -d, --adsb-decode          decode ADS-B\n"
	"  -g, --gui                  display FFT in realtime\n"
	"      --scan                 use scan mode\n"
	"\n"
	"Radio front-end\n"
#ifdef USE_ALSA
	"      --alsa                 read from ALSA device\n"
#endif
	"      --file=FILE            read from raw file FILE\n"
#ifdef HAVE_LIBHACKRF
	"      --hackrf-one           use HackRF One frontend\n"
#endif
#ifdef HAVE_LIBRTLSDR
	"      --rtlsdr               use rtl-sdr frontend\n"
#endif
#ifdef HAVE_UHD
	"      --uhd                  use UHD frontend\n"
#endif
	"\n"
	"Basic mandatory options\n"
	"  -f, --frequency=FREQ       set tuner frequency to FREQ\n"
	"  -s, --sample-rate=RATE     specify sample rate\n"
	"\n"
	"Other options\n"
	"      --adsb-from-raw        read 2MS/s IQ stream on stdin\n"
	"      --adsb-to-bitstring    output ADS-B data as bit string\n"
#ifdef USE_ALSA
	"      --alsa-name=NAME       read from ALSA device NAME\n"
#endif
	"      --fcdhid=HID           set parameters of FCD on given HID\n"
	"      --file-encoding=FORMAT specify encoding of file as FORMAT\n"
	"              FORMAT can be any of uc8, sc8, sc16, u8, s16\n"
	"  -q, --quiet                be less verbose\n"
#ifdef HAVE_LIBRTLSDR
	"      --rtlsdr-index=INDEX   specify rtl-sdr device index\n"
#endif
	"      --squelch=DB           squelch value in dB for scan mode\n"
#ifdef HAVE_UHD
	"      --uhd-addr=ARGS        use ARGS as UHD arguments\n"
	"      --uhd-ant=ANT          use antenna ANT for UHD\n"
	"      --uhd-spec=SPEC        use specification SPEC for UHD\n"
#endif
	"  -v, --verbose              be more verbose\n"
	"", argv0);
    exit(status);
}

int
main(int argc, char *argv[]) {
    struct radio *radio = NULL;
    int c;
    int ret;
    int status = EXIT_SUCCESS;

    program_name = argv[0];
    hash_init();

    while ((c = getopt_long(argc, argv, "df:gqs:v", options, NULL)) != -1) {
	switch (c) {
	case 'd':
	    option_adsb_decode = 1;
	    break;
	case 'f':
	    if (!frequency_parse(optarg, &current_frequency)) {
		fprintf(stderr, "'%s' couldn't be parsed as a frequency\n",
			optarg);
		goto err;
	    }
	    option_do_set_frequency = 1;
	    break;
	case 'g':
	    option_gui = 1;
	    break;
	case 'q': 
	    option_quiet = 1;
	    break;
	case 's':
	    if (!frequency_parse(optarg, &current_sample_rate)) {
		fprintf(stderr, "'%s' couldn't be parsed as a sample rate\n",
			optarg);
		goto err;
	    }
	    option_do_set_sample_rate = 1;
	    break;
	case 'v':
	    option_verbose = 1;
	    break;
	case OPTION_ALSA_NAME:
#ifdef USE_ALSA
	    option_alsa_name = optarg;
#endif
	    break;
	case OPTION_FCDAUDIO:
	    option_fcdaudio_path = optarg;
	    break;
	case OPTION_FCDHID:
	    option_fcdhid_path = optarg;
	    break;
	case OPTION_FILE_NAME:
	    option_file_name = optarg;
	    break;
	case OPTION_FILE_ENCODING:
	    if (strcmp(optarg, "uc8") == 0)
		option_file_encoding = RADIO_FILE_ENCODING_UC8;
	    else if (strcmp(optarg, "sc8") == 0)
		option_file_encoding = RADIO_FILE_ENCODING_SC8;
	    else if (strcmp(optarg, "sc16") == 0)
		option_file_encoding = RADIO_FILE_ENCODING_SC16;
	    else if (strcmp(optarg, "u8") == 0)
		option_file_encoding = RADIO_FILE_ENCODING_U8;
	    else if (strcmp(optarg, "s16") == 0)
		option_file_encoding = RADIO_FILE_ENCODING_S16;
	    else
		fprintf(stderr, "Unknown encoding %s\n", optarg);
	    break;
	case OPTION_RTLSDR_INDEX:
#ifdef HAVE_LIBRTLSDR
	    option_rtlsdr_index = atoi(optarg);
#endif
	    break;
	case OPTION_UHD_ADDR:
#ifdef HAVE_UHD
	    option_uhd_addr = optarg;
#endif
	    break;
	case OPTION_UHD_ANT:
#ifdef HAVE_UHD
	    option_uhd_ant = optarg;
#endif
	    break;
	case OPTION_UHD_SPEC:
#ifdef HAVE_UHD
	    option_uhd_spec = optarg;
#endif
	    break;
	case OPTION_SQUELCH:
	    option_squelch_db = atof(optarg);
	    break;
	case 0:
	    break;
	default:
	    usage(EXIT_FAILURE, argv[0]);
	}
    }

    argc -= optind;
    argv += optind;

    if (option_adsb_decode) {
	int flags = 0;
	if (option_adsb_to_bitstring)
	    flags |= PLANES_OUTPUT_AS_BITSTRING;
	else
	    flags |= PLANES_OUTPUT_AS_DECODED;
	if (option_adsb_from_raw)
	    flags |= PLANES_INPUT_FROM_RAW;
	planes_main_loop(stdin, flags);
	return EXIT_SUCCESS;
    }

    if (argc != 0)
	usage(EXIT_FAILURE, argv[0]);

    if (option_fcdhid_path != NULL) {
	radio = fcdhid_open(option_fcdhid_path, option_fcdaudio_path);
	if (radio == NULL) {
	    perror(option_fcdhid_path);
	    goto err;
	}
    }

    if (option_file_name != NULL) {
	if (radio != NULL) {
	    fprintf(stderr, "cannot use multiple radios\n");
	    goto err;
	}
	radio = radio_file_open(option_file_name, option_file_encoding);
	if (radio == NULL) {
	    fprintf(stderr, "can't open file\n");
	    goto err;
	}
    }

#ifdef USE_ALSA
    if (option_use_alsa) {
	if (radio != NULL) {
	    fprintf(stderr, "cannot use multiple radios\n");
	    goto err;
	}
	radio = radio_audio_open(option_alsa_name);
	if (radio == NULL) {
	    fprintf(stderr, "can't open alsa input\n");
	    goto err;
	}
    }
#endif

#ifdef HAVE_LIBHACKRF
    if (option_use_hackrf_one) {
	if (radio != NULL) {
	    fprintf(stderr, "cannot use multiple radios\n");
	    goto err;
	}
	radio = hackrf_radio_open();
	if (radio == NULL) {
	    fprintf(stderr, "can't open hackrf\n");
	    goto err;
	}
    }
#endif

#ifdef HAVE_LIBRTLSDR
    if (option_use_rtlsdr) {
	if (radio != NULL) {
	    fprintf(stderr, "cannot use multiple radios\n");
	    goto err;
	}
	radio = rtlsdr_radio_open(option_rtlsdr_index);
	if (radio == NULL) {
	    fprintf(stderr, "can't open rtlsdr\n");
	    goto err;
	}
    }
#endif

#ifdef HAVE_UHD
    if (option_use_uhd) {
	if (radio != NULL) {
	    fprintf(stderr, "cannot use multiple radios\n");
	    goto err;
	}
	radio = uhd_radio_open(option_uhd_addr, option_uhd_spec,
		option_uhd_ant);
	if (radio == NULL) {
	    fprintf(stderr, "can't open uhd\n");
	    goto err;
	}
    }
#endif

    if (option_do_set_frequency) {
	if (radio == NULL) {
	    fprintf(stderr, "can't set frequency without radio yet\n");
	    goto err;
	}
	if (radio->m->set_frequency(radio, current_frequency) == -1) {
	    fprintf(stderr, "couldn't set frequency\n");
	    goto err;
	}

	if (option_verbose) {
	    char *f_pprint;
	    t_frequency freq;
	    ret = radio->m->get_frequency(radio, &freq);
	    if (ret == -1) {
		fprintf(stderr, "Failed to read back frequency\n");
		goto err;
	    }
	    f_pprint = frequency_human_print(freq);
	    printf("Frequency: %s\n", f_pprint);
	    memory_free(f_pprint);
	}
    }

    if (option_do_set_sample_rate) {
	if (radio == NULL) {
	    fprintf(stderr, "can't set sample rate without radio\n");
	    goto err;
	}
	if (radio->m->set_sample_rate(radio, current_sample_rate) == -1) {
	    fprintf(stderr, "couldn't set sample rate\n");
	    goto err;
	}
    } else {
	fprintf(stderr, "setting the sample rate is mandatory\n");
	goto err;
    }

#if 0
    if (radio != NULL && !option_gui) {
	struct sample buf[4096];

	for (;;) {
	    ssize_t nread = radio->m->read(radio, buf, sizeof buf / sizeof buf[0]);
	    if (nread == -1) {
		fprintf(stderr, "radio read error\n");
		goto err;
	    }
	    write(1, buf, nread * sizeof buf[0]);
	}
    }
#endif

    if (option_do_scan) {
	scan_main_loop(radio, option_squelch_db);
	return EXIT_SUCCESS;
    }

    if (option_gui) {
	struct widget *w;
	if (gtk_gui_setup(&argc, &argv) == -1)
	    goto err;
	w = widget_fft_new(radio);
	gtk_gui_add_widget(w);
	gtk_gui_run_main();
    }

    if (0) {
err:
	status = EXIT_FAILURE;
    }
    if (radio != NULL)
	radio->m->close(radio);

    return status;
}

